#!/bin/bash

# Check if the script name is provided as an argument
if [[ -z $1 ]]; then
    echo "Usage: $0 <script_name>"
    exit 1
fi

# Assign the first argument to SCRIPT_NAME
SCRIPT_NAME=$1

# Check if the directory exists and remove it if it does
if [[ -d "test.app" ]]; then
    echo "Directory test.app already exists. Removing..."
    rm -rf test.app

    # Check if removal was successful
    if [[ $? -ne 0 ]]; then
        echo "Failed to remove test.app directory."
        exit 1
    fi
fi

# Creating required directory
mkdir -p test.app/Contents/MacOS

# Compiling the objective-C file
clang -c fssimplewindowobjc.m -o fssimplewindowobjc.o
clang -c yssimplesound.m -o yssimplesound.o
clang++ -c yspng.cpp -o yspng.o

# Check if the compilation was successful
if [[ $? -ne 0 ]]; then
    echo "Compilation of files failed."
    exit 1
fi

# Compiling the C++ files with the frameworks
clang++ "$SCRIPT_NAME" yspng.o yssimplesound.o yssimplesound.cpp fssimplewindowobjc.o fssimplewindowcpp.cpp -framework Cocoa -framework OpenGL -framework AVFoundation -o test.app/Contents/MacOS/exe

# Check if the final compilation was successful
if [[ $? -ne 0 ]]; then
    echo "Final compilation failed."
    exit 1
fi

echo "Compilation Successful."
