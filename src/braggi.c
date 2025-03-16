/*
 * Braggi - Main Implementation
 * 
 * "The heart of a compiler is like the heart of Texas - 
 * big, bold, and ready for anything!" - Compiler Cowboy
 */

#include "braggi/braggi.h"
#include "braggi/error.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Initialize the Braggi compiler
BraggiResult braggi_init(void) {
    // Initialize error handling
    braggi_error_init();
    
    return BRAGGI_SUCCESS;
}

// Clean up the Braggi compiler
void braggi_cleanup(void) {
    // Clean up error handling
    braggi_error_cleanup();
}

// Get the Braggi version string
const char* braggi_version(void) {
    static char version[32];
    sprintf(version, "%d.%d.%d", BRAGGI_VERSION_MAJOR, BRAGGI_VERSION_MINOR, BRAGGI_VERSION_PATCH);
    return version;
}

// Compile a file
BraggiResult braggi_compile_file(BraggiContext* context, const char* filename) {
    if (!context || !filename) {
        return BRAGGI_ERROR_GENERAL;
    }
    
    // Load the file using the context API
    if (!braggi_context_load_file(context, filename)) {
        return BRAGGI_ERROR_FILE_NOT_FOUND;
    }
    
    // Compile the source
    if (!braggi_context_compile(context)) {
        return BRAGGI_ERROR_GENERAL;
    }
    
    return BRAGGI_SUCCESS;
}

// Compile a string
BraggiResult braggi_compile_string(BraggiContext* context, const char* name, const char* content) {
    if (!context || !name || !content) {
        return BRAGGI_ERROR_GENERAL;
    }
    
    // Load the string using the context API
    if (!braggi_context_load_string(context, content, name)) {
        return BRAGGI_ERROR_MEMORY;
    }
    
    // Compile the source
    if (!braggi_context_compile(context)) {
        return BRAGGI_ERROR_GENERAL;
    }
    
    return BRAGGI_SUCCESS;
}

// Eval a string (for REPL)
BraggiResult braggi_eval(BraggiContext* context, const char* content, char** result) {
    if (!context || !content || !result) {
        return BRAGGI_ERROR_GENERAL;
    }
    
    // Compile the string
    BraggiResult compile_result = braggi_compile_string(context, "<repl>", content);
    if (compile_result != BRAGGI_SUCCESS) {
        return compile_result;
    }
    
    // TODO: Implement evaluation
    // This is just a stub for now
    
    // Return a simple result for now
    *result = strdup("Evaluation not implemented yet");
    
    return BRAGGI_SUCCESS;
}

// Print an error message for a result code
void braggi_print_error(BraggiResult result) {
    const char* message;
    
    switch (result) {
        case BRAGGI_SUCCESS:
            message = "Success";
            break;
        case BRAGGI_ERROR_GENERAL:
            message = "General error";
            break;
        case BRAGGI_ERROR_FILE_NOT_FOUND:
            message = "File not found";
            break;
        case BRAGGI_ERROR_SYNTAX:
            message = "Syntax error";
            break;
        case BRAGGI_ERROR_SEMANTIC:
            message = "Semantic error";
            break;
        case BRAGGI_ERROR_CODEGEN:
            message = "Code generation error";
            break;
        case BRAGGI_ERROR_SYSTEM:
            message = "System error";
            break;
        case BRAGGI_ERROR_MEMORY:
            message = "Memory allocation error";
            break;
        default:
            message = "Unknown error";
            break;
    }
    
    fprintf(stderr, "Braggi error: %s\n", message);
}

// Standalone compile function for simpler use cases
// This is a separate function that creates its own context
int braggi_compile_standalone(const char* content, const char* name) {
    if (!content) {
        return -1;
    }
    
    // Create a compiler context
    BraggiContext* context = braggi_context_create();
    if (!context) {
        return -1;
    }
    
    // Load the source from string
    if (!braggi_context_load_string(context, content, name ? name : "<string>")) {
        braggi_context_destroy(context);
        return -1;
    }
    
    // Compile the source
    bool success = braggi_context_compile(context);
    
    // Get the status code
    int status = braggi_context_get_status(context);
    
    // Clean up the context
    braggi_context_destroy(context);
    
    return success ? status : -1;
} 