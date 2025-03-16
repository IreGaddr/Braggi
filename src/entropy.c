/*
 * Braggi - Entropy System Implementation
 * 
 * "Entropy ain't just physics, it's a way of life! The more possibilities you have,
 * the more uncertain things get - just like trying to herd cats on a windy day."
 * - Irish-Texan Programming Philosophy
 */

#include "braggi/entropy.h"
#include "braggi/allocation.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <float.h>

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

// Create a new entropy state with uniform distribution
EntropyState* braggi_entropy_state_create(uint32_t dimension) {
    if (dimension == 0) return NULL;
    
    EntropyState* state = (EntropyState*)malloc(sizeof(EntropyState));
    if (!state) return NULL;
    
    state->probabilities = (float*)calloc(dimension, sizeof(float));
    if (!state->probabilities) {
        free(state);
        return NULL;
    }
    
    // Initialize to uniform distribution
    float uniform = 1.0f / (float)dimension;
    for (uint32_t i = 0; i < dimension; i++) {
        state->probabilities[i] = uniform;
    }
    
    state->dimension = dimension;
    state->entropy = calculate_entropy(state->probabilities, dimension);
    state->is_collapsed = false;
    state->observed_value = 0;
    
    return state;
}

// Destroy an entropy state
void braggi_entropy_state_destroy(EntropyState* state) {
    if (!state) return;
    
    if (state->probabilities) {
        free(state->probabilities);
    }
    
    free(state);
}

// Observe an entropy state, collapsing it to a definite value
uint32_t braggi_entropy_state_observe(EntropyState* state) {
    if (!state || !state->probabilities || state->dimension == 0) return 0;
    
    if (state->is_collapsed) {
        return state->observed_value;
    }
    
    // Random number between 0 and 1
    float random = (float)rand() / (float)RAND_MAX;
    
    // Cumulative probability
    float cumulative = 0.0f;
    uint32_t selected = 0;
    
    // Select a value based on the probability distribution
    for (uint32_t i = 0; i < state->dimension; i++) {
        cumulative += state->probabilities[i];
        if (random <= cumulative) {
            selected = i;
            break;
        }
    }
    
    // Collapse the state to the selected value
    for (uint32_t i = 0; i < state->dimension; i++) {
        state->probabilities[i] = 0.0f;
    }
    state->probabilities[selected] = 1.0f;
    
    state->entropy = 0.0f;
    state->is_collapsed = true;
    state->observed_value = selected;
    
    return selected;
}

// Entangle two entropy states, mixing their probability distributions
void braggi_entropy_state_entangle(EntropyState* state1, EntropyState* state2, float strength) {
    if (!state1 || !state2 || !state1->probabilities || !state2->probabilities) return;
    if (state1->dimension != state2->dimension) return;
    
    if (state1->is_collapsed || state2->is_collapsed) {
        // If either state is collapsed, the entanglement has no effect
        return;
    }
    
    // Create temporary arrays for the new probability distributions
    float* new_probs1 = (float*)calloc(state1->dimension, sizeof(float));
    float* new_probs2 = (float*)calloc(state2->dimension, sizeof(float));
    
    if (!new_probs1 || !new_probs2) {
        if (new_probs1) free(new_probs1);
        if (new_probs2) free(new_probs2);
        return;
    }
    
    // Mix the probability distributions
    for (uint32_t i = 0; i < state1->dimension; i++) {
        new_probs1[i] = (1.0f - strength) * state1->probabilities[i] + strength * state2->probabilities[i];
        new_probs2[i] = (1.0f - strength) * state2->probabilities[i] + strength * state1->probabilities[i];
    }
    
    // Normalize the new distributions
    normalize_probabilities(new_probs1, state1->dimension);
    normalize_probabilities(new_probs2, state2->dimension);
    
    // Copy the new distributions back to the states
    memcpy(state1->probabilities, new_probs1, state1->dimension * sizeof(float));
    memcpy(state2->probabilities, new_probs2, state2->dimension * sizeof(float));
    
    // Update the entropy values
    state1->entropy = calculate_entropy(state1->probabilities, state1->dimension);
    state2->entropy = calculate_entropy(state2->probabilities, state2->dimension);
    
    free(new_probs1);
    free(new_probs2);
}

// Apply a constraint to an entropy state
void braggi_entropy_state_constrain(EntropyState* state, float* weights, float strength) {
    if (!state || !state->probabilities || !weights) return;
    
    if (state->is_collapsed) {
        // If the state is already collapsed, constraints have no effect
        return;
    }
    
    // Apply the constraint weights to the probability distribution
    for (uint32_t i = 0; i < state->dimension; i++) {
        state->probabilities[i] *= (1.0f + strength * (weights[i] - 1.0f));
        if (state->probabilities[i] < 0.0f) {
            state->probabilities[i] = 0.0f;
        }
    }
    
    // Renormalize the probability distribution
    normalize_probabilities(state->probabilities, state->dimension);
    
    // Update entropy
    state->entropy = calculate_entropy(state->probabilities, state->dimension);
    
    // Check if the state has collapsed (i.e., one probability is close to 1.0)
    for (uint32_t i = 0; i < state->dimension; i++) {
        if (state->probabilities[i] > 0.99f) {
            state->is_collapsed = true;
            state->observed_value = i;
            break;
        }
    }
}

// Get the current entropy value of a state
float braggi_entropy_state_get_entropy(const EntropyState* state) {
    if (!state) return 0.0f;
    return state->entropy;
}

// Check if an entropy state is fully collapsed
bool braggi_entropy_state_is_collapsed(const EntropyState* state) {
    if (!state) return false;
    return state->is_collapsed;
}

// Create a new entropy constraint
EntropyConstraint* braggi_entropy_constraint_create(int* offsets, float** weight_matrix, uint32_t dimension) {
    if (!offsets || !weight_matrix || dimension == 0) return NULL;
    
    EntropyConstraint* constraint = (EntropyConstraint*)malloc(sizeof(EntropyConstraint));
    if (!constraint) return NULL;
    
    // Copy offsets
    constraint->offsets = (int*)malloc(dimension * sizeof(int));
    if (!constraint->offsets) {
        free(constraint);
        return NULL;
    }
    memcpy(constraint->offsets, offsets, dimension * sizeof(int));
    
    // Copy weight matrix
    constraint->weight_matrix = (float**)malloc(dimension * sizeof(float*));
    if (!constraint->weight_matrix) {
        free(constraint->offsets);
        free(constraint);
        return NULL;
    }
    
    for (uint32_t i = 0; i < dimension; i++) {
        constraint->weight_matrix[i] = (float*)malloc(dimension * sizeof(float));
        if (!constraint->weight_matrix[i]) {
            for (uint32_t j = 0; j < i; j++) {
                free(constraint->weight_matrix[j]);
            }
            free(constraint->weight_matrix);
            free(constraint->offsets);
            free(constraint);
            return NULL;
        }
        memcpy(constraint->weight_matrix[i], weight_matrix[i], dimension * sizeof(float));
    }
    
    constraint->dimension = dimension;
    
    return constraint;
}

// Destroy an entropy constraint
void braggi_entropy_constraint_destroy(EntropyConstraint* constraint) {
    if (!constraint) return;
    
    if (constraint->offsets) {
        free(constraint->offsets);
    }
    
    if (constraint->weight_matrix) {
        for (uint32_t i = 0; i < constraint->dimension; i++) {
            if (constraint->weight_matrix[i]) {
                free(constraint->weight_matrix[i]);
            }
        }
        free(constraint->weight_matrix);
    }
    
    free(constraint);
}

// Create a new entropy rule
EntropyRule* braggi_entropy_rule_create(uint32_t dimension) {
    if (dimension == 0) return NULL;
    
    EntropyRule* rule = (EntropyRule*)malloc(sizeof(EntropyRule));
    if (!rule) return NULL;
    
    rule->constraints = NULL;
    rule->constraint_count = 0;
    rule->dimension = dimension;
    
    return rule;
}

// Destroy an entropy rule
void braggi_entropy_rule_destroy(EntropyRule* rule) {
    if (!rule) return;
    
    if (rule->constraints) {
        // Note: We don't destroy the constraints as they might be shared
        free(rule->constraints);
    }
    
    free(rule);
}

// Add a constraint to a rule
bool braggi_entropy_rule_add_constraint(EntropyRule* rule, EntropyConstraint* constraint) {
    if (!rule || !constraint) return false;
    
    // Grow the constraint array
    EntropyConstraint** new_constraints = (EntropyConstraint**)realloc(
        rule->constraints, (rule->constraint_count + 1) * sizeof(EntropyConstraint*));
    
    if (!new_constraints) return false;
    
    rule->constraints = new_constraints;
    rule->constraints[rule->constraint_count++] = constraint;
    
    return true;
}

// Create a new entropy field
EntropyField* braggi_entropy_field_create(uint32_t width, uint32_t height, uint32_t num_possible_states) {
    if (width == 0 || height == 0 || num_possible_states == 0) return NULL;
    
    EntropyField* field = (EntropyField*)malloc(sizeof(EntropyField));
    if (!field) return NULL;
    
    field->width = width;
    field->height = height;
    field->num_possible_states = num_possible_states;
    field->cells = NULL;
    field->cell_count = 0;
    field->constraints = NULL;
    field->constraint_count = 0;
    field->fully_collapsed = false;
    field->collapse_threshold = 0.001f; // Default threshold
    field->wrap_x = false;
    field->wrap_y = false;
    
    return field;
}

// Destroy an entropy field
void braggi_entropy_field_destroy(EntropyField* field) {
    if (!field) return;
    
    // Free cells
    if (field->cells) {
        for (size_t i = 0; i < field->cell_count; i++) {
            Cell* cell = field->cells[i];
            if (cell) {
                // Free states in this cell
                if (cell->states) {
                    for (size_t j = 0; j < cell->num_states; j++) {
                        EntropyState* state = cell->states[j];
                        if (state) {
                            braggi_entropy_state_destroy(state);
                        }
                    }
                    free(cell->states);
                }
                free(cell);
            }
        }
        free(field->cells);
    }
    
    // Free constraints
    if (field->constraints) {
        for (size_t i = 0; i < field->constraint_count; i++) {
            Constraint* constraint = field->constraints[i];
            if (constraint) {
                braggi_constraint_destroy(constraint);
            }
        }
        free(field->constraints);
    }
    
    free(field);
}

// Add a cell to an entropy field
Cell* braggi_entropy_field_add_cell(EntropyField* field, SourcePosition position) {
    if (!field) return NULL;
    
    // Create a new cell
    Cell* cell = (Cell*)malloc(sizeof(Cell));
    if (!cell) return NULL;
    
    cell->id = field->cell_count;
    cell->states = NULL;
    cell->num_states = 0;
    cell->position = position;
    
    // Grow the cell array
    Cell** new_cells = (Cell**)realloc(field->cells, (field->cell_count + 1) * sizeof(Cell*));
    if (!new_cells) {
        free(cell);
        return NULL;
    }
    
    field->cells = new_cells;
    field->cells[field->cell_count++] = cell;
    
    return cell;
}

// Get a cell at a specific position
Cell* braggi_entropy_field_get_cell(const EntropyField* field, uint32_t x, uint32_t y) {
    if (!field || !field->cells) return NULL;
    
    // Handle wrap-around
    if (field->wrap_x) {
        x = x % field->width;
    } else if (x >= field->width) {
        return NULL;
    }
    
    if (field->wrap_y) {
        y = y % field->height;
    } else if (y >= field->height) {
        return NULL;
    }
    
    uint32_t index = y * field->width + x;
    if (index >= field->cell_count) return NULL;
    
    return field->cells[index];
}

// Get a cell by ID
Cell* braggi_entropy_field_get_cell_by_id(const EntropyField* field, uint32_t id) {
    if (!field || !field->cells || id >= field->cell_count) return NULL;
    
    return field->cells[id];
}

// Add a constraint to an entropy field
bool braggi_entropy_field_add_constraint(EntropyField* field, Constraint* constraint) {
    if (!field || !constraint) return false;
    
    // Grow the constraint array
    Constraint** new_constraints = (Constraint**)realloc(
        field->constraints, (field->constraint_count + 1) * sizeof(Constraint*));
    
    if (!new_constraints) return false;
    
    field->constraints = new_constraints;
    field->constraints[field->constraint_count++] = constraint;
    
    return true;
}

// Find the cell with the lowest non-zero entropy
Cell* braggi_entropy_field_find_lowest_entropy(const EntropyField* field) {
    if (!field || !field->cells || field->cell_count == 0) return NULL;
    
    Cell* lowest_cell = NULL;
    float lowest_entropy = FLT_MAX;
    
    for (size_t i = 0; i < field->cell_count; i++) {
        Cell* cell = field->cells[i];
        if (!cell || cell->num_states <= 1) continue; // Skip collapsed cells
        
        // Calculate entropy for this cell
        float entropy = 0.0f;
        for (size_t j = 0; j < cell->num_states; j++) {
            EntropyState* state = cell->states[j];
            if (state) {
                entropy += braggi_entropy_state_get_entropy(state);
            }
        }
        
        if (entropy > 0.0f && entropy < lowest_entropy) {
            lowest_entropy = entropy;
            lowest_cell = cell;
        }
    }
    
    return lowest_cell;
}

// Collapse a cell to a specific state
bool braggi_entropy_field_collapse_cell(EntropyField* field, Cell* cell, uint32_t state_index) {
    if (!field || !cell || state_index >= cell->num_states) return false;
    
    // Keep only the selected state
    EntropyState* selected_state = cell->states[state_index];
    
    // Observe the selected state to finalize its collapse
    if (selected_state) {
        braggi_entropy_state_observe(selected_state);
    }
    
    // Free other states
    for (size_t i = 0; i < cell->num_states; i++) {
        if (i != state_index && cell->states[i]) {
            braggi_entropy_state_destroy(cell->states[i]);
        }
    }
    
    // Create a new array with just the selected state
    EntropyState** new_states = (EntropyState**)malloc(sizeof(EntropyState*));
    if (!new_states) return false;
    
    new_states[0] = selected_state;
    
    free(cell->states);
    cell->states = new_states;
    cell->num_states = 1;
    
    return true;
}

// Propagate constraints from a cell
bool braggi_entropy_field_propagate_constraints(EntropyField* field, Cell* cell) {
    if (!field || !cell || !field->constraints) return false;
    
    // Find constraints that affect this cell
    bool changed = false;
    
    for (size_t i = 0; i < field->constraint_count; i++) {
        Constraint* constraint = field->constraints[i];
        if (!constraint || !constraint->affected_cells) continue;
        
        // Check if this constraint affects the cell
        bool affects_cell = false;
        for (size_t j = 0; j < constraint->affected_cell_count; j++) {
            if (constraint->affected_cells[j] == cell->id) {
                affects_cell = true;
                break;
            }
        }
        
        if (!affects_cell) continue;
        
        // Apply the constraint
        bool result = braggi_constraint_apply(constraint, field);
        changed = changed || result;
    }
    
    return changed;
}

// Check if the field has any contradictions
bool braggi_entropy_field_has_contradiction(const EntropyField* field, Cell** contradiction_source) {
    if (!field || !field->cells) return false;
    
    for (size_t i = 0; i < field->cell_count; i++) {
        Cell* cell = field->cells[i];
        if (!cell) continue;
        
        if (cell->num_states == 0) {
            // A cell with no valid states is a contradiction
            if (contradiction_source) {
                *contradiction_source = cell;
            }
            return true;
        }
    }
    
    return false;
}

// Add a state to a cell
bool braggi_cell_add_state(Cell* cell, State* state) {
    if (!cell || !state) return false;
    
    // Create an entropy state for this state
    EntropyState* entropy_state = braggi_entropy_state_create(1);
    if (!entropy_state) return false;
    
    // Store the original state data in the entropy state
    entropy_state->observed_value = state->id;
    
    // Grow the state array
    EntropyState** new_states = (EntropyState**)realloc(
        cell->states, (cell->num_states + 1) * sizeof(EntropyState*));
    
    if (!new_states) {
        braggi_entropy_state_destroy(entropy_state);
        return false;
    }
    
    cell->states = new_states;
    cell->states[cell->num_states++] = entropy_state;
    
    return true;
}

// Create a new constraint
Constraint* braggi_constraint_create(int type, 
                               bool (*validator)(State** states, size_t state_count, void* context),
                               void* context, const char* description) {
    if (!validator) return NULL;
    
    Constraint* constraint = (Constraint*)malloc(sizeof(Constraint));
    if (!constraint) return NULL;
    
    static uint32_t next_constraint_id = 1;
    constraint->id = next_constraint_id++;
    constraint->affected_cells = NULL;
    constraint->affected_cell_count = 0;
    constraint->context = context;
    constraint->validator = validator;
    constraint->type = type;
    
    if (description) {
        constraint->description = strdup(description);
    } else {
        constraint->description = strdup("Unnamed constraint");
    }
    
    return constraint;
}

// Destroy a constraint
void braggi_constraint_destroy(Constraint* constraint) {
    if (!constraint) return;
    
    if (constraint->affected_cells) {
        free(constraint->affected_cells);
    }
    
    if (constraint->description) {
        free(constraint->description);
    }
    
    // Note: We don't free the context as it might be managed externally
    
    free(constraint);
}

// Add a cell to a constraint's affected cells
bool braggi_constraint_add_cell(Constraint* constraint, uint32_t cell_id) {
    if (!constraint) return false;
    
    // Check if the cell is already in the affected cells
    for (size_t i = 0; i < constraint->affected_cell_count; i++) {
        if (constraint->affected_cells[i] == cell_id) {
            return true; // Already added
        }
    }
    
    // Grow the affected cells array
    uint32_t* new_cells = (uint32_t*)realloc(
        constraint->affected_cells, (constraint->affected_cell_count + 1) * sizeof(uint32_t));
    
    if (!new_cells) return false;
    
    constraint->affected_cells = new_cells;
    constraint->affected_cells[constraint->affected_cell_count++] = cell_id;
    
    return true;
}

// Apply a constraint to the field
bool braggi_constraint_apply(Constraint* constraint, EntropyField* field) {
    if (!constraint || !field || !constraint->validator || !constraint->affected_cells) return false;
    
    // Collect states from affected cells
    State** states = (State**)malloc(constraint->affected_cell_count * sizeof(State*));
    if (!states) return false;
    
    for (size_t i = 0; i < constraint->affected_cell_count; i++) {
        uint32_t cell_id = constraint->affected_cells[i];
        Cell* cell = braggi_entropy_field_get_cell_by_id(field, cell_id);
        
        if (!cell || cell->num_states == 0) {
            free(states);
            return false;
        }
        
        // Use the first state of each cell for simplicity
        // In a real implementation, we would consider all possible combinations
        EntropyState* entropy_state = cell->states[0];
        
        // Prepare a State struct for the validator
        State* state = (State*)malloc(sizeof(State));
        if (!state) {
            for (size_t j = 0; j < i; j++) {
                free(states[j]);
            }
            free(states);
            return false;
        }
        
        state->id = entropy_state->observed_value;
        state->type = 0; // Simplified - would be based on the actual state type
        state->data = NULL; // Simplified - would be the actual state data
        
        states[i] = state;
    }
    
    // Call the validator
    bool valid = constraint->validator(states, constraint->affected_cell_count, constraint->context);
    
    // Clean up
    for (size_t i = 0; i < constraint->affected_cell_count; i++) {
        free(states[i]);
    }
    free(states);
    
    return valid;
} 