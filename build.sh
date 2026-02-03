#!/bin/bash
# Build script for DiffyInAJiffy

set -e

echo "==================================="
echo "Building DiffyInAJiffy"
echo "==================================="

# Check for required tools
if ! command -v cmake &> /dev/null; then
    echo "Error: CMake not found. Please install CMake 3.16 or higher."
    exit 1
fi

if ! command -v g++ &> /dev/null; then
    echo "Error: g++ not found. Please install a C++ compiler."
    exit 1
fi

# Check for Qt6
if ! pkg-config --exists Qt6Core Qt6Gui Qt6Widgets; then
    echo "Error: Qt6 not found. Please install Qt6 development packages."
    echo "  Ubuntu/Debian: sudo apt-get install qt6-base-dev"
    echo "  Fedora: sudo dnf install qt6-qtbase-devel"
    echo "  Arch: sudo pacman -S qt6-base"
    exit 1
fi

echo "✓ CMake found: $(cmake --version | head -1)"
echo "✓ Compiler found: $(g++ --version | head -1)"
echo "✓ Qt6 found: $(pkg-config --modversion Qt6Core)"

# Check for optional dependencies
if pkg-config --exists poppler-qt6; then
    echo "✓ Poppler-Qt6 found: PDF support enabled"
else
    echo "⚠ Poppler-Qt6 not found: PDF support will be disabled"
fi

# Create build directory
mkdir -p build
cd build

# Configure
echo ""
echo "Configuring..."
cmake ..

# Build
echo ""
echo "Building..."
make -j$(nproc)

echo ""
echo "==================================="
echo "Build completed successfully!"
echo "==================================="
echo ""
echo "To install: sudo make install"
echo "To run: ./diffyinajiffy"
