#!/bin/bash

# Check if we're root (needed for port 80)
if [ "$EUID" -ne 0 ]; then
    echo "Please run as root (needed for port 80)"
    exit 1
fi

# Run the application
./build/car_rental_backend
