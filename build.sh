#!/bin/bash

set -e

BUILD_DIR="build"
CMAKE_ARGS="-DCMAKE_BUILD_TYPE=Release"

if [[ "$@" == *"c"* ]]; then
    rm -rf "$BUILD_DIR/CMakeCache.txt" "$BUILD_DIR/CMakeFiles"
    rm -rf "$BUILD_DIR/"
fi

if [[ "$@" == *"t"* ]]; then
    CMAKE_ARGS="$CMAKE_ARGS -DBUILD_TESTS=ON -DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG"
fi

if [[ "$@" == *"b"* ]]; then
    CMAKE_ARGS="$CMAKE_ARGS -DBUILD_BENCHMARKS=ON"
fi

if [[ "$@" == *"d"* ]]; then
    CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_BUILD_TYPE=Debug"
fi

echo "🔨 Creating build directory..."
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo "⚙️ Running CMake..."
cmake .. $CMAKE_ARGS

echo "🚀 Compiling project..."
make -j$(nproc)

echo "✅ Build completed!"

