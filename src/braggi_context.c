/*
 * Braggi - Compiler Context Implementation
 * 
 * "A context without implementation is like a cow without a pasture -
 * it just don't make no sense!" - Texas Programming Wisdom
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// Internal headers - make sure order doesn't cause conflicts
#include "braggi/braggi_context.h"
#include "braggi/error.h"
#include "braggi/source.h"

// This is where all the magic happens
#include "braggi/region.h"
#include "braggi/token.h"
#include "braggi/allocation.h"

// Must include after the core types are defined
#include "braggi/error_handler.h"

// Keep entropy and constraint imports last to avoid redefinition issues
#include "braggi/entropy.h"
#include "braggi/constraint.h"

// Use the BraggiContext defined in the header, don't redefine it
// Just implement the functions here

bool braggi_context_init(BraggiContext* context) {
    if (!context) {
        return false;
    }
    
    // Initialize error handler
    context->error_handler = braggi_error_handler_create();
    if (!context->error_handler) {
        return false;
    }
    context->owns_error_handler = true;
    
    // Initialize empty source
    context->source = NULL;
    
    // Initialize main region
    context->main_region = NULL;
    
    // Initialize symbol table
    context->symbols = NULL;
    
    // Initialize flags and IO handles
    context->flags = 0;
    context->stdout_handle = stdout;
    context->stderr_handle = stderr;
    context->stdin_handle = stdin;
    
    // Set initialization status
    context->initialized = true;
    context->status_code = 0;
    
    // Initialize entropy field and other WFCCC components
    context->entropy_field = NULL;
    context->region_manager = NULL;
    context->token_manager = NULL;
    
    // Initialize with default options
    context->options = braggi_options_get_defaults();
    
    return true;
}

void braggi_context_cleanup(BraggiContext* context) {
    if (!context) {
        return;
    }
    
    // Clean up error handler
    if (context->error_handler && context->owns_error_handler) {
        braggi_error_handler_destroy(context->error_handler);
        context->error_handler = NULL;
    }
    
    // Clean up source if allocated
    if (context->source) {
        braggi_source_file_destroy(context->source);
        context->source = NULL;
    }
    
    // Cleanup entropy field
    if (context->entropy_field) {
        // TODO: Implement entropy field cleanup 
        context->entropy_field = NULL;
    }
    
    // Cleanup region manager 
    if (context->region_manager) {
        // TODO: Implement region manager cleanup
        context->region_manager = NULL;
    }
    
    // Cleanup token manager
    if (context->token_manager) {
        // TODO: Implement token manager cleanup
        context->token_manager = NULL;
    }
    
    // Cleanup main region
    if (context->main_region) {
        // TODO: Implement region cleanup
        context->main_region = NULL;
    }
    
    // Cleanup symbol table
    if (context->symbols) {
        // TODO: Implement symbol table cleanup
        context->symbols = NULL;
    }
    
    // Reset flags
    context->initialized = false;
}

BraggiContext* braggi_context_create(void) {
    BraggiContext* context = (BraggiContext*)malloc(sizeof(BraggiContext));
    if (!context) {
        return NULL;
    }
    
    if (!braggi_context_init(context)) {
        free(context);
        return NULL;
    }
    
    return context;
}

void braggi_context_destroy(BraggiContext* context) {
    if (!context) {
        return;
    }
    
    braggi_context_cleanup(context);
    free(context);
}

bool braggi_context_load_file(BraggiContext* context, const char* filename) {
    if (!context || !filename) {
        return false;
    }
    
    // Clean up existing source file if any
    if (context->source) {
        braggi_source_file_destroy(context->source);
        context->source = NULL;
    }
    
    // Load source from file
    context->source = braggi_source_file_create(filename);
    if (!context->source) {
        return false;
    }
    
    return true;
}

bool braggi_context_load_string(BraggiContext* context, const char* source, const char* name) {
    if (!context || !source) {
        return false;
    }
    
    // Clean up existing source file if any
    if (context->source) {
        braggi_source_file_destroy(context->source);
        context->source = NULL;
    }
    
    // Load source from string
    context->source = braggi_source_string_create(source, name ? name : "<string>");
    if (!context->source) {
        return false;
    }
    
    return true;
}

/*
 * Get the error handler for a context
 * "Every good cowboy knows where his first aid kit is!" - Texas Error Philosophy
 */
ErrorHandler* braggi_context_get_error_handler(BraggiContext* context) {
    if (!context) return NULL;
    return context->error_handler;
}

/*
 * Report an error in the context
 * "When your horse throws ya, best to record exactly how it happened!" - Texas Debugging
 */
void braggi_context_report_error(BraggiContext* context, ErrorCategory category,
                            ErrorSeverity severity, uint32_t line, uint32_t column,
                            const char* file, const char* message, const char* hint) {
    if (!context || !context->error_handler) return;
    
    /* 
     * The ACTUAL declaration from error.h is:
     * void braggi_error_add(ErrorHandler* handler, 
     *                        ErrorSeverity severity, 
     *                        const char* message,
     *                        const char* file, 
     *                        uint32_t line, 
     *                        uint32_t column);
     */
    
    /* Call with the CORRECT parameter order according to the declaration */
    braggi_error_add(
        context->error_handler,  /* handler */
        severity,                /* severity (not category) */
        message,                 /* message comes THIRD */
        file,                    /* file comes FOURTH */
        line,                    /* line comes FIFTH */
        column                   /* column comes SIXTH */
    );
    
    /* Category and hint parameters are ignored in this implementation */
    (void)category;
    (void)hint;
}

/*
 * Check if the context has any errors
 * "Checkin' for rattlesnakes before you put your boots on" - Texan Wisdom
 */
bool braggi_context_has_errors(BraggiContext* context) {
    if (!context || !context->error_handler) return false;
    return braggi_error_handler_has_errors(context->error_handler);
}

/*
 * Get the number of errors in the context
 * "Countin' up troubles like fence posts on a bad day" - Cowboy Coding
 */
size_t braggi_context_get_error_count(BraggiContext* context) {
    if (!context || !context->error_handler) return 0;
    return braggi_error_handler_get_error_count(context->error_handler);
}

/*
 * Get a specific error from the context
 * "Pullin' one bad apple from the barrel" - Texas Debugging
 */
const Error* braggi_context_get_error(BraggiContext* context, size_t index) {
    if (!context || !context->error_handler) return NULL;
    return braggi_error_handler_get_error(context->error_handler, index);
}

bool braggi_context_compile(BraggiContext* context) {
    if (!context || !context->source) {
        return false;
    }
    
    // TODO: Initialize entropy field
    // TODO: Tokenize source
    // TODO: Apply constraints 
    // TODO: Check for errors
    
    return !braggi_context_has_errors(context);
}

bool braggi_context_execute(BraggiContext* context) {
    if (!context || !context->source || braggi_context_has_errors(context)) {
        return false;
    }
    
    // TODO: Execute compiled code
    
    return !braggi_context_has_errors(context);
}

int braggi_context_get_status(const BraggiContext* context) {
    if (!context) {
        return -1;
    }
    return context->status_code;
}

void braggi_context_set_status(BraggiContext* context, int status) {
    if (!context) {
        return;
    }
    context->status_code = status;
}

void braggi_context_set_flags(BraggiContext* context, int flags) {
    if (!context) {
        return;
    }
    context->flags = flags;
}

int braggi_context_get_flags(const BraggiContext* context) {
    if (!context) {
        return 0;
    }
    return context->flags;
}

void braggi_context_set_stdout(BraggiContext* context, FILE* handle) {
    if (!context || !handle) {
        return;
    }
    context->stdout_handle = handle;
}

void braggi_context_set_stderr(BraggiContext* context, FILE* handle) {
    if (!context || !handle) {
        return;
    }
    context->stderr_handle = handle;
}

void braggi_context_set_stdin(BraggiContext* context, FILE* handle) {
    if (!context || !handle) {
        return;
    }
    context->stdin_handle = handle;
}

Source* braggi_context_get_source(const BraggiContext* context) {
    if (!context) {
        return NULL;
    }
    return context->source;
}

Region* braggi_context_get_region(const BraggiContext* context) {
    if (!context) {
        return NULL;
    }
    return context->main_region;
}

BraggiOptions braggi_options_get_defaults(void) {
    BraggiOptions options;
    
    options.optimization_level = 0;
    options.enable_debug = true;
    options.verbose_output = false;
    options.error_limit = 10;
    options.memory_limit_kb = 8 * 1024; // 8MB default memory limit
    
    return options;
}

/*
 * Process source code in the context
 * "Processing source is like breaking a wild horse - 
 * ya gotta take it step by step or you'll get thrown off!"
 */
bool braggi_context_process_source(BraggiContext* context, Source* source) {
    if (!context || !source) {
        return false;
    }
    
    // Set the source in the context
    context->source = source;
    
    // Compile the source
    if (!braggi_context_compile(context)) {
        return false;
    }
    
    // Execute if no errors and not in test mode (using test mode as compile-only flag)
    if (!braggi_context_has_errors(context) && 
        !(context->flags & BRAGGI_FLAG_TEST)) {
        return braggi_context_execute(context);
    }
    
    return !braggi_context_has_errors(context);
}

/*
 * Get the last error from the context
 * "Always good to know what spooked your code - 
 * like trackin' the last critter that ran through your ranch!"
 */
const Error* braggi_context_get_last_error(BraggiContext* context) {
    if (!context || !context->error_handler) {
        return NULL;
    }
    
    size_t error_count = braggi_context_get_error_count(context);
    if (error_count == 0) {
        return NULL;
    }
    
    // Return the last error (most recent)
    return braggi_context_get_error(context, error_count - 1);
} 