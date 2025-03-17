/*
 * Braggi - Constraint Patterns
 *
 * "Constraints are like a good Texas BBQ sauce - they bind everything together
 * and give the language some real Irish-Texan kick!" - Claude O'Malley
 */

#ifndef BRAGGI_CONSTRAINT_PATTERNS_H
#define BRAGGI_CONSTRAINT_PATTERNS_H

#include <stdbool.h>
#include "braggi/entropy.h"
#include "braggi/util/vector.h"

// Pattern types
typedef enum PatternType {
    PATTERN_TYPE_SYNTAX,     // Syntax patterns (e.g., sequence, adjacency)
    PATTERN_TYPE_SEMANTIC,   // Semantic patterns (e.g., naming, typing)
    PATTERN_TYPE_GRAMMAR,    // Grammar patterns (e.g., expression, statement)
    PATTERN_TYPE_USER        // User-defined patterns
} PatternType;

// Initialize the constraint patterns system
bool braggi_constraint_patterns_initialize(void);

// Register a pattern type
bool register_pattern_type(PatternType type, const char* name);

// Register a pattern function
bool register_pattern_function(const char* name, bool (*func)(EntropyField*, Vector*, void*), void* data);

// Get a pattern function by name
bool (*get_pattern_function(const char* name))(EntropyField*, Vector*, void*);

// Built-in pattern functions
bool adjacency_pattern(EntropyField* field, Vector* tokens, void* data);
bool sequence_pattern(EntropyField* field, Vector* tokens, void* data);
bool grammar_pattern(EntropyField* field, Vector* tokens, void* data);
bool variable_pattern(EntropyField* field, Vector* tokens, void* data);
bool function_pattern(EntropyField* field, Vector* tokens, void* data);
bool type_pattern(EntropyField* field, Vector* tokens, void* data);
bool control_flow_pattern(EntropyField* field, Vector* tokens, void* data);

// Validator functions
bool braggi_default_adjacency_validator(EntropyConstraint* constraint, EntropyField* field);
bool braggi_default_sequence_validator(EntropyConstraint* constraint, EntropyField* field);

// Helper function for cell ID normalization
uint32_t braggi_normalize_field_cell_id(EntropyField* field, uint32_t cell_id);

// Create a constraint from a pattern
EntropyConstraint* braggi_constraint_from_pattern(const char* pattern_name, EntropyField* field, Vector* cells);

#endif /* BRAGGI_CONSTRAINT_PATTERNS_H */ 