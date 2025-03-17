#!/bin/bash

# run_tests.sh - Script to run Braggi tests and optionally update expected outputs
# 
# Usage: ./scripts/run_tests.sh [--update-expected]
#
# Options:
#   --update-expected: Update the expected output files with the current test results

set -e  # Exit on error

# Determine script directory and project root
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$( cd "$SCRIPT_DIR/.." && pwd )"
BUILD_DIR="$PROJECT_ROOT/build"
TEST_DIR="$PROJECT_ROOT/tests"
EXPECTED_DIR="$TEST_DIR/expected_outputs"
OUTPUT_DIR="$BUILD_DIR/test_outputs"

# Create output directory if it doesn't exist
mkdir -p "$OUTPUT_DIR"

# Parse arguments
UPDATE_EXPECTED=0
for arg in "$@"; do
  case $arg in
    --update-expected)
      UPDATE_EXPECTED=1
      shift
      ;;
    *)
      # Unknown option
      ;;
  esac
done

# Make sure the project is built
echo "Building Braggi..."
cd "$PROJECT_ROOT"
./scripts/run_build.sh

# Run the tests
echo "Running tests..."
cd "$BUILD_DIR"
bin/braggi_test_harness --test-dir="$TEST_DIR" --output-dir="$OUTPUT_DIR" --verbose

# If requested, update the expected output files
if [ $UPDATE_EXPECTED -eq 1 ]; then
  echo "Updating expected output files..."
  mkdir -p "$EXPECTED_DIR"
  cp "$OUTPUT_DIR"/*.out "$EXPECTED_DIR/"
  echo "Expected output files updated!"
fi

echo "All done! Y'all come back now, ya hear? 🤠" 