/*
 * Braggi - x86_64 Code Generation
 *
 * "The x86_64 is like the trusty pickup truck of CPUs - 
 * been around forever and still gets the job done right!" - Texas Hardware Rancher
 */

#include "braggi/codegen.h"
#include "braggi/codegen_arch.h"
#include "braggi/region.h"
#include "braggi/token.h"
#include "braggi/entropy.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Debug helper macro - safer than direct fprintf
#define DEBUG_PRINT(fmt, ...) do { fprintf(stderr, "X86_64 BACKEND: " fmt "\n", ##__VA_ARGS__); fflush(stderr); } while (0)

// x86_64-specific data structure
typedef struct {
    // Assembly output buffer
    char* asm_buffer;
    size_t asm_size;
    size_t asm_capacity;
    
    // Add a flag to track initialization
    bool initialized;
    
    // Add a marker to catch double-free or use-after-free bugs
    uint32_t magic_number;
} X86_64Data;

// Forward declarations for helper functions
static void x86_64_generate_function_declaration(X86_64Data* data, Token* token);
static void x86_64_generate_identifier_usage(X86_64Data* data, Token* token);
static void x86_64_generate_numeric_literal(X86_64Data* data, Token* token);
static void x86_64_generate_string_literal(X86_64Data* data, Token* token);
static void x86_64_generate_keyword(X86_64Data* data, Token* token);
static void x86_64_generate_operator(X86_64Data* data, Token* token);
static void x86_64_generate_punctuation(X86_64Data* data, Token* token);

// Magic number to verify the data structure is valid
#define X86_64_MAGIC 0xC0DEC0DE

// Basic initialization function
static bool x86_64_init(CodeGenerator* generator, ErrorHandler* error_handler) {
    DEBUG_PRINT("Initializing x86_64 backend");
    
    if (!generator) {
        DEBUG_PRINT("ERROR: Null generator pointer");
        return false;
    }
    
    // Allocate backend data
    X86_64Data* data = (X86_64Data*)calloc(1, sizeof(X86_64Data));
    if (!data) {
        DEBUG_PRINT("ERROR: Failed to allocate backend data");
        return false;
    }
    
    // Initialize data
    data->asm_buffer = NULL;
    data->asm_size = 0;
    data->asm_capacity = 0;
    data->initialized = true;
    data->magic_number = X86_64_MAGIC;
    
    // Store it in the generator
    generator->arch_data = data;
    
    DEBUG_PRINT("Successfully initialized x86_64 backend (data=%p)", (void*)data);
    return true;
}

// Verify the data structure is valid
static bool verify_data(X86_64Data* data, const char* func_name) {
    if (!data) {
        DEBUG_PRINT("%s: ERROR: Null data pointer", func_name);
        return false;
    }
    
    if (data->magic_number != X86_64_MAGIC) {
        DEBUG_PRINT("%s: ERROR: Invalid magic number (0x%08x != 0x%08x) - possible use after free", 
                   func_name, data->magic_number, X86_64_MAGIC);
        return false;
    }
    
    if (!data->initialized) {
        DEBUG_PRINT("%s: ERROR: Data not initialized", func_name);
        return false;
    }
    
    return true;
}

// Cleanup function with safety checks
static void x86_64_destroy(CodeGenerator* generator) {
    DEBUG_PRINT("Destroying x86_64 backend (generator=%p)", (void*)generator);
    
    if (!generator) {
        DEBUG_PRINT("WARNING: Null generator in destroy");
        return;
    }
    
    X86_64Data* data = (X86_64Data*)generator->arch_data;
    if (!data) {
        DEBUG_PRINT("WARNING: Null backend data in destroy");
        return;
    }
    
    // Check if the data is valid before accessing it
    if (!verify_data(data, "destroy")) {
        DEBUG_PRINT("WARNING: Invalid data structure detected, cleanup may be incomplete");
        generator->arch_data = NULL;
        return;
    }
    
    DEBUG_PRINT("Data verification passed, proceeding with cleanup");
    
    // Free assembly buffer if it exists
    if (data->asm_buffer) {
        DEBUG_PRINT("Freeing assembly buffer at %p", (void*)data->asm_buffer);
        
        // Take a safety copy of the pointer and clear it first
        void* buffer_to_free = data->asm_buffer;
        data->asm_buffer = NULL;
        data->asm_size = 0;
        data->asm_capacity = 0;
        
        // Fill the memory with zeros to catch use-after-free bugs
        size_t buffer_size = strlen(buffer_to_free) + 1;
        memset(buffer_to_free, 0, buffer_size);
        
        // Now free the buffer
        free(buffer_to_free);
        DEBUG_PRINT("Assembly buffer freed successfully");
    } else {
        DEBUG_PRINT("No assembly buffer to free");
    }
    
    // Invalidate the data structure before freeing
    DEBUG_PRINT("Invalidating data structure");
    data->initialized = false;
    data->magic_number = 0xDEADBEEF;  // Use a different pattern to indicate explicitly freed
    
    // Free backend data
    DEBUG_PRINT("Freeing backend data at %p", (void*)data);
    
    // Take a safety copy of the pointer and clear it first
    void* data_to_free = data;
    generator->arch_data = NULL;
    
    // Fill with zeros to catch use-after-free bugs
    memset(data_to_free, 0, sizeof(X86_64Data));
    
    // Now free the data
    free(data_to_free);
    
    DEBUG_PRINT("Successfully destroyed x86_64 backend - generator is no longer valid");
}

// Code generation function
static bool x86_64_generate(CodeGenerator* generator, EntropyField* field) {
    DEBUG_PRINT("Generating x86_64 code");
    
    if (!generator || !field) {
        DEBUG_PRINT("ERROR: Invalid parameters for x86_64_generate");
        return false;
    }
    
    X86_64Data* data = (X86_64Data*)generator->arch_data;
    if (!verify_data(data, "x86_64_generate")) {
        return false;
    }
    
    // Reset the assembly buffer
    if (data->asm_buffer) {
        free(data->asm_buffer);
    }
    
    data->asm_capacity = 8192; // Start with 8KB
    data->asm_buffer = (char*)malloc(data->asm_capacity);
    if (!data->asm_buffer) {
        DEBUG_PRINT("ERROR: Failed to allocate asm_buffer");
        return false;
    }
    data->asm_size = 0;
    
    // Add standard assembly header
    const char* header =
        "# Generated by Braggi Compiler\n"
        "# x86_64 assembly output\n"
        ".intel_syntax noprefix\n\n"
        ".section .text\n"
        ".global main\n\n";
    
    size_t header_len = strlen(header);
    if (data->asm_capacity < header_len + 1) {
        // Resize the buffer if needed
        char* new_buffer = (char*)realloc(data->asm_buffer, header_len + 8192);
        if (!new_buffer) {
            DEBUG_PRINT("ERROR: Failed to resize asm_buffer");
            return false;
        }
        data->asm_buffer = new_buffer;
        data->asm_capacity = header_len + 8192;
    }
    
    // Copy the header
    memcpy(data->asm_buffer, header, header_len);
    data->asm_size = header_len;
    data->asm_buffer[data->asm_size] = '\0';
    
    // Process the entropy field to generate code
    size_t cell_count = field->cell_count;
    DEBUG_PRINT("Processing %zu cells in entropy field", cell_count);
    
    // Extract tokens from the entropy field and generate code
    for (size_t i = 0; i < cell_count; i++) {
        if (i >= field->cell_count) break; // Safety check
        
        EntropyCell* cell = field->cells[i];
        if (!cell) continue;
        
        // Skip cells that have no collapsed state
        if (!braggi_entropy_cell_is_collapsed(cell)) continue;
        
        // Find the non-eliminated state (the collapsed one)
        EntropyState* state = NULL;
        for (size_t j = 0; j < cell->state_count; j++) {
            if (cell->states[j] && cell->states[j]->probability > 0) {
                state = cell->states[j];
                break;
            }
        }
        
        if (!state) continue;
        
        // Extract the token from the state
        Token* token = (Token*)state->data;
        if (!token) continue;
        
        // Generate code based on token type
        switch (token->type) {
            case TOKEN_KEYWORD:
                if (strcmp(token->text, "func") == 0 || strcmp(token->text, "fn") == 0) {
                    x86_64_generate_function_declaration(data, token);
                } else {
                    x86_64_generate_keyword(data, token);
                }
                break;
                
            case TOKEN_IDENTIFIER:
                // Handle identifiers in context (variables, function calls, etc.)
                x86_64_generate_identifier_usage(data, token);
                break;
                
            case TOKEN_LITERAL_INT:
            case TOKEN_LITERAL_FLOAT:
                // Handle numeric literals
                x86_64_generate_numeric_literal(data, token);
                break;
                
            case TOKEN_LITERAL_STRING:
                // Handle string literals
                x86_64_generate_string_literal(data, token);
                break;
                
            case TOKEN_OPERATOR:
                // Handle operators (+, -, *, /, etc.)
                x86_64_generate_operator(data, token);
                break;
                
            case TOKEN_PUNCTUATION:
                // Handle punctuation (parentheses, braces, semicolons, etc.)
                x86_64_generate_punctuation(data, token);
                break;
                
            default:
                // Ignore other token types
                break;
        }
    }
    
    // Add a basic main function if none was found
    if (strstr(data->asm_buffer, "main:") == NULL) {
        const char* main_func =
            "main:\n"
            "    push rbp\n"
            "    mov rbp, rsp\n"
            "    # Basic main function (auto-generated)\n"
            "    mov rax, 0\n"
            "    pop rbp\n"
            "    ret\n";
        
        size_t main_len = strlen(main_func);
        if (data->asm_size + main_len + 1 > data->asm_capacity) {
            // Resize the buffer if needed
            char* new_buffer = (char*)realloc(data->asm_buffer, data->asm_size + main_len + 8192);
            if (!new_buffer) {
                DEBUG_PRINT("ERROR: Failed to resize asm_buffer for main function");
                return false;
            }
            data->asm_buffer = new_buffer;
            data->asm_capacity = data->asm_size + main_len + 8192;
        }
        
        // Append the main function
        memcpy(data->asm_buffer + data->asm_size, main_func, main_len);
        data->asm_size += main_len;
        data->asm_buffer[data->asm_size] = '\0';
    }
    
    DEBUG_PRINT("Code generation completed successfully (%zu bytes)", data->asm_size);
    return true;
}

// Helper functions for code generation
static void x86_64_generate_function_declaration(X86_64Data* data, Token* token) {
    // Only process this if we have a valid function name
    if (!token || !token->text) return;
    
    const char* func_template = 
        "%s:\n"
        "    push rbp\n"
        "    mov rbp, rsp\n"
        "    # Function body\n";
    
    // Format the function declaration
    char func_code[1024];
    snprintf(func_code, sizeof(func_code), func_template, token->text);
    
    // Append to the assembly buffer
    size_t func_len = strlen(func_code);
    if (data->asm_size + func_len + 1 > data->asm_capacity) {
        // Resize the buffer if needed
        char* new_buffer = (char*)realloc(data->asm_buffer, data->asm_size + func_len + 8192);
        if (!new_buffer) {
            DEBUG_PRINT("ERROR: Failed to resize asm_buffer for function");
            return;
        }
        data->asm_buffer = new_buffer;
        data->asm_capacity = data->asm_size + func_len + 8192;
    }
    
    // Append the function code
    memcpy(data->asm_buffer + data->asm_size, func_code, func_len);
    data->asm_size += func_len;
    data->asm_buffer[data->asm_size] = '\0';
}

static void x86_64_generate_identifier_usage(X86_64Data* data, Token* token) {
    // Implementation will depend on context (variable access, function call, etc.)
    // For now, just add a comment
    if (!token || !token->text) return;
    
    const char* comment_template = "    # Identifier: %s\n";
    
    // Format the comment
    char comment[1024];
    snprintf(comment, sizeof(comment), comment_template, token->text);
    
    // Append to the assembly buffer
    size_t comment_len = strlen(comment);
    if (data->asm_size + comment_len + 1 > data->asm_capacity) {
        // Resize the buffer if needed
        char* new_buffer = (char*)realloc(data->asm_buffer, data->asm_size + comment_len + 8192);
        if (!new_buffer) {
            DEBUG_PRINT("ERROR: Failed to resize asm_buffer for identifier");
            return;
        }
        data->asm_buffer = new_buffer;
        data->asm_capacity = data->asm_size + comment_len + 8192;
    }
    
    // Append the comment
    memcpy(data->asm_buffer + data->asm_size, comment, comment_len);
    data->asm_size += comment_len;
    data->asm_buffer[data->asm_size] = '\0';
}

static void x86_64_generate_numeric_literal(X86_64Data* data, Token* token) {
    // Implementation for numeric literals
    if (!token || !token->text) return;
    
    const char* comment_template = "    # Numeric literal: %s\n";
    
    // Format the comment
    char comment[1024];
    snprintf(comment, sizeof(comment), comment_template, token->text);
    
    // Append to the assembly buffer
    size_t comment_len = strlen(comment);
    if (data->asm_size + comment_len + 1 > data->asm_capacity) {
        // Resize the buffer if needed
        char* new_buffer = (char*)realloc(data->asm_buffer, data->asm_size + comment_len + 8192);
        if (!new_buffer) {
            DEBUG_PRINT("ERROR: Failed to resize asm_buffer for numeric literal");
            return;
        }
        data->asm_buffer = new_buffer;
        data->asm_capacity = data->asm_size + comment_len + 8192;
    }
    
    // Append the comment
    memcpy(data->asm_buffer + data->asm_size, comment, comment_len);
    data->asm_size += comment_len;
    data->asm_buffer[data->asm_size] = '\0';
}

static void x86_64_generate_string_literal(X86_64Data* data, Token* token) {
    // Implementation for string literals
    if (!token || !token->text) return;
    
    // First, add the string to the .data section if not already present
    // Look for .data section
    if (strstr(data->asm_buffer, ".section .data") == NULL) {
        // Add the .data section
        const char* data_section = 
            "\n.section .data\n";
        
        // Append to the assembly buffer
        size_t section_len = strlen(data_section);
        if (data->asm_size + section_len + 1 > data->asm_capacity) {
            // Resize the buffer if needed
            char* new_buffer = (char*)realloc(data->asm_buffer, data->asm_size + section_len + 8192);
            if (!new_buffer) {
                DEBUG_PRINT("ERROR: Failed to resize asm_buffer for data section");
                return;
            }
            data->asm_buffer = new_buffer;
            data->asm_capacity = data->asm_size + section_len + 8192;
        }
        
        // Append the data section
        memcpy(data->asm_buffer + data->asm_size, data_section, section_len);
        data->asm_size += section_len;
        data->asm_buffer[data->asm_size] = '\0';
    }
    
    // Generate a unique label for the string
    static int string_counter = 0;
    char label[64];
    snprintf(label, sizeof(label), "str_%d", string_counter++);
    
    // Format the string definition
    char string_def[2048];
    snprintf(string_def, sizeof(string_def), 
             "%s: .string %s\n", 
             label, token->text);
    
    // Append to the assembly buffer
    size_t def_len = strlen(string_def);
    if (data->asm_size + def_len + 1 > data->asm_capacity) {
        // Resize the buffer if needed
        char* new_buffer = (char*)realloc(data->asm_buffer, data->asm_size + def_len + 8192);
        if (!new_buffer) {
            DEBUG_PRINT("ERROR: Failed to resize asm_buffer for string definition");
            return;
        }
        data->asm_buffer = new_buffer;
        data->asm_capacity = data->asm_size + def_len + 8192;
    }
    
    // Append the string definition
    memcpy(data->asm_buffer + data->asm_size, string_def, def_len);
    data->asm_size += def_len;
    data->asm_buffer[data->asm_size] = '\0';
    
    // Add a comment about the string usage
    const char* comment_template = "    # String literal (ref: %s): %s\n";
    
    // Format the comment
    char comment[1024];
    snprintf(comment, sizeof(comment), comment_template, label, token->text);
    
    // Append to the assembly buffer
    size_t comment_len = strlen(comment);
    if (data->asm_size + comment_len + 1 > data->asm_capacity) {
        // Resize the buffer if needed
        char* new_buffer = (char*)realloc(data->asm_buffer, data->asm_size + comment_len + 8192);
        if (!new_buffer) {
            DEBUG_PRINT("ERROR: Failed to resize asm_buffer for string comment");
            return;
        }
        data->asm_buffer = new_buffer;
        data->asm_capacity = data->asm_size + comment_len + 8192;
    }
    
    // Append the comment
    memcpy(data->asm_buffer + data->asm_size, comment, comment_len);
    data->asm_size += comment_len;
    data->asm_buffer[data->asm_size] = '\0';
}

static void x86_64_generate_keyword(X86_64Data* data, Token* token) {
    // Implementation for keywords
    if (!token || !token->text) return;
    
    const char* comment_template = "    # Keyword: %s\n";
    
    // Format the comment
    char comment[1024];
    snprintf(comment, sizeof(comment), comment_template, token->text);
    
    // Append to the assembly buffer
    size_t comment_len = strlen(comment);
    if (data->asm_size + comment_len + 1 > data->asm_capacity) {
        // Resize the buffer if needed
        char* new_buffer = (char*)realloc(data->asm_buffer, data->asm_size + comment_len + 8192);
        if (!new_buffer) {
            DEBUG_PRINT("ERROR: Failed to resize asm_buffer for keyword");
            return;
        }
        data->asm_buffer = new_buffer;
        data->asm_capacity = data->asm_size + comment_len + 8192;
    }
    
    // Append the comment
    memcpy(data->asm_buffer + data->asm_size, comment, comment_len);
    data->asm_size += comment_len;
    data->asm_buffer[data->asm_size] = '\0';
    
    // Special handling for "return" keyword
    if (strcmp(token->text, "return") == 0) {
        const char* return_code = 
            "    mov rax, 0  # Return value\n"
            "    pop rbp\n"
            "    ret\n";
        
        // Append to the assembly buffer
        size_t code_len = strlen(return_code);
        if (data->asm_size + code_len + 1 > data->asm_capacity) {
            // Resize the buffer if needed
            char* new_buffer = (char*)realloc(data->asm_buffer, data->asm_size + code_len + 8192);
            if (!new_buffer) {
                DEBUG_PRINT("ERROR: Failed to resize asm_buffer for return");
                return;
            }
            data->asm_buffer = new_buffer;
            data->asm_capacity = data->asm_size + code_len + 8192;
        }
        
        // Append the return code
        memcpy(data->asm_buffer + data->asm_size, return_code, code_len);
        data->asm_size += code_len;
        data->asm_buffer[data->asm_size] = '\0';
    }
}

static void x86_64_generate_operator(X86_64Data* data, Token* token) {
    // Implementation for operators
    if (!token || !token->text) return;
    
    const char* comment_template = "    # Operator: %s\n";
    
    // Format the comment
    char comment[1024];
    snprintf(comment, sizeof(comment), comment_template, token->text);
    
    // Append to the assembly buffer
    size_t comment_len = strlen(comment);
    if (data->asm_size + comment_len + 1 > data->asm_capacity) {
        // Resize the buffer if needed
        char* new_buffer = (char*)realloc(data->asm_buffer, data->asm_size + comment_len + 8192);
        if (!new_buffer) {
            DEBUG_PRINT("ERROR: Failed to resize asm_buffer for operator");
            return;
        }
        data->asm_buffer = new_buffer;
        data->asm_capacity = data->asm_size + comment_len + 8192;
    }
    
    // Append the comment
    memcpy(data->asm_buffer + data->asm_size, comment, comment_len);
    data->asm_size += comment_len;
    data->asm_buffer[data->asm_size] = '\0';
}

static void x86_64_generate_punctuation(X86_64Data* data, Token* token) {
    // Implementation for punctuation
    if (!token || !token->text) return;
    
    // Mostly ignore punctuation except for specific cases
    if (strcmp(token->text, "{") == 0) {
        const char* comment = "    # Begin block\n";
        
        // Append to the assembly buffer
        size_t comment_len = strlen(comment);
        if (data->asm_size + comment_len + 1 > data->asm_capacity) {
            // Resize the buffer if needed
            char* new_buffer = (char*)realloc(data->asm_buffer, data->asm_size + comment_len + 8192);
            if (!new_buffer) {
                DEBUG_PRINT("ERROR: Failed to resize asm_buffer for punctuation");
                return;
            }
            data->asm_buffer = new_buffer;
            data->asm_capacity = data->asm_size + comment_len + 8192;
        }
        
        // Append the comment
        memcpy(data->asm_buffer + data->asm_size, comment, comment_len);
        data->asm_size += comment_len;
        data->asm_buffer[data->asm_size] = '\0';
    }
    else if (strcmp(token->text, "}") == 0) {
        const char* comment = "    # End block\n";
        
        // Append to the assembly buffer
        size_t comment_len = strlen(comment);
        if (data->asm_size + comment_len + 1 > data->asm_capacity) {
            // Resize the buffer if needed
            char* new_buffer = (char*)realloc(data->asm_buffer, data->asm_size + comment_len + 8192);
            if (!new_buffer) {
                DEBUG_PRINT("ERROR: Failed to resize asm_buffer for punctuation");
                return;
            }
            data->asm_buffer = new_buffer;
            data->asm_capacity = data->asm_size + comment_len + 8192;
        }
        
        // Append the comment
        memcpy(data->asm_buffer + data->asm_size, comment, comment_len);
        data->asm_size += comment_len;
        data->asm_buffer[data->asm_size] = '\0';
    }
}

// Emit function
static bool x86_64_emit(CodeGenerator* generator, const char* filename, OutputFormat format) {
    DEBUG_PRINT("Emitting code to %s (format=%d)", filename, format);
    
    if (!generator) {
        DEBUG_PRINT("ERROR: Null generator pointer");
        return false;
    }
    
    if (!filename) {
        DEBUG_PRINT("ERROR: Null filename");
        return false;
    }
    
    X86_64Data* data = (X86_64Data*)generator->arch_data;
    if (!verify_data(data, "x86_64_emit")) {
        return false;
    }
    
    // Check if we have any code to emit
    if (!data->asm_buffer || data->asm_size == 0) {
        DEBUG_PRINT("ERROR: No code to emit");
        return false;
    }
    
    // Open the output file
    FILE* file = fopen(filename, "w");
    if (!file) {
        DEBUG_PRINT("ERROR: Could not open output file: %s", filename);
        return false;
    }
    
    // Add a safety check for buffer overflow
    if (data->asm_size >= data->asm_capacity) {
        DEBUG_PRINT("WARNING: Buffer size is at capacity, might be truncated");
        data->asm_size = data->asm_capacity - 1;
        data->asm_buffer[data->asm_size] = '\0';
    }
    
    // Write the code
    size_t written = fwrite(data->asm_buffer, 1, data->asm_size, file);
    
    // Close the file
    fclose(file);
    
    if (written != data->asm_size) {
        DEBUG_PRINT("ERROR: Failed to write all code (%zu of %zu bytes written)", 
            written, data->asm_size);
        return false;
    }
    
    DEBUG_PRINT("Successfully emitted %zu bytes of code to %s", written, filename);
    return true;
}

// Legacy init function - keep it for backward compatibility
void braggi_codegen_x86_64_init(void) {
    DEBUG_PRINT("Legacy x86_64 init called");
    // This is now just a stub that calls the registration function
    braggi_register_x86_64_backend();
}

// Backend registration - THIS is the function that codegen_manager.c is looking for!
void braggi_register_x86_64_backend(void) {
    DEBUG_PRINT("Registering x86_64 backend");
    
    // Create the generator
    CodeGenerator* generator = (CodeGenerator*)calloc(1, sizeof(CodeGenerator));
    if (!generator) {
        DEBUG_PRINT("ERROR: Failed to allocate generator");
        return;
    }
    
    // Initialize fields with strdup to ensure proper memory management
    generator->name = strdup("x86_64");
    generator->description = strdup("x86_64 backend");
    if (!generator->name || !generator->description) {
        DEBUG_PRINT("ERROR: Failed to allocate strings for generator");
        if (generator->name) free((void*)generator->name);
        if (generator->description) free((void*)generator->description);
        free(generator);
        return;
    }
    
    generator->arch_data = NULL;
    
    // Set function pointers
    generator->init = x86_64_init;
    generator->destroy = x86_64_destroy;
    generator->generate = x86_64_generate;
    generator->emit = x86_64_emit;
    generator->register_function = NULL;
    generator->optimize = NULL;
    generator->generate_debug_info = NULL;
    
    // Register with manager
    extern bool braggi_codegen_manager_register_backend(CodeGenerator* generator);
    DEBUG_PRINT("Calling manager register function");
    if (!braggi_codegen_manager_register_backend(generator)) {
        DEBUG_PRINT("ERROR: Failed to register with manager");
        if (generator->name) free((void*)generator->name);
        if (generator->description) free((void*)generator->description);
        free(generator);
        return;
    }
    
    DEBUG_PRINT("Successfully registered x86_64 backend (generator=%p)", (void*)generator);
}
