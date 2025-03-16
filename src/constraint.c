/*
 * Braggi - Constraint System Implementation
 * 
 * "Constraints are like the fences on me grandpappy's ranch - 
 * they keep the good stuff in and the bad stuff out!" - Texan wisdom
 */

#include "braggi/constraint.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Initial capacity for affected cells list
#define CONSTRAINT_INITIAL_CELL_CAPACITY 8

// Create a new constraint
Constraint* braggi_constraint_create(ConstraintType type, 
                                 ConstraintValidator validator,
                                 void* context,
                                 const char* description) {
    if (!validator) return NULL;
    
    Constraint* constraint = (Constraint*)malloc(sizeof(Constraint));
    if (!constraint) return NULL;
    
    // Initialize constraint structure
    constraint->type = type;
    constraint->validate = validator;
    constraint->context = context;
    constraint->description = description ? strdup(description) : NULL;
    
    // Initialize cell array
    constraint->cell_ids = (uint32_t*)malloc(sizeof(uint32_t) * CONSTRAINT_INITIAL_CELL_CAPACITY);
    if (!constraint->cell_ids) {
        if (constraint->description) free(constraint->description);
        free(constraint);
        return NULL;
    }
    
    constraint->num_cells = 0;
    constraint->cells_capacity = CONSTRAINT_INITIAL_CELL_CAPACITY;
    
    // Generate a unique ID (in a real implementation, we'd have a more sophisticated ID generator)
    static uint32_t next_id = 1;  // Start at 1, 0 is reserved
    constraint->id = next_id++;
    
    return constraint;
}

// Destroy a constraint and free resources
void braggi_constraint_destroy(Constraint* constraint) {
    if (!constraint) return;
    
    if (constraint->cell_ids) {
        free(constraint->cell_ids);
    }
    
    if (constraint->description) {
        free(constraint->description);
    }
    
    // Free the context if it's a dynamically allocated object
    // Note: This depends on how contexts are managed in your system
    // For now, we assume the caller handles context cleanup
    
    free(constraint);
}

// Add a cell to the constraint's affected cells
bool braggi_constraint_add_cell(Constraint* constraint, uint32_t cell_id) {
    if (!constraint) return false;
    
    // Check if cell is already in the list
    for (size_t i = 0; i < constraint->num_cells; i++) {
        if (constraint->cell_ids[i] == cell_id) {
            return true;  // Already in the list, nothing to do
        }
    }
    
    // Ensure there's capacity
    if (constraint->num_cells >= constraint->cells_capacity) {
        size_t new_capacity = constraint->cells_capacity * 2;
        uint32_t* new_cells = (uint32_t*)realloc(constraint->cell_ids, sizeof(uint32_t) * new_capacity);
        if (!new_cells) return false;
        
        constraint->cell_ids = new_cells;
        constraint->cells_capacity = new_capacity;
    }
    
    // Add cell to constraint
    constraint->cell_ids[constraint->num_cells++] = cell_id;
    
    return true;
}

// Apply a constraint to the entropy field
bool braggi_constraint_apply(Constraint* constraint, EntropyField* field) {
    if (!constraint || !field) return false;
    
    // Gather all affected cells
    Cell** affected_cells = (Cell**)malloc(sizeof(Cell*) * constraint->num_cells);
    if (!affected_cells) return false;
    
    for (size_t i = 0; i < constraint->num_cells; i++) {
        uint32_t cell_id = constraint->cell_ids[i];
        
        // Ensure cell ID is valid
        if (cell_id >= field->num_cells) {
            free(affected_cells);
            return false;
        }
        
        affected_cells[i] = field->cells[cell_id];
    }
    
    // Check if any cell has been fully collapsed
    bool has_collapsed = false;
    for (size_t i = 0; i < constraint->num_cells; i++) {
        if (affected_cells[i]->num_states == 1) {
            has_collapsed = true;
            break;
        }
    }
    
    // If any cell has collapsed, we need to check if the current states are valid
    if (has_collapsed) {
        // Collect all possible state combinations and validate them
        // For cells that have collapsed, there's only one state
        // For other cells, we need to try all combinations
        
        // For now, we'll use a simplified approach that works best when most cells
        // have already collapsed to a single state:
        
        // First, collect all states from collapsed cells
        State** collapsed_states = (State**)malloc(sizeof(State*) * constraint->num_cells);
        if (!collapsed_states) {
            free(affected_cells);
            return false;
        }
        
        size_t num_collapsed = 0;
        for (size_t i = 0; i < constraint->num_cells; i++) {
            if (affected_cells[i]->num_states == 1) {
                collapsed_states[num_collapsed++] = affected_cells[i]->possible_states[0];
            }
        }
        
        // For each non-collapsed cell, check which states are compatible with collapsed states
        for (size_t i = 0; i < constraint->num_cells; i++) {
            Cell* cell = affected_cells[i];
            
            // Skip cells that have already collapsed
            if (cell->num_states <= 1) continue;
            
            // Try each possible state
            size_t valid_count = 0;
            for (size_t j = 0; j < cell->num_states; j++) {
                // Create a temporary state array with this state and all collapsed states
                State** test_states = (State**)malloc(sizeof(State*) * (num_collapsed + 1));
                if (!test_states) {
                    free(collapsed_states);
                    free(affected_cells);
                    return false;
                }
                
                // Copy collapsed states
                for (size_t k = 0; k < num_collapsed; k++) {
                    test_states[k] = collapsed_states[k];
                }
                
                // Add this state
                test_states[num_collapsed] = cell->possible_states[j];
                
                // Check if this combination is valid
                if (braggi_constraint_check(constraint, test_states, num_collapsed + 1)) {
                    // This state is valid - keep it
                    if (valid_count != j) {
                        cell->possible_states[valid_count] = cell->possible_states[j];
                    }
                    valid_count++;
                }
                
                free(test_states);
            }
            
            // Update the number of valid states
            cell->num_states = valid_count;
        }
        
        free(collapsed_states);
    }
    
    free(affected_cells);
    return true;
}

// Check if a specific state combination satisfies the constraint
bool braggi_constraint_check(Constraint* constraint, State** states, size_t state_count) {
    if (!constraint || !states || state_count == 0) return false;
    
    // Use the constraint's validator function
    return constraint->validate(states, state_count, constraint->context);
}

// Validator functions for built-in constraints
static bool syntax_rule_validator(State** states, size_t state_count, void* context) {
    // The context contains the rule name
    const char* rule = (const char*)context;
    
    // Real implementation would check if these states form a valid syntax structure
    // according to the named rule
    
    // For now, just a placeholder that accepts everything
    return true;
}

static bool region_lifetime_validator(State** states, size_t state_count, void* context) {
    // Real implementation would check if the region lifetimes are compatible
    
    // For now, just a placeholder that accepts everything
    return true;
}

static bool regime_compatibility_validator(State** states, size_t state_count, void* context) {
    // Real implementation would check if the regimes are compatible
    // using the regime_compatibility_matrix
    
    // For now, just a placeholder that accepts everything
    return true;
}

static bool type_check_validator(State** states, size_t state_count, void* context) {
    // Real implementation would perform type checking
    
    // For now, just a placeholder that accepts everything
    return true;
}

// Example built-in constraint: syntax rule
Constraint* braggi_constraint_syntax_rule(const char* rule_name) {
    // Create the constraint
    char description[128];
    snprintf(description, sizeof(description), "Syntax rule: %s", rule_name);
    
    // Duplicate the rule name for the context
    char* rule_context = strdup(rule_name);
    
    return braggi_constraint_create(CONSTRAINT_SYNTAX, syntax_rule_validator, rule_context, description);
}

// Example built-in constraint: region lifetime
Constraint* braggi_constraint_region_lifetime(void) {
    return braggi_constraint_create(CONSTRAINT_REGION, region_lifetime_validator, NULL, 
                                 "Region lifetime compatibility rule");
}

// Example built-in constraint: regime compatibility
Constraint* braggi_constraint_regime_compatibility(void) {
    return braggi_constraint_create(CONSTRAINT_REGIME, regime_compatibility_validator, NULL, 
                                 "Regime compatibility rule");
}

// Example built-in constraint: type checking
Constraint* braggi_constraint_type_check(void) {
    return braggi_constraint_create(CONSTRAINT_TYPE, type_check_validator, NULL, 
                                 "Type compatibility rule");
} 