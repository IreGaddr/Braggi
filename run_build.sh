#!/bin/bash

# Braggi simplified build script
# "If at first ya don't succeed, try rebuildin' from scratch!" - Texas Build Philosophy

set -e  # Exit on error

echo "==== BRAGGI SIMPLIFIED BUILD SCRIPT ===="

# Remove the build directory
echo "Cleaning build directory..."
rm -rf build

# Create a clean build directory
echo "Creating fresh build directory..."
mkdir -p build
cd build

# Configure with CMake
echo "Configuring with CMake..."
cmake ..

# Build
echo "Building..."
cmake --build . --verbose

echo "==== BUILD COMPLETED ====" 