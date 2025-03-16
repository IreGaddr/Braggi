/*
 * Braggi - Entropy Implementation Header
 * 
 * "Entropy isn't just a physics concept, it's the mischievous leprechaun 
 * that turns perfectly good code into a Texas-sized mess!"
 * - Braggi Wisdom
 */

#ifndef BRAGGI_ENTROPY_H
#define BRAGGI_ENTROPY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "braggi/source.h"
#include "braggi/error.h"
#include "braggi/util/vector.h"

// Forward declarations
typedef struct EntropyState EntropyState;
typedef struct EntropyCell EntropyCell;
typedef struct EntropyField EntropyField;
typedef struct EntropyConstraint EntropyConstraint;
typedef struct EntropyRule EntropyRule;
typedef struct Cell Cell;  // Alias for EntropyCell for transitional code

/* Optional common type alias for compatibility */
typedef struct EntropyState EntropyState;
typedef EntropyState State;  // Legacy alias for code that uses State

// Entropy state structure - shared between modules
struct EntropyState {
    uint32_t id;
    uint32_t type;
    const char* label;
    void* data;
    uint32_t probability;  // 0-100 scale
};

// Constraint types
typedef enum EntropyConstraintType {
    CONSTRAINT_NONE = 0,
    CONSTRAINT_SYNTAX = 1,        // Syntax constraint
    CONSTRAINT_SEMANTIC = 2,      // Semantic constraint
    CONSTRAINT_TYPE = 3,          // Type constraint
    CONSTRAINT_REGION = 4,        // Region constraint
    CONSTRAINT_REGIME = 5,        // Regime constraint
    CONSTRAINT_PERISCOPE = 6,     // Periscope constraint
    CONSTRAINT_CUSTOM = 7         // Custom constraint
} EntropyConstraintType;

// Constraint structure - shared between modules
struct EntropyConstraint {
    uint32_t id;
    EntropyConstraintType type;
    const char* description;
    bool (*validate)(EntropyConstraint* constraint, EntropyField* field);
    void* context;
    uint32_t* cell_ids;       // Array of cell IDs affected by this constraint
    size_t cell_count;        // Number of cells affected
};

// Cell structure
struct EntropyCell {
    uint32_t id;              // Unique identifier
    EntropyState** states;    // Possible states
    size_t state_count;       // Number of possible states
    size_t state_capacity;    // Allocated capacity for states
    EntropyConstraint** constraints; // Constraints affecting this cell
    size_t constraint_count;  // Number of constraints
    size_t constraint_capacity; // Allocated capacity for constraints
    void* data;               // Additional data
    uint32_t position_offset; // Position in source (offset)
    uint32_t position_line;   // Position in source (line)
    uint32_t position_column; // Position in source (column)
};

// Entropy field structure
struct EntropyField {
    uint32_t id;              // Unique identifier
    EntropyCell** cells;      // Array of cells
    size_t cell_count;        // Number of cells
    size_t cell_capacity;     // Capacity of cells array
    EntropyConstraint** constraints; // Array of constraints
    size_t constraint_count;  // Number of constraints
    size_t constraint_capacity; // Capacity of constraints array
    bool has_contradiction;   // True if a contradiction has been detected
    uint32_t contradiction_cell_id; // ID of cell with contradiction
    uint32_t source_id;       // ID of source file
    void* error_handler;      // Error handler
};

// Rule represents a pattern-based rule for entropy propagation
struct EntropyRule {
    uint32_t id;                  // Unique identifier for this rule
    Vector* constraints;          // Vector of EntropyConstraint* created by this rule
    bool (*apply)(EntropyRule* rule, EntropyField* field); // Application function
    void* context;                // Custom context for applying the rule
    char* description;            // Human-readable description
};

// Function declarations

// State functions
EntropyState* braggi_state_create(uint32_t id, uint32_t type, const char* label, void* data, uint32_t probability);
void braggi_state_destroy(EntropyState* state);
void braggi_state_set_probability(EntropyState* state, uint32_t probability);
bool braggi_state_is_eliminated(EntropyState* state);
void braggi_state_eliminate(EntropyState* state);
void braggi_state_restore(EntropyState* state);

// Cell functions
EntropyCell* braggi_entropy_cell_create(uint32_t id);
void braggi_entropy_cell_destroy(EntropyCell* cell);
bool braggi_entropy_cell_add_state(EntropyCell* cell, EntropyState* state);
bool braggi_entropy_cell_remove_state(EntropyCell* cell, uint32_t state_id);
EntropyState* braggi_entropy_cell_get_state(EntropyCell* cell, uint32_t state_id);
double braggi_entropy_cell_get_entropy(EntropyCell* cell);
bool braggi_entropy_cell_is_collapsed(EntropyCell* cell);
bool braggi_entropy_cell_collapse(EntropyCell* cell, uint32_t state_index);
bool braggi_entropy_cell_collapse_random(EntropyCell* cell);
bool braggi_entropy_cell_has_contradiction(EntropyCell* cell);
bool braggi_entropy_cell_add_constraint(EntropyCell* cell, EntropyConstraint* constraint);

// Field functions
EntropyField* braggi_entropy_field_create(uint32_t source_id, void* error_handler);
void braggi_entropy_field_destroy(EntropyField* field);
EntropyCell* braggi_entropy_field_add_cell(EntropyField* field, uint32_t position);
EntropyCell* braggi_entropy_field_get_cell(EntropyField* field, uint32_t cell_id);
EntropyCell* braggi_entropy_field_get_cell_at(EntropyField* field, uint32_t x, uint32_t y);
bool braggi_entropy_field_add_constraint(EntropyField* field, EntropyConstraint* constraint);
bool braggi_entropy_field_add_rule(EntropyField* field, EntropyRule* rule);
bool braggi_entropy_field_is_fully_collapsed(EntropyField* field);
bool braggi_entropy_field_has_contradiction(EntropyField* field);
bool braggi_entropy_field_get_contradiction_info(EntropyField* field, void* position, char** message);
EntropyCell* braggi_entropy_field_find_lowest_entropy_cell(EntropyField* field);
bool braggi_entropy_field_collapse_cell(EntropyField* field, uint32_t cell_id, uint32_t state_index);
bool braggi_entropy_field_propagate_constraints(EntropyField* field, uint32_t cell_id);
bool braggi_entropy_field_apply_rules(EntropyField* field);
bool braggi_entropy_field_get_neighbors(EntropyField* field, uint32_t cell_id, EntropyCell** neighbors, uint32_t* neighbor_count);

// Constraint functions
EntropyConstraint* braggi_constraint_create(uint32_t type, bool (*validate)(EntropyConstraint*, EntropyField*), void* context, const char* description);
void braggi_constraint_destroy(EntropyConstraint* constraint);
bool braggi_constraint_add_cell(EntropyConstraint* constraint, uint32_t cell_id);
bool braggi_constraint_remove_cell(EntropyConstraint* constraint, uint32_t cell_id);
bool braggi_constraint_apply(EntropyConstraint* constraint, EntropyField* field);
bool braggi_constraint_affects_cell(EntropyConstraint* constraint, uint32_t cell_id);

// Rule functions
EntropyRule* braggi_rule_create(bool (*apply)(EntropyRule*, EntropyField*), void* context, const char* description);
void braggi_rule_destroy(EntropyRule* rule);
bool braggi_rule_add_constraint(EntropyRule* rule, EntropyConstraint* constraint);
bool braggi_rule_apply(EntropyRule* rule, EntropyField* field);

// Utility functions
EntropyConstraint* braggi_create_adjacency_constraint(void* token, EntropyCell** cells, size_t cell_count, uint32_t pattern_id);

// For backward compatibility - Cell is an alias for EntropyCell
#define Cell EntropyCell

// Additional helper functions for entropy system
double braggi_entropy_calculate(size_t possibilities);
uint32_t braggi_entropy_random_state_index(EntropyCell* cell);
const char* braggi_entropy_constraint_type_to_string(EntropyConstraintType type);

#endif /* BRAGGI_ENTROPY_H */ 