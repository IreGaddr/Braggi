/*
 * Braggi - Token Propagator
 * 
 * "When tokens meet constraints, their possibilities collapse like a
 * line of dominoes at the Dallas County Fair!" - Parsing wisdom
 */

#ifndef BRAGGI_TOKEN_PROPAGATOR_H
#define BRAGGI_TOKEN_PROPAGATOR_H

#include <stdbool.h>
#include "braggi/token.h"
#include "braggi/entropy.h"
#include "braggi/util/vector.h"
#include "braggi/ecs.h"

// Forward declarations
typedef struct BraggiContext BraggiContext;
typedef struct TokenPropagator TokenPropagator;
typedef struct Pattern Pattern;

// Create a new token propagator
TokenPropagator* braggi_token_propagator_create(void);

// Destroy a token propagator
void braggi_token_propagator_destroy(TokenPropagator* propagator);

// Add a token to the propagator
bool braggi_token_propagator_add_token(TokenPropagator* propagator, Token* token);

// Add a pattern to the propagator
bool braggi_token_propagator_add_pattern(TokenPropagator* propagator, Pattern* pattern);

// Initialize the entropy field from tokens
bool braggi_token_propagator_initialize_field(TokenPropagator* propagator);

// Create constraints from patterns
bool braggi_token_propagator_create_constraints(TokenPropagator* propagator);

// Apply constraints to the field
bool braggi_token_propagator_apply_constraints(TokenPropagator* propagator);

// Collapse the field using Wave Function Collapse
bool braggi_token_propagator_collapse_field(TokenPropagator* propagator);

// Run the complete propagation process
bool braggi_token_propagator_run(TokenPropagator* propagator);

// Run the complete propagation process using the enhanced WFC algorithm
bool braggi_token_propagator_run_with_wfc(TokenPropagator* propagator);

// Get the generated tokens after collapse
Vector* braggi_token_propagator_get_output_tokens(TokenPropagator* propagator);

// Get errors from the propagation process
Vector* braggi_token_propagator_get_errors(TokenPropagator* propagator);

// Get the entropy field from the propagator
EntropyField* braggi_token_propagator_get_field(TokenPropagator* propagator);

// Helper functions (these are defined in the .c file)
void braggi_token_to_entropy_states(Token* token, EntropyCell* cell, float bias);

// Token-specific adjacency constraint function (renamed to avoid conflict with entropy.h)
EntropyConstraint* braggi_token_create_adjacency_constraint(Token* token, EntropyCell** cells, size_t cell_count, uint32_t pattern_id);

// Convert a pattern to a constraint
EntropyConstraint* braggi_pattern_to_constraint(Pattern* pattern, EntropyCell** affected_cells, size_t cell_count, void* context);

// Get the number of tokens in the propagator
size_t braggi_token_propagator_get_token_count(TokenPropagator* propagator);

// Get a token by index
Token* braggi_token_propagator_get_token(TokenPropagator* propagator, size_t index);

// Initialize periscope for the token propagator
bool braggi_token_propagator_init_periscope(TokenPropagator* propagator, ECSWorld* ecs_world);

// Register tokens with periscope
bool braggi_token_propagator_register_tokens_with_periscope(TokenPropagator* propagator);

#endif /* BRAGGI_TOKEN_PROPAGATOR_H */ 