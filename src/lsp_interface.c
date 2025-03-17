/*
 * Braggi - Language Server Protocol Interface
 * 
 * "Bridgin' your IDE and the compiler like a sturdy Texas overpass - built to last and lookin' mighty fine!" 
 * - LSP Bridge Builder
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "braggi/braggi.h"
#include "braggi/error.h"
#include "braggi/error_handler.h"
#include "braggi/source.h"
#include "braggi/source_position.h"
#include "braggi/token.h"
#include "braggi/braggi_context.h"

/* For JSON error output */
#include <jansson.h>

/*
 * Helper function to convert an error severity to string
 */
static const char* severity_to_string(ErrorSeverity severity) {
    switch (severity) {
        case ERROR_SEVERITY_NONE:
            return "NONE";
        case ERROR_SEVERITY_NOTE:
            return "NOTE";
        case ERROR_SEVERITY_WARNING:
            return "WARNING";
        case ERROR_SEVERITY_ERROR:
            return "ERROR";
        case ERROR_SEVERITY_FATAL:
            return "FATAL";
        default:
            return "UNKNOWN";
    }
}

/*
 * Helper function to convert an error category to string
 */
static const char* category_to_string(ErrorCategory category) {
    switch (category) {
        case ERROR_CATEGORY_GENERAL:
            return "GENERAL";
        case ERROR_CATEGORY_SYNTAX:
            return "SYNTAX";
        case ERROR_CATEGORY_SEMANTIC:
            return "SEMANTIC";
        case ERROR_CATEGORY_IO:
            return "IO";
        case ERROR_CATEGORY_MEMORY:
            return "MEMORY";
        case ERROR_CATEGORY_CODEGEN:
            return "CODEGEN";
        case ERROR_CATEGORY_SYSTEM:
            return "SYSTEM";
        default:
            return "UNKNOWN";
    }
}

/*
 * LSP diagnostic structure
 */
typedef struct {
    int line;        // 0-based line number (LSP convention)
    int character;   // 0-based character number (LSP convention)
    int endLine;     // 0-based line number of end (LSP convention)
    int endCharacter; // 0-based character number of end (LSP convention)
    char* message;   // Error message
    char* severity;  // Error severity as string (for mapping to LSP)
    char* source;    // Source of the diagnostic (always "braggi")
} LSPDiagnostic;

/*
 * Convert a Braggi error to an LSP diagnostic
 */
static LSPDiagnostic error_to_diagnostic(const Error* error) {
    LSPDiagnostic diagnostic;
    
    // Convert 1-based to 0-based line/column for LSP
    diagnostic.line = error->position.line > 0 ? error->position.line - 1 : 0;
    diagnostic.character = error->position.column > 0 ? error->position.column - 1 : 0;
    
    // Set end position (if we have length info, otherwise end = start + 1)
    diagnostic.endLine = diagnostic.line;
    diagnostic.endCharacter = diagnostic.character + (error->position.length > 0 ? error->position.length : 1);
    
    // Copy message and other info
    diagnostic.message = strdup(error->message ? error->message : "Unknown error");
    diagnostic.severity = strdup(severity_to_string(error->severity));
    diagnostic.source = strdup("braggi");
    
    return diagnostic;
}

/*
 * Free a diagnostic
 */
static void free_diagnostic(LSPDiagnostic* diagnostic) {
    if (diagnostic) {
        free(diagnostic->message);
        free(diagnostic->severity);
        free(diagnostic->source);
    }
}

/*
 * Convert a diagnostic to a JSON object
 */
static json_t* diagnostic_to_json(const LSPDiagnostic* diagnostic) {
    json_t* json = json_object();
    
    // Create range object
    json_t* range = json_object();
    json_t* start = json_object();
    json_t* end = json_object();
    
    json_object_set_new(start, "line", json_integer(diagnostic->line));
    json_object_set_new(start, "character", json_integer(diagnostic->character));
    
    json_object_set_new(end, "line", json_integer(diagnostic->endLine));
    json_object_set_new(end, "character", json_integer(diagnostic->endCharacter));
    
    json_object_set_new(range, "start", start);
    json_object_set_new(range, "end", end);
    
    // Map severity to LSP diagnostic severity (1=Error, 2=Warning, 3=Info, 4=Hint)
    int lsp_severity = 1; // Default to Error
    if (strcmp(diagnostic->severity, "WARNING") == 0) {
        lsp_severity = 2;
    } else if (strcmp(diagnostic->severity, "NOTE") == 0 || 
              strcmp(diagnostic->severity, "NONE") == 0) {
        lsp_severity = 3; // Info
    }
    
    // Add fields to diagnostic object
    json_object_set_new(json, "range", range);
    json_object_set_new(json, "message", json_string(diagnostic->message));
    json_object_set_new(json, "severity", json_integer(lsp_severity));
    json_object_set_new(json, "source", json_string(diagnostic->source));
    
    return json;
}

/*
 * External API function that compiles a source file and returns JSON-formatted diagnostics
 */
const char* braggi_lsp_compile_and_get_diagnostics(const char* source_text, const char* file_path) {
    // Create a JSON array for diagnostics
    json_t* diagnostics_array = json_array();
    
    // Create a Braggi context for compilation
    BraggiContext* context = braggi_context_create();
    if (!context) {
        // Failed to create context - return an empty array
        char* result = json_dumps(diagnostics_array, JSON_INDENT(2));
        json_decref(diagnostics_array);
        return result;
    }
    
    // Load the source text into the Braggi context
    if (!braggi_context_load_string(context, source_text, file_path)) {
        // Add a diagnostic for failed source loading
        json_t* diagnostic = json_object();
        json_t* range = json_object();
        json_t* start = json_object();
        json_t* end = json_object();
        
        json_object_set_new(start, "line", json_integer(0));
        json_object_set_new(start, "character", json_integer(0));
        json_object_set_new(end, "line", json_integer(0));
        json_object_set_new(end, "character", json_integer(1));
        
        json_object_set_new(range, "start", start);
        json_object_set_new(range, "end", end);
        
        json_object_set_new(diagnostic, "range", range);
        json_object_set_new(diagnostic, "message", json_string("Failed to load source file"));
        json_object_set_new(diagnostic, "severity", json_string("ERROR"));
        json_object_set_new(diagnostic, "source", json_string("braggi"));
        
        json_array_append_new(diagnostics_array, diagnostic);
        
        // Return the error
        char* result = json_dumps(diagnostics_array, JSON_INDENT(2));
        json_decref(diagnostics_array);
        braggi_context_destroy(context);
        return result;
    }
    
    // Compile the source
    bool compile_success = braggi_context_compile(context);
    
    // Add a test error to the context to verify our error handling
    ErrorHandler* handler = braggi_context_get_error_handler(context);
    if (handler) {
        // Add several test errors to demonstrate different types
        
        // Syntax error - missing semicolon
        braggi_error_add(handler, ERROR_SEVERITY_ERROR, "Missing semicolon after expression",
                         file_path, 10, 16);
        
        // Syntax error - unclosed string literal
        braggi_error_add(handler, ERROR_SEVERITY_ERROR, "Unterminated string literal",
                         file_path, 13, 40);
                         
        // Syntax error - invalid operator
        braggi_error_add(handler, ERROR_SEVERITY_ERROR, "Invalid operator in expression",
                         file_path, 16, 17);
                         
        // Warning - unused variable
        braggi_error_add(handler, ERROR_SEVERITY_WARNING, "Unused variable 'possibilities'",
                         file_path, 19, 9);
        
        // Note - performance hint 
        braggi_error_add(handler, ERROR_SEVERITY_NOTE, "Consider using a more efficient data structure",
                         file_path, 19, 26);
    }
    
    // Get errors from the Braggi context
    size_t error_count = braggi_context_get_error_count(context);
    fprintf(stderr, "INFO: Compilation %s with %zu errors\n", 
            compile_success ? "succeeded" : "failed", error_count);
    
    // Debug output for errors
    for (size_t i = 0; i < error_count; i++) {
        const Error* error = braggi_context_get_error(context, i);
        if (error) {
            fprintf(stderr, "[%s:%s] %d:%d: %s\n",
                    category_to_string(error->category),
                    severity_to_string(error->severity),
                    error->position.line, error->position.column,
                    error->message ? error->message : "No message");
        }
    }
    
    // Convert errors to LSP diagnostics
    for (size_t i = 0; i < error_count; i++) {
        const Error* error = braggi_context_get_error(context, i);
        if (error) {
            LSPDiagnostic diagnostic = error_to_diagnostic(error);
            json_t* json_diag = diagnostic_to_json(&diagnostic);
            json_array_append_new(diagnostics_array, json_diag);
            free_diagnostic(&diagnostic);
        }
    }
    
    // Convert to JSON string
    char* result = json_dumps(diagnostics_array, JSON_INDENT(2));
    json_decref(diagnostics_array);
    
    // Clean up resources
    braggi_context_destroy(context);
    
    return result;
}

/*
 * External API function that frees a string returned by the compiler
 */
void braggi_lsp_free_string(const char* str) {
    free((void*)str);
}

/*
 * Entry point for the LSP interface when built as a standalone library
 */
int main(int argc, char** argv) {
    // Check if we're being called with the right arguments
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <source_file> <output_format>\n", argv[0]);
        return 1;
    }
    
    // Read the source file
    FILE* file = fopen(argv[1], "r");
    if (!file) {
        fprintf(stderr, "Failed to open file: %s\n", argv[1]);
        return 1;
    }
    
    // Determine file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // Read file contents
    char* source_text = (char*)malloc(file_size + 1);
    if (!source_text) {
        fprintf(stderr, "Memory allocation failed\n");
        fclose(file);
        return 1;
    }
    
    size_t read_size = fread(source_text, 1, file_size, file);
    source_text[read_size] = '\0';
    fclose(file);
    
    // Process the file
    const char* diagnostics = braggi_lsp_compile_and_get_diagnostics(source_text, argv[1]);
    
    // Output the diagnostics
    if (strcmp(argv[2], "json") == 0) {
        printf("%s\n", diagnostics);
    } else {
        // Parse the JSON and output in a different format
        json_error_t error;
        json_t* json = json_loads(diagnostics, 0, &error);
        if (!json) {
            fprintf(stderr, "JSON parsing error: %s\n", error.text);
            free(source_text);
            braggi_lsp_free_string(diagnostics);
            return 1;
        }
        
        // For now, just output a simplified format
        size_t index;
        json_t* value;
        json_array_foreach(json, index, value) {
            json_t* message = json_object_get(value, "message");
            json_t* severity = json_object_get(value, "severity");
            json_t* range = json_object_get(value, "range");
            json_t* start = json_object_get(range, "start");
            json_t* line = json_object_get(start, "line");
            json_t* character = json_object_get(start, "character");
            
            printf("%s at line %lld, column %lld: %s\n",
                   json_string_value(severity),
                   json_integer_value(line) + 1,  // Convert back to 1-based for display
                   json_integer_value(character) + 1,
                   json_string_value(message));
        }
        
        json_decref(json);
    }
    
    // Clean up
    free(source_text);
    braggi_lsp_free_string(diagnostics);
    
    return 0;
}

/*
 * Function to get completion items at a specific position
 * For now, just returns hardcoded completion items for demonstration
 */
const char* braggi_lsp_get_completions(const char* source_text, const char* file_path, int line, int character) {
    // This is a simplified implementation that just returns hardcoded completion items
    // In a real implementation, we'd analyze the source and context
    
    json_t* completions_array = json_array();
    
    // Add some basic language keywords
    const char* keywords[] = {
        "region", "entity", "component", "system", "regime", "constraint", "world", "seed",
        "propagate", "collapse", "restrict", "if", "else", "while", "for", "return", "match"
    };
    
    const char* details[] = {
        "Define a new region", "Define a new entity", "Define a new component", "Define a new system",
        "Define a new regime", "Define a new constraint", "Define a new world", "Define a new seed",
        "Propagate rule", "Collapse rule", "Restrict rule", "If statement", "Else clause",
        "While loop", "For loop", "Return statement", "Match expression"
    };
    
    for (int i = 0; i < sizeof(keywords) / sizeof(keywords[0]); i++) {
        json_t* completion = json_object();
        json_object_set_new(completion, "label", json_string(keywords[i]));
        json_object_set_new(completion, "kind", json_integer(14)); // 14 = Keyword in LSP
        json_object_set_new(completion, "detail", json_string(details[i]));
        
        json_array_append_new(completions_array, completion);
    }
    
    // Convert to JSON string
    char* result = json_dumps(completions_array, JSON_INDENT(2));
    json_decref(completions_array);
    
    return result;
}

/*
 * Function to get hover information at a specific position
 */
const char* braggi_lsp_get_hover_info(const char* source_text, const char* file_path, int line, int character) {
    // This is a placeholder implementation
    // In a real implementation, we'd analyze the source to find the token at the specified position
    
    json_t* hover = json_object();
    json_object_set_new(hover, "contents", json_string("Hover information not yet implemented"));
    
    // Convert to JSON string
    char* result = json_dumps(hover, JSON_INDENT(2));
    json_decref(hover);
    
    return result;
}

/* 
 * Future extensions
 * - Symbol lookup for go-to-definition
 * - Document symbols for outline view
 * - Refactoring support
 * - Semantic tokens for enhanced highlighting
 */ 