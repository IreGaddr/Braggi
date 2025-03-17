/*
 * Braggi - Code Generation Implementation
 * 
 * "Good code generation is like a good cattle brand - 
 * clear, distinctive, and gets the job done!" - Texas Compiler Ranch
 */

#include "braggi/codegen.h"
#include "braggi/braggi_context.h"
#include "braggi/token_propagator.h"
#include "braggi/codegen_arch.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>  // For PATH_MAX
#include <unistd.h>  // For usleep

// Forward declaration of internal function
static bool braggi_codegen_generate_file(CodeGenContext* ctx, Source* source, const char* output_path);

// Initialize the code generator
bool braggi_codegen_init(CodeGenContext* ctx, BraggiContext* braggi_ctx, CodeGenOptions options) {
    if (!ctx || !braggi_ctx) {
        fprintf(stderr, "ERROR: Invalid context parameters in braggi_codegen_init\n");
        return false;
    }
    
    ctx->braggi_ctx = braggi_ctx;
    ctx->options = options;
    ctx->arch_data = NULL;
    ctx->generator = NULL; // Initialize the generator pointer
    
    // Add extra safety checks to ensure context data is valid
    if (!braggi_ctx->source) {
        fprintf(stderr, "ERROR: No source files loaded in context\n");
        return false;
    }
    
    // Critical safety check: ensure token propagator results are valid
    if (braggi_ctx->propagator) {
        fprintf(stderr, "DEBUG: Checking for tokens in propagator\n");
        
        // Use the API function to get output tokens safely
        Vector* output_tokens = braggi_token_propagator_get_output_tokens(braggi_ctx->propagator);
        
        if (output_tokens) {
            size_t token_count = braggi_vector_size(output_tokens);
            fprintf(stderr, "DEBUG: Found %zu tokens in propagator output\n", token_count);
            
            // Check if we need to update tokens from propagator to context
            if (!braggi_ctx->tokens || braggi_vector_size(braggi_ctx->tokens) == 0) {
                fprintf(stderr, "DEBUG: Transferring %zu output tokens from propagator to context\n", token_count);
                
                // If context has no tokens but propagator has output tokens, use those
                if (!braggi_ctx->tokens) {
                    // Create a new vector for tokens if needed
                    braggi_ctx->tokens = braggi_vector_create(sizeof(Token*));
                    if (!braggi_ctx->tokens) {
                        fprintf(stderr, "ERROR: Failed to create tokens vector in context\n");
                        return false;
                    }
                } else {
                    // If tokens vector exists but is empty, clear it to be safe
                    braggi_vector_clear(braggi_ctx->tokens);
                }
                
                // Copy tokens from propagator to context with careful error checking
                size_t tokens_transferred = 0;
                for (size_t i = 0; i < token_count; i++) {
                    Token** token_ptr = (Token**)braggi_vector_get(output_tokens, i);
                    if (token_ptr && *token_ptr) {
                        if (braggi_vector_push_back(braggi_ctx->tokens, token_ptr)) {
                            tokens_transferred++;
                        } else {
                            fprintf(stderr, "WARNING: Failed to transfer token %zu to context\n", i);
                        }
                    } else {
                        fprintf(stderr, "WARNING: NULL token at index %zu in propagator output\n", i);
                    }
                }
                
                fprintf(stderr, "DEBUG: Transferred %zu of %zu tokens from propagator to context\n", 
                        tokens_transferred, token_count);
            } else {
                // Context already has tokens
                fprintf(stderr, "DEBUG: Context already has %zu tokens, no transfer needed\n", 
                       braggi_vector_size(braggi_ctx->tokens));
            }
        } else {
            fprintf(stderr, "WARNING: No output tokens available from propagator API function\n");
            
            // Try another approach - just create empty tokens vector if needed
            if (!braggi_ctx->tokens) {
                fprintf(stderr, "DEBUG: Creating empty tokens vector in context\n");
                braggi_ctx->tokens = braggi_vector_create(sizeof(Token*));
                if (!braggi_ctx->tokens) {
                    fprintf(stderr, "ERROR: Failed to create tokens vector in context\n");
                }
            }
        }
    } else {
        fprintf(stderr, "WARNING: No token propagator available in context\n");
    }
    
    // Final check for tokens
    if (braggi_ctx->tokens) {
        fprintf(stderr, "DEBUG: Context has %zu tokens after initialization\n", 
               braggi_vector_size(braggi_ctx->tokens));
    }
    
    // Initialize the code generation manager if needed
    extern bool braggi_codegen_manager_init(void);
    if (!braggi_codegen_manager_init()) {
        fprintf(stderr, "ERROR: Failed to initialize code generation manager\n");
        return false;
    }
    
    // Get the appropriate backend from the manager
    extern CodeGenerator* braggi_codegen_manager_get_backend(TargetArch arch);
    ctx->generator = braggi_codegen_manager_get_backend(options.arch);
    
    if (!ctx->generator) {
        fprintf(stderr, "ERROR: Failed to get backend for architecture %s\n", 
                braggi_codegen_arch_to_string(options.arch));
        return false;
    }
    
    // Initialize the backend
    if (ctx->generator->init && !ctx->generator->init(ctx->generator, NULL)) {
        fprintf(stderr, "ERROR: Failed to initialize backend %s\n", ctx->generator->name);
        return false;
    }
    
    fprintf(stderr, "DEBUG: Code generator initialized successfully with backend: %s\n", 
            ctx->generator->name);
    return true;
}

// Generate code from the AST
bool braggi_codegen_generate(CodeGenContext* ctx) {
    if (!ctx || !ctx->braggi_ctx) {
        fprintf(stderr, "ERROR: Invalid context parameters in braggi_codegen_generate\n");
        return false;
    }
    
    // Validate that we have a generator
    if (!ctx->generator) {
        fprintf(stderr, "ERROR: No code generator backend initialized\n");
        return false;
    }
    
    // Debug output to help track code generation progress
    fprintf(stderr, "DEBUG: Starting code generation process with %s backend\n", 
            braggi_codegen_arch_to_string(ctx->options.arch));
    
    // Get the token propagator from the context
    TokenPropagator* propagator = ctx->braggi_ctx->propagator;
    if (!propagator) {
        fprintf(stderr, "ERROR: No token propagator available in context\n");
        return false;
    }
    
    // Ensure the token propagator has run successfully
    if (!braggi_token_propagator_run(propagator)) {
        fprintf(stderr, "ERROR: Token propagator failed to run\n");
        return false;
    }
    
    // Get the entropy field from the token propagator
    EntropyField* field = braggi_token_propagator_get_field(propagator);
    if (!field) {
        fprintf(stderr, "ERROR: No entropy field available from token propagator\n");
        return false;
    }
    
    // Set the entropy field in the code generator options
    ctx->options.entropy_field = field;
    
    // Call the backend's generate function
    fprintf(stderr, "DEBUG: Calling backend's generate function\n");
    if (!ctx->generator->generate(ctx->generator, field)) {
        fprintf(stderr, "ERROR: Backend code generation failed\n");
        return false;
    }
    
    fprintf(stderr, "DEBUG: Code generation completed successfully\n");
    return true;
}

// Implementation of internal function to generate code for a specific file
static bool braggi_codegen_generate_file(CodeGenContext* ctx, Source* source, const char* output_path) {
    if (!ctx || !source || !output_path) {
        fprintf(stderr, "ERROR: Invalid parameters for braggi_codegen_generate_file\n");
        return false;
    }
    
    // Set the current source in the context
    ctx->braggi_ctx->source = source;
    
    // Create a new token propagator 
    TokenPropagator* propagator = braggi_token_propagator_create();
    if (!propagator) {
        fprintf(stderr, "ERROR: Failed to create token propagator\n");
        return false;
    }
    
    // Set the token propagator in the context
    ctx->braggi_ctx->propagator = propagator;
    
    // Log the source file being processed
    fprintf(stderr, "DEBUG: Processing source file: %s (ID: %u)\n", 
            source->filename ? source->filename : "(unnamed)", 
            source->file_id);
    
    // Initialize the token propagator's entropy field
    if (!braggi_token_propagator_initialize_field(propagator)) {
        fprintf(stderr, "ERROR: Failed to initialize token propagator's entropy field\n");
        return false;
    }
    
    // Tokenize the source file and add tokens to the propagator
    Vector* tokens = braggi_tokenize_all(source, true, true);
    if (!tokens) {
        fprintf(stderr, "ERROR: Failed to tokenize source file\n");
        return false;
    }
    
    for (size_t i = 0; i < braggi_vector_size(tokens); i++) {
        Token* token = (Token*)braggi_vector_get(tokens, i);
        if (!braggi_token_propagator_add_token(propagator, token)) {
            fprintf(stderr, "ERROR: Failed to add token to propagator\n");
            braggi_vector_destroy(tokens);
            return false;
        }
    }
    
    // Clean up the token vector (but not the tokens themselves, as they're now owned by the propagator)
    braggi_vector_destroy(tokens);
    
    // Run the token propagator to apply constraints and collapse the field
    if (!braggi_token_propagator_run(propagator)) {
        fprintf(stderr, "ERROR: Token propagator failed to run\n");
        return false;
    }
    
    // Generate code
    if (!braggi_codegen_generate(ctx)) {
        fprintf(stderr, "ERROR: Code generation failed\n");
        return false;
    }
    
    // Write the output
    if (!braggi_codegen_write_output(ctx, output_path)) {
        fprintf(stderr, "ERROR: Failed to write output to %s\n", output_path);
        return false;
    }
    
    return true;
}

// Write output to a file
bool braggi_codegen_write_output(CodeGenContext* ctx, const char* filename) {
    if (!ctx || !filename) {
        fprintf(stderr, "ERROR: Invalid parameters for braggi_codegen_write_output\n");
        return false;
    }
    
    if (!ctx->generator) {
        fprintf(stderr, "ERROR: No code generator backend available\n");
        return false;
    }
    
    if (!ctx->generator->emit) {
        fprintf(stderr, "ERROR: Backend does not support emit operation\n");
        return false;
    }
    
    fprintf(stderr, "DEBUG: Writing output to %s using %s backend\n", 
            filename, ctx->generator->name);
    
    // Call the backend's emit function
    if (!ctx->generator->emit(ctx->generator, filename, ctx->options.format)) {
        fprintf(stderr, "ERROR: Failed to emit code to %s\n", filename);
        return false;
    }
    
    fprintf(stderr, "DEBUG: Successfully wrote output to %s\n", filename);
    return true;
}

// Clean up the code generator
void braggi_codegen_cleanup(CodeGenContext* ctx) {
    if (!ctx) {
        fprintf(stderr, "DEBUG: Cleanup called with NULL context\n");
        return;
    }
    
    fprintf(stderr, "DEBUG: Starting code generator cleanup\n");
    
    // Clean up the generator
    if (ctx->generator) {
        fprintf(stderr, "DEBUG: Cleaning up generator (addr=%p)\n", (void*)ctx->generator);
        
        // Check if the generator has a destroy function
        if (ctx->generator->destroy) {
            fprintf(stderr, "DEBUG: Calling generator->destroy\n");
            
            // Save a copy of the pointer for safety
            CodeGenerator* temp_generator = ctx->generator;
            
            // Clear the pointer in the context first to prevent use-after-free
            ctx->generator = NULL;
            
            // Call the destroy function
            temp_generator->destroy(temp_generator);
            
            // Notify the code generator manager that this generator has been destroyed
            // This helps prevent issues where the manager might try to use the generator
            // after it's been destroyed
            extern void braggi_codegen_manager_mark_generator_destroyed(CodeGenerator* generator);
            braggi_codegen_manager_mark_generator_destroyed(temp_generator);
            
            fprintf(stderr, "DEBUG: Generator destroy completed\n");
        } else {
            fprintf(stderr, "DEBUG: Generator has no destroy function, just clearing pointer\n");
            ctx->generator = NULL;
        }
    } else {
        fprintf(stderr, "DEBUG: No generator to clean up\n");
    }
    
    // Free architecture-specific data if it exists
    if (ctx->arch_data) {
        fprintf(stderr, "DEBUG: Freeing architecture-specific data (addr=%p)\n", (void*)ctx->arch_data);
        free(ctx->arch_data);
        ctx->arch_data = NULL;
    }
    
    // Handle the entropy field reference (don't free, just clear)
    if (ctx->options.entropy_field) {
        fprintf(stderr, "DEBUG: Clearing entropy field reference\n");
        // We don't free the entropy field here - it's owned by the BraggiContext
        ctx->options.entropy_field = NULL;
    }
    
    // Handle output file reference (we might own this)
    if (ctx->options.output_file) {
        // In a real implementation, we'd have ownership tracking
        // For now, we'll just clear the reference
        fprintf(stderr, "DEBUG: Clearing output file reference\n");
        ctx->options.output_file = NULL;
    }
    
    // Reset the context to a clean state
    fprintf(stderr, "DEBUG: Resetting context to clean state\n");
    ctx->braggi_ctx = NULL;
    ctx->options.arch = ARCH_X86_64;
    ctx->options.format = FORMAT_EXECUTABLE;
    ctx->options.optimize = false;
    ctx->options.optimization_level = 0;
    ctx->options.emit_debug_info = true;
    
    fprintf(stderr, "DEBUG: Code generator cleanup complete\n");
    
    // Add a small delay to ensure everything is flushed
    usleep(1000);  // Sleep for 1ms to prevent immediate segfault
}

// Get default code generator options for an architecture
CodeGenOptions braggi_codegen_get_default_options(TargetArch arch) {
    CodeGenOptions options;
    
    options.arch = arch;
    options.format = FORMAT_EXECUTABLE;
    options.optimize = false;
    options.optimization_level = 0;
    options.emit_debug_info = true;
    options.output_file = NULL;
    
    return options;
}

// Get string representation of an architecture
const char* braggi_codegen_arch_to_string(TargetArch arch) {
    switch (arch) {
        case ARCH_X86:
            return "x86";
        case ARCH_X86_64:
            return "x86_64";
        case ARCH_ARM:
            return "ARM";
        case ARCH_ARM64:
            return "ARM64";
        case ARCH_WASM:
            return "WebAssembly";
        case ARCH_BYTECODE:
            return "Bytecode";
        default:
            return "Unknown";
    }
}

// Get string representation of an output format
const char* braggi_codegen_format_to_string(OutputFormat format) {
    switch (format) {
        case FORMAT_OBJECT:
            return "Object";
        case FORMAT_EXECUTABLE:
            return "Executable";
        case FORMAT_LIBRARY:
            return "Library";
        case FORMAT_ASM:
            return "Assembly";
        default:
            return "Unknown";
    }
} 
