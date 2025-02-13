#!/bin/bash

clear

set -e

BUILD_DIR="build"

echo "🔨 Creating build directory..."
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo "⚙️ Running CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release

echo "🚀 Compiling project..."
make -j$(nproc)

echo "✅ Build completed!"
