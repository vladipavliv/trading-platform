#!/bin/bash

clear

set -e

BUILD_DIR="build"

if [ "$1" == "c" ]; then
    rm -rf CMakeCache.txt CMakeFiles
    rm -rf build/
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
