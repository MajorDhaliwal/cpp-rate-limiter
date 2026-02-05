#!/bin/bash
set -e

# Usage: ./build.sh [--clean]
if [[ "$1" == "--clean" ]]; then
    echo "Cleaning old build files..."
    rm -rf build
fi

mkdir -p build
cd build

# We use Release mode for the 10k+ RPS performance
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build using all CPU cores
make -j$(nproc)

echo "---------------------------------------"
echo "Build Successful! Executable: build/limiter_app"