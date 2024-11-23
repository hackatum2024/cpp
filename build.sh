#!/bin/bash

# Script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
cd "$SCRIPT_DIR"

# Ensure crow header exists
if [ ! -f "deps/crow_all.h" ]; then
    echo "Downloading Crow framework..."
    mkdir -p deps
    curl -L https://github.com/CrowCpp/Crow/releases/download/v1.0%2B5/crow_all.h -o deps/crow_all.h
    
    if [ ! -f "deps/crow_all.h" ]; then
        echo "Error: Failed to download Crow framework"
        exit 1
    fi
fi

# Create build directory
echo "Creating build directory..."
mkdir -p build
cd build

# Configure and build
echo "Configuring with CMake..."
cmake .. || { echo "CMake configuration failed"; exit 1; }

echo "Building..."
make -j$(nproc) || { echo "Build failed"; exit 1; }

if [ -f "car_rental_backend" ]; then
    echo "Build successful! Binary is in build/car_rental_backend"
else
    echo "Error: Binary not created"
    exit 1
fi
