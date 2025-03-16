/*
 * Braggi - Builtin Functions
 * 
 * "Builtin functions are like the tools on a Texan's belt - 
 * always there when you need 'em, sharp as can be!"
 */

#include "braggi/builtins/builtins.h"
#include "braggi/braggi.h"
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

// Structure for the builtin registry
struct BraggiBuiltinRegistry {
    // This is just a placeholder for now
    int dummy;
};

// Create a new builtin registry
BraggiBuiltinRegistry* braggi_builtin_registry_create(void) {
    BraggiBuiltinRegistry* registry = (BraggiBuiltinRegistry*)malloc(sizeof(BraggiBuiltinRegistry));
    if (!registry) {
        return NULL;
    }
    
    // Initialize registry
    registry->dummy = 0;
    
    return registry;
}

// Destroy a builtin registry
void braggi_builtin_registry_destroy(BraggiBuiltinRegistry* registry) {
    if (!registry) {
        return;
    }
    
    // Free the registry itself
    free(registry);
}

// Initialize the standard library builtins
bool braggi_initialize_stdlib(BraggiBuiltinRegistry* registry) {
    if (!registry) {
        return false;
    }
    
    // This is just a stub implementation for now
    printf("Initializing stdlib builtins...\n");
    
    return true;
}

// Sample builtin functions (these would be real implementations in production)
static BraggiValue* builtin_print(BraggiValue** args, size_t arg_count, void* context) {
    // This is just a stub - it would print the values in args
    printf("Print builtin called with %zu args\n", arg_count);
    (void)args;
    (void)context;
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