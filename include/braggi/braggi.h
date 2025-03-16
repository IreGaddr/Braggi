/*
 * Braggi - Main Header
 * 
 * "A good header file is like a good welcome sign to your Texas ranch - 
 * it tells folks what they're in for!" - Texirish Programming Wisdom
 */

#ifndef BRAGGI_H
#define BRAGGI_H

#include <stdbool.h>
#include <stddef.h>

/* First include context definition to avoid forward reference issues */
#include "braggi/braggi_context.h"

/* Now include the rest */
#include "braggi/source.h"
#include "braggi/error.h"
#include "braggi/token.h"

/* Library version */
#define BRAGGI_VERSION_MAJOR 0
#define BRAGGI_VERSION_MINOR 1
#define BRAGGI_VERSION_PATCH 0

/* Compiler result codes */
typedef enum {
    BRAGGI_SUCCESS = 0,
    BRAGGI_ERROR_GENERAL,
    BRAGGI_ERROR_FILE_NOT_FOUND,
    BRAGGI_ERROR_SYNTAX,
    BRAGGI_ERROR_SEMANTIC,
    BRAGGI_ERROR_CODEGEN,
    BRAGGI_ERROR_SYSTEM,
    BRAGGI_ERROR_MEMORY
} BraggiResult;

/* Function prototypes */

/* Initialize the Braggi compiler */
BraggiResult braggi_init(void);

/* Clean up the Braggi compiler */
void braggi_cleanup(void);

/* Get the Braggi version string */
const char* braggi_version(void);

/* Create a new compiler context */
BraggiContext* braggi_create_context(void);

/* Destroy a compiler context */
void braggi_destroy_context(BraggiContext* context);

/* Compile a file */
BraggiResult braggi_compile_file(BraggiContext* context, const char* filename);

/* Compile a string */
BraggiResult braggi_compile_string(BraggiContext* context, const char* name, const char* content);

/* Eval a string (for REPL) */
BraggiResult braggi_eval(BraggiContext* context, const char* content, char** result);

/* Print an error message for a result code */
void braggi_print_error(BraggiResult result);

// Forward declarations
typedef struct BraggiContext BraggiContext;

/* Add a proper definition for BraggiValue */
#ifndef BRAGGI_VALUE_DEFINED
typedef struct BraggiValue {
    int type;        /* Value type */
    void* data;      /* Value data */
} BraggiValue;
#define BRAGGI_VALUE_DEFINED
#endif

/**
 * Create a new Braggi context
 *
 * @return A new Braggi context, or NULL if creation failed
 */
BraggiContext* braggi_context_create(void);

/**
 * Destroy a Braggi context and free all associated resources
 *
 * @param context The context to destroy
 */
void braggi_context_destroy(BraggiContext* context);

/* Process a source file in the context */
bool braggi_context_process_source(BraggiContext* context, Source* source);

/**
 * Get the last error from a context
 *
 * @param context The Braggi context
 * @return The last error, or NULL if no error occurred
 */
const struct Error* braggi_context_get_last_error(BraggiContext* context);

#endif /* BRAGGI_H */ 