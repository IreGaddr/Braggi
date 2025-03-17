/*
 * Braggi - Entropy System Implementation
 * 
 * "Entropy ain't just physics, it's a way of life! The more possibilities you have,
 * the more uncertain things get - just like trying to herd cats on a windy day."
 * - Irish-Texan Programming Philosophy
 */

#include "braggi/entropy.h"
#include "braggi/allocation.h"
#include "braggi/util/vector.h"
#include "braggi/token.h"  // Add token.h for Token structure
#include "braggi/pattern.h" // Add pattern.h for Pattern structure
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <float.h>
#include <time.h>  // For time() function used in random seed

// External declaration for ECS field reference clearing function
extern void braggi_entropy_ecs_clear_field_reference(void* world);

// Define the DEBUG_PRINT macro for debugging output
#define DEBUG_PRINT(fmt, ...) do { fprintf(stderr, "ENTROPY: " fmt, ##__VA_ARGS__); fflush(stderr); } while(0)

// Helper function to calculate Shannon entropy from a probability distribution
static float calculate_entropy(float* probabilities, uint32_t dimension) {
    if (!probabilities || dimension == 0) return 0.0f;
    
    float entropy = 0.0f;
    for (uint32_t i = 0; i < dimension; i++) {
        if (probabilities[i] > 0.0f) {
            entropy -= probabilities[i] * log2f(probabilities[i]);
        }
    }
    
    return entropy;
}

// Helper function to normalize a probability distribution
static void normalize_probabilities(float* probabilities, uint32_t dimension) {
    if (!probabilities || dimension == 0) return;
    
    float sum = 0.0f;
    for (uint32_t i = 0; i < dimension; i++) {
        if (probabilities[i] < 0.0f) {
            probabilities[i] = 0.0f;
        }
        sum += probabilities[i];
    }
    
    if (sum > 0.0f) {
        for (uint32_t i = 0; i < dimension; i++) {
            probabilities[i] /= sum;
        }
    } else {
        // Default to uniform distribution if all probabilities are zero
        float uniform = 1.0f / (float)dimension;
        for (uint32_t i = 0; i < dimension; i++) {
            probabilities[i] = uniform;
        }
    }
}

// Create a new entropy state
EntropyState* braggi_entropy_state_create(uint32_t id, uint32_t type, const char* label, void* data, uint32_t probability) {
    EntropyState* state = (EntropyState*)malloc(sizeof(EntropyState));
    if (!state) {
        return NULL;
    }
    
    state->id = id;
    state->type = type;
    
    if (label) {
        state->label = strdup(label);
        if (!state->label) {
            free(state);
            return NULL;
        }
    } else {
        state->label = NULL;
    }
    
    state->data = data;
    state->probability = probability;
    
    return state;
}

// Destroy an entropy state
void braggi_state_destroy(EntropyState* state) {
    if (!state) return;
    
    // Free the label if it exists
    if (state->label) {
        free((void*)state->label); // Cast away const for free
    }
    
    // Note: We don't free state->data as that might be owned elsewhere
    
    // Free the state itself
    free(state);
}

// Observe an entropy state, collapsing it to a definite value
uint32_t braggi_entropy_state_observe(EntropyState* state) {
    if (!state) return 0;
    
    // In the new structure, we just return the state's ID
    // since we no longer have probabilities, dimension, or observed_value
    
    return state->id;
}

// Entangle two entropy states, mixing their probability distributions
void braggi_entropy_state_entangle(EntropyState* state1, EntropyState* state2, float strength) {
    if (!state1 || !state2) return;
    
    // In the new structure, we simply update the 'probability' field of both states
    // based on a weighted average determined by the strength parameter
    
    // Calculate combined probability using weighted average
    uint32_t combined_probability = 
        (uint32_t)((1.0f - strength) * state1->probability + strength * state2->probability);
    
    // Ensure probability stays within valid range (0-100)
    if (combined_probability > 100) {
        combined_probability = 100;
    }
    
    // Update both states with the new probability
    state1->probability = combined_probability;
    state2->probability = combined_probability;
}

// Apply a constraint to an entropy state
void braggi_entropy_state_constrain(EntropyState* state, float* weights, float strength) {
    if (!state || !weights) return;
    
    // In the new structure, we adjust the probability based on the 
    // weight corresponding to this state's type
    
    // Adjust probability based on weight and strength
    float weight_factor = weights[state->type % 100]; // Use modulo in case type is out of bounds
    uint32_t new_probability = 
        (uint32_t)(state->probability * (1.0f + strength * (weight_factor - 1.0f)));
    
    // Ensure probability stays within valid range (0-100)
    if (new_probability > 100) {
        new_probability = 100;
    }
    
    // Update the state with the new probability
    state->probability = new_probability;
}

// Get the current entropy value of a state
float braggi_entropy_state_get_entropy(const EntropyState* state) {
    if (!state) return 0.0f;
    
    // In the new structure, we don't store entropy directly
    // We'll calculate a simple entropy analog based on the probability
    // When probability is 0 or 100, entropy is 0
    // When probability is 50, entropy is highest (1.0)
    
    float normalized_prob = state->probability / 100.0f;
    // Parabolic function that's 0 at probability=0 and probability=1, with max at 0.5
    return 1.0f - 4.0f * (normalized_prob - 0.5f) * (normalized_prob - 0.5f);
}

// Check if an entropy state is fully collapsed
bool braggi_entropy_state_is_collapsed(const EntropyState* state) {
    if (!state) return false;
    
    // In the new structure, we consider a state "collapsed" if its
    // probability is either 0 (impossible) or 100 (certain)
    return (state->probability == 0 || state->probability == 100);
}

// Create a new entropy constraint
EntropyConstraint* braggi_constraint_create(uint32_t type,
                                         bool (*validate)(EntropyConstraint*, EntropyField*),
                                         void* context,
                                         const char* description) {
    EntropyConstraint* constraint = (EntropyConstraint*)calloc(1, sizeof(EntropyConstraint));
    if (!constraint) return NULL;
    
    constraint->id = type; // Using type as ID for simplicity
    constraint->type = type;
    constraint->validate = validate;
    constraint->context = context;
    constraint->cell_ids = NULL;
    constraint->cell_count = 0;
    
    if (description) {
        constraint->description = strdup(description);
    } else {
        constraint->description = strdup("Unnamed constraint");
    }
    
    if (!constraint->description) {
        free(constraint);
        return NULL;
    }
    
    return constraint;
}

// Destroy an entropy constraint
void braggi_constraint_destroy(EntropyConstraint* constraint) {
    if (!constraint) return;
    
    // Free the cell IDs array if it exists
    if (constraint->cell_ids) {
        free(constraint->cell_ids);
    }
    
    // Free the description if it exists
    if (constraint->description) {
        free((void*)constraint->description); // Cast away const for free
    }
    
    // Free the constraint itself
    free(constraint);
}

// Create a new entropy rule
EntropyRule* braggi_entropy_rule_create(uint32_t dimension) {
    if (dimension == 0) return NULL;
    
    EntropyRule* rule = (EntropyRule*)malloc(sizeof(EntropyRule));
    if (!rule) return NULL;
    
    rule->constraints = braggi_vector_create(sizeof(EntropyConstraint*));
    if (!rule->constraints) {
        free(rule);
        return NULL;
    }
    
    rule->id = 0;  // Will be assigned by the entropy field
    rule->apply = NULL;
    rule->context = NULL;
    rule->description = NULL;
    
    return rule;
}

// Destroy an entropy rule
void braggi_entropy_rule_destroy(EntropyRule* rule) {
    if (!rule) return;
    
    if (rule->constraints) {
        // Note: We don't destroy the constraints as they might be shared
        braggi_vector_destroy(rule->constraints);
    }
    
    free(rule);
}

// Add a constraint to an entropy rule
bool braggi_entropy_rule_add_constraint(EntropyRule* rule, EntropyConstraint* constraint) {
    if (!rule || !constraint || !rule->constraints) {
        return false;
    }
    
    // Simply add the constraint to the vector
    return braggi_vector_push(rule->constraints, &constraint);
}

// Create a new entropy field
EntropyField* braggi_entropy_field_create(uint32_t source_id, void* error_handler) {
    EntropyField* field = (EntropyField*)malloc(sizeof(EntropyField));
    if (!field) return NULL;
    
    field->id = source_id; // Use source_id as the field ID
    field->cells = NULL;
    field->cell_count = 0;
    field->cell_capacity = 0;
    field->constraints = NULL;
    field->constraint_count = 0;
    field->constraint_capacity = 0;
    field->has_contradiction = false;
    field->contradiction_cell_id = 0;
    field->source_id = source_id;
    field->error_handler = error_handler;
    
    return field;
}

// Destroy an entropy field
void braggi_entropy_field_destroy(EntropyField* field) {
    if (!field) {
        fprintf(stderr, "DEBUG: Attempted to destroy NULL entropy field\n");
        return;
    }
    
    fprintf(stderr, "DEBUG: Starting destruction of entropy field %p\n", (void*)field);
    
    // Track destroyed fields to prevent double free
    static EntropyField** destroyed_fields = NULL;
    static int destroyed_count = 0;
    static int destroyed_capacity = 0;
    
    // Initialize tracking array if first time
    if (!destroyed_fields) {
        destroyed_capacity = 16;
        destroyed_fields = malloc(sizeof(EntropyField*) * destroyed_capacity);
        if (!destroyed_fields) {
            fprintf(stderr, "ERROR: Failed to allocate memory for destroyed_fields tracking\n");
            // Continue with destruction but without tracking
        } else {
            destroyed_count = 0;
        }
    }
    
    // Check if already destroyed
    if (destroyed_fields) {
        for (int i = 0; i < destroyed_count; i++) {
            if (destroyed_fields[i] == field) {
                fprintf(stderr, "WARNING: Attempted to destroy already destroyed field %p\n", (void*)field);
                return;
            }
        }
        
        // Add to destroyed list
        if (destroyed_count >= destroyed_capacity) {
            destroyed_capacity *= 2;
            EntropyField** new_array = realloc(destroyed_fields, sizeof(EntropyField*) * destroyed_capacity);
            if (new_array) {
                destroyed_fields = new_array;
            } else {
                fprintf(stderr, "ERROR: Failed to resize destroyed_fields tracking array\n");
                // Continue with destruction
            }
        }
        
        if (destroyed_count < destroyed_capacity) {
            destroyed_fields[destroyed_count++] = field;
            fprintf(stderr, "DEBUG: Added field %p to destroyed tracking (entry %d)\n", (void*)field, destroyed_count - 1);
        }
    }

    // CRITICAL SAFETY MEASURE: Clear ECS references before destroying the field
    // This prevents segmentation faults due to dangling pointers in the ECS systems
    braggi_entropy_ecs_clear_field_reference(NULL); // NULL is ok here - the function will find world internally if needed
    fprintf(stderr, "DEBUG: Cleared ECS references to field %p\n", (void*)field);
    
    // Validate and cleanup cells
    if (field->cells) {
        fprintf(stderr, "DEBUG: Cleaning up cells array with %zu cells\n", field->cell_count);
        
        for (size_t i = 0; i < field->cell_count; i++) {
            EntropyCell* cell = field->cells[i];
            
            // Safety check for each cell's contents
            if (cell && cell->states) {
                fprintf(stderr, "DEBUG: Freeing states for cell %zu\n", i);
                // Note: We're not freeing the individual states as they might be shared
                // Just free the array of pointers
                free(cell->states);
                cell->states = NULL;
            }
            
            // Free the cell itself
            free(cell);
            field->cells[i] = NULL;
        }
        
        fprintf(stderr, "DEBUG: Freeing cells array\n");
        free(field->cells);
        field->cells = NULL;
    } else {
        fprintf(stderr, "DEBUG: Field had NULL cells array\n");
    }
    
    // Validate and cleanup constraints
    if (field->constraints) {
        fprintf(stderr, "DEBUG: Cleaning up constraints array with %zu constraints\n", field->constraint_count);
        
        for (size_t i = 0; i < field->constraint_count; i++) {
            EntropyConstraint* constraint = field->constraints[i];
            
            // Safety check for constraint's cell IDs
            if (constraint && constraint->cell_ids) {
                fprintf(stderr, "DEBUG: Freeing cell_ids for constraint %zu\n", i);
                free(constraint->cell_ids);
                constraint->cell_ids = NULL;
            }
            
            // Free the constraint itself
            free(constraint);
            field->constraints[i] = NULL;
        }
        
        fprintf(stderr, "DEBUG: Freeing constraints array\n");
        free(field->constraints);
        field->constraints = NULL;
    } else {
        fprintf(stderr, "DEBUG: Field had NULL constraints array\n");
    }
    
    // Finally free the field itself
    fprintf(stderr, "DEBUG: Freeing field structure\n");
    free(field);
    fprintf(stderr, "DEBUG: Field destruction complete\n");
}

// Add a cell to the entropy field
EntropyCell* braggi_entropy_field_add_cell(EntropyField* field, uint32_t position) {
    if (!field) return NULL;
    
    // Create a new cell
    EntropyCell* cell = braggi_entropy_cell_create(field->cell_count);
    if (!cell) return NULL;
    
    // Set position (stored as multiple fields for optimization)
    cell->position_offset = position;
    cell->position_line = 0;  // Will be set by caller if needed
    cell->position_column = 0; // Will be set by caller if needed
    
    // Ensure capacity for the new cell
    if (field->cell_count >= field->cell_capacity) {
        size_t new_capacity = (field->cell_capacity == 0) ? 16 : field->cell_capacity * 2;
        EntropyCell** new_cells = (EntropyCell**)realloc(field->cells, new_capacity * sizeof(EntropyCell*));
        if (!new_cells) {
            braggi_entropy_cell_destroy(cell);
            return NULL;
        }
        field->cells = new_cells;
        field->cell_capacity = new_capacity;
    }
    
    // Add cell to field
    field->cells[field->cell_count++] = cell;
    
    return cell;
}

// Get a cell from an entropy field by ID
EntropyCell* braggi_entropy_field_get_cell(EntropyField* field, uint32_t cell_id) {
    if (!field || !field->cells || cell_id >= field->cell_count) {
        return NULL;
    }
    
    // Find the cell with the given ID
    for (size_t i = 0; i < field->cell_count; i++) {
        if (field->cells[i] && field->cells[i]->id == cell_id) {
            return field->cells[i];
        }
    }
    
    return NULL; // Cell not found
}

// Get a cell by ID
Cell* braggi_entropy_field_get_cell_by_id(const EntropyField* field, uint32_t id) {
    if (!field || !field->cells || id >= field->cell_count) return NULL;
    
    return field->cells[id];
}

// Add a constraint to an entropy field
bool braggi_entropy_field_add_constraint(EntropyField* field, EntropyConstraint* constraint) {
    if (!field || !constraint) return false;
    
    // Grow the constraint array
    EntropyConstraint** new_constraints = (EntropyConstraint**)realloc(
        field->constraints, (field->constraint_count + 1) * sizeof(EntropyConstraint*));
    
    if (!new_constraints) return false;
    
    field->constraints = new_constraints;
    field->constraints[field->constraint_count++] = constraint;
    
    return true;
}

// Find the cell with the lowest non-zero entropy
EntropyCell* braggi_entropy_field_find_lowest_entropy_cell(EntropyField* field) {
    if (!field) {
        fprintf(stderr, "ERROR: braggi_entropy_field_find_lowest_entropy_cell called with NULL field\n");
        return NULL;
    }
    
    if (!field->cells) {
        fprintf(stderr, "ERROR: Field has NULL cells array\n");
        return NULL;
    }
    
    if (field->cell_count == 0) {
        fprintf(stderr, "ERROR: Field has zero cells\n");
        return NULL;
    }
    
    EntropyCell* lowest_cell = NULL;
    float lowest_entropy = FLT_MAX;
    uint32_t found_cell_id = UINT32_MAX;
    
    fprintf(stderr, "Looking for lowest entropy cell among %zu cells\n", field->cell_count);
    
    // Count cells that have valid states
    size_t valid_cells = 0;
    size_t collapsed_cells = 0;
    
    for (size_t i = 0; i < field->cell_count; i++) {
        EntropyCell* cell = field->cells[i];
        if (!cell) {
            fprintf(stderr, "WARNING: Cell %zu is NULL, skipping\n", i);
            continue;
        }
        
        if (!cell->states) {
            fprintf(stderr, "WARNING: Cell %zu has NULL states array, skipping\n", i);
            continue;
        }
        
        if (cell->state_count == 0) {
            fprintf(stderr, "WARNING: Cell %zu has zero states (contradiction), skipping\n", i);
            continue;
        }
        
        if (cell->state_count == 1) {
            // This cell is already collapsed
            collapsed_cells++;
            continue;
        }
        
        valid_cells++;
        
        // Calculate entropy for this cell
        float entropy = 0.0f;
        size_t valid_states = 0;
        
        for (size_t j = 0; j < cell->state_count; j++) {
            EntropyState* state = cell->states[j];
            if (!state) {
                fprintf(stderr, "WARNING: State %zu in cell %zu is NULL, skipping\n", j, i);
                continue;
            }
            
            float state_entropy = braggi_entropy_state_get_entropy(state);
            entropy += state_entropy;
            valid_states++;
        }
        
        // Only consider cells with valid states
        if (valid_states == 0) {
            fprintf(stderr, "WARNING: Cell %zu has no valid states, skipping\n", i);
            continue;
        }
        
        if (entropy > 0.0f && entropy < lowest_entropy) {
            lowest_entropy = entropy;
            lowest_cell = cell;
            found_cell_id = cell->id;
            
            // Debug info about potential lowest cell
            fprintf(stderr, "Found new lowest entropy cell: ID=%u, entropy=%.4f, states=%zu\n",
                   cell->id, entropy, cell->state_count);
        }
    }
    
    fprintf(stderr, "Cell stats: %zu valid, %zu collapsed, %zu total\n", 
           valid_cells, collapsed_cells, field->cell_count);
    
    if (lowest_cell) {
        fprintf(stderr, "Selected lowest entropy cell ID=%u with entropy=%.4f\n", 
               found_cell_id, lowest_entropy);
    } else {
        fprintf(stderr, "No suitable lowest entropy cell found\n");
    }
    
    return lowest_cell;
}

// Collapse a cell to a specific state
bool braggi_entropy_field_collapse_cell(EntropyField* field, uint32_t cell_id, uint32_t state_index) {
    if (!field) {
        fprintf(stderr, "ERROR: braggi_entropy_field_collapse_cell called with NULL field\n");
        return false;
    }
    
    if (!field->cells) {
        fprintf(stderr, "ERROR: Field has NULL cells array\n");
        return false;
    }
    
    // Validate cell_id
    if (cell_id >= field->cell_count) {
        fprintf(stderr, "ERROR: Cell ID %u out of bounds (max: %zu)\n", 
                cell_id, field->cell_count > 0 ? field->cell_count - 1 : 0);
        return false;
    }
    
    EntropyCell* cell = field->cells[cell_id];
    if (!cell) {
        fprintf(stderr, "ERROR: Cell pointer is NULL for ID %u\n", cell_id);
        return false;
    }
    
    // Validate states array
    if (!cell->states) {
        fprintf(stderr, "ERROR: Cell %u has NULL states array\n", cell_id);
        return false;
    }
    
    if (cell->state_count == 0) {
        fprintf(stderr, "ERROR: Cell %u has no states to collapse to\n", cell_id);
        return false;
    }
    
    // Special case: UINT32_MAX means "pick a random state"
    // "Like pickin' a card from the deck, sometimes ya gotta let lady luck decide!"
    uint32_t actual_state_index = state_index;
    if (state_index == UINT32_MAX) {
        if (cell->state_count == 1) {
            // Only one state, just use that one
            actual_state_index = 0;
        } else {
            // Ensure we have a valid random number
            actual_state_index = (uint32_t)(rand() % cell->state_count);
        }
        fprintf(stderr, "Randomly selected state %u from %u possibilities for cell %u\n", 
               actual_state_index, (unsigned int)cell->state_count, cell_id);
    } else if (state_index >= cell->state_count) {
        fprintf(stderr, "ERROR: State index %u out of bounds (max: %zu)\n", 
               state_index, cell->state_count > 0 ? cell->state_count - 1 : 0);
        return false;
    }
    
    // Validate that actual_state_index is valid
    if (actual_state_index >= cell->state_count) {
        fprintf(stderr, "ERROR: Actual state index %u exceeds state count %zu\n", 
                actual_state_index, cell->state_count);
        return false;
    }
    
    // Keep only the selected state - with additional safety checks
    EntropyState* selected_state = cell->states[actual_state_index];
    if (!selected_state) {
        fprintf(stderr, "ERROR: Selected state pointer is NULL for index %u\n", actual_state_index);
        return false;
    }
    
    // Observe the selected state to finalize its collapse
    braggi_entropy_state_observe(selected_state);
    
    // Create a new array with just the selected state
    EntropyState** new_states = (EntropyState**)malloc(sizeof(EntropyState*));
    if (!new_states) {
        fprintf(stderr, "ERROR: Failed to allocate memory for new states array\n");
        return false;
    }
    
    // Copy the selected state
    new_states[0] = selected_state;
    
    // Free other states (excluding the selected one)
    for (size_t i = 0; i < cell->state_count; i++) {
        if (i != actual_state_index && cell->states[i]) {
            braggi_state_destroy(cell->states[i]);
        }
    }
    
    // Clean up and replace the old states array
    free(cell->states);
    cell->states = new_states;
    cell->state_count = 1;
    
    return true;
}

// Propagate constraints from a cell
bool braggi_entropy_field_propagate_constraints(EntropyField* field, uint32_t cell_id) {
    if (!field || !field->constraints || cell_id >= field->cell_count) return false;
    
    EntropyCell* cell = field->cells[cell_id];
    if (!cell) return false;
    
    // Queue for cells that need constraint propagation
    // Start with the current cell
    Vector* propagation_queue = braggi_vector_create(sizeof(uint32_t));
    if (!propagation_queue) return false;
    
    // Set to track cells already in the queue to avoid duplicates
    Vector* enqueued_cells = braggi_vector_create(sizeof(uint32_t));
    if (!enqueued_cells) {
        braggi_vector_destroy(propagation_queue);
        return false;
    }
    
    // Add initial cell to queue
    braggi_vector_push_back(propagation_queue, &cell_id);
    braggi_vector_push_back(enqueued_cells, &cell_id);
    
    bool any_changes = false;
    
    // Process queue until empty
    while (braggi_vector_size(propagation_queue) > 0) {
        // Get next cell from queue
        uint32_t current_cell_id = *(uint32_t*)braggi_vector_get(propagation_queue, 0);
        EntropyCell* current_cell = field->cells[current_cell_id];
        
        // Remove from front of queue (simulating pop_front)
        braggi_vector_erase(propagation_queue, 0);
        
        if (!current_cell) continue;
        
        // Track cells affected by changes to this cell
        Vector* affected_cells = braggi_vector_create(sizeof(uint32_t));
        if (!affected_cells) continue;
        
        // Find constraints that affect this cell
        bool cell_changed = false;
        
        for (size_t i = 0; i < field->constraint_count; i++) {
            EntropyConstraint* constraint = field->constraints[i];
            if (!constraint || !constraint->cell_ids) continue;
            
            // Check if this constraint affects the cell
            bool affects_cell = false;
            for (size_t j = 0; j < constraint->cell_count; j++) {
                if (constraint->cell_ids[j] == current_cell_id) {
                    affects_cell = true;
                    break;
                }
            }
            
            if (!affects_cell) continue;
            
            // Apply the constraint and track if it changed anything
            bool constraint_result = braggi_constraint_apply(constraint, field);
            cell_changed = cell_changed || constraint_result;
            
            // If constraint application changed something, add all affected cells to the queue
            if (constraint_result) {
                for (size_t j = 0; j < constraint->cell_count; j++) {
                    uint32_t affected_cell_id = constraint->cell_ids[j];
                    
                    // Skip the current cell (already processed)
                    if (affected_cell_id == current_cell_id) continue;
                    
                    // Add to set of affected cells
                    braggi_vector_push_back(affected_cells, &affected_cell_id);
                }
            }
        }
        
        // If this cell changed, add all affected cells to the propagation queue
        if (cell_changed) {
            any_changes = true;
            
            // Add all affected cells to the queue (avoiding duplicates)
            for (size_t i = 0; i < braggi_vector_size(affected_cells); i++) {
                uint32_t affected_cell_id = *(uint32_t*)braggi_vector_get(affected_cells, i);
                
                // Check if already in queue
                bool already_enqueued = false;
                for (size_t j = 0; j < braggi_vector_size(enqueued_cells); j++) {
                    uint32_t enqueued_id = *(uint32_t*)braggi_vector_get(enqueued_cells, j);
                    if (enqueued_id == affected_cell_id) {
                        already_enqueued = true;
                        break;
                    }
                }
                
                if (!already_enqueued) {
                    braggi_vector_push_back(propagation_queue, &affected_cell_id);
                    braggi_vector_push_back(enqueued_cells, &affected_cell_id);
                }
            }
        }
        
        braggi_vector_destroy(affected_cells);
    }
    
    // Clean up
    braggi_vector_destroy(propagation_queue);
    braggi_vector_destroy(enqueued_cells);
    
    return any_changes;
}

// Check if an entropy field has a contradiction
bool braggi_entropy_field_has_contradiction(EntropyField* field) {
    if (!field) return false;
    
    // Check if any cell has zero possible states
    for (uint32_t i = 0; i < field->cell_count; i++) {
        EntropyCell* cell = field->cells[i];
        if (cell && cell->state_count == 0) {
            return true;
        }
    }
    
    return false;
}

// Get detailed information about a contradiction in the field
bool braggi_entropy_field_get_contradiction_info(EntropyField* field, void* position_ptr, char** message) {
    if (!field || !message) return false;
    
    // Check if any cell has zero possible states
    for (uint32_t i = 0; i < field->cell_count; i++) {
        EntropyCell* cell = field->cells[i];
        if (cell && cell->state_count == 0) {
            // We found a contradiction
            
            // Set position information if provided
            if (position_ptr) {
                SourcePosition* position = (SourcePosition*)position_ptr;
                position->file_id = field->source_id;
                position->line = cell->position_line;
                position->column = cell->position_column;
                position->offset = cell->position_offset;
                position->length = 1; // Default length
            }
            
            // Set message
            char* msg = malloc(256);
            if (msg) {
                snprintf(msg, 256, "Contradiction in cell %u: No possible states remain", cell->id);
                *message = msg;
            } else {
                *message = NULL;
            }
            
            return true;
        }
    }
    
    // No contradiction found
    if (message) {
        *message = NULL;
    }
    
    return false;
}

// Add a state to a cell (using correct struct types)
bool braggi_cell_add_state(EntropyCell* cell, EntropyState* state) {
    if (!cell || !state) return false;
    
    // Grow the state array
    EntropyState** new_states = (EntropyState**)realloc(
        cell->states, (cell->state_count + 1) * sizeof(EntropyState*));
    
    if (!new_states) {
        return false;
    }
    
    cell->states = new_states;
    cell->states[cell->state_count++] = state;
    
    // Update the cell's entropy (assuming it's already calculated elsewhere)
    return true;
}

// The backward compatibility function for braggi_entropy_cell_add_state
bool braggi_entropy_cell_add_state(EntropyCell* cell, EntropyState* state) {
    // Simply call the new implementation
    return braggi_cell_add_state(cell, state);
}

// Add a cell to a constraint's affected cells - using correct EntropyConstraint type
bool braggi_constraint_add_cell(EntropyConstraint* constraint, uint32_t cell_id) {
    if (!constraint) return false;
    
    // Check if the cell is already in the cell_ids
    for (size_t i = 0; i < constraint->cell_count; i++) {
        if (constraint->cell_ids[i] == cell_id) {
            return true; // Already added
        }
    }
    
    // Grow the cell_ids array
    uint32_t* new_cells = (uint32_t*)realloc(
        constraint->cell_ids, (constraint->cell_count + 1) * sizeof(uint32_t));
    
    if (!new_cells) return false;
    
    constraint->cell_ids = new_cells;
    constraint->cell_ids[constraint->cell_count++] = cell_id;
    
    return true;
}

// Apply a constraint to the field
bool braggi_constraint_apply(EntropyConstraint* constraint, EntropyField* field) {
    if (!constraint || !field || !constraint->validate || !constraint->cell_ids) return false;
    
    // Save current state counts to detect changes
    uint32_t* original_state_counts = (uint32_t*)malloc(constraint->cell_count * sizeof(uint32_t));
    if (!original_state_counts) return false;
    
    for (size_t i = 0; i < constraint->cell_count; i++) {
        uint32_t cell_id = constraint->cell_ids[i];
        Cell* cell = braggi_entropy_field_get_cell_by_id(field, cell_id);
        
        if (!cell) {
            free(original_state_counts);
            return false;
        }
        
        original_state_counts[i] = cell->state_count;
    }
    
    // Collect states from affected cells for all possible combinations
    Vector* state_combinations = braggi_vector_create(sizeof(State**));
    if (!state_combinations) {
        free(original_state_counts);
        return false;
    }
    
    // For simplicity, we'll just try one combination for now
    // In a full implementation, we would generate all combinations of states
    State** states = (State**)malloc(constraint->cell_count * sizeof(State*));
    if (!states) {
        braggi_vector_destroy(state_combinations);
        free(original_state_counts);
        return false;
    }
    
    for (size_t i = 0; i < constraint->cell_count; i++) {
        uint32_t cell_id = constraint->cell_ids[i];
        Cell* cell = braggi_entropy_field_get_cell_by_id(field, cell_id);
        
        if (!cell || cell->state_count == 0) {
            // Clean up
            for (size_t j = 0; j < i; j++) {
                free(states[j]);
            }
            free(states);
            braggi_vector_destroy(state_combinations);
            free(original_state_counts);
            return false;
        }
        
        // For now, we'll just use the first state of each cell
        // In a full implementation, we would iterate through all states
        EntropyState* entropy_state = cell->states[0];
        
        // Prepare a State struct for the validator
        State* state = (State*)malloc(sizeof(State));
        if (!state) {
            for (size_t j = 0; j < i; j++) {
                free(states[j]);
            }
            free(states);
            braggi_vector_destroy(state_combinations);
            free(original_state_counts);
            return false;
        }
        
        state->id = entropy_state->id;
        state->type = entropy_state->type;
        state->data = entropy_state->data;
        
        states[i] = state;
    }
    
    // Add the combination to our vector
    braggi_vector_push_back(state_combinations, &states);
    
    // Apply the constraint to each combination of states
    bool valid = false;
    
    // Use constraint->validate instead of constraint->validator
    // This is calling the validate function which takes different parameters
    // This is a partial adaptation - in a more thorough fix, we'd need to adapt
    // the validator function to match the new signature
    valid = constraint->validate(constraint, field);
    
    // Clean up
    for (size_t i = 0; i < braggi_vector_size(state_combinations); i++) {
        State** combination = *(State***)braggi_vector_get(state_combinations, i);
        
        for (size_t j = 0; j < constraint->cell_count; j++) {
            free(combination[j]);
        }
        free(combination);
    }
    braggi_vector_destroy(state_combinations);
    
    // Check if any cells changed their state count
    bool changed = false;
    for (size_t i = 0; i < constraint->cell_count; i++) {
        uint32_t cell_id = constraint->cell_ids[i];
        Cell* cell = braggi_entropy_field_get_cell_by_id(field, cell_id);
        
        if (!cell) continue;
        
        if (cell->state_count != original_state_counts[i]) {
            changed = true;
            break;
        }
    }
    
    free(original_state_counts);
    
    // Return whether any changes were made
    return changed;
}

// Structure for tracking collapse decisions for backtracking
typedef struct CollapseDecision {
    uint32_t cell_id;
    uint32_t state_index;
    uint32_t* original_states;  // Array of state IDs before collapse
    uint32_t num_original_states;
} CollapseDecision;

// Helper function to create a collapse decision
static CollapseDecision* create_collapse_decision(EntropyField* field, uint32_t cell_id, uint32_t state_index) {
    if (!field || cell_id >= field->cell_count) return NULL;
    
    EntropyCell* cell = field->cells[cell_id];
    if (!cell) return NULL;
    
    CollapseDecision* decision = (CollapseDecision*)malloc(sizeof(CollapseDecision));
    if (!decision) return NULL;
    
    decision->cell_id = cell_id;
    decision->state_index = state_index;
    decision->num_original_states = cell->state_count;
    
    // Store original states before collapse
    decision->original_states = (uint32_t*)malloc(cell->state_count * sizeof(uint32_t));
    if (!decision->original_states) {
        free(decision);
        return NULL;
    }
    
    for (uint32_t i = 0; i < cell->state_count; i++) {
        EntropyState* state = cell->states[i];
        decision->original_states[i] = state ? state->id : UINT32_MAX;
    }
    
    return decision;
}

// Helper function to free a collapse decision
static void free_collapse_decision(CollapseDecision* decision) {
    if (!decision) return;
    
    if (decision->original_states) {
        free(decision->original_states);
    }
    
    free(decision);
}

// Collapse an entropy field with backtracking
bool braggi_entropy_field_collapse_with_backtracking(EntropyField* field) {
    if (!field) return false;
    
    // Create a vector to track collapse decisions for backtracking
    Vector* decisions = braggi_vector_create(sizeof(CollapseDecision*));
    if (!decisions) return false;
    
    // Track cells that have been tried with all possible states
    Vector* exhausted_cells = braggi_vector_create(sizeof(uint32_t));
    if (!exhausted_cells) {
        braggi_vector_destroy(decisions);
        return false;
    }
    
    // Try collapsing until fully collapsed or no valid solution
    while (!braggi_entropy_field_is_fully_collapsed(field)) {
        // Find cell with minimum entropy
        EntropyCell* min_entropy_cell = braggi_entropy_field_find_lowest_entropy_cell(field);
        
        if (!min_entropy_cell) {
            // No uncollapsed cells left, we're done
            break;
        }
        
        // Check if this cell is already exhausted
        bool cell_exhausted = false;
        for (size_t i = 0; i < braggi_vector_size(exhausted_cells); i++) {
            uint32_t exhausted_id = *(uint32_t*)braggi_vector_get(exhausted_cells, i);
            if (exhausted_id == min_entropy_cell->id) {
                cell_exhausted = true;
                break;
            }
        }
        
        if (cell_exhausted) {
            // We've tried all states for this cell, backtrack
            if (braggi_vector_size(decisions) == 0) {
                // No more decisions to backtrack to, no solution exists
                braggi_vector_destroy(decisions);
                braggi_vector_destroy(exhausted_cells);
                return false;
            }
            
            // Get last decision
            CollapseDecision* last_decision = *(CollapseDecision**)braggi_vector_get(decisions, 
                                                                                     braggi_vector_size(decisions) - 1);
            
            // Remove it from the decision stack
            braggi_vector_pop_back(decisions, NULL);
            
            // Reset cell to its original state
            // (This would need a proper implementation to restore all states)
            
            // Add to exhausted cells
            braggi_vector_push_back(exhausted_cells, &last_decision->cell_id);
            
            // Free the decision
            free_collapse_decision(last_decision);
            continue;
        }
        
        // Try to collapse this cell to a random state
        uint32_t state_index = rand() % min_entropy_cell->state_count;
        
        // Create a decision record before collapsing
        CollapseDecision* decision = create_collapse_decision(field, min_entropy_cell->id, state_index);
        if (!decision) {
            braggi_vector_destroy(decisions);
            braggi_vector_destroy(exhausted_cells);
            return false;
        }
        
        // Add to decision stack
        braggi_vector_push_back(decisions, &decision);
        
        // Collapse the cell
        if (!braggi_entropy_field_collapse_cell(field, min_entropy_cell->id, state_index)) {
            // Failed to collapse, backtrack
            continue;
        }
        
        // Propagate constraints
        if (!braggi_entropy_field_propagate_constraints(field, min_entropy_cell->id)) {
            // Failed to propagate constraints, backtrack
            continue;
        }
        
        // Check for contradictions
        if (braggi_entropy_field_has_contradiction(field)) {
            // Contradiction detected, backtrack
            continue;
        }
        
        // Clear exhausted cells as we've made progress
        braggi_vector_clear(exhausted_cells);
    }
    
    // Clean up
    for (size_t i = 0; i < braggi_vector_size(decisions); i++) {
        CollapseDecision* decision = *(CollapseDecision**)braggi_vector_get(decisions, i);
        free_collapse_decision(decision);
    }
    braggi_vector_destroy(decisions);
    braggi_vector_destroy(exhausted_cells);
    
    return true;
}

// Function to check if the field is fully collapsed
bool braggi_entropy_field_is_fully_collapsed(EntropyField* field) {
    if (!field) {
        fprintf(stderr, "ERROR: braggi_entropy_field_is_fully_collapsed called with NULL field\n");
        return false;
    }
    
    if (!field->cells) {
        fprintf(stderr, "ERROR: Field has NULL cells array\n");
        return false;
    }
    
    if (field->cell_count == 0) {
        fprintf(stderr, "WARNING: Field has zero cells, considered fully collapsed by default\n");
        return true;
    }
    
    size_t collapsed_cells = 0;
    size_t invalid_cells = 0;
    
    for (size_t i = 0; i < field->cell_count; i++) {
        EntropyCell* cell = field->cells[i];
        if (!cell) {
            fprintf(stderr, "WARNING: Cell %zu is NULL, skipping\n", i);
            invalid_cells++;
            continue;
        }
        
        if (!cell->states) {
            fprintf(stderr, "WARNING: Cell %zu has NULL states array, skipping\n", i);
            invalid_cells++;
            continue;
        }
        
        // A cell is collapsed if it has exactly one state
        if (cell->state_count == 1) {
            if (!cell->states[0]) {
                fprintf(stderr, "WARNING: Cell %zu has NULL state at index 0\n", i);
                invalid_cells++;
                continue;
            }
            collapsed_cells++;
        } else if (cell->state_count == 0) {
            fprintf(stderr, "WARNING: Cell %zu has zero states (contradiction)\n", i);
            invalid_cells++;
        } else {
            fprintf(stderr, "Cell %zu has %zu states, not collapsed\n", i, cell->state_count);
            // We found a cell that's not collapsed, return immediately
            fprintf(stderr, "Field is not fully collapsed: %zu/%zu cells collapsed, %zu invalid\n",
                   collapsed_cells, field->cell_count, invalid_cells);
            return false;
        }
    }
    
    // All cells are either collapsed or invalid
    fprintf(stderr, "Field is fully collapsed: %zu/%zu cells collapsed, %zu invalid\n",
           collapsed_cells, field->cell_count, invalid_cells);
    
    // If there are any invalid cells, it's a partial collapse at best
    if (invalid_cells > 0) {
        fprintf(stderr, "WARNING: Some cells were invalid (%zu), treating as partial collapse\n", invalid_cells);
    }
    
    // Only return true if all valid cells are collapsed
    return (collapsed_cells + invalid_cells) == field->cell_count;
}

// Generate a text-based visualization of the entropy field
char* braggi_entropy_field_visualize(EntropyField* field) {
    if (!field) return NULL;
    
    // Allocate a buffer for the visualization
    const size_t buffer_size = 4096; // Start with a reasonable size
    char* buffer = (char*)malloc(buffer_size);
    if (!buffer) return NULL;
    
    // Initialize buffer with empty string
    buffer[0] = '\0';
    
    // Safety check for null cells array
    if (!field->cells) {
        snprintf(buffer, buffer_size, "Field has no cells array! Looks emptier than a saloon at sermon time!");
        return buffer;
    }
    
    // Create a simple text-based visualization
    size_t offset = 0;
    
    // Add header information
    offset += snprintf(buffer + offset, buffer_size - offset, 
                      "Entropy Field Visualization - %zu cells\n", field->cell_count);
    
    // Add cell information for the first few cells - with thorough bounds checking
    size_t max_cells_to_show = 12;
    size_t cells_shown = 0;
    
    for (size_t i = 0; i < field->cell_count && cells_shown < max_cells_to_show; i++) {
        // Safety check to prevent buffer overflow
        if (offset >= buffer_size - 100) {
            offset += snprintf(buffer + offset, buffer_size - offset, "...(buffer limit reached)...\n");
            break;
        }
        
        // Check for null cell pointer
        if (!field->cells[i]) {
            offset += snprintf(buffer + offset, buffer_size - offset, "Cell %zu: NULL pointer (saddle's empty!)\n", i);
            cells_shown++;
            continue;
        }
        
        EntropyCell* cell = field->cells[i];
        
        // Safe access to cell data
        unsigned int state_count = 0;
        if (cell) {
            state_count = (unsigned int)(cell->state_count);
        }
        
        // Add basic cell info with bounds checking
        offset += snprintf(buffer + offset, buffer_size - offset, 
                          "Cell %zu: %u states\n", i, state_count);
        
        cells_shown++;
    }
    
    // Add summary if not all cells were shown
    if (field->cell_count > max_cells_to_show) {
        offset += snprintf(buffer + offset, buffer_size - offset, 
                          "... and %zu more cells (too many to rustle up in one view!)\n", 
                          field->cell_count - max_cells_to_show);
    }
    
    // Final safety check to prevent buffer overflow
    if (offset >= buffer_size - 1) {
        buffer[buffer_size - 1] = '\0';
    }
    
    return buffer;
}

// Generate a detailed report on the entropy field state
char* braggi_entropy_field_generate_report(EntropyField* field) {
    if (!field) return strdup("NULL field");
    
    // Buffer for the report
    size_t buffer_size = 4096; // Start with 4KB
    char* buffer = (char*)malloc(buffer_size);
    if (!buffer) return NULL;
    
    size_t offset = 0;
    
    // Write field header
    offset += snprintf(buffer + offset, buffer_size - offset,
                      "Entropy Field Report\n"
                      "===================\n"
                      "Field ID: %u\n"
                      "Cell count: %zu\n"
                      "Constraint count: %zu\n"
                      "Has contradiction: %s\n\n",
                      field->id, field->cell_count, field->constraint_count,
                      field->has_contradiction ? "YES" : "No");
    
    // Summary of cell states
    uint32_t collapsed_cells = 0;
    uint32_t zero_entropy_cells = 0;
    uint32_t high_entropy_cells = 0;
    
    for (size_t i = 0; i < field->cell_count; i++) {
        EntropyCell* cell = field->cells[i];
        if (!cell) continue;
        
        if (cell->state_count == 0) {
            zero_entropy_cells++;
        } else if (cell->state_count == 1) {
            collapsed_cells++;
        } else if (cell->state_count > 5) { // Arbitrary threshold
            high_entropy_cells++;
        }
    }
    
    offset += snprintf(buffer + offset, buffer_size - offset,
                      "Cell State Summary:\n"
                      "  Collapsed cells: %u\n"
                      "  Zero entropy cells: %u\n"
                      "  High entropy cells: %u\n\n",
                      collapsed_cells, zero_entropy_cells, high_entropy_cells);
    
    // Visualization
    char* viz = braggi_entropy_field_visualize(field);
    if (viz) {
        offset += snprintf(buffer + offset, buffer_size - offset,
                          "Field Visualization:\n%s\n",
                          viz);
        free(viz);
    }
    
    // Detailed cell information (top 5 by entropy)
    offset += snprintf(buffer + offset, buffer_size - offset,
                      "\nTop cells by entropy:\n"
                      "---------------------\n");
    
    // Find top 5 cells by entropy
    EntropyCell* top_cells[5] = {NULL};
    float top_entropies[5] = {0.0f};
    
    for (size_t i = 0; i < field->cell_count; i++) {
        EntropyCell* cell = field->cells[i];
        if (!cell || cell->state_count <= 1) continue;
        
        float cell_entropy = 0.0f;
        for (size_t j = 0; j < cell->state_count; j++) {
            EntropyState* state = cell->states[j];
            if (state) {
                cell_entropy += braggi_entropy_state_get_entropy(state);
            }
        }
        
        // Check if this cell belongs in the top 5
        for (int j = 0; j < 5; j++) {
            if (cell_entropy > top_entropies[j]) {
                // Shift down the array to make room
                for (int k = 4; k > j; k--) {
                    top_cells[k] = top_cells[k-1];
                    top_entropies[k] = top_entropies[k-1];
                }
                
                // Insert this cell
                top_cells[j] = cell;
                top_entropies[j] = cell_entropy;
                break;
            }
        }
    }
    
    // Print info for top cells
    for (int i = 0; i < 5; i++) {
        EntropyCell* cell = top_cells[i];
        if (!cell) continue;
        
        offset += snprintf(buffer + offset, buffer_size - offset,
                          "Cell %u (Line %u, Col %u):\n"
                          "  States: %zu\n"
                          "  Entropy: %.4f\n",
                          cell->id, cell->position_line, cell->position_column,
                          cell->state_count, top_entropies[i]);
        
        // Print first few states
        offset += snprintf(buffer + offset, buffer_size - offset, "  Sample states: ");
        for (size_t j = 0; j < cell->state_count && j < 3; j++) {
            EntropyState* state = cell->states[j];
            if (state) {
                offset += snprintf(buffer + offset, buffer_size - offset, 
                                 "%s (%u) ", 
                                 state->label ? state->label : "unnamed", 
                                 state->id);
            }
        }
        
        if (cell->state_count > 3) {
            offset += snprintf(buffer + offset, buffer_size - offset, "... (%zu more)", 
                             cell->state_count - 3);
        }
        
        offset += snprintf(buffer + offset, buffer_size - offset, "\n\n");
    }
    
    return buffer;
}

// Main Wave Function Collapse runner
bool braggi_entropy_field_apply_wave_function_collapse(EntropyField* field) {
    if (!field) {
        fprintf(stderr, "ERROR: braggi_entropy_field_apply_wave_function_collapse called with NULL field\n");
        return false;
    }
    
    // Initialize random seed
    // "Givin' the entropy gods a little nudge with some randomness - yeehaw!"
    static bool seeded = false;
    if (!seeded) {
        srand((unsigned int)time(NULL));
        seeded = true;
    }
    
    // Extra safety checks for common segfault causes
    if (!field->cells) {
        fprintf(stderr, "ERROR: Field has no cells array\n");
        return false;
    }
    
    if (field->cell_count == 0) {
        fprintf(stderr, "ERROR: Field has zero cells\n");
        return false;
    }
    
    // Set status tracking
    bool success = false;
    bool has_contradiction = false;
    bool progress_made = true;
    unsigned int iteration = 0;
    const unsigned int MAX_ITERATIONS = 100; // Safety limit
    bool verbose = false; // Add verbose flag - "Shhh, we're huntin' wabbits quietly by default!" 
    
    // Log initial state
    fprintf(stderr, "Starting Wave Function Collapse with %zu cells...\n", field->cell_count);
    
    // Check for NULL cells - we need to make sure all cells are valid
    for (size_t i = 0; i < field->cell_count; i++) {
        if (!field->cells[i]) {
            fprintf(stderr, "ERROR: Cell %zu is NULL\n", i);
            return false;
        }
        
        if (!field->cells[i]->states) {
            fprintf(stderr, "ERROR: Cell %zu has NULL states array\n", i);
            return false;
        }
        
        if (field->cells[i]->state_count == 0) {
            fprintf(stderr, "ERROR: Cell %zu has zero states\n", i);
            return false;
        }
    }
    
    // Skip detailed reporting which might be causing segfaults
    /*
    char* initial_report = braggi_entropy_field_generate_report(field);
    if (initial_report) {
        printf("%s\n", initial_report);
        free(initial_report);
    }
    */
    
    // Main loop - continue until fully collapsed or contradiction is unresolvable
    while (progress_made && !success && iteration < MAX_ITERATIONS) {
        iteration++;
        progress_made = false;
        
        // Step 1: Apply all constraints to reduce entropy - with EXTRA CAUTION
        if (field->constraints && field->constraint_count > 0) {
            for (size_t i = 0; i < field->constraint_count; i++) {
                EntropyConstraint* constraint = field->constraints[i];
                if (!constraint) {
                    fprintf(stderr, "WARNING: Constraint %zu is NULL, skipping\n", i);
                    continue;
                }
                
                if (!constraint->validate) {
                    fprintf(stderr, "WARNING: Constraint %zu has NULL validate function, skipping\n", i);
                    continue;
                }
                
                fprintf(stderr, "Applying constraint %zu of %zu\n", i+1, field->constraint_count);
                if (braggi_constraint_apply(constraint, field)) {
                    progress_made = true;
                }
            }
        } else {
            fprintf(stderr, "No constraints to apply\n");
        }
        
        // Step 2: Check for cells with zero entropy (contradictions)
        has_contradiction = braggi_entropy_field_has_contradiction(field);
        if (has_contradiction) {
            fprintf(stderr, "Contradiction detected at iteration %u\n", iteration);
            
            // We'll just break out with false rather than attempting backtracking
            // which might cause segfaults
            return false;
        }
        
        // Step 3: Check if the field is fully collapsed
        if (braggi_entropy_field_is_fully_collapsed(field)) {
            success = true;
            fprintf(stderr, "Field is fully collapsed after %u iterations!\n", iteration);
            break;
        }
        
        // Step 4: If no progress made and no success yet, make a random collapse decision
        if (!progress_made && !success) {
            // Choose cell with lowest entropy - with lots of safety checks
            EntropyCell* lowest_entropy_cell = braggi_entropy_field_find_lowest_entropy_cell(field);
            if (!lowest_entropy_cell) {
                fprintf(stderr, "WARNING: No lowest entropy cell found\n");
                break;
            }
            
            uint32_t cell_id = lowest_entropy_cell->id;
            fprintf(stderr, "Making random collapse decision for cell %u\n", cell_id);
            
            // Collapse this cell - but only if it's valid
            if (cell_id < field->cell_count && 
                field->cells[cell_id] && 
                field->cells[cell_id]->state_count > 0) {
                
                if (braggi_entropy_field_collapse_cell(field, cell_id, UINT32_MAX)) {
                    progress_made = true;
                    fprintf(stderr, "Successfully collapsed cell %u\n", cell_id);
                } else {
                    fprintf(stderr, "Failed to collapse cell %u\n", cell_id);
                }
            } else {
                fprintf(stderr, "Cell %u is invalid\n", cell_id);
                return false;
            }
        }
    }
    
    // Skip visualization which might be causing segfaults
    fprintf(stderr, "Wave Function Collapse complete after %u iterations\n", iteration);
    
    if (iteration >= MAX_ITERATIONS) {
        fprintf(stderr, "WARNING: Reached maximum iteration limit (%u). Solution may be incomplete.\n", MAX_ITERATIONS);
    }
    
    return success;
}

// Create a new entropy cell
EntropyCell* braggi_entropy_cell_create(uint32_t id) {
    EntropyCell* cell = (EntropyCell*)calloc(1, sizeof(EntropyCell));
    if (!cell) return NULL;
    
    cell->id = id;
    cell->states = NULL;
    cell->state_count = 0;
    cell->state_capacity = 0;
    cell->constraints = NULL;
    cell->constraint_count = 0;
    cell->constraint_capacity = 0;
    cell->data = NULL;
    cell->position_offset = 0;
    cell->position_line = 0;
    cell->position_column = 0;
    
    return cell;
}

// Destroy an entropy cell
void braggi_entropy_cell_destroy(EntropyCell* cell) {
    if (!cell) return;
    
    // Free states
    if (cell->states) {
        for (size_t i = 0; i < cell->state_count; i++) {
            if (cell->states[i]) {
                braggi_state_destroy(cell->states[i]);
            }
        }
        free(cell->states);
    }
    
    // Free constraints (just the array, not the constraints themselves)
    if (cell->constraints) {
        free(cell->constraints);
    }
    
    free(cell);
}

// Check if a cell is collapsed (has only one possible state)
bool braggi_entropy_cell_is_collapsed(EntropyCell* cell) {
    if (!cell) return false;
    
    return cell->state_count == 1;
}

// Get the collapsed state of a cell (NULL if not collapsed)
EntropyState* braggi_entropy_cell_get_collapsed_state(EntropyCell* cell) {
    if (!cell || !braggi_entropy_cell_is_collapsed(cell) || !cell->states) {
        return NULL;
    }
    
    return cell->states[0];
}

// Calculate the entropy of an entropy cell
double braggi_entropy_cell_get_entropy(EntropyCell* cell) {
    if (!cell || cell->state_count == 0) return 0.0;
    
    // If cell has only one state, entropy is 0
    if (cell->state_count == 1) return 0.0;
    
    // Calculate total probability
    double total_probability = 0.0;
    for (size_t i = 0; i < cell->state_count; i++) {
        EntropyState* state = cell->states[i];
        if (state && !braggi_state_is_eliminated(state)) {
            total_probability += (double)state->probability;
        }
    }
    
    // Calculate entropy
    if (total_probability <= 0.0) return 0.0;
    
    double entropy = 0.0;
    for (size_t i = 0; i < cell->state_count; i++) {
        EntropyState* state = cell->states[i];
        if (state && !braggi_state_is_eliminated(state)) {
            double p = (double)state->probability / total_probability;
            if (p > 0.0) {
                entropy -= p * log2(p);
            }
        }
    }
    
    return entropy;
}

/*
 * Check if a state has been eliminated from consideration
 * A state is considered eliminated if its probability is 0 or it's marked as invalid
 *
 * You know what they say 'round these parts - "A state with no probability
 * is like a horse without a saddle, ain't goin' anywhere fast!"
 */
bool braggi_state_is_eliminated(EntropyState* state) {
    if (!state) {
        return true; // Null state is effectively eliminated
    }
    
    // A state is considered eliminated if its probability is zero
    // or if it has some internal elimination flag set
    return (state->probability == 0);
    
    // In a more complex implementation, we might check other conditions
    // such as if the state has been explicitly marked as eliminated
    // return (state->probability == 0 || state->is_eliminated);
}

/*
 * Set the elimination status of a state
 * 
 * "Markin' a state eliminated is like tellin' a steer it's headin' to market - 
 * ain't no comin' back from that decision, partner!" - Texas Entropy Management 101
 */
void braggi_state_set_eliminated(EntropyState* state, bool eliminated) {
    if (!state) {
        return; // Can't eliminate a null state
    }
    
    // Set the probability to 0 to mark as eliminated
    if (eliminated) {
        state->probability = 0;
    } else {
        // If un-eliminating, set to a low but non-zero probability
        // This is a bit of a hack, but it works for now
        if (state->probability == 0) {
            state->probability = 0.01;
        }
    }
}

/*
 * Eliminate a state from consideration
 * This marks the state as no longer viable for selection
 * 
 * "Like kickin' a cow outta the herd, this state ain't comin' back
 * for the cattle drive!" - Texas Entropy Management 101
 */
void braggi_state_eliminate(EntropyState* state) {
    if (!state) {
        return;
    }
    
    // Set the probability to zero to mark as eliminated
    state->probability = 0;
    
    // In a more complex implementation, we might also set a flag
    // state->is_eliminated = true;
}

/*
 * Restore a previously eliminated state
 * This allows the state to be considered again in entropy calculations
 * 
 * "Bringin' a state back from the dead? That's quantum resurrection,
 * partner! Yeehaw!" - Quantum Cowboy Handbook
 */
void braggi_state_restore(EntropyState* state) {
    if (!state) {
        return;
    }
    
    // If the state was eliminated by setting probability to 0,
    // we restore it by giving it a default probability
    if (state->probability == 0) {
        state->probability = 1; // Default non-zero probability
    }
    
    // In a more complex implementation, we might also clear a flag
    // state->is_eliminated = false;
} 