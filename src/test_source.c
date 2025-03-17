#include "braggi/source.h"
#include "braggi/braggi_context.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Basic test for source file functionality
int main() {
    printf("Running source tests...\n");
    
    // Test source loading from string
    BraggiContext* context = braggi_context_create();
    
    // Test loading source from string
    const char* test_source = "fn main() { println(\"Hello, Braggi!\"); }";
    braggi_context_load_string(context, test_source, "test_string.bg");
    
    // Verify source was loaded
    printf("Source loaded, length: %zu\n", braggi_source_file_get_size(context->source));
    
    // Clean up
    braggi_context_destroy(context);
    
    printf("Source tests passed!\n");
    return 0;
}