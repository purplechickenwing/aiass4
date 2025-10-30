#!/bin/bash

# Check if correct number of arguments provided
if [ "$#" -ne 2 ]; then
    echo "Usage: ./run.sh <network.bif> <data.dat>"
    exit 1
fi

# Run the compiled program
./solution "$1" "$2"

# Check if execution was successful
if [ $? -eq 0 ]; then
    exit 0
else
    echo "Execution failed"
    exit 1
fi
