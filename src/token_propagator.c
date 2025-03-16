/*
 * Braggi - Token Propagator Implementation
 * 
 * "In the rodeo of compilation, token propagation is how we tell 
 * which types are true mustangs and which are just donkeys with fancy saddles!"
 * - Compiler Philosopher
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "braggi/braggi_context.h"
#include "braggi/token_propagator.h"
#include "braggi/entropy.h"  // This has all our entropy definitions, including the field stuff
#include "braggi/util/vector.h"
#include "braggi/error.h"
#include "braggi/constraint.h"

// Define enum for TokenPropagator status
typedef enum {
    TOKEN_PROPAGATOR_STATUS_READY,
    TOKEN_PROPAGATOR_STATUS_RUNNING,
    TOKEN_PROPAGATOR_STATUS_COMPLETED,
    TOKEN_PROPAGATOR_STATUS_ERROR
} TokenPropagatorStatus;

// Define the TokenPattern struct
typedef struct {
    const char* name;
    uint32_t id;
    // Add other fields as needed
} TokenPattern;

// Define TokenConstraint as an alias for EntropyConstraint
typedef EntropyConstraint TokenConstraint;

// Define the Pattern struct - forward declared in token_propagator.h
struct Pattern {
    const char* name;
    uint32_t id;
    // Add other fields as needed
};

// Define the TokenPropagator struct - forward declared in token_propagator.h
struct TokenPropagator {
    BraggiContext* context;
    EntropyField* field;
    Vector* tokens;
    Vector* patterns;
    Vector* constraints;
    uint32_t source_file_id;
    ErrorHandler* error_handler;
    TokenPropagatorStatus status;
    bool initialized;
    bool collapsed;
    void* entropy_field; // Temporary, until proper field integration
};

// Helper function prototypes (internal use only)
static bool propagate_token_constraints(TokenPropagator* propagator, uint32_t cell_id);
static void report_propagation_error(TokenPropagator* propagator, 
                                    uint32_t cell_id, 
                                    const char* message);

// Function prototypes for constraint management (use existing entropy functions)
// EntropyConstraint* braggi_constraint_create(EntropyConstraintType type, bool (*validate)(EntropyConstraint*, EntropyField*), void* context, const char* description);
// void braggi_constraint_destroy(EntropyConstraint* constraint);
// bool braggi_constraint_apply(EntropyConstraint* constraint, EntropyField* field);

// Prototype for state creation
EntropyState* braggi_state_create(uint32_t state_id, uint32_t type, const char* label, void* data, uint32_t probability);

// Create a new token propagator
TokenPropagator* braggi_token_propagator_create(BraggiContext* context) {
    TokenPropagator* propagator = malloc(sizeof(TokenPropagator));
    if (!propagator) return NULL;
    
    propagator->context = context;
    propagator->field = NULL;
    propagator->entropy_field = NULL;
    propagator->source_file_id = 0;
    propagator->error_handler = NULL;
    propagator->initialized = false;
    propagator->collapsed = false;
    
    // Fix vector creation calls by adding elem_size parameters
    propagator->tokens = braggi_vector_create(sizeof(Token*));
    propagator->patterns = braggi_vector_create(sizeof(TokenPattern*));
    propagator->constraints = braggi_vector_create(sizeof(TokenConstraint*));
    
    propagator->status = TOKEN_PROPAGATOR_STATUS_READY;
    
    return propagator;
}

// Destroy a token propagator
void braggi_token_propagator_destroy(TokenPropagator* propagator) {
    if (!propagator) {
        return;
    }
    
    // Free the entropy field
    if (propagator->field) {
        braggi_entropy_field_destroy(propagator->field);
    }
    
    // Free the vectors (but not their contents, which are owned elsewhere)
    if (propagator->tokens) {
        braggi_vector_destroy(propagator->tokens);
    }
    
    if (propagator->patterns) {
        braggi_vector_destroy(propagator->patterns);
    }
    
    // Free the constraints vector and its contents (owned by this propagator)
    if (propagator->constraints) {
        for (size_t i = 0; i < braggi_vector_size(propagator->constraints); i++) {
            EntropyConstraint* constraint = *(EntropyConstraint**)braggi_vector_get(propagator->constraints, i);
            if (constraint) {
                braggi_constraint_destroy(constraint);
            }
        }
        braggi_vector_destroy(propagator->constraints);
    }
    
    free(propagator);
}

// Add a token to the propagator
bool braggi_token_propagator_add_token(TokenPropagator* propagator, Token* token) {
    if (!propagator || !token || !propagator->tokens) {
        return false;
    }
    
    // We only store a reference to the token
    // Use braggi_vector_push_back instead of push
    if (!braggi_vector_push_back(propagator->tokens, &token)) {
        return false;
    }
    
    return true;
}

// Add a pattern to the propagator
bool braggi_token_propagator_add_pattern(TokenPropagator* propagator, Pattern* pattern) {
    if (!propagator || !pattern || !propagator->patterns) {
        return false;
    }
    
    // We only store a reference to the pattern
    // Use braggi_vector_push_back instead of push
    if (!braggi_vector_push_back(propagator->patterns, &pattern)) {
        return false;
    }
    
    return true;
}

// Initialize the entropy field from tokens
bool braggi_token_propagator_initialize_field(TokenPropagator* propagator) {
    if (!propagator) return false;
    
    // If already initialized, do nothing
    if (propagator->initialized) {
        return true;
    }
    
    // Create the entropy field
    propagator->field = braggi_entropy_field_create(propagator->source_file_id, propagator->error_handler);
    if (!propagator->field) {
        return false;
    }
    
    // Fix vector creation call by adding elem_size parameter
    Vector* cells = braggi_vector_create(sizeof(EntropyCell*));
    
    // For each token, create a cell in the entropy field
    for (size_t i = 0; i < braggi_vector_size(propagator->tokens); i++) {
        Token* current_token = *(Token**)braggi_vector_get(propagator->tokens, i);
        if (!current_token) {
            continue;
        }
        
        // Add a cell to the entropy field for this token - use position.offset instead of position
        EntropyCell* cell = braggi_entropy_field_add_cell(propagator->field, current_token->position.offset);
        if (!cell) {
            braggi_vector_destroy(cells);
            return false;
        }
        
        // Set the cell's position information from the token
        cell->position_line = current_token->position.line;
        cell->position_column = current_token->position.column;
        cell->position_offset = current_token->position.offset;
        
        // Add the cell to our tracking vector
        if (!braggi_vector_push_back(cells, &cell)) {
            braggi_vector_destroy(cells);
            return false;
        }
        
        // Convert the token to entropy states
        braggi_token_to_entropy_states(current_token, cell, 1.0f);
    }
    
    // Create adjacency constraints between tokens
    // First, convert cells vector to array for easier access
    EntropyCell** cell_array = (EntropyCell**)malloc(sizeof(EntropyCell*) * braggi_vector_size(cells));
    if (!cell_array) {
        braggi_vector_destroy(cells);
        return false;
    }
    
    for (size_t i = 0; i < braggi_vector_size(cells); i++) {
        cell_array[i] = *(EntropyCell**)braggi_vector_get(cells, i);
    }
    
    // Create constraints between tokens
    for (size_t i = 0; i < braggi_vector_size(propagator->tokens); i++) {
        Token* current_token = *(Token**)braggi_vector_get(propagator->tokens, i);
        if (!current_token) {
            continue;
        }
        
        // Find tokens that are adjacent to this one
        // For now, just create a simple constraint between consecutive tokens
        if (i < braggi_vector_size(propagator->tokens) - 1) {
            // Create a constraint between this token and the next one
            // Use the renamed function
            EntropyConstraint* adjacency_constraint = braggi_token_create_adjacency_constraint(
                current_token,
                cell_array,
                braggi_vector_size(cells),
                i  // Use the token index as a placeholder pattern ID
            );
            
            if (adjacency_constraint) {
                // Add the constraint to both the field and our tracking vector
                if (!braggi_entropy_field_add_constraint(propagator->field, adjacency_constraint)) {
                    braggi_constraint_destroy(adjacency_constraint);
                } else {
                    braggi_vector_push_back(propagator->constraints, &adjacency_constraint);
                }
            }
        }
    }
    
    // Clean up
    free(cell_array);
    braggi_vector_destroy(cells);
    
    // Mark as initialized
    propagator->initialized = true;
    
    return true;
}

// Create constraints from patterns
bool braggi_token_propagator_create_constraints(TokenPropagator* propagator) {
    if (!propagator || !propagator->field || !propagator->patterns) {
        return false;
    }
    
    // For each pattern, create a corresponding constraint
    for (size_t i = 0; i < braggi_vector_size(propagator->patterns); i++) {
        Pattern* pattern = *(Pattern**)braggi_vector_get(propagator->patterns, i);
        if (!pattern) {
            continue;
        }
        
        // Get affected cells from the field (placeholder implementation)
        // In a real implementation, you would determine which cells match the pattern
        EntropyCell* current_cell = braggi_entropy_field_get_cell(propagator->field, i);
        if (!current_cell) {
            continue;
        }
        
        EntropyCell* cells[1] = { current_cell };
        
        // Create a constraint from the pattern
        EntropyConstraint* pattern_constraint = braggi_pattern_to_constraint(
            pattern,
            cells,
            1,  // Just one cell for now
            NULL  // No context for now
        );
        
        if (pattern_constraint) {
            // Add the constraint to both the field and our tracking vector
            if (!braggi_entropy_field_add_constraint(propagator->field, pattern_constraint)) {
                braggi_constraint_destroy(pattern_constraint);
            } else {
                braggi_vector_push_back(propagator->constraints, &pattern_constraint);
            }
        }
    }
    
    return true;
}

// Apply constraints to the field
bool braggi_token_propagator_apply_constraints(TokenPropagator* propagator) {
    if (!propagator || !propagator->field || !propagator->constraints) {
        return false;
    }
    
    // Apply each constraint to the field
    for (size_t i = 0; i < braggi_vector_size(propagator->constraints); i++) {
        EntropyConstraint* constraint = *(EntropyConstraint**)braggi_vector_get(propagator->constraints, i);
        if (!constraint) {
            continue;
        }
        
        if (!braggi_constraint_apply(constraint, propagator->field)) {
            // Report the failure
            report_propagation_error(propagator, 0, "Failed to apply constraint");
            return false;
        }
    }
    
    return true;
}

// Collapse the field using Wave Function Collapse
bool braggi_token_propagator_collapse_field(TokenPropagator* propagator) {
    if (!propagator || !propagator->field) {
        return false;
    }
    
    // Loop until the field is fully collapsed or we hit a contradiction
    while (!braggi_entropy_field_is_fully_collapsed(propagator->field)) {
        // Find the cell with the lowest entropy
        EntropyCell* cell = braggi_entropy_field_find_lowest_entropy_cell(propagator->field);
        if (!cell) {
            // Check if there's a contradiction
            SourcePosition position;
            char* message = NULL;
            
            if (braggi_entropy_field_has_contradiction(propagator->field)) {
                // Get contradiction info if possible
                if (braggi_entropy_field_get_contradiction_info(propagator->field, &position, &message)) {
                    // Report the contradiction
                    report_propagation_error(propagator, 0, message ? message : "Contradiction detected");
                    free(message);
                } else {
                    report_propagation_error(propagator, 0, "Unknown contradiction");
                }
                return false;
            }
            
            // If no contradiction but no more cells, we're done
            propagator->collapsed = true;
            break;
        }
        
        // Get the entropy of the cell
        double entropy = braggi_entropy_cell_get_entropy(cell);
        
        // If the cell has zero entropy, we have a contradiction
        if (entropy <= 0.0) {
            report_propagation_error(propagator, cell->id, "Cell has zero entropy");
            return false;
        }
        
        // Collapse the cell to a random valid state
        if (!braggi_entropy_field_collapse_cell(propagator->field, cell->id, 0)) {
            report_propagation_error(propagator, cell->id, "Failed to collapse cell");
            return false;
        }
        
        // Propagate constraints from the collapsed cell
        if (!braggi_entropy_field_propagate_constraints(propagator->field, cell->id)) {
            report_propagation_error(propagator, cell->id, "Failed to propagate constraints");
            return false;
        }
    }
    
    // Mark as collapsed
    propagator->collapsed = true;
    
    return true;
}

// Run the complete propagation process
bool braggi_token_propagator_run(TokenPropagator* propagator) {
    if (!propagator) {
        return false;
    }
    
    // Initialize the field if not already done
    if (!propagator->initialized && !braggi_token_propagator_initialize_field(propagator)) {
        return false;
    }
    
    // Create constraints from patterns
    if (!braggi_token_propagator_create_constraints(propagator)) {
        return false;
    }
    
    // Apply constraints to the field
    if (!braggi_token_propagator_apply_constraints(propagator)) {
        return false;
    }
    
    // Collapse the field
    if (!braggi_token_propagator_collapse_field(propagator)) {
        return false;
    }
    
    return true;
}

// Get the generated tokens after collapse
Vector* braggi_token_propagator_get_output_tokens(TokenPropagator* propagator) {
    if (!propagator) return NULL;
    
    // TODO: Implement actual token extraction from the entropy field
    
    // Fix vector creation call by adding elem_size parameter
    return braggi_vector_create(sizeof(Token*));
}

// Get errors from the propagation process
Vector* braggi_token_propagator_get_errors(TokenPropagator* propagator) {
    if (!propagator) return NULL;
    
    // TODO: Implement actual error extraction
    
    // Fix vector creation call by adding elem_size parameter
    return braggi_vector_create(sizeof(Error*));
}

// Helper function to report propagation errors
static void report_propagation_error(TokenPropagator* propagator, 
                                     uint32_t cell_id, 
                                     const char* message) {
    if (!propagator || !message) {
        return;
    }
    
    // Get the cell's position
    EntropyCell* cell = NULL;
    SourcePosition position = { 0, 0, 0 };
    
    if (cell_id > 0) {
        cell = braggi_entropy_field_get_cell(propagator->field, cell_id);
        if (cell) {
            // Use the individual fields instead of the position member
            position.line = cell->position_line;
            position.column = cell->position_column;
            position.offset = cell->position_offset;
        }
    }
    
    // Report the error
    if (propagator->error_handler) {
        // Using the error handler if available
        // Use the correct error enums from error.h
        Error* error = braggi_error_create(0, ERROR_CATEGORY_SYNTAX, ERROR_SEVERITY_ERROR,
                                          position, NULL, message, NULL);
        if (error) {
            // TODO: Add to error handler
            braggi_error_free(error);  // Use braggi_error_free instead of destroy
        }
    } else {
        // Just print to stderr if no error handler
        fprintf(stderr, "Propagation error at %u:%u:%u: %s\n",
                position.line, position.column, position.offset, message);
    }
}

// Implementation of helper functions declared in the header

// Convert a token to entropy states in a cell
void braggi_token_to_entropy_states(Token* token, EntropyCell* cell, float bias) {
    if (!token || !cell) {
        return;
    }
    
    // Create a state for the token type
    // Use the correct signature for braggi_state_create
    uint32_t state_id = token->type; // Use type as ID for simplicity
    const char* label = token->text; // Use text instead of lexeme
    void* data = token;
    uint32_t probability = (uint32_t)(100.0f * bias); // Scale 0.0-1.0 to 0-100
    
    EntropyState* state = braggi_state_create(state_id, token->type, label, data, probability);
    
    if (state) {
        // TODO: Add state to cell
        // This would normally use a function like braggi_entropy_cell_add_state
    }
}

// Find adjacent tokens and create constraints - renamed to avoid conflict
EntropyConstraint* braggi_token_create_adjacency_constraint(Token* token,
                                              EntropyCell** cells,
                                              size_t cell_count,
                                              uint32_t pattern_id) {
    if (!token || !cells || cell_count == 0) {
        return NULL;
    }
    
    // Create adjacency constraint using the entropy.h constraint creation function
    return braggi_constraint_create(CONSTRAINT_SYNTAX, 
                                   NULL,  // No validator function yet
                                   NULL,  // No context
                                   "Adjacency constraint");
}

// Convert a pattern to a constraint
EntropyConstraint* braggi_pattern_to_constraint(Pattern* pattern, EntropyCell** affected_cells, size_t cell_count,
                                        void* context) {
    if (!pattern || !affected_cells || cell_count == 0) {
        return NULL;
    }
    
    // Create pattern constraint
    return braggi_constraint_create(CONSTRAINT_SEMANTIC, 
                                   NULL,  // No validator function yet
                                   context,
                                   pattern->name ? pattern->name : "Pattern constraint");
} 