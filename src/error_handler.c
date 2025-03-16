/*
 * Braggi - Error Handler
 * 
 * "An error handler without good details is like a cowboy without his boots - 
 * technically functional, but missin' something important!" - Texas Programming Proverb
 */

#include "braggi/error.h"
#include "braggi/error_handler.h"
#include "braggi/allocation.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
 * "When errors come a-knockin', a good handler's there to wrangle 'em up
 * before they cause a stampede in your codebase." - Texan Error Philosophy
 */

// We're not redefining ErrorHandler here - it's already defined in error.h
// Using the struct from error.h which looks like:
// struct ErrorHandler {
//     Vector* errors;
//     void* user_data;
//     ...other fields...
// };

/*
 * Create a new error handler
 */
ErrorHandler* braggi_error_handler_create(void) {
    // Allocate the handler
    ErrorHandler* handler = (ErrorHandler*)malloc(sizeof(ErrorHandler));
    if (!handler) {
        return NULL;
    }
    
    // Create a vector for errors
    handler->errors = braggi_vector_create(sizeof(Error*));
    if (!handler->errors) {
        free(handler);
        return NULL;
    }
    
    // Initialize with defaults
    handler->user_data = NULL;
    
    // We'll use the global region for simplicity
    Region* region = braggi_named_region_get_global();
    
    return handler;
}

/*
 * Free the error handler and its resources
 */
void braggi_error_handler_destroy(ErrorHandler* handler) {
    if (!handler) {
        return;
    }
    
    // Free the errors vector and all errors it contains
    if (handler->errors) {
        for (size_t i = 0; i < braggi_vector_size(handler->errors); i++) {
            Error* error = *(Error**)braggi_vector_get(handler->errors, i);
            if (error) {
                braggi_error_free(error);
            }
        }
        braggi_vector_destroy(handler->errors);
    }
    
    // Free the handler itself
    free(handler);
}

/*
 * Report an error to a handler
 * Match the signature from error.h:
 * void braggi_error_report(ErrorHandler* handler, uint32_t code, ErrorCategory category,
 *                        ErrorSeverity severity, SourcePosition pos, const char* filename,
 *                        const char* message, const char* details);
 */
void braggi_error_report_impl(ErrorHandler* handler, uint32_t code, ErrorCategory category, 
                        ErrorSeverity severity, SourcePosition pos, const char* filename,
                        const char* message, const char* details) {
    if (!handler || !message) {
        return;
    }
    
    // Create an error
    Error* error = braggi_error_create(code, category, severity, pos, filename, message, details);
    if (!error) {
        return;
    }
    
    // Add to the errors vector
    braggi_vector_push(handler->errors, &error);
    
    // Report to stderr
    fprintf(stderr, "[%s:%s] %s:%u:%u: %s\n",
            braggi_error_category_to_string(category),
            braggi_error_severity_to_string(severity),
            filename ? filename : "<unknown>",
            pos.line,
            pos.column,
            message);
    
    // Check if we should exit based on severity
    if (severity >= ERROR_SEVERITY_FATAL) {
        exit(EXIT_FAILURE);
    }
}

/*
 * Add an error to the handler (simplified implementation to match error.h signature)
 */
void braggi_error_add(ErrorHandler* handler, ErrorSeverity severity, const char* message,
                     const char* file, uint32_t line, uint32_t column) {
    if (!handler || !message) {
        return;
    }
    
    SourcePosition pos = {0};
    pos.line = line;
    pos.column = column;
    
    braggi_error_report_impl(handler, 0, ERROR_CATEGORY_GENERAL, severity, pos, file, message, NULL);
}

/*
 * Get the number of errors in the handler
 */
size_t braggi_error_handler_get_error_count(const ErrorHandler* handler) {
    if (!handler) {
        return 0;
    }
    
    return braggi_vector_size(handler->errors);
}

/*
 * Get an error from the handler by index
 */
const Error* braggi_error_handler_get_error(const ErrorHandler* handler, size_t index) {
    if (!handler || index >= braggi_vector_size(handler->errors)) {
        return NULL;
    }
    
    return *(const Error**)braggi_vector_get(handler->errors, index);
}

/*
 * Clear all errors from the handler
 */
void braggi_error_handler_clear(ErrorHandler* handler) {
    if (!handler) {
        return;
    }
    
    if (handler->errors) {
        for (size_t i = 0; i < braggi_vector_size(handler->errors); i++) {
            Error* error = *(Error**)braggi_vector_get(handler->errors, i);
            if (error) {
                braggi_error_free(error);
            }
        }
        braggi_vector_clear(handler->errors);
    }
}

/* Get the error handler function accessor */
ErrorHandler* braggi_error_handler_get_handler(void* context) {
    // Not implemented yet - always returns NULL
    (void)context;
    return NULL;
}

/*
 * Check if the error handler has any errors
 * "Like checkin' if there's any cattle missin' from the herd - 
 * a quick count tells ya if you've got problems to wrangle!"
 */
bool braggi_error_handler_has_errors(const ErrorHandler* handler) {
    if (!handler) {
        return false;
    }
    
    return braggi_error_handler_get_error_count(handler) > 0;
} 