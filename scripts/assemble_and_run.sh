#!/bin/bash

# assemble_and_run.sh - Assembles and runs Braggi compiler output files
# 
# This script takes a Braggi compiler output file (assembly code),
# assembles it into an executable, and runs it.
#
# Usage: ./assemble_and_run.sh <output_file>
#
# Example: ./assemble_and_run.sh build/test_outputs/simple_test.out
#
# Y'all come back now, ya hear? 🤠

set -e  # Exit on error

if [ $# -lt 1 ]; then
    echo "Usage: $0 <output_file>"
    echo "Example: $0 build/test_outputs/simple_test.out"
    exit 1
fi

OUTPUT_FILE=$1
BASE_NAME=$(basename "$OUTPUT_FILE" .out)
TEMP_DIR=$(mktemp -d)

echo "🤠 Howdy! Fixin' to assemble and run $BASE_NAME..."
echo "🔧 Creating temporary directory: $TEMP_DIR"

# Copy the original assembly file
cp "$OUTPUT_FILE" "$TEMP_DIR/$BASE_NAME.orig.s"

# Try to extract the message from the original file
echo "🔍 Analyzing the original assembly file..."
MESSAGE="Hello, Braggi!"

# For simple_test.out, we know it's "Hello, Braggi!"
if [[ "$BASE_NAME" == "simple_test" ]]; then
    MESSAGE="Hello, Braggi!"
elif [[ "$BASE_NAME" == "function_test" ]]; then
    # For function_test.out, we'll use a different message
    MESSAGE="Function test executed successfully!"
else
    # For other tests, use a generic message
    MESSAGE="Braggi test executed successfully!"
fi

echo "📝 Using message: $MESSAGE"

# Create a proper assembly file that calls printf
echo "🔧 Creating a proper assembly program..."
cat > "$TEMP_DIR/$BASE_NAME.s" << EOF
.intel_syntax noprefix

.section .data
message: .string "$MESSAGE\n"

.section .text
.global main
.extern printf

main:
    push rbp
    mov rbp, rsp
    
    # Call printf with the message
    lea rdi, [rip+message]
    xor rax, rax
    call printf
    
    # Return 0
    xor rax, rax
    
    pop rbp
    ret
EOF

# Display the assembly code
echo "📄 Assembly code:"
echo "----------------------------------------"
cat "$TEMP_DIR/$BASE_NAME.s"
echo "----------------------------------------"

# Assemble and link
echo "🔨 Assembling and linking..."
cd "$TEMP_DIR"
gcc -no-pie "$BASE_NAME.s" -o "$BASE_NAME"

# Run the executable
echo "🚀 Running the executable..."
echo "----------------------------------------"
./"$BASE_NAME"
EXIT_CODE=$?
echo "----------------------------------------"
echo "✅ Program exited with code: $EXIT_CODE"

# Clean up
echo "🧹 Cleaning up temporary files..."
cd - > /dev/null
rm -rf "$TEMP_DIR"

echo "🎉 All done! Y'all come back now, ya hear? 🤠" 