/*
 * Braggi - Error Handling Interface
 * 
 * "In Texas, we don't hide from problems - we lasso 'em, tag 'em, and solve 'em!" 
 * - Texas Error Wrangler
 */

#ifndef BRAGGI_ERROR_H
#define BRAGGI_ERROR_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>  // Added for size_t
#include <stdio.h>
#include "braggi/source.h"
#include "braggi/util/vector.h"
#include "source_position.h"

// Error severity levels
typedef enum {
    ERROR_LEVEL_INFO,     // Informational message
    ERROR_LEVEL_WARNING,  // Warning, but can continue
    ERROR_LEVEL_ERROR,    // Error, operation failed but can continue
    ERROR_LEVEL_FATAL     // Fatal error, cannot continue
} ErrorLevel;

// Error categories
typedef enum {
    ERROR_CATEGORY_GENERAL,   // General errors
    ERROR_CATEGORY_SYNTAX,    // Syntax errors
    ERROR_CATEGORY_SEMANTIC,  // Semantic errors
    ERROR_CATEGORY_IO,        // I/O errors
    ERROR_CATEGORY_MEMORY,    // Memory errors
    ERROR_CATEGORY_CODEGEN,   // Code generation errors
    ERROR_CATEGORY_SYSTEM     // System errors
} ErrorCategory;

// Error handler interface
typedef struct ErrorHandler {
    // Data storage
    Vector* errors;              // Vector of Error pointers
    void* user_data;             // User-provided context data
    
    // Context for the error handler
    void* context;
    
    // Error callback
    void (*error)(struct ErrorHandler* handler, const char* message);
    
    // Warning callback
    void (*warning)(struct ErrorHandler* handler, const char* message);
    
    // Set error source file and line (optional)
    void (*set_source)(struct ErrorHandler* handler, const char* file, int line);
    
    // Get last error message (optional)
    const char* (*get_last_error)(struct ErrorHandler* handler);
    
    // Get error count (optional)
    int (*get_error_count)(struct ErrorHandler* handler);
    
    // Reset error state (optional)
    void (*reset)(struct ErrorHandler* handler);
} ErrorHandler;

// Extended error information
typedef struct ErrorInfo {
    ErrorLevel level;           // Severity level
    ErrorCategory category;     // Error category
    const char* message;        // Error message
    const char* source_file;    // Source file where error occurred
    int source_line;            // Line number where error occurred
    uint64_t error_code;        // Error code (if applicable)
    void* context;              // Additional context (if applicable)
} ErrorInfo;

/*
 * Error severity - how serious the error is
 */
typedef enum {
    ERROR_SEVERITY_NONE = 0,        /* No error */
    ERROR_SEVERITY_NOTE,            /* Note - informational message */
    ERROR_SEVERITY_WARNING,         /* Warning - possible issue */
    ERROR_SEVERITY_ERROR,           /* Error - definite issue */
    ERROR_SEVERITY_FATAL            /* Fatal error - cannot continue */
} ErrorSeverity;

/*
 * Error structure for tracking and reporting
 */
typedef struct Error {
    uint32_t id;                    /* Error identifier */
    ErrorCategory category;         /* Error category */
    ErrorSeverity severity;         /* Error severity */
    SourcePosition position;        /* Position in source code */
    const char* filename;           /* Source filename (or NULL) */
    const char* message;            /* Error message */
    const char* detail;             /* Detailed explanation (or NULL) */
    void* context;                  /* Additional context (or NULL) */
} Error;

/* Initialize the error system */
void braggi_error_init(void);

/* Clean up the error system */
void braggi_error_cleanup(void);

/* Report an error to a specific handler or the global one if handler is NULL */
void braggi_error_report(ErrorHandler* handler, uint32_t code, ErrorCategory category, 
                       ErrorSeverity severity, SourcePosition pos, const char* filename,
                       const char* message, const char* details);

/* Report an error with source context (simplified version using global handler) */
void braggi_error_report_ctx(ErrorCategory category, ErrorSeverity severity, 
                           uint32_t line, uint32_t column,
                           const char* file, const char* message, const char* details);

/* Get the most recent error */
Error* braggi_error_get_latest(void);

/* Get all errors */
Vector* braggi_error_get_all(void);

/* Check if there are any errors in the global handler */
bool braggi_error_has_errors(void);

/* Check if there are any fatal errors in the global handler */
bool braggi_error_has_fatal(void);

/* Format an error as a string */
char* braggi_error_format(const Error* error);

/* Print an error to stderr */
void braggi_error_print(const Error* error);

/* Print all errors from the global handler to stderr */
void braggi_error_print_all_global(void);

/* Clear all errors from the global handler */
void braggi_error_clear(void);

/*
 * Error handler functions
 */

/* Create a new error handler */
ErrorHandler* braggi_error_handler_create(void);

/* Destroy an error handler */
void braggi_error_handler_destroy(ErrorHandler* handler);

/* Add an error to the handler */
void braggi_error_add(ErrorHandler* handler, ErrorSeverity severity, const char* message,
                     const char* file, uint32_t line, uint32_t column);

/* Get the number of errors */
size_t braggi_error_count(const ErrorHandler* handler);

/* Get the number of errors of a specific severity */
size_t braggi_error_count_severity(const ErrorHandler* handler, ErrorSeverity severity);

/* Get an error by index */
const Error* braggi_error_get(const ErrorHandler* handler, size_t index);

/* Check if any errors have been reported */
bool braggi_error_handler_has_errors(const ErrorHandler* handler);

/* Count errors of a given severity or higher */
int braggi_error_count_by_severity(const ErrorHandler* handler, ErrorSeverity min_severity);

/* Print all errors from a specific handler to a stream */
void braggi_error_print_all(const ErrorHandler* handler, FILE* stream);

/* Clear all errors */
void braggi_error_clear_all(ErrorHandler* handler);

/* Convert an error severity to a string */
const char* braggi_error_severity_string(ErrorSeverity severity);

/* Convert an error category to a string */
const char* braggi_error_category_string(ErrorCategory category);

/* Convert an error to a string representation (must be freed by caller) */
char* braggi_error_to_string(const Error* error);

/* Error creation and destruction */
Error* braggi_error_create(uint32_t id, ErrorCategory category, ErrorSeverity severity, 
                          SourcePosition position, const char* file_name, 
                          const char* message, const char* details);
void braggi_error_free(Error* error);

/* Error accessors */
uint32_t braggi_error_get_id(const Error* error);
ErrorCategory braggi_error_get_category(const Error* error);
ErrorSeverity braggi_error_get_severity(const Error* error);
SourcePosition braggi_error_get_position(const Error* error);
const char* braggi_error_get_file_name(const Error* error);
const char* braggi_error_get_message(const Error* error);
const char* braggi_error_get_details(const Error* error);

/* Helper functions */
const char* braggi_error_category_to_string(ErrorCategory category);
const char* braggi_error_severity_to_string(ErrorSeverity severity);

/* Report an error with extended information */
void braggi_report_error(ErrorHandler* handler, ErrorInfo* info);

/* Report a simple error */
void braggi_report_simple_error(ErrorHandler* handler, 
                               ErrorLevel level, 
                               ErrorCategory category,
                               const char* message);

/* Get a string representation of an error level */
const char* braggi_error_level_string(ErrorLevel level);

#endif /* BRAGGI_ERROR_H */ 