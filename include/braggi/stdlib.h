/*
 * Braggi - Standard Library Interface
 * 
 * "A standard library is the handshake between your compiler and the real world -
 * strong, firm, and revealing of your character!"
 */

#ifndef BRAGGI_STDLIB_H
#define BRAGGI_STDLIB_H

/**
 * Braggi - Standard Library Functions
 * 
 * "A good standard library is like having your grandpappy's secret recipes -
 * makes everything taste better without reinventin' the wheel!"
 * - Texan Programming Wisdom
 */

#include "braggi/braggi_context.h"  // Include the complete context definition
#include "braggi/braggi.h"
#include <stdbool.h>
#include <stddef.h>

// Forward declarations
typedef struct BraggiContext BraggiContext;
typedef struct BraggiValue BraggiValue;
typedef struct BraggiBuiltinRegistry BraggiBuiltinRegistry;

// Builtin function type - make sure it's consistent with builtins.h
typedef BraggiValue* (*BraggiBuiltinFunc)(BraggiValue** args, size_t arg_count, void* context);

// Module loading functions
bool braggi_stdlib_load_module(BraggiContext* context, const char* module_name);
char* braggi_stdlib_find_file(const char* name);

// Standard library initialization/cleanup
bool braggi_stdlib_initialize(BraggiContext* context);
void braggi_stdlib_cleanup(BraggiContext* context);

// Debug utility function
void braggi_stdlib_debug_paths(void);

/**
 * Look up a builtin function by name
 *
 * @param context The Braggi context
 * @param name The name of the builtin function
 * @return Function pointer if found, NULL otherwise
 */
BraggiBuiltinFunc braggi_stdlib_lookup_builtin(BraggiContext* context, const char* name);

#endif /* BRAGGI_STDLIB_H */ 