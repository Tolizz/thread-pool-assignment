#!/bin/bash

# Οι τιμές των consumers που θέλουμε να δοκιμάσουμε
CONSUMERS_LIST="1 2 4 8 16"

for q in $CONSUMERS_LIST
do
    echo "======================================"
    echo " Πείραμα με $q Consumers"
    echo "======================================"
    
    # Μεταγλώττιση περνώντας τον αριθμό των consumers (-DNUM_CONSUMERS=$q)
    gcc -O3 -Wall -DNUM_CONSUMERS=$q -o pc prod_cons.c -lpthread -lm
    
    # Τρέχουμε το πρόγραμμα 5 φορές
    for i in {1..20}
    do
        echo -n "Εκτέλεση $i: "
        # Τρέχουμε το πρόγραμμα και απομονώνουμε (grep) μόνο τη γραμμή με τον χρόνο
        ./pc | grep "Average Wait Time"
    done
    
    echo "" # Κενή γραμμή για ομορφιά
done

echo "Όλα τα πειράματα ολοκληρώθηκαν!"