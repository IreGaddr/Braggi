/*
 * Braggi - Builtin Functions
 * 
 * "Builtin functions are like the tools on a Texan's belt - 
 * always there when you need 'em, sharp as can be!"
 */

#include "braggi/builtins/builtins.h"
#include "braggi/braggi.h"
#include "braggi/stdlib.h"  // Include stdlib.h to use its registry functions
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Define a simple BraggiValue struct if it's not already defined elsewhere
#ifndef BRAGGI_VALUE_DEFINED
struct BraggiValue {
    int type;  // Simple placeholder for now
    void* data;
};
#define BRAGGI_VALUE_DEFINED
#endif

// Structure for the builtin registry is defined in stdlib.h
// Avoid duplicating the registry functions here

// Static function implementations for builtins

// Create a builtin print function
static BraggiValue* builtin_print(BraggiValue** args, size_t arg_count, void* context) {
    // This is just a placeholder implementation
    printf("builtin_print called with %zu arguments\n", arg_count);
    
    (void)args;    // Suppress unused parameter warning
    (void)context; // Suppress unused parameter warning
    
    return NULL;
}

static BraggiValue* builtin_exit(BraggiValue** args, size_t arg_count, void* context) {
    // This is just a stub - it would exit the program
    printf("Exit builtin called with %zu args\n", arg_count);
    (void)args;
    (void)context;
    return NULL;
}

// Lookup implementation that matches the declaration
BraggiBuiltinFunc braggi_builtin_registry_lookup(BraggiBuiltinRegistry* registry, const char* name, void** out_context) {
    if (!registry || !name) {
        return NULL;
    }
    
    // Simple stub implementation - in a real implementation we'd search the registry
    if (strcmp(name, "print") == 0) {
        if (out_context) *out_context = NULL;
        return builtin_print;
    }
    else if (strcmp(name, "exit") == 0) {
        if (out_context) *out_context = NULL;
        return builtin_exit;
    }
    
    // Not found
    return NULL;
}

/**
 * Initialize the builtin function registry
 * 
 * @return true if successful, false otherwise
 */
bool braggi_initialize_builtins(void) {
    // Nothing to initialize yet
    return true;
} 