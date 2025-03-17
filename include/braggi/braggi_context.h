/*
 * Braggi Context - Central state management
 * 
 * "In Texas, we keep all our important stuff in one place - our hats on our heads,
 * and our program state in the context!" - Irish-Texan Programming Wisdom
 */

#ifndef BRAGGI_CONTEXT_H
#define BRAGGI_CONTEXT_H

/**
 * Braggi - Context Definitions
 * 
 * "The context is where the magic happens - like the kitchen of a Texas cookout,
 * it's where all the ingredients come together!" - Texirish Coding Wisdom
 */

#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include "braggi/region.h"
#include "braggi/source.h"
#include "braggi/error.h"  /* Include for ErrorSeverity and ErrorCategory enums */
#include "braggi/token_ecs.h"

/* Forward declarations - no longer redefined Vector */
typedef struct ErrorHandler ErrorHandler;
typedef struct SymbolTable SymbolTable;
typedef struct EntropyField EntropyField;
typedef struct RegionManager RegionManager;
typedef struct TokenManager TokenManager;
typedef struct Error Error;
typedef struct Vector Vector;
typedef struct ECSWorld ECSWorld;
typedef struct TokenPropagator TokenPropagator;

/* Execution flags */
#define BRAGGI_FLAG_VERBOSE   0x0001  /* Enable verbose output */
#define BRAGGI_FLAG_DEBUG     0x0002  /* Enable debug mode */
#define BRAGGI_FLAG_STRICT    0x0004  /* Enable strict mode */
#define BRAGGI_FLAG_TEST      0x0008  /* Run in test mode */
#define BRAGGI_FLAG_OPTIMIZE  0x0010  /* Enable optimizations */
#define BRAGGI_FLAG_CODEGEN_CLEANUP_IN_PROGRESS 0x0020 /* Indicates codegen cleanup is in progress */
#define BRAGGI_FLAG_FINAL_CLEANUP 0x0040 /* Indicates final cleanup is in progress, skip validation checks */

/* Compiler options structure */
typedef struct BraggiOptions {
    int optimization_level;    // Optimization level (0-3)
    bool enable_debug;         // Include debug symbols
    bool verbose_output;       // Verbose output during compilation
    int error_limit;           // Maximum number of errors before stopping
    size_t memory_limit_kb;    // Memory limit in KB
} BraggiOptions;

/**
 * The main context structure for the Braggi compiler
 */
typedef struct BraggiContext {
    Region* main_region;       /* Main memory region */
    Source* source;            /* Current source */
    SymbolTable* symbols;      /* Global symbol table */
    ErrorHandler* error_handler;       // Error tracking
    Vector* sources;                   // Loaded source files
    void* user_data;                   // User-provided data
    const Error* last_error;           // Last reported error
    bool owns_error_handler;           // Whether we own the error handler
    
    /* Configuration */
    int flags;                 /* Execution flags */
    FILE* stdout_handle;       /* Standard output handle */
    FILE* stderr_handle;       /* Standard error handle */
    FILE* stdin_handle;        /* Standard input handle */
    
    /* State */
    bool initialized;          /* True if context has been initialized */
    int status_code;           /* Current execution status code */
    
    // Compilation components
    EntropyField* entropy_field;
    RegionManager* region_manager;
    TokenManager* token_manager;
    ECSWorld* ecs_world;      /* Entity Component System world */
    
    // Tokenization and propagation state
    Vector* tokens;           /* Vector of tokenized source */
    TokenPropagator* propagator;  /* Token propagator for WFC */
    bool verbose;             /* Verbose output flag */
    
    // Output configuration
    char* output_file;        /* Output file path for code generation */
    
    // Options
    BraggiOptions options;

    bool wfc_completed;             // Whether WFC has completed
} BraggiContext;

/**
 * Create a new Braggi context
 * 
 * @return A newly allocated context, or NULL on failure
 */
BraggiContext* braggi_context_create(void);

/**
 * Initialize a Braggi context
 * 
 * @param context The context to initialize
 * @return true if successful, false otherwise
 */
bool braggi_context_init(BraggiContext* context);

/**
 * Destroy a Braggi context
 * 
 * @param context The context to destroy
 */
void braggi_context_destroy(BraggiContext* context);

/**
 * Set execution flags
 * 
 * @param context The context
 * @param flags The flags to set
 */
void braggi_context_set_flags(BraggiContext* context, int flags);

/**
 * Get execution flags
 * 
 * @param context The context
 * @return The execution flags
 */
int braggi_context_get_flags(const BraggiContext* context);

/**
 * Load source code from a file
 * 
 * @param context The context
 * @param filename The filename of the source code
 * @return true if successful, false otherwise
 */
bool braggi_context_load_file(BraggiContext* context, const char* filename);

/**
 * Load source code from a string
 * 
 * @param context The context
 * @param source The source code
 * @param name The name of the source
 * @return true if successful, false otherwise
 */
bool braggi_context_load_string(BraggiContext* context, const char* source, const char* name);

/**
 * Get the current source
 * 
 * @param context The context
 * @return The current source
 */
Source* braggi_context_get_source(const BraggiContext* context);

/**
 * Set the error handler for a context
 * 
 * @param context The context
 * @param handler The error handler to use
 */
void braggi_context_set_error_handler(BraggiContext* context, ErrorHandler* handler);

/**
 * Get the error handler from a context
 * 
 * @param context The context
 * @return The error handler, or NULL if none is set
 */
ErrorHandler* braggi_context_get_error_handler(BraggiContext* context);

/**
 * Process a source file
 * 
 * @param context The context
 * @param source The source to process
 * @return true if successful, false otherwise
 */
bool braggi_context_process_source(BraggiContext* context, Source* source);

/**
 * Get the last error from a context
 * 
 * @param context The context
 * @return The last error, or NULL if no error has occurred
 */
const Error* braggi_context_get_last_error(BraggiContext* context);

/**
 * Set user data on a context
 * 
 * @param context The context
 * @param user_data The user data to set
 */
void braggi_context_set_user_data(BraggiContext* context, void* user_data);

/**
 * Get user data from a context
 * 
 * @param context The context
 * @return The user data, or NULL if none is set
 */
void* braggi_context_get_user_data(BraggiContext* context);

/**
 * Get the status code
 * 
 * @param context The context
 * @return The current execution status code
 */
int braggi_context_get_status(const BraggiContext* context);

/**
 * Set the status code
 * 
 * @param context The context
 * @param status The status code to set
 */
void braggi_context_set_status(BraggiContext* context, int status);

/**
 * Redirect stdout
 * 
 * @param context The context
 * @param handle The handle to redirect stdout to
 */
void braggi_context_set_stdout(BraggiContext* context, FILE* handle);

/**
 * Redirect stderr
 * 
 * @param context The context
 * @param handle The handle to redirect stderr to
 */
void braggi_context_set_stderr(BraggiContext* context, FILE* handle);

/**
 * Redirect stdin
 * 
 * @param context The context
 * @param handle The handle to redirect stdin to
 */
void braggi_context_set_stdin(BraggiContext* context, FILE* handle);

/**
 * Get the main memory region
 * 
 * @param context The context
 * @return The main memory region
 */
Region* braggi_context_get_region(const BraggiContext* context);

/**
 * Get default compiler options
 * 
 * @return The default compiler options
 */
BraggiOptions braggi_options_get_defaults(void);

/**
 * Clean up a compiler context without freeing it
 * 
 * @param context The context to clean up
 */
void braggi_context_cleanup(BraggiContext* context);

/**
 * Get the number of errors
 * 
 * @param context The context
 * @return The number of errors
 */
size_t braggi_context_get_error_count(BraggiContext* context);

/**
 * Get an error by index
 * 
 * @param context The context
 * @param index The index of the error
 * @return The error at the specified index
 */
const Error* braggi_context_get_error(BraggiContext* context, size_t index);

/**
 * Check if there are any errors
 * 
 * @param context The context
 * @return true if there are errors, false otherwise
 */
bool braggi_context_has_errors(BraggiContext* context);

/**
 * Report an error
 * 
 * @param context The context
 * @param category The error category
 * @param severity The error severity
 * @param line The line number
 * @param column The column number
 * @param filename The filename
 * @param message The error message
 * @param details The error details
 */
void braggi_context_report_error(BraggiContext* context, ErrorCategory category, 
                               ErrorSeverity severity, uint32_t line, uint32_t column,
                               const char* filename, const char* message, const char* details);

/**
 * Compile the loaded source
 * 
 * @param context The context
 * @return true if successful, false otherwise
 */
bool braggi_context_compile(BraggiContext* context);

/**
 * Execute the compiled code
 * 
 * @param context The context
 * @return true if successful, false otherwise
 */
bool braggi_context_execute(BraggiContext* context);

// Set the output file for code generation
bool braggi_set_output_file(BraggiContext* context, const char* output_file);

#endif /* BRAGGI_CONTEXT_H */ 