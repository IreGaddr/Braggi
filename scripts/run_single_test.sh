#!/bin/bash

# run_single_test.sh - Script to run a single Braggi test
# 
# Usage: ./scripts/run_single_test.sh <test_name> [--update-expected]
#
# Arguments:
#   test_name: The name of the test to run (without the .bg extension)
#
# Options:
#   --update-expected: Update the expected output file with the current test result

set -e  # Exit on error

# Check if test name is provided
if [ $# -lt 1 ]; then
  echo "Error: Test name is required"
  echo "Usage: ./scripts/run_single_test.sh <test_name> [--update-expected]"
  exit 1
fi

TEST_NAME="$1"
shift

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

# Check if test file exists
TEST_FILE="$TEST_DIR/${TEST_NAME}.bg"
if [ ! -f "$TEST_FILE" ]; then
  echo "Error: Test file not found: $TEST_FILE"
  exit 1
fi

# Make sure the project is built
echo "Building Braggi..."
cd "$PROJECT_ROOT"
./scripts/run_build.sh

# Run the test
echo "Running test: $TEST_NAME..."
cd "$BUILD_DIR"
bin/braggi_test_harness --test-dir="$TEST_DIR" --output-dir="$OUTPUT_DIR" --test="$TEST_NAME" --verbose

# If requested, update the expected output file
if [ $UPDATE_EXPECTED -eq 1 ]; then
  echo "Updating expected output file..."
  mkdir -p "$EXPECTED_DIR"
  cp "$OUTPUT_DIR/${TEST_NAME}.out" "$EXPECTED_DIR/"
  echo "Expected output file updated!"
fi

echo "All done! Y'all come back now, ya hear? 🤠" 