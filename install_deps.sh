#!/bin/bash

# For Ubuntu/Debian
if [ -f /etc/debian_version ]; then
    sudo apt-get update
    sudo apt-get install -y \
        build-essential \
        cmake \
        libboost-all-dev \
        libssl-dev \
        zlib1g-dev \
        wget \
        curl
