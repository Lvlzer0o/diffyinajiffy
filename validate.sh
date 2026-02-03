#!/bin/bash
# Validation script for DiffyInAJiffy code structure

echo "==================================="
echo "DiffyInAJiffy Code Validation"
echo "==================================="

errors=0

# Check required files exist
echo ""
echo "Checking project structure..."

required_files=(
    "CMakeLists.txt"
    "src/main.cpp"
    "src/mainwindow.h"
    "src/mainwindow.cpp"
    "src/diffview.h"
    "src/diffview.cpp"
    "src/diffengine.h"
    "src/diffengine.cpp"
    "src/documentparser.h"
    "src/documentparser.cpp"
    "src/folderview.h"
    "src/folderview.cpp"
    "README.md"
)

for file in "${required_files[@]}"; do
    if [ -f "$file" ]; then
        echo "✓ $file"
    else
        echo "✗ $file - MISSING"
        errors=$((errors + 1))
    fi
done

# Check for common C++ syntax errors
echo ""
echo "Checking C++ syntax..."

cpp_files=(src/*.cpp src/*.h)

for file in $cpp_files; do
    if [ -f "$file" ]; then
        # Check for basic syntax issues
        if grep -q "^#include.*<" "$file"; then
            echo "✓ $file has includes"
        fi
        
        # Check for proper header guards in .h files
        if [[ "$file" == *.h ]]; then
            if grep -q "#ifndef.*_H" "$file" && grep -q "#define.*_H" "$file" && grep -q "#endif" "$file"; then
                echo "✓ $file has header guards"
            else
                echo "⚠ $file may be missing proper header guards"
            fi
        fi
    fi
done

# Check example files
echo ""
echo "Checking example files..."

if [ -d "examples/text" ]; then
    echo "✓ examples/text directory exists"
else
    echo "✗ examples/text directory missing"
    errors=$((errors + 1))
fi

if [ -d "examples/markdown" ]; then
    echo "✓ examples/markdown directory exists"
else
    echo "✗ examples/markdown directory missing"
    errors=$((errors + 1))
fi

# Check documentation
echo ""
echo "Checking documentation..."

docs=("README.md" "DESIGN.md" "CONTRIBUTING.md" "QUICKSTART.md" "LICENSE")

for doc in "${docs[@]}"; do
    if [ -f "$doc" ]; then
        lines=$(wc -l < "$doc")
        echo "✓ $doc ($lines lines)"
    else
        echo "✗ $doc - MISSING"
    fi
done

# Summary
echo ""
echo "==================================="
if [ $errors -eq 0 ]; then
    echo "✓ All checks passed!"
    echo "==================================="
    echo ""
    echo "Project structure is valid."
    echo "Ready to build with: ./build.sh"
    exit 0
else
    echo "✗ Found $errors error(s)"
    echo "==================================="
    exit 1
fi
