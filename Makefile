CC = gcc
CFLAGS = -O3 -Wall
LDFLAGS = -lpthread -lm

all: pc

pc: prod_cons.c
	$(CC) $(CFLAGS) -o pc prod_cons.c $(LDFLAGS)

clean:
	rm -f pc