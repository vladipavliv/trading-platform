#!/bin/bash

clear

set -e

BUILD_DIR="build"

if [ "$1" == "c" ]; then
    rm -rf CMakeCache.txt CMakeFiles
    rm -rf build/
fi

echo "🔨 Creating build directory..."
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo "⚙️ Running CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release

echo "🚀 Compiling project..."
make -j$(nproc)

echo "✅ Build completed!"

if [ "$1" == "s" ]; then
    clear
    ./hft_server
elif [ "$1" == "t" ]; then
    clear
    ./hft_trader
fi