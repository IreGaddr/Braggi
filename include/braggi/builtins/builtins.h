#ifndef BRAGGI_BUILTINS_H
#define BRAGGI_BUILTINS_H

/**
 * Braggi - Builtin Functions Interface
 * 
 * "The best cowboys ain't the ones with the fanciest lasso,
 * but the ones who know which built-in functions to call!" - Texas Programming Proverb
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>  // For size_t

// Forward declarations
typedef struct BraggiContext BraggiContext;
typedef struct BraggiValue BraggiValue;
typedef struct BraggiBuiltinRegistry BraggiBuiltinRegistry;

// Builtin function type
typedef struct BuiltinFunction {
    const char* name;
    int (*func)(BraggiContext* context);
    const char* description;
} BuiltinFunction;

// Define the proper builtin function type
typedef BraggiValue* (*BraggiBuiltinFunc)(BraggiValue** args, size_t arg_count, void* context);

/**
 * Look up a builtin function by name
 * 
 * @param registry The builtin registry to search in
 * @param name The name of the builtin function to look up
 * @param out_context If not NULL, will be set to the function's context
 * @return Function pointer if found, NULL otherwise
 */
BraggiBuiltinFunc braggi_builtin_registry_lookup(BraggiBuiltinRegistry* registry, const char* name, void** out_context);

/**
 * Initialize the builtin function registry
 * 
 * @return true if successful, false otherwise
 */
bool braggi_initialize_builtins(void);

// Registry creation and destruction
BraggiBuiltinRegistry* braggi_builtin_registry_create(void);
void braggi_builtin_registry_destroy(BraggiBuiltinRegistry* registry);

#endif /* BRAGGI_BUILTINS_H */ 