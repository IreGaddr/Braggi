/*
 * Braggi - State Implementation
 * 
 * "Every quantum cowboy knows that good states make good possibilities,
 * and good possibilities make a darn fine wave function collapse!" - Texas Quantum Logic
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "braggi/state.h"
#include "braggi/entropy.h" // Include entropy.h for EntropyState

EntropyState* braggi_state_create(uint32_t id, uint32_t type, const char* label, void* data, uint32_t probability) {
    EntropyState* state = (EntropyState*)malloc(sizeof(EntropyState));
    if (!state) {
        return NULL;
    }
    
    state->id = id;
    state->type = type;
    state->label = label ? strdup(label) : NULL;
    state->data = data;
    state->probability = probability;
    
    return state;
}

void braggi_state_destroy_wrapper(EntropyState* state) {
    if (!state) {
        return;
    }
    
    if (state->label) {
        free((void*)state->label);
    }
    
    // Note: data is not freed here as it might be owned by someone else
    // The caller is responsible for freeing data if needed
    
    free(state);
}

bool braggi_state_equals(const EntropyState* a, const EntropyState* b) {
    if (!a || !b) {
        return false;
    }
    
    // Two states are equal if they have the same ID
    return a->id == b->id;
}

EntropyState* braggi_state_copy(const EntropyState* state) {
    if (!state) {
        return NULL;
    }
    
    return braggi_state_create(state->id, state->type, state->label, state->data, state->probability);
}

char* braggi_state_to_string(const EntropyState* state) {
    if (!state) {
        return strdup("<null>");
    }
    
    // Format: "State(id=X, type=Y, label=Z, prob=W)"
    size_t size = 64; // Base size for the format string
    if (state->label) {
        size += strlen(state->label);
    }
    
    char* result = (char*)malloc(size);
    if (!result) {
        return NULL;
    }
    
    snprintf(result, size, "State(id=%u, type=%u, label=%s, prob=%u)",
             state->id, state->type, state->label ? state->label : "<null>", state->probability);
    
    return result;
} 