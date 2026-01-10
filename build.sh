#!/bin/bash

set -e

BUILD_DIR="build"
CMAKE_ARGS="-DCMAKE_BUILD_TYPE=Release -DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_ERROR -DSERIALIZATION=SBE"

for arg in "$@"; do
  case "$arg" in
    tel)
      CMAKE_ARGS="$CMAKE_ARGS -DTELEMETRY_ENABLED=ON"
      ;;
    c|clean)
      rm -rf "$BUILD_DIR/"
      ;;
    t|tests)
      CMAKE_ARGS="$CMAKE_ARGS -DBUILD_TESTS=ON"
      ;;
    b|benchmarks)
      CMAKE_ARGS="$CMAKE_ARGS -DBUILD_BENCHMARKS=ON"
      ;;
    d|debug)
      CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_BUILD_TYPE=Debug -DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE"
      ;;
    s|shm)
      CMAKE_ARGS="$CMAKE_ARGS -DCOMM_TYPE_SHM=ON"
      ;;
    p|profile)
      CMAKE_ARGS="$CMAKE_ARGS -DPROFILING=ON"
      ;;
    valgr)
      CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_BUILD_TYPE=CMAKE_CXX_FLAGS_DEBUG_VALGRIND -DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG"
      ;;
  esac
done

echo "🔨 Creating build directory..."
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo "⚙️ Running CMake..."
cmake .. $CMAKE_ARGS

echo "🚀 Compiling project..."
make -j$(nproc)

echo "✅ Build completed!"

