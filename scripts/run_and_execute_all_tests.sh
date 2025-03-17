#!/bin/bash

# run_and_execute_all_tests.sh - Runs all Braggi tests and executes the generated assembly files
# 
# This script runs all Braggi tests, generates the assembly output files,
# and then executes each output file.
#
# Usage: ./run_and_execute_all_tests.sh
#
# Y'all come back now, ya hear? 🤠

set -e  # Exit on error

echo "🤠 Howdy! Fixin' to run all Braggi tests and execute the outputs..."

# Run all tests
echo "🧪 Running all tests..."
./scripts/run_tests.sh

# Get a list of all output files
OUTPUT_DIR="build/test_outputs"
OUTPUT_FILES=$(ls "$OUTPUT_DIR"/*.out 2>/dev/null || echo "")

if [ -z "$OUTPUT_FILES" ]; then
    echo "❌ No output files found in $OUTPUT_DIR"
    exit 1
fi

echo "🎯 Found $(echo "$OUTPUT_FILES" | wc -l) output files"

# Execute each output file
for OUTPUT_FILE in $OUTPUT_FILES; do
    echo ""
    echo "🚀 Executing $(basename "$OUTPUT_FILE")..."
    ./scripts/assemble_and_run.sh "$OUTPUT_FILE"
done

echo ""
echo "🎉 All tests executed successfully! Y'all come back now, ya hear? 🤠" 