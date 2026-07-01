#!/bin/bash

# The name of the output file
CSV_FILE="results.csv"

# 1. Create the CSV header (Consumers, Run 1, Run 2... Run 20)
echo -n "Consumers" > $CSV_FILE
for i in {1..20}; do
    echo -n ",Run $i" >> $CSV_FILE
done
echo "" >> $CSV_FILE

# The values of consumers to test
CONSUMERS_LIST="1 2 4 8 16"

for q in $CONSUMERS_LIST
do
    echo "Running experiments for $q Consumers (please wait)..."
    
    # Compile with the new NUM_CONSUMERS value
    gcc -O3 -Wall -DNUM_CONSUMERS=$q -o pc prod_cons.c -lpthread -lm
    
    # Start the CSV row with the number of consumers (e.g., "4")
    echo -n "$q" >> $CSV_FILE
    
    # Run the program 20 times
    for i in {1..20}
    do
        # Run the program.
        # 'grep' finds the line with the average wait time.
        # 'awk '{print $5}'' extracts ONLY the 5th word of the line (which is the time value).
        WAIT_TIME=$(./pc | grep "Average Wait Time" | awk '{print $5}')
        
        # Append the time value preceded by a comma (e.g., ,56.22)
        echo -n ",$WAIT_TIME" >> $CSV_FILE
    done
    
    # Add a newline to the CSV for the next Consumer set
    echo "" >> $CSV_FILE
done

echo "========================================================="
echo "All experiments completed successfully!"
echo "Data saved to: $CSV_FILE"
echo "You can now open it directly in Excel."
echo "========================================================="