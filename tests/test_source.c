/*
 * Braggi - Source Code Handler Tests
 * 
 * "If ya ain't testin' yer code, yer just hopin' it works - 
 * and hope ain't a strategy, partner!" - Texas Testing Philosophy
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "braggi/source.h"
#include "braggi/source_position.h"

// Assert macro for testing
#define ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "Assertion failed: %s\n", message); \
            exit(1); \
        } \
    } while (0)

// Test function prototypes
void test_source_file_create(void);
void test_source_position(void);
void test_source_snippets(void);

// Test fixture - sample source code
const char* test_source = 
    "// This is a test file\n"
    "function main() {\n"
    "    let x = 42;\n"
    "    return x;\n"
    "}\n";

void test_source_file_create(void) {
    printf("Testing source file creation...\n");
    
    // Create a source file from a string
    const char* test_source = "line1\nline2\nline3\n";
    
    // Fix: Use the proper function and Source* type instead of SourceFile*
    Source* source = braggi_source_string_create(test_source, "test.bg");
    ASSERT(source != NULL, "Source creation failed");
    
    // Test file properties
    ASSERT(source->num_lines == 3, "Expected 3 lines");
    ASSERT(strcmp(source->filename, "test.bg") == 0, "Filename mismatch");
    ASSERT(strcmp(source->lines[0], "line1") == 0, "Line 1 content mismatch");
    ASSERT(strcmp(source->lines[1], "line2") == 0, "Line 2 content mismatch");
    ASSERT(strcmp(source->lines[2], "line3") == 0, "Line 3 content mismatch");
    
    // Fix: Use the correct function signature for destroy
    braggi_source_file_destroy(source);
    
    printf("Source file creation test passed\n");
}

void test_source_position(void) {
    printf("Testing source positions...\n");
    
    // Create a source file from a string
    const char* test_source = "line1\nline2\nline3\nline4\nline5\n";
    
    // Fix: Use the proper function and Source* type
    Source* source = braggi_source_string_create(test_source, "test.bg");
    ASSERT(source != NULL, "Source creation failed");
    
    // Fix: Use the proper function signature with all 5 parameters
    SourcePosition pos = braggi_source_position_create(1, 30, 1, 0, 5);
    ASSERT(pos.file_id == 1, "File ID mismatch");
    ASSERT(pos.line == 30, "Line mismatch");
    ASSERT(pos.column == 1, "Column mismatch");
    ASSERT(pos.offset == 0, "Offset mismatch");
    ASSERT(pos.length == 5, "Length mismatch");
    
    // Test position from line/column
    pos = braggi_source_position_from_line_col(2, 3);
    ASSERT(pos.line == 2, "Line mismatch");
    ASSERT(pos.column == 3, "Column mismatch");
    
    // Fix: Use correct function signature for destroy
    braggi_source_file_destroy(source);
    
    printf("Source position test passed\n");
}

void test_source_snippets(void) {
    printf("Testing source snippets...\n");
    
    // Create a source file from a string
    const char* test_source = "line1\nline2\nline3\nline4\nline5\n";
    
    // Fix: Use the proper function and Source* type
    Source* source = braggi_source_string_create(test_source, "test.bg");
    ASSERT(source != NULL, "Source creation failed");
    
    // Fix: Use the proper function signature with all 5 parameters
    SourcePosition pos = braggi_source_position_create(1, 3, 2, 12, 5);
    
    // Create a SourceFile from Source to test snippet generation
    SourceFile file;
    file.filename = source->filename;
    file.content = (char*)test_source;
    file.length = strlen(test_source);
    
    // Create a line map (simplified for testing)
    uint32_t line_map[5] = {0, 6, 12, 18, 24};  // Offsets for each line
    file.line_map = line_map;
    file.line_count = 5;
    file.file_id = source->file_id;
    
    // Get a snippet
    char* snippet = braggi_source_position_get_snippet(&file, &pos, 1);
    ASSERT(snippet != NULL, "Snippet creation failed");
    
    // Print the snippet
    printf("Snippet:\n%s\n", snippet);
    
    // Clean up
    free(snippet);
    braggi_source_file_destroy(source);
    
    printf("Source snippet test passed\n");
}

int main(void) {
    printf("=== Running Source Tests ===\n");
    
    // Run all tests
    test_source_file_create();
    test_source_position();
    test_source_snippets();
    
    printf("=== All Source Tests Passed ===\n");
    return 0;
} 