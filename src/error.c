/*
 * Braggi - Error Handling Implementation
 * 
 * "Errors are like ornery bulls - you gotta catch 'em,
 * tag 'em, and show 'em who's boss!" - Texan Error Philosophy
 */

#include "braggi/error.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

// Global error handler instance
static ErrorHandler* global_error_handler = NULL;

// Forward declaration of function from error_handler.h
void braggi_error_handler_clear(ErrorHandler* handler);

// Initialize the error system
void braggi_error_init(void) {
    if (global_error_handler) {
        braggi_error_clear();
        braggi_error_handler_destroy(global_error_handler);
    }
    
    global_error_handler = braggi_error_handler_create();
}

// Clean up the error system
void braggi_error_cleanup(void) {
    if (global_error_handler) {
        // Call the handler's destroy function which will clean up errors
        braggi_error_handler_destroy(global_error_handler);
        global_error_handler = NULL;
    }
}

// Report an error to a specific handler or the global one if handler is NULL
void braggi_error_report(ErrorHandler* handler, uint32_t code, ErrorCategory category, 
                       ErrorSeverity severity, SourcePosition pos, const char* filename,
                       const char* message, const char* details) {
    // Use the global handler if none provided
    if (!handler) {
        if (!global_error_handler) {
            braggi_error_init();
        }
        handler = global_error_handler;
    }
    
    // Forward to the implementation in error_handler.c
    // This avoids the duplicate definition issue
    extern void braggi_error_report_impl(ErrorHandler* handler, uint32_t code, ErrorCategory category,
                                      ErrorSeverity severity, SourcePosition pos, const char* filename,
                                      const char* message, const char* details);
    
    braggi_error_report_impl(handler, code, category, severity, pos, filename, message, details);
}

// Report an error with source context (simplified version using global handler)
void braggi_error_report_ctx(ErrorCategory category, ErrorSeverity severity, 
                           uint32_t line, uint32_t column,
                           const char* filename, const char* message, const char* details) {
    if (!global_error_handler) {
        braggi_error_init();
    }
    
    SourcePosition pos = {0};
    pos.line = line;
    pos.column = column;
    braggi_error_report(global_error_handler, 0, category, severity, pos, filename, message, details);
}

// Get the most recent error
Error* braggi_error_get_latest(void) {
    if (!global_error_handler) {
        return NULL;
    }
    
    size_t size = braggi_vector_size(global_error_handler->errors);
    if (size == 0) {
        return NULL;
    }
    
    return *(Error**)braggi_vector_get(global_error_handler->errors, size - 1);
}

// Get all errors
Vector* braggi_error_get_all(void) {
    if (!global_error_handler) {
        return NULL;
    }
    
    return global_error_handler->errors;
}

// Check if there are any errors in the global handler
bool braggi_error_has_errors(void) {
    if (!global_error_handler) {
        return false;
    }
    
    return braggi_vector_size(global_error_handler->errors) > 0;
}

// Check if there are any fatal errors in the global handler
bool braggi_error_has_fatal(void) {
    if (!global_error_handler) {
        return false;
    }
    
    for (size_t i = 0; i < braggi_vector_size(global_error_handler->errors); i++) {
        Error* error = *(Error**)braggi_vector_get(global_error_handler->errors, i);
        if (error && error->severity == ERROR_SEVERITY_FATAL) {
            return true;
        }
    }
    
    return false;
}

// Format an error as a string
char* braggi_error_format(const Error* error) {
    if (!error) {
        return NULL;
    }
    
    // Convert severity to string
    const char* severity_str;
    switch (error->severity) {
        case ERROR_SEVERITY_NOTE:     severity_str = "note"; break;
        case ERROR_SEVERITY_WARNING:  severity_str = "warning"; break;
        case ERROR_SEVERITY_ERROR:    severity_str = "error"; break;
        case ERROR_SEVERITY_FATAL:    severity_str = "fatal"; break;
        default:                      severity_str = "unknown"; break;
    }
    
    // Convert category to string
    const char* category_str;
    switch (error->category) {
        case ERROR_CATEGORY_SYNTAX:    category_str = "syntax"; break;
        case ERROR_CATEGORY_SEMANTIC:  category_str = "semantic"; break;
        case ERROR_CATEGORY_TYPE:      category_str = "type"; break;
        case ERROR_CATEGORY_REGION:    category_str = "region"; break;
        case ERROR_CATEGORY_LIFETIME:  category_str = "lifetime"; break;
        case ERROR_CATEGORY_PROPAGATION: category_str = "propagation"; break;
        case ERROR_CATEGORY_CONSTRAINT: category_str = "constraint"; break;
        case ERROR_CATEGORY_SYSTEM:    category_str = "system"; break;
        case ERROR_CATEGORY_IO:        category_str = "io"; break;
        case ERROR_CATEGORY_MEMORY:    category_str = "memory"; break;
        case ERROR_CATEGORY_INTERNAL:  category_str = "internal"; break; 
        case ERROR_CATEGORY_USER:      category_str = "user"; break;
        case ERROR_CATEGORY_GENERAL:   category_str = "general"; break;
        default:                       category_str = "unknown"; break;
    }
    
    // Allocate buffer for the formatted string
    // Format: "<filename>:<line>:<column>: <severity>: <category>: <message>"
    char* buffer = NULL;
    
    if (error->filename && error->position.line > 0) {
        // Full error with location
        size_t buffer_size = snprintf(NULL, 0, "%s:%u:%u: %s: %s: %s",
                 error->filename, error->position.line, error->position.column,
                 severity_str, category_str, error->message ? error->message : "unknown error") + 1;
        
        buffer = (char*)malloc(buffer_size);
        if (buffer) {
            snprintf(buffer, buffer_size, "%s:%u:%u: %s: %s: %s",
                    error->filename, error->position.line, error->position.column,
                    severity_str, category_str, error->message ? error->message : "unknown error");
        }
    } else {
        // Error without location
        size_t buffer_size = snprintf(NULL, 0, "%s: %s: %s",
                 severity_str, category_str, error->message ? error->message : "unknown error") + 1;
        
        buffer = (char*)malloc(buffer_size);
        if (buffer) {
            snprintf(buffer, buffer_size, "%s: %s: %s",
                    severity_str, category_str, error->message ? error->message : "unknown error");
        }
    }
    
    return buffer;
}

// Print an error to stderr
void braggi_error_print(const Error* error) {
    if (!error) return;
    
    char* formatted = braggi_error_format(error);
    if (formatted) {
        fprintf(stderr, "%s\n", formatted);
        
        if (error->detail) {
            fprintf(stderr, "  %s\n", error->detail);
        }
        
        free(formatted);
    }
}

// Print all errors from the global handler to stderr
void braggi_error_print_all_global(void) {
    if (!global_error_handler) return;
    
    braggi_error_print_all(global_error_handler, stderr);
}

// Print all errors from a specific handler to a stream
void braggi_error_print_all(const ErrorHandler* handler, FILE* stream) {
    if (!handler || !stream) return;
    
    for (size_t i = 0; i < braggi_vector_size(handler->errors); i++) {
        Error* error = *(Error**)braggi_vector_get(handler->errors, i);
        if (!error) continue;
        
        char* formatted = braggi_error_format(error);
        if (formatted) {
            fprintf(stream, "%s\n", formatted);
            
            if (error->detail) {
                fprintf(stream, "  %s\n", error->detail);
            }
            
            free(formatted);
        }
    }
}

// Forward declaration from error_handler.h
void braggi_error_handler_clear(ErrorHandler* handler);

// Clear all errors from the global handler
void braggi_error_clear(void) {
    if (!global_error_handler) return;
    
    braggi_error_handler_clear(global_error_handler);
}

// Count errors of a given severity or higher
int braggi_error_count_by_severity(const ErrorHandler* handler, ErrorSeverity min_severity) {
    if (!handler) return 0;
    
    int count = 0;
    for (size_t i = 0; i < braggi_vector_size(handler->errors); i++) {
        Error* error = *(Error**)braggi_vector_get(handler->errors, i);
        if (error && error->severity >= min_severity) {
            count++;
        }
    }
    
    return count;
}

// Create a new error
Error* braggi_error_create(uint32_t id, ErrorCategory category, ErrorSeverity severity,
                          SourcePosition position, const char* filename,
                          const char* message, const char* detail) {
    Error* error = (Error*)malloc(sizeof(Error));
    if (!error) {
        return NULL;
    }
    
    error->id = id;
    error->category = category;
    error->severity = severity;
    error->position = position;
    error->filename = filename ? strdup(filename) : NULL;
    error->message = message ? strdup(message) : NULL;
    error->detail = detail ? strdup(detail) : NULL;
    error->context = NULL;
    
    return error;
}

// Free an error
void braggi_error_free(Error* error) {
    if (!error) return;
    
    if (error->message) free((void*)error->message);
    if (error->filename) free((void*)error->filename);
    if (error->detail) free((void*)error->detail);
    
    free(error);
}

// Copy an error
Error* braggi_error_copy(const Error* error) {
    if (!error) return NULL;
    
    return braggi_error_create(error->id, error->category, error->severity,
                              error->position, error->filename,
                              error->message, error->detail);
}

// Convert an error to a string
char* braggi_error_to_string(const Error* error) {
    return braggi_error_format(error);
}

// Get a string representation of an error category
const char* braggi_error_category_to_string(ErrorCategory category) {
    switch (category) {
        case ERROR_CATEGORY_NONE:       return "none";
        case ERROR_CATEGORY_SYNTAX:     return "syntax";
        case ERROR_CATEGORY_SEMANTIC:   return "semantic";
        case ERROR_CATEGORY_TYPE:       return "type";
        case ERROR_CATEGORY_REGION:     return "region";
        case ERROR_CATEGORY_LIFETIME:   return "lifetime";
        case ERROR_CATEGORY_PROPAGATION: return "propagation";
        case ERROR_CATEGORY_CONSTRAINT: return "constraint";
        case ERROR_CATEGORY_SYSTEM:     return "system";
        case ERROR_CATEGORY_IO:         return "io";
        case ERROR_CATEGORY_MEMORY:     return "memory";
        case ERROR_CATEGORY_INTERNAL:   return "internal";
        case ERROR_CATEGORY_USER:       return "user";
        case ERROR_CATEGORY_GENERAL:    return "general";
        default:                        return "unknown";
    }
}

// Get a string representation of an error severity
const char* braggi_error_severity_to_string(ErrorSeverity severity) {
    switch (severity) {
        case ERROR_SEVERITY_NONE:     return "none";
        case ERROR_SEVERITY_NOTE:     return "note";
        case ERROR_SEVERITY_WARNING:  return "warning";
        case ERROR_SEVERITY_ERROR:    return "error";
        case ERROR_SEVERITY_FATAL:    return "fatal";
        default:                      return "unknown";
    }
}

// REMOVING DUPLICATE FUNCTIONS THAT ARE ALREADY IN error_handler.c
// The following functions have been implemented in error_handler.c:
// - braggi_error_handler_create
// - braggi_error_handler_destroy
// - braggi_error_add
// - braggi_error_handler_get_error_count
// - braggi_error_handler_get_error
// - braggi_error_handler_clear 