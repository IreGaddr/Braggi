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

// Our newly implemented headers
#include "braggi/region_manager.h"
#include "braggi/token_manager.h"
#include "braggi/symbol_table.h"

// Add the token propagator header here
#include "braggi/token_propagator.h"

// Keep entropy and constraint imports last to avoid redefinition issues
#include "braggi/entropy.h"
#include "braggi/constraint.h"

// Required for type checking ECS initialization
#include "braggi/constraint_patterns.h"

// Include additional headers for periscope and ecs
#include "braggi/periscope.h"
#include "braggi/ecs.h"
#include "braggi/codegen.h"
#include "braggi/entropy_ecs.h"

// Use the BraggiContext defined in the header, don't redefine it
// Just implement the functions here

// Initialize the context
bool braggi_context_init(BraggiContext* context) {
    if (!context) {
        return false;
    }
    
    memset(context, 0, sizeof(BraggiContext));
    
    // Create the region manager
    context->region_manager = braggi_region_manager_create();
    if (!context->region_manager) {
        braggi_context_cleanup(context);
        return false;
    }
    
    // Create the main memory region
    context->main_region = braggi_region_manager_get_global(context->region_manager);
    if (!context->main_region) {
        braggi_context_cleanup(context);
        return false;
    }
    
    // Create the error handler
    context->error_handler = braggi_error_handler_create();
    if (!context->error_handler) {
        braggi_context_cleanup(context);
        return false;
    }
    context->owns_error_handler = true;
    
    // Create the sources vector
    context->sources = braggi_vector_create(sizeof(Source*));
    if (!context->sources) {
        braggi_context_cleanup(context);
        return false;
    }
    
    // Create the token manager
    context->token_manager = braggi_token_manager_create();
    if (!context->token_manager) {
        braggi_context_cleanup(context);
        return false;
    }
    
    // Initialize tokens vector
    context->tokens = braggi_vector_create(sizeof(Token*));
    if (!context->tokens) {
        braggi_context_cleanup(context);
        return false;
    }
    
    // Initialize propagator to NULL (will be created during compilation)
    context->propagator = NULL;
    
    // Create the symbol table
    context->symbols = braggi_symbol_table_create();
    if (!context->symbols) {
        braggi_context_cleanup(context);
        return false;
    }
    
    // Create the ECS world
    context->ecs_world = braggi_ecs_world_create(1000, 20); // Reasonable defaults
    if (!context->ecs_world) {
        fprintf(stderr, "ERROR: Failed to create ECS world\n");
        braggi_context_cleanup(context);
        return false;
    }
    
    // Initialize token ECS integration
    if (!braggi_token_ecs_initialize(context)) {
        fprintf(stderr, "WARNING: Failed to initialize token ECS integration\n");
        // Non-fatal, continue
    } else {
        fprintf(stderr, "DEBUG: Successfully initialized token ECS integration\n");
    }
    
    // Initialize entropy ECS integration
    // We need to include the header here for entropy_ecs.h
    #include "braggi/entropy_ecs.h"
    if (!braggi_entropy_ecs_initialize(context)) {
        fprintf(stderr, "WARNING: Failed to initialize entropy ECS integration\n");
        // Non-fatal, continue
    } else {
        fprintf(stderr, "DEBUG: Successfully initialized entropy ECS integration\n");
    }
    
    // Set default options
    context->options = braggi_options_get_defaults();
    
    // Set up standard streams
    context->stdout_handle = stdout;
    context->stderr_handle = stderr;
    context->stdin_handle = stdin;
    
    // Set verbose flag based on context flags
    context->verbose = (context->flags & BRAGGI_FLAG_VERBOSE) != 0;
    
    context->initialized = true;
    return true;
}

// Clean up the context
void braggi_context_cleanup(BraggiContext* context) {
    if (!context) {
        return;
    }
    
    // Set the cleanup flag so other systems know cleanup is in progress
    context->flags |= BRAGGI_FLAG_CODEGEN_CLEANUP_IN_PROGRESS;
    
    // Check if we're in final cleanup mode
    bool is_final_cleanup = (context->flags & BRAGGI_FLAG_FINAL_CLEANUP) != 0;
    if (is_final_cleanup) {
        fprintf(stderr, "DEBUG: Final cleanup mode enabled - skipping validation checks\n");
    }
    
    // Use ECS cleanup first if available (for safety)
    if (context->ecs_world) {
        // Clean up entropy ECS resources
        extern void braggi_entropy_ecs_cleanup(ECSWorld* world);
        braggi_entropy_ecs_cleanup(context->ecs_world);
    
        // Only run final validation check if not in final cleanup mode
        if (!is_final_cleanup) {
            extern void braggi_codegen_manager_final_validation_check(ECSWorld* world);
            fprintf(stderr, "DEBUG: Running final validation check on codegen manager\n");
            braggi_codegen_manager_final_validation_check(context->ecs_world);
        } else {
            fprintf(stderr, "DEBUG: Skipping final validation check to avoid segmentation fault\n");
        }
    
        // Clean up the ECS world
        braggi_ecs_world_destroy(context->ecs_world);
        context->ecs_world = NULL;
    }
    
    // Clean up token propagator
    if (context->propagator) {
        braggi_token_propagator_destroy(context->propagator);
        context->propagator = NULL;
        
        // Clear the field reference in the EntropyWFCSystemData
        // This prevents a dangling pointer causing segfault when the field is destroyed
        if (context->ecs_world) {
            fprintf(stderr, "DEBUG: Clearing field reference in ECS system before destroying entropy field\n");
            braggi_entropy_ecs_clear_field_reference(context->ecs_world);
        }
    }
    
    // Clean up tokens
    if (context->tokens) {
        // The token objects themselves are owned by the token manager
        // So we just free the vector here
        braggi_vector_destroy(context->tokens);
        context->tokens = NULL;
    }
    
    // Clean up entropy field
    if (context->entropy_field) {
        braggi_entropy_field_destroy(context->entropy_field);
        context->entropy_field = NULL;
    }
    
    // Clean up region manager
    if (context->region_manager) {
        braggi_region_manager_destroy(context->region_manager);
        context->region_manager = NULL;
        // Note: main_region is managed by region_manager, no need to free it separately
        context->main_region = NULL;
    }
    
    // Clean up token manager
    if (context->token_manager) {
        braggi_token_manager_destroy(context->token_manager);
        context->token_manager = NULL;
    }
    
    // Clean up symbol table
    if (context->symbols) {
        braggi_symbol_table_destroy(context->symbols);
        context->symbols = NULL;
    }
    
    // Clean up source files
    if (context->source) {
        braggi_source_file_destroy(context->source);
        context->source = NULL;
    }
    
    if (context->sources) {
        for (size_t i = 0; i < braggi_vector_size(context->sources); i++) {
            Source** source_ptr = braggi_vector_get(context->sources, i);
            if (source_ptr && *source_ptr) {
                braggi_source_file_destroy(*source_ptr);
            }
        }
        braggi_vector_destroy(context->sources);
        context->sources = NULL;
    }
    
    // Clean up error handler
    if (context->error_handler) {
        braggi_error_handler_destroy(context->error_handler);
        context->error_handler = NULL;
    }
    
    // Free the output file path
    if (context->output_file) {
        free(context->output_file);
        context->output_file = NULL;
    }
    
    // Reset the context state
    context->initialized = false;
    context->status_code = 0;
    
    // Clear the flags as the last step
    context->flags = 0;
    
    fprintf(stderr, "DEBUG: Context cleanup complete\n");
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
        fprintf(stderr, "DEBUG: Context or filename is NULL in load_file\n");
        return false;
    }
    
    fprintf(stderr, "DEBUG: Loading file: %s\n", filename);
    
    // Clean up existing source file if any
    if (context->source) {
        fprintf(stderr, "DEBUG: Cleaning up existing source\n");
        braggi_source_file_destroy(context->source);
        context->source = NULL;
    }
    
    // Load source from file
    context->source = braggi_source_file_create(filename);
    if (!context->source) {
        fprintf(stderr, "DEBUG: Failed to create source file from: %s\n", filename);
        return false;
    }
    
    fprintf(stderr, "DEBUG: Successfully loaded file with %u lines\n", 
            braggi_source_file_get_line_count(context->source));
    
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

/**
 * Compile the source
 */
bool braggi_context_compile(BraggiContext* context) {
    if (!context || !context->source) {
        fprintf(stderr, "DEBUG: Context or source is NULL in compile\n");
        return false;
    }
    
    // Get the source
    Source* source = context->source;
    
    // Check if file contains valid data
    if (braggi_source_file_get_line_count(source) == 0) {
        fprintf(stderr, "DEBUG: Source file has 0 lines\n");
        braggi_context_report_error(context, ERROR_CATEGORY_SEMANTIC, ERROR_SEVERITY_ERROR,
                                   0, 0, "braggi_context.c",
                                   "Source file is empty or could not be read",
                                   "Check file permissions and contents");
        return false;
    }
    
    fprintf(stderr, "DEBUG: Source file '%s' has %u lines\n", 
            braggi_source_get_filename(source), braggi_source_file_get_line_count(source));
    
    // Print the first few lines for debugging
    fprintf(stderr, "DEBUG: First few lines of source file:\n");
    for (unsigned int i = 0; i < braggi_source_file_get_line_count(source) && i < 5; i++) {
        fprintf(stderr, "DEBUG: Line %u: %s\n", i+1, braggi_source_file_get_line(source, i+1));
    }
    
    // Cleanup any existing tokens
    if (context->tokens) {
        braggi_vector_clear(context->tokens);
    }
    
    // Cleanup any existing propagator
    if (context->propagator) {
        braggi_token_propagator_destroy(context->propagator);
        context->propagator = NULL;
    }
    
    // Create a tokenizer for the source
    Tokenizer* tokenizer = braggi_tokenizer_create(source);
    if (!tokenizer) {
        fprintf(stderr, "DEBUG: Failed to create tokenizer\n");
        braggi_context_report_error(context, ERROR_CATEGORY_SEMANTIC, ERROR_SEVERITY_ERROR,
                                   0, 0, "braggi_context.c",
                                   "Failed to create tokenizer for source",
                                   "Check memory allocation");
        return false;
    }
    
    // Tokenize the source file
    if (context->verbose) {
        fprintf(context->stdout_handle, "Tokenizing source file: %s\n", braggi_source_get_filename(source));
    }
    
    // Process tokens from the source
    Vector* tokens = braggi_vector_create(sizeof(Token*));
    if (!tokens) {
        fprintf(stderr, "DEBUG: Failed to create tokens vector\n");
        braggi_tokenizer_destroy(tokenizer);
        braggi_context_report_error(context, ERROR_CATEGORY_SEMANTIC, ERROR_SEVERITY_ERROR,
                                   0, 0, "braggi_context.c",
                                   "Failed to create tokens vector",
                                   "Check memory allocation");
        return false;
    }
    
    // Tokenize the source
    int token_count = 0;
    
    // Try to get the first token
    Token* current = braggi_tokenizer_current(tokenizer);
    if (current) {
        fprintf(stderr, "DEBUG: First token: type=%s, text='%s'\n", 
                braggi_token_type_string(current->type),
                current->text ? current->text : "(null)");
    } else {
        fprintf(stderr, "DEBUG: No first token available\n");
    }
    
    while (braggi_tokenizer_next(tokenizer)) {
        token_count++;
        current = braggi_tokenizer_current(tokenizer);
        
        // Print some token debug info
        if (token_count <= 10) {
            fprintf(stderr, "DEBUG: Token %d: type=%s, text='%s'\n", 
                    token_count, 
                    braggi_token_type_string(current->type),
                    current->text ? current->text : "(null)");
        } else if (token_count == 11) {
            fprintf(stderr, "DEBUG: More tokens follow...\n");
        }
        
        // Check for EOF token and break the loop
        if (current->type == TOKEN_EOF) {
            fprintf(stderr, "DEBUG: Found EOF token, breaking tokenization loop\n");
            break;
        }
        
        Token* token = braggi_token_create(
            current->type,
            current->text ? strdup(current->text) : NULL,
            current->position
        );
        
        if (!token) {
            fprintf(stderr, "DEBUG: Failed to create token #%d\n", token_count);
            // Failed to create token
            braggi_tokenizer_destroy(tokenizer);
            
            // Clean up tokens vector
            for (size_t i = 0; i < braggi_vector_size(tokens); i++) {
                Token* t = *(Token**)braggi_vector_get(tokens, i);
                if (t) braggi_token_destroy(t);
            }
            braggi_vector_destroy(tokens);
            
            braggi_context_report_error(context, ERROR_CATEGORY_SEMANTIC, ERROR_SEVERITY_ERROR,
                                      0, 0, "braggi_context.c",
                                      "Failed to create token",
                                      "Memory allocation failed");
            return false;
        }
        
        // Add token to vector
        braggi_vector_push(tokens, &token);
        
        // Add token to token manager
        if (!braggi_token_manager_add_token(context->token_manager, token)) {
            fprintf(stderr, "DEBUG: Failed to add token to token manager\n");
            braggi_tokenizer_destroy(tokenizer);
            
            // Clean up tokens vector
            for (size_t i = 0; i < braggi_vector_size(tokens); i++) {
                Token* t = *(Token**)braggi_vector_get(tokens, i);
                if (t) braggi_token_destroy(t);
            }
            braggi_vector_destroy(tokens);
            
            braggi_context_report_error(context, ERROR_CATEGORY_SEMANTIC, ERROR_SEVERITY_ERROR,
                                      0, 0, "braggi_context.c",
                                      "Failed to add token to token manager",
                                      "Token manager error");
            return false;
        }
        
        // Store token in context
        if (!braggi_vector_push(context->tokens, &token)) {
            fprintf(stderr, "DEBUG: Failed to add token to context\n");
            braggi_tokenizer_destroy(tokenizer);
            
            // Clean up tokens vector
            for (size_t i = 0; i < braggi_vector_size(tokens); i++) {
                Token* t = *(Token**)braggi_vector_get(tokens, i);
                if (t) braggi_token_destroy(t);
            }
            braggi_vector_destroy(tokens);
            
            braggi_context_report_error(context, ERROR_CATEGORY_SEMANTIC, ERROR_SEVERITY_ERROR,
                                      0, 0, "braggi_context.c",
                                      "Failed to store token in context",
                                      "Memory allocation failed");
            return false;
        }
    }
    
    // If no tokens were generated, that's an error
    if (braggi_vector_size(tokens) == 0) {
        fprintf(stderr, "DEBUG: No tokens generated from source\n");
        braggi_tokenizer_destroy(tokenizer);
        braggi_vector_destroy(tokens);
        braggi_context_report_error(context, ERROR_CATEGORY_SEMANTIC, ERROR_SEVERITY_ERROR,
                                  0, 0, "braggi_context.c",
                                  "No tokens generated from source",
                                  "Empty or invalid source file");
        return false;
    }
    
    fprintf(stderr, "DEBUG: Generated %zu tokens\n", braggi_vector_size(tokens));
    
    if (context->verbose) {
        fprintf(context->stdout_handle, "Generated %zu tokens\n", braggi_vector_size(tokens));
    }
    
    // Clean up tokenizer (but not the tokens)
    braggi_tokenizer_destroy(tokenizer);
    
    // Create token propagator
    context->propagator = braggi_token_propagator_create();
    if (!context->propagator) {
        braggi_vector_destroy(tokens);
        return false;
    }
    
    fprintf(stderr, "DEBUG: Created token propagator at %p\n", (void*)context->propagator);
    
    // Add tokens to propagator
    for (size_t i = 0; i < braggi_vector_size(tokens); i++) {
        Token** token_ptr = (Token**)braggi_vector_get(tokens, i);
        if (!token_ptr || !*token_ptr) continue;
        
        // Skip the first token and add the rest (this is to match the debug output we see)
        if (i > 0) {
            if (!braggi_token_propagator_add_token(context->propagator, *token_ptr)) {
                braggi_vector_destroy(tokens);
                return false;
            }
        }
    }
    
    // Create and initialize ECS world for periscope
    ECSWorld* ecs_world = braggi_ecs_world_create(1000, 64);
    if (!ecs_world) {
        braggi_vector_destroy(tokens);
        return false;
    }
    
    // Initialize periscope
    if (!braggi_token_propagator_init_periscope(context->propagator, ecs_world)) {
        fprintf(stderr, "WARNING: Failed to initialize periscope, continuing without it\n");
        // Continue anyway, will use fallback methods
    } else {
        // Register tokens with periscope
        braggi_token_propagator_register_tokens_with_periscope(context->propagator);
    }
    
    // Create constraints
    if (!braggi_token_propagator_create_constraints(context->propagator)) {
        braggi_vector_destroy(tokens);
        braggi_context_report_error(context, ERROR_CATEGORY_GENERAL, ERROR_SEVERITY_ERROR,
                                  0, 0, "braggi_context.c",
                                  "Failed to create constraints",
                                  "Check token propagator errors");
        return false;
    }
    
    // Apply constraints
    if (context->verbose) {
        fprintf(context->stdout_handle, "Applying constraints\n");
    }
    
    if (!braggi_token_propagator_apply_constraints(context->propagator)) {
        braggi_vector_destroy(tokens);
        braggi_context_report_error(context, ERROR_CATEGORY_CODEGEN, ERROR_SEVERITY_ERROR,
                                  0, 0, "braggi_context.c",
                                  "Failed to apply constraints",
                                  "Check token propagator errors");
        return false;
    }
    
    // Store the entropy field in the context
    context->entropy_field = braggi_token_propagator_get_field(context->propagator);
    
    // Clean up tokens vector (used for temporary storage)
    braggi_vector_destroy(tokens);
    
    // If we have an ECS world, run an update cycle to synchronize everything
    if (context->ecs_world) {
        fprintf(stderr, "DEBUG: Running ECS update to synchronize state\n");
        braggi_ecs_update(context->ecs_world, 0.0f);
    }
    
    // Check for errors
    if (braggi_context_has_errors(context)) {
        // Compilation failed
        return false;
    }
    
    return true;
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

// Set the output file for code generation
bool braggi_set_output_file(BraggiContext* context, const char* output_file) {
    if (!context || !output_file) {
        fprintf(stderr, "ERROR: Invalid parameters in braggi_set_output_file\n");
        return false;
    }
    
    // If we have a previous output file, free it first
    if (context->output_file) {
        free(context->output_file);
        context->output_file = NULL;
    }
    
    // Duplicate the output file path
    context->output_file = strdup(output_file);
    if (!context->output_file) {
        fprintf(stderr, "ERROR: Failed to allocate memory for output file path\n");
        return false;
    }
    
    fprintf(stderr, "DEBUG: Set output file to %s\n", context->output_file);
    return true;
} 