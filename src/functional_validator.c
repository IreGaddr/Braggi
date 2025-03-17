/*
 * Braggi - Functional Validator Implementation
 * 
 * "When yer tokens need to dance like a Texas two-step,
 * ya need a functional validator that can call the tune!"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "braggi/token_propagator.h"
#include "braggi/entropy.h"
#include "braggi/periscope.h"
#include "braggi/pattern.h"
#include "braggi/constraint.h"
#include "braggi/token.h"
#include "braggi/util/vector.h"

// Define logging macros
#define DEBUG(msg, ...) fprintf(stderr, "DEBUG: " msg "\n", ##__VA_ARGS__)
#define WARNING(msg, ...) fprintf(stderr, "WARNING: " msg "\n", ##__VA_ARGS__)
#define ERROR(msg, ...) fprintf(stderr, "ERROR: " msg "\n", ##__VA_ARGS__)

// External function declarations
extern Periscope* braggi_constraint_patterns_get_periscope(void);
extern bool (*get_pattern_function(const char* name))(EntropyField*, Vector*, void*);

// State is EntropyState - for backward compatibility
typedef EntropyState State;

// Function type for token-based validators
typedef bool (*TokenValidator)(Token**, size_t, void*);

// Functional context for pattern validation
typedef struct {
    bool initialized;            // Whether the context has been initialized
    Token* token;                // Current token being matched
    Vector* tokens;              // All tokens to match
    size_t current_token_index;  // Index of the current token
    ConstraintPatternLibrary* library;   // The pattern library
    Pattern* root_pattern;       // The root pattern to match
    Vector* pattern_stack;       // Stack of patterns being matched
    bool matched;                // Whether the validation succeeded
    bool exhausted;              // Whether all tokens have been matched
    bool error;                  // Whether an error occurred
    const char* error_message;   // Error message if an error occurred
    PatternType pattern_type;    // Current pattern type
    void* user_context;          // Custom context data
    Periscope* periscope;        // Reference to periscope system
    EntropyField* field;         // Reference to entropy field
    TokenValidator validator_func; // Function validator for tokens
} FunctionalContext;

// State stack entry for backtracking
typedef struct StateStackEntry {
    size_t state_index;          // Current state index
    size_t pattern_index;        // Current pattern index
    PatternType pattern_type;    // Current pattern type
} StateStackEntry;

// Helper function to check if a token matches a specific pattern
static bool token_matches_pattern(EntropyState* state, Pattern* pattern) {
    if (!state || !pattern) return false;
    
    // EntropyState doesn't have a direct token member; 
    // Using the state's data member as it might store the token
    Token* token = (Token*)state->data;
    if (!token) return false;
    
    if (pattern->type != PATTERN_TOKEN) return false;
    
    // Check if the token type matches
    if (pattern->token_type != token->type) return false;
    
    // If the pattern specifies a token value, check if it matches
    if (pattern->token_value) {
        return token->text && strcmp(pattern->token_value, token->text) == 0;
    }
    
    // If no specific value is required, any token of the right type matches
    return true;
}

// Create a functional context for validation
void* braggi_create_functional_context(Pattern* pattern, ConstraintPatternLibrary* library) {
    if (!pattern || !library) return NULL;
    
    FunctionalContext* context = malloc(sizeof(FunctionalContext));
    if (!context) return NULL;
    
    memset(context, 0, sizeof(FunctionalContext));
    
    context->root_pattern = pattern;
    context->library = library;
    context->pattern_stack = braggi_vector_create(sizeof(Pattern*));
    context->tokens = braggi_vector_create(sizeof(Token*));
    
    if (!context->pattern_stack || !context->tokens) {
        // Cleanup on error
        if (context->pattern_stack) braggi_vector_destroy(context->pattern_stack);
        if (context->tokens) braggi_vector_destroy(context->tokens);
        free(context);
        return NULL;
    }
    
    // Push the root pattern to the stack
    if (!braggi_vector_push_back(context->pattern_stack, &pattern)) {
        braggi_vector_destroy(context->pattern_stack);
        braggi_vector_destroy(context->tokens);
        free(context);
        return NULL;
    }
    
    // Get periscope from constraint patterns system
    context->periscope = braggi_constraint_patterns_get_periscope();
    if (!context->periscope) {
        DEBUG("WARNING: No periscope available for functional context");
    } else {
        DEBUG("Connected functional context to periscope");
    }
    
    context->initialized = true;
    return context;
}

// Free a functional context
void braggi_free_functional_context(void* context) {
    if (!context) return;
    
    FunctionalContext* fc = (FunctionalContext*)context;
    
    // Just need to free the pattern_stack
    if (fc->pattern_stack) {
        braggi_vector_destroy(fc->pattern_stack);
    }
    
    if (fc->tokens) {
        braggi_vector_destroy(fc->tokens);
    }
    
    free(fc);
}

// Helper to get cell ID for token, leveraging periscope if available
static uint32_t get_cell_id_for_token(FunctionalContext* ctx, Token* token) {
    if (!ctx || !token) {
        return 0;
    }
    
    // Use periscope if available
    if (ctx->periscope && ctx->field) {
        uint32_t cell_id = braggi_periscope_get_cell_id_for_token(ctx->periscope, token, ctx->field);
        if (cell_id != UINT32_MAX) {
            return cell_id;
        }
        // Fallback to other methods if periscope doesn't have this token
    }
    
    // Fallback based on token source position
    // Use line as an approximation for cell ID if valid
    if (token->position.line > 0 && ctx->field && ctx->field->cell_count > 0) {
        uint32_t cell_id = token->position.line - 1; // Convert 1-based to 0-based
        if (cell_id < ctx->field->cell_count) {
            return cell_id;
        }
    }
    
    // Last resort: just return 0 (the minimum cell ID)
    WARNING("Using last-resort fallback cell ID for token");
    return 0;
}

// Validator function that adapts from EntropyState** to Token**
bool braggi_functional_validator(EntropyConstraint* constraint, EntropyField* field) {
    if (!constraint || !field || !constraint->context) {
        ERROR("Invalid parameters for functional validator");
        return false;
    }
    
    FunctionalContext* ctx = (FunctionalContext*)constraint->context;
    
    // Collect the states from all the cells affected by this constraint
    size_t cell_count = constraint->cell_count;
    if (cell_count == 0) {
        DEBUG("Constraint affects no cells, auto-pass");
        return true;  // No cells to validate
    }
    
    // For each cell, get the current states
    EntropyState** states = (EntropyState**)malloc(cell_count * sizeof(EntropyState*));
    if (!states) {
        ERROR("Failed to allocate memory for states array");
        return false;
    }
    
    for (size_t i = 0; i < cell_count; i++) {
        uint32_t cell_id = constraint->cell_ids[i];
        EntropyCell* cell = braggi_entropy_field_get_cell(field, cell_id);
        if (!cell || cell->state_count == 0) {
            states[i] = NULL;
            continue;
        }
        
        // For simplicity, just take the first state
        // In a collapsed cell, this will be the only state
        states[i] = cell->states[0];
    }
    
    // If this is a function validator, use it directly
    if (ctx->validator_func) {
        DEBUG("Using function validator path");
        
        // Extract tokens from states
        Token** tokens = (Token**)malloc(cell_count * sizeof(Token*));
        if (!tokens) {
            ERROR("Failed to allocate memory for tokens array");
            free(states);
            return false;
        }
        
        // Extract tokens from states
        for (size_t i = 0; i < cell_count; i++) {
            if (!states[i] || !states[i]->data) {
                tokens[i] = NULL;
                continue;
            }
            tokens[i] = (Token*)states[i]->data;
        }
        
        // Call the validator function
        bool result = ctx->validator_func(tokens, cell_count, ctx->user_context);
        
        // Clean up
        free(tokens);
        free(states);
        
        return result;
    }
    
    // Check if we need to refresh periscope connection
    if (!ctx->periscope) {
        ctx->periscope = braggi_constraint_patterns_get_periscope();
        if (ctx->periscope) {
            DEBUG("Reconnected functional validator to periscope");
        }
    }
    
    // Full pattern matching path
    DEBUG("Using pattern matching validator path with %zu states", cell_count);
    
    // First pass - validate patterns against states
    for (size_t i = 0; i < cell_count; i++) {
        EntropyState* state = states[i];
        if (!state) {
            WARNING("NULL state at index %zu in functional validator", i);
            continue;
        }
        
        // Get the current pattern from the stack
        if (ctx->pattern_stack->size == 0) {
            ctx->error = true;
            ctx->error_message = "Pattern stack is empty";
            ERROR("%s", ctx->error_message);
            free(states);
            return false;
        }
        
        Pattern** pattern_ptr = braggi_vector_get(ctx->pattern_stack, ctx->pattern_stack->size - 1);
        if (!pattern_ptr || !(*pattern_ptr)) {
            ctx->error = true;
            ctx->error_message = "Invalid pattern on stack";
            ERROR("%s", ctx->error_message);
            free(states);
            return false;
        }
        
        Pattern* pattern = *pattern_ptr;
        
        // Check if the current token matches the pattern
        if (token_matches_pattern(state, pattern)) {
            // Matches, update context
            ctx->matched = true;
            ctx->current_token_index++;
            
            // Pop the pattern from the stack
            Pattern* dummy = NULL;  // dummy to receive the popped value
            braggi_vector_pop_back(ctx->pattern_stack, &dummy);
            
            // If it was the last pattern, we're done
            if (ctx->pattern_stack->size == 0) {
                ctx->exhausted = true;
                free(states);
                return true;
            }
        } else {
            // No match, check if we can expand the pattern
            switch (pattern->type) {
                case PATTERN_REFERENCE: {
                    // Get the referenced pattern from the library
                    Pattern* ref = braggi_constraint_pattern_library_get_pattern(
                        ctx->library, pattern->reference_name);
                    if (!ref) {
                        ctx->error = true;
                        ctx->error_message = "Referenced pattern not found";
                        ERROR("%s - '%s'", ctx->error_message, 
                                pattern->reference_name ? pattern->reference_name : "(null)");
                        free(states);
                        return false;
                    }
                    
                    // Pop the reference pattern and push the actual pattern
                    Pattern* dummy = NULL;
                    braggi_vector_pop_back(ctx->pattern_stack, &dummy);
                    if (!braggi_vector_push_back(ctx->pattern_stack, &ref)) {
                        ctx->error = true;
                        ctx->error_message = "Failed to push pattern to stack";
                        ERROR("%s", ctx->error_message);
                        free(states);
                        return false;
                    }
                    break;
                }
                
                case PATTERN_SEQUENCE: {
                    // Pop the sequence pattern and push all sub-patterns in reverse order
                    Pattern* dummy = NULL;
                    braggi_vector_pop_back(ctx->pattern_stack, &dummy);
                    for (int j = pattern->sub_pattern_count - 1; j >= 0; j--) {
                        if (!braggi_vector_push_back(ctx->pattern_stack, &pattern->sub_patterns[j])) {
                            ctx->error = true;
                            ctx->error_message = "Failed to push sub-pattern to stack";
                            ERROR("%s", ctx->error_message);
                            free(states);
                            return false;
                        }
                    }
                    break;
                }
                
                case PATTERN_SUPERPOSITION: {
                    // For superposition, we should try each sub-pattern
                    // This is a simplified implementation that doesn't handle backtracking
                    bool any_match = false;
                    for (size_t j = 0; j < pattern->sub_pattern_count; j++) {
                        if (token_matches_pattern(state, pattern->sub_patterns[j])) {
                            any_match = true;
                            
                            // Pop the superposition pattern and push the matching sub-pattern
                            Pattern* dummy = NULL;
                            braggi_vector_pop_back(ctx->pattern_stack, &dummy);
                            if (!braggi_vector_push_back(ctx->pattern_stack, &pattern->sub_patterns[j])) {
                                ctx->error = true;
                                ctx->error_message = "Failed to push matching sub-pattern to stack";
                                ERROR("%s", ctx->error_message);
                                free(states);
                                return false;
                            }
                            break;
                        }
                    }
                    
                    if (!any_match) {
                        // No sub-pattern matched
                        ctx->error = true;
                        ctx->error_message = "No matching pattern in superposition";
                        ERROR("%s", ctx->error_message);
                        free(states);
                        return false;
                    }
                    break;
                }
                
                case PATTERN_REPETITION: {
                    // For repetition, we can match zero or more times
                    // This simplified implementation just checks if we can match once
                    if (pattern->sub_pattern_count > 0 && 
                        token_matches_pattern(state, pattern->sub_patterns[0])) {
                        // Don't pop the repetition pattern, so we can match again
                        // But also push the sub-pattern to match now
                        if (!braggi_vector_push_back(ctx->pattern_stack, &pattern->sub_patterns[0])) {
                            ctx->error = true;
                            ctx->error_message = "Failed to push repetition sub-pattern to stack";
                            ERROR("%s", ctx->error_message);
                            free(states);
                            return false;
                        }
                    } else {
                        // No match but that's okay for repetition (0 or more)
                        Pattern* dummy = NULL;
                        braggi_vector_pop_back(ctx->pattern_stack, &dummy);
                    }
                    break;
                }
                
                case PATTERN_OPTIONAL: {
                    // For optional, we can match zero or one time
                    if (pattern->sub_pattern_count > 0 && 
                        token_matches_pattern(state, pattern->sub_patterns[0])) {
                        // Pop the optional pattern and push the sub-pattern
                        Pattern* dummy = NULL;
                        braggi_vector_pop_back(ctx->pattern_stack, &dummy);
                        if (!braggi_vector_push_back(ctx->pattern_stack, &pattern->sub_patterns[0])) {
                            ctx->error = true;
                            ctx->error_message = "Failed to push optional sub-pattern to stack";
                            ERROR("%s", ctx->error_message);
                            free(states);
                            return false;
                        }
                    } else {
                        // No match but that's okay for optional (0 or 1)
                        Pattern* dummy = NULL;
                        braggi_vector_pop_back(ctx->pattern_stack, &dummy);
                    }
                    break;
                }
                
                default: {
                    // Pattern didn't match and we can't expand it
                    ctx->error = true;
                    ctx->error_message = "Pattern type not handled";
                    ERROR("%s (type %d)", ctx->error_message, pattern->type);
                    free(states);
                    return false;
                }
            }
        }
    }
    
    // If we're here and the pattern stack is empty, we've matched everything
    ctx->exhausted = (ctx->pattern_stack->size == 0);
    free(states);
    return ctx->exhausted;
}

// Create a functional constraint with a simple validator function
EntropyConstraint* braggi_functional_constraint_create(
    uint32_t type, 
    TokenValidator validator_func, 
    void* user_context,
    const char* description,
    Periscope* periscope,
    EntropyField* field) {
    
    // Create functional context
    FunctionalContext* ctx = (FunctionalContext*)malloc(sizeof(FunctionalContext));
    if (!ctx) {
        ERROR("Failed to allocate memory for functional context");
        return NULL;
    }
    
    // Initialize context
    memset(ctx, 0, sizeof(FunctionalContext));
    ctx->initialized = true;
    ctx->validator_func = validator_func;
    ctx->user_context = user_context;
    ctx->periscope = periscope;
    ctx->field = field;
    
    // Create pattern stack in case it's needed
    ctx->pattern_stack = braggi_vector_create(sizeof(Pattern*));
    if (!ctx->pattern_stack) {
        ERROR("Failed to create pattern stack");
        free(ctx);
        return NULL;
    }
    
    // Create actual constraint with our functional validator
    EntropyConstraint* constraint = braggi_constraint_create(
        type,
        braggi_functional_validator,  // Use our validator adapter
        ctx,
        description
    );
    
    if (!constraint) {
        ERROR("Failed to create constraint");
        braggi_vector_destroy(ctx->pattern_stack);
        free(ctx);
        return NULL;
    }
    
    DEBUG("Created functional constraint: %s", description);
    return constraint;
}

// Create a pattern-based functional constraint 
EntropyConstraint* braggi_pattern_constraint_create(
    uint32_t type,
    Pattern* pattern,
    ConstraintPatternLibrary* library,
    const char* description,
    Periscope* periscope,
    EntropyField* field) {
    
    // Create a functional context for this pattern
    FunctionalContext* ctx = braggi_create_functional_context(pattern, library);
    if (!ctx) {
        ERROR("Failed to create functional context for pattern constraint");
        return NULL;
    }
    
    // Set the periscope and field
    ctx->periscope = periscope;
    ctx->field = field;
    
    // Create the constraint
    EntropyConstraint* constraint = braggi_constraint_create(
        type,
        braggi_functional_validator,
        ctx,
        description
    );
    
    if (!constraint) {
        ERROR("Failed to create constraint for pattern");
        braggi_free_functional_context(ctx);
        return NULL;
    }
    
    DEBUG("Created pattern constraint: %s", description);
    return constraint;
} 