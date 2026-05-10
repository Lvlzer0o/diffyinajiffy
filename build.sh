#!/usr/bin/env bash
# Convenience build script for DiffyInAJiffy.

set -euo pipefail

echo "==================================="
echo "Building DiffyInAJiffy"
echo "==================================="

# Check for required tools. CMake handles compiler and Qt discovery across platforms.
if ! command -v cmake >/dev/null 2>&1; then
    echo "Error: CMake 3.16 or higher is required."
    exit 1
fi

echo "CMake: $(cmake --version | head -1)"

# Configure
echo ""
echo "Configuring..."
cmake -S . -B build

# Build
echo ""
echo "Building..."
cmake --build build --parallel

echo ""
echo "==================================="
echo "Build completed successfully."
echo "==================================="
echo ""
echo "To install: cmake --install build"
echo "To run: ./build/diffyinajiffy"
