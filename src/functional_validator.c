/*
 * Braggi - Functional Validator for Token Constraint Propagation
 * 
 * "When you're checkin' if somethin' makes sense, remember - 
 * a square peg won't fit in a round hole no matter how much you sweet talk it!"
 * - Irish-Texan Programming Wisdom
 */

#include "braggi/token_propagator.h"
#include "braggi/error.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "braggi/token.h"
#include "braggi/pattern.h"      // For ConstraintPatternLibrary and PatternType
#include "braggi/entropy.h"      // For EntropyState type
#include "braggi/util/vector.h"

// State is EntropyState - for backward compatibility
typedef EntropyState State;

// Functional context for pattern validation
typedef struct FunctionalContext {
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

// Functional validator for pattern matching
bool braggi_functional_validator(EntropyState** states, size_t state_count, void* context) {
    FunctionalContext* ctx = (FunctionalContext*)context;
    if (!ctx || !ctx->initialized || !states || state_count == 0) return false;
    
    // First pass - validate patterns against states
    for (size_t i = 0; i < state_count; i++) {
        EntropyState* state = states[i];
        if (!state) continue;
        
        // Get the current pattern from the stack
        if (ctx->pattern_stack->size == 0) {
            ctx->error = true;
            ctx->error_message = "Pattern stack is empty";
            return false;
        }
        
        Pattern** pattern_ptr = braggi_vector_get(ctx->pattern_stack, ctx->pattern_stack->size - 1);
        if (!pattern_ptr || !(*pattern_ptr)) {
            ctx->error = true;
            ctx->error_message = "Invalid pattern on stack";
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
                        return false;
                    }
                    
                    // Pop the reference pattern and push the actual pattern
                    Pattern* dummy = NULL;
                    braggi_vector_pop_back(ctx->pattern_stack, &dummy);
                    if (!braggi_vector_push_back(ctx->pattern_stack, &ref)) {
                        ctx->error = true;
                        ctx->error_message = "Failed to push pattern to stack";
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
                                return false;
                            }
                            break;
                        }
                    }
                    
                    if (!any_match) {
                        // No sub-pattern matched
                        ctx->error = true;
                        ctx->error_message = "No matching pattern in superposition";
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
                    return false;
                }
            }
        }
    }
    
    // If we're here and the pattern stack is empty, we've matched everything
    ctx->exhausted = (ctx->pattern_stack->size == 0);
    return ctx->exhausted;
} 