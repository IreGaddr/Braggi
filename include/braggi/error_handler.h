/*
 * Braggi - Error Handler Interface
 * 
 * "A good error message is like a treasure map - it points straight to the problem,
 * and helps ya fix it faster than a jackrabbit on a hot griddle!" 
 * - Texan Debugging Wisdom
 */

#ifndef BRAGGI_ERROR_HANDLER_H
#define BRAGGI_ERROR_HANDLER_H

#include "braggi/error.h"
#include <stdio.h>

/*
 * Error handler functions - these operate on the ErrorHandler structure
 * defined in error.h
 */

/* Create a new error handler */
ErrorHandler* braggi_error_handler_create(void);

/* Destroy an error handler and free its resources */
void braggi_error_handler_destroy(ErrorHandler* handler);

/* Get the number of errors in the handler */
size_t braggi_error_handler_get_error_count(const ErrorHandler* handler);

/* Get an error from the handler by index */
const Error* braggi_error_handler_get_error(const ErrorHandler* handler, size_t index);

/* Clear all errors from the handler */
void braggi_error_handler_clear(ErrorHandler* handler);

/* Get the error handler associated with a context (e.g., parser, compiler) */
ErrorHandler* braggi_error_handler_get_handler(void* context);

/* 
 * Helper functions for getting string representations of error information
 * these are implemented in error.c but declared here for convenience
 */
const char* braggi_error_severity_to_string(ErrorSeverity severity);
const char* braggi_error_category_to_string(ErrorCategory category);

#endif /* BRAGGI_ERROR_HANDLER_H */ 