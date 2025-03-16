#!/bin/bash

# Braggi build and test script
# "If yer gonna build somethin', ya might as well test it too!" - Texirish Build Philosophy

set -e  # Exit on error

echo "==== BRAGGI BUILD AND TEST SCRIPT ===="
echo "Creating build directory..."
mkdir -p build
cd build

echo "Configuring with CMake..."
cmake ..

echo "Building..."
cmake --build .

echo "Running tests..."
ctest -V

echo "==== BUILD AND TESTS COMPLETED SUCCESSFULLY ===="
echo "You can now run the REPL with:"
echo "  ./bin/braggi-repl"

# Make script executable
chmod +x ../build_and_test.sh 