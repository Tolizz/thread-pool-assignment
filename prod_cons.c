/*
 * File    : prod_cons.c
 * Title   : Thread Pool Producer/Consumer Demo
 * Short   : A solution to the producer consumer problem using pthreads,
 * adapted to work as a thread pool executing function pointers.
 */

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>

#define QUEUESIZE 10
#define PRODUCER_LOOPS 10000 // Number of tasks each producer will create
#define NUM_PRODUCERS 2      // Parameter p
#ifndef NUM_CONSUMERS
#define NUM_CONSUMERS 4      // Parameter q
#endif    

// --- STRUCTURES ---

// Arguments for our work function
typedef struct {
    double angles[10];
    struct timeval enqueue_time;
} WorkArgs;

// The function wrapper structure required by the assignment
struct workFunction {
    void * (*work)(void *);
    void * arg;
};

// The FIFO Queue structure
typedef struct {
    struct workFunction buf[QUEUESIZE];
    long head, tail;
    int full, empty;
    pthread_mutex_t *mut;
    pthread_cond_t *notFull, *notEmpty;
} queue;

// --- GLOBAL VARIABLES FOR STATISTICS ---
double total_wait_time = 0.0;
long tasks_consumed = 0;
pthread_mutex_t stats_mut = PTHREAD_MUTEX_INITIALIZER;

// Flag to signal consumers that producers are done
int production_done = 0;

// --- FUNCTION DECLARATIONS ---
queue *queueInit(void);
void queueDelete(queue *q);
void queueAdd(queue *q, struct workFunction in);
void queueDel(queue *q, struct workFunction *out);
void *producer(void *args);
void *consumer(void *args);
void *calculate_sine(void *arg);

// --- MAIN ---
int main() {
    queue *fifo;
    pthread_t pro[NUM_PRODUCERS], con[NUM_CONSUMERS];

    printf("Starting Thread Pool with %d Producers and %d Consumers...\n", NUM_PRODUCERS, NUM_CONSUMERS);

    fifo = queueInit();
    if (fifo == NULL) {
        fprintf(stderr, "main: Queue Init failed.\n");
        exit(1);
    }

    // Create Consumer threads
    for (int i = 0; i < NUM_CONSUMERS; i++) {
        pthread_create(&con[i], NULL, consumer, fifo);
    }

    // Create Producer threads
    for (int i = 0; i < NUM_PRODUCERS; i++) {
        pthread_create(&pro[i], NULL, producer, fifo);
    }

    // Wait for all Producers to finish
    for (int i = 0; i < NUM_PRODUCERS; i++) {
        pthread_join(pro[i], NULL);
    }

    // Signal Consumers that production is done
    pthread_mutex_lock(fifo->mut);
    production_done = 1;
    pthread_cond_broadcast(fifo->notEmpty); // Wake up any waiting consumers
    pthread_mutex_unlock(fifo->mut);

    // Wait for all Consumers to finish
    for (int i = 0; i < NUM_CONSUMERS; i++) {
        pthread_join(con[i], NULL);
    }

    // Cleanup
    queueDelete(fifo);
    pthread_mutex_destroy(&stats_mut);

    // Calculate and print statistics
    if (tasks_consumed > 0) {
        double avg_wait_time = total_wait_time / tasks_consumed;
        printf("All tasks completed.\n");
        printf("Total Tasks Consumed : %ld\n", tasks_consumed);
        printf("Average Wait Time    : %.2f microseconds\n", avg_wait_time);
    }

    return 0;
}

// --- WORK FUNCTION ---
void *calculate_sine(void *arg) {
    WorkArgs *args = (WorkArgs *)arg;
    volatile double dummy_sum = 0.0; // To prevent compiler optimization
    
    // Simple work: calculate sine for 10 angles
    for (int i = 0; i < 10; i++) {
        dummy_sum += sin(args->angles[i]);
    }
    
    // Free the memory allocated by the producer
    free(args);
    return NULL;
}

// --- THREAD FUNCTIONS ---
void *producer(void *q) {
    queue *fifo = (queue *)q;

    for (int i = 0; i < PRODUCER_LOOPS; i++) {
        // Prepare arguments
        WorkArgs *w_args = (WorkArgs *)malloc(sizeof(WorkArgs));
        for (int j = 0; j < 10; j++) {
            w_args->angles[j] = (double)(rand() % 360) * M_PI / 180.0;
        }

        // Prepare the task
        struct workFunction task;
        task.work = calculate_sine;
        task.arg = w_args;

        pthread_mutex_lock(fifo->mut);
        while (fifo->full) {
            pthread_cond_wait(fifo->notFull, fifo->mut);
        }

        // Record time right before adding to the queue
        gettimeofday(&(w_args->enqueue_time), NULL);
        queueAdd(fifo, task);

        pthread_mutex_unlock(fifo->mut);
        pthread_cond_signal(fifo->notEmpty);
    }
    
    return NULL;
}

void *consumer(void *q) {
    queue *fifo = (queue *)q;
    struct timeval dequeue_time;

    while (1) {
        struct workFunction task;

        pthread_mutex_lock(fifo->mut);
        while (fifo->empty && !production_done) {
            pthread_cond_wait(fifo->notEmpty, fifo->mut);
        }

        // If queue is empty and producers are done, exit the loop
        if (fifo->empty && production_done) {
            pthread_mutex_unlock(fifo->mut);
            break;
        }

        queueDel(fifo, &task);
        pthread_mutex_unlock(fifo->mut);
        pthread_cond_signal(fifo->notFull);

        // Record time right after dequeuing (before execution)
        gettimeofday(&dequeue_time, NULL);

        WorkArgs *w_args = (WorkArgs *)task.arg;
        
        // Calculate elapsed time in microseconds
        double elapsed_time = (dequeue_time.tv_sec - w_args->enqueue_time.tv_sec) * 1000000.0;
        elapsed_time += (dequeue_time.tv_usec - w_args->enqueue_time.tv_usec);

        // Safely update global statistics
        pthread_mutex_lock(&stats_mut);
        total_wait_time += elapsed_time;
        tasks_consumed++;
        pthread_mutex_unlock(&stats_mut);

        // Execute the function
        task.work(task.arg);
    }
    
    return NULL;
}

// --- QUEUE HELPER FUNCTIONS ---
queue *queueInit(void) {
    queue *q = (queue *)malloc(sizeof(queue));
    if (q == NULL) return NULL;

    q->empty = 1;
    q->full = 0;
    q->head = 0;
    q->tail = 0;
    
    q->mut = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(q->mut, NULL);
    
    q->notFull = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
    pthread_cond_init(q->notFull, NULL);
    
    q->notEmpty = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
    pthread_cond_init(q->notEmpty, NULL);
    
    return q;
}

void queueDelete(queue *q) {
    pthread_mutex_destroy(q->mut);
    free(q->mut);
    pthread_cond_destroy(q->notFull);
    free(q->notFull);
    pthread_cond_destroy(q->notEmpty);
    free(q->notEmpty);
    free(q);
}

void queueAdd(queue *q, struct workFunction in) {
    q->buf[q->tail] = in;
    q->tail++;
    if (q->tail == QUEUESIZE)
        q->tail = 0;
    if (q->tail == q->head)
        q->full = 1;
    q->empty = 0;
}

void queueDel(queue *q, struct workFunction *out) {
    *out = q->buf[q->head];
    q->head++;
    if (q->head == QUEUESIZE)
        q->head = 0;
    if (q->head == q->tail)
        q->empty = 1;
    q->full = 0;
}