#!/bin/bash
set -e

# Usage: ./build.sh [--clean]
if [[ "$1" == "--clean" ]]; then
    echo "Cleaning old build files..."
    rm -rf build
fi

# Create build directory
mkdir -p build

cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

cmake --build build

echo "---------------------------------------"
echo "Build Successful! Executable: build/limiter_app"
