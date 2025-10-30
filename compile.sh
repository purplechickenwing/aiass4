#!/bin/bash

# Compile the C++ program using the starter code
g++ -std=c++11 -O2 -o solution solution.cpp

# Check if compilation was successful
if [ $? -eq 0 ]; then
    echo "Compilation successful"
    exit 0
else
    echo "Compilation failed"
    exit 1
fi
