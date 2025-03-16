/*
 * Braggi - State Definitions
 * 
 * "States are like honky-tonk dancers - they all move together,
 * but only a few are gonna make it to the final round!" 
 */

#ifndef BRAGGI_STATE_H
#define BRAGGI_STATE_H

#include <stdint.h>
#include <stdbool.h>

// Forward declarations instead of including entropy.h
typedef struct EntropyState EntropyState;
typedef struct EntropyCell EntropyCell;

// Define state types
typedef enum {
    STATE_TYPE_TOKEN,
    STATE_TYPE_SYNTAX,
    STATE_TYPE_SEMANTIC,
    STATE_TYPE_TYPE,
    STATE_TYPE_REGION,
    STATE_TYPE_CUSTOM
} StateType;

// State operations - these operate on EntropyState defined in entropy.h
uint32_t braggi_state_get_id(const EntropyState* state);
StateType braggi_state_get_type(const EntropyState* state);
const char* braggi_state_get_label(const EntropyState* state);
void* braggi_state_get_data(const EntropyState* state);
uint32_t braggi_state_get_probability(const EntropyState* state);
void braggi_state_set_probability(EntropyState* state, uint32_t probability);

// State comparison
bool braggi_state_equals(const EntropyState* state1, const EntropyState* state2);
int braggi_state_compare(const EntropyState* state1, const EntropyState* state2);

#endif // BRAGGI_STATE_H 