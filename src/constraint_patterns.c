/*
 * Braggi - Constraint Pattern Validators Implementation
 * 
 * "In the quantum world of parsing, the constraints don't just check what's right -
 * they collapse all wrong possibilities until only the correct ones remain!"
 * - Texan quantum computing wisdom
 */

#include "braggi/token_propagator.h"
#include "braggi/constraint.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Forward declaration of validator function types
typedef bool (*SequenceValidator)(State** states, size_t state_count, Pattern** sub_patterns, size_t sub_pattern_count);
typedef bool (*SuperpositionValidator)(State** states, size_t state_count, Pattern** sub_patterns, size_t sub_pattern_count);
typedef bool (*OptionalValidator)(State** states, size_t state_count, Pattern* sub_pattern);
typedef bool (*RepeatValidator)(State** states, size_t state_count, Pattern* sub_pattern, bool require_one);
typedef bool (*TokenValidator)(State** states, size_t state_count, TokenType token_type, const char* token_value);
typedef bool (*ReferenceValidator)(State** states, size_t state_count, const char* reference_name, ConstraintPatternLibrary* library);

// Pattern-specific validator implementation functions
static bool validate_sequence(State** states, size_t state_count, Pattern** sub_patterns, size_t sub_pattern_count) {
    // A sequence must have exactly enough states to match all sub-patterns
    size_t total_required_states = 0;
    
    // First, count how many states are required
    for (size_t i = 0; i < sub_pattern_count; i++) {
        Pattern* pattern = sub_patterns[i];
        
        switch (pattern->type) {
            case PATTERN_TOKEN:
                total_required_states += 1;
                break;
            case PATTERN_SEQUENCE:
                // Recursive calculation for nested sequences
                total_required_states += pattern->sub_pattern_count;
                break;
            case PATTERN_OPTIONAL:
                // Optional patterns may or may not consume a state
                // We'll take the optimistic approach and assume they don't
                break;
            case PATTERN_SUPERPOSITION:
                // Assume the minimum of any choice
                total_required_states += 1;
                break;
            case PATTERN_REPEAT:
                // Zero or more, so assume zero
                break;
            case PATTERN_REPEAT_ONE:
                // One or more, so assume one
                total_required_states += 1;
                break;
            case PATTERN_REFERENCE:
                // For references, assume one state (simplified)
                total_required_states += 1;
                break;
        }
    }
    
    if (state_count < total_required_states) {
        return false; // Not enough states for this sequence
    }
    
    // Now try to match each sub-pattern against the states
    size_t state_index = 0;
    
    for (size_t i = 0; i < sub_pattern_count; i++) {
        Pattern* pattern = sub_patterns[i];
        
        // Check if we've run out of states
        if (state_index >= state_count) {
            // If this pattern is optional or zero-repeating, we can continue
            if (pattern->type == PATTERN_OPTIONAL || pattern->type == PATTERN_REPEAT) {
                continue;
            }
            return false; // Otherwise, we've failed
        }
        
        // Try to match this sub-pattern
        switch (pattern->type) {
            case PATTERN_TOKEN:
                {
                    // For a token, match exactly one state
                    State* state = states[state_index];
                    
                    // If token type doesn't match, this fails
                    if (state->type != pattern->token_type) {
                        return false;
                    }
                    
                    // If token value is specified, check it
                    if (pattern->token_value && state->data) {
                        const char* state_value = (const char*)state->data;
                        if (strcmp(state_value, pattern->token_value) != 0) {
                            return false;
                        }
                    }
                    
                    state_index++;
                }
                break;
                
            case PATTERN_SEQUENCE:
                {
                    // For a nested sequence, we need to call recursively
                    // Determine how many states this sequence consumes
                    size_t remaining = state_count - state_index;
                    
                    // Try to validate the subsequence
                    if (!validate_sequence(&states[state_index], remaining, 
                                         pattern->sub_patterns, pattern->sub_pattern_count)) {
                        return false;
                    }
                    
                    // If successful, advance by the number of states consumed
                    // For simplicity, we'll just advance by the minimum required
                    state_index += pattern->sub_pattern_count;
                }
                break;
                
            case PATTERN_OPTIONAL:
                {
                    // For optional, try with and without consuming the state
                    // First, try without consuming
                    // (Nothing to do in this case, we just continue)
                    
                    // Then try consuming one state
                    if (state_index < state_count) {
                        // Check if the sub-pattern matches the current state
                        Pattern* sub = pattern->sub_patterns[0];
                        if (sub->type == PATTERN_TOKEN && states[state_index]->type == sub->token_type) {
                            // If it matches, consume the state
                            state_index++;
                        }
                    }
                }
                break;
                
            case PATTERN_SUPERPOSITION:
                {
                    // For superposition (choice), try each alternative
                    bool any_matched = false;
                    
                    for (size_t j = 0; j < pattern->sub_pattern_count; j++) {
                        Pattern* sub = pattern->sub_patterns[j];
                        
                        // For simplicity, we'll just handle TOKEN patterns directly
                        if (sub->type == PATTERN_TOKEN) {
                            if (states[state_index]->type == sub->token_type) {
                                any_matched = true;
                                state_index++;
                                break;
                            }
                        }
                        // For other types, we'd need more complex logic
                    }
                    
                    if (!any_matched) {
                        return false;
                    }
                }
                break;
                
            case PATTERN_REPEAT:
                {
                    // For zero or more, try to match as many as possible
                    Pattern* sub = pattern->sub_patterns[0];
                    
                    // For simplicity, we'll just handle TOKEN patterns directly
                    if (sub->type == PATTERN_TOKEN) {
                        while (state_index < state_count && 
                               states[state_index]->type == sub->token_type) {
                            state_index++;
                        }
                    }
                    // For other types, we'd need more complex logic
                }
                break;
                
            case PATTERN_REPEAT_ONE:
                {
                    // For one or more, must match at least once
                    Pattern* sub = pattern->sub_patterns[0];
                    
                    // For simplicity, we'll just handle TOKEN patterns directly
                    if (sub->type == PATTERN_TOKEN) {
                        if (state_index < state_count && 
                            states[state_index]->type == sub->token_type) {
                            state_index++;
                            
                            // Then match as many more as possible
                            while (state_index < state_count && 
                                   states[state_index]->type == sub->token_type) {
                                state_index++;
                            }
                        } else {
                            return false; // Must match at least once
                        }
                    }
                    // For other types, we'd need more complex logic
                }
                break;
                
            case PATTERN_REFERENCE:
                // For references, we'd need to look up the pattern and validate recursively
                // This is a simplified placeholder
                state_index++;
                break;
        }
    }
    
    // If we've consumed all states, the sequence is valid
    return (state_index == state_count);
}

static bool validate_superposition(State** states, size_t state_count, Pattern** sub_patterns, size_t sub_pattern_count) {
    // A superposition is valid if any of its sub-patterns is valid
    for (size_t i = 0; i < sub_pattern_count; i++) {
        Pattern* pattern = sub_patterns[i];
        
        switch (pattern->type) {
            case PATTERN_TOKEN:
                {
                    // For a token pattern, check if the first state matches
                    if (state_count > 0 && states[0]->type == pattern->token_type) {
                        // If token value is specified, check it
                        if (pattern->token_value && states[0]->data) {
                            const char* state_value = (const char*)states[0]->data;
                            if (strcmp(state_value, pattern->token_value) == 0) {
                                return true;
                            }
                        } else {
                            return true;
                        }
                    }
                }
                break;
                
            case PATTERN_SEQUENCE:
                {
                    // For a sequence pattern, check if the states match the sequence
                    if (validate_sequence(states, state_count, 
                                        pattern->sub_patterns, pattern->sub_pattern_count)) {
                        return true;
                    }
                }
                break;
                
            // Handle other pattern types similarly...
            
            default:
                // For other pattern types, we'd need specific validation logic
                break;
        }
    }
    
    // If none of the sub-patterns matched, the superposition is invalid
    return false;
}

static bool validate_optional(State** states, size_t state_count, Pattern* sub_pattern) {
    // An optional pattern is valid if either:
    // 1. The sub-pattern is valid for the states, or
    // 2. There are no states (the optional element is omitted)
    
    if (state_count == 0) {
        return true; // Valid if there are no states (option omitted)
    }
    
    // Otherwise, validate against the sub-pattern
    switch (sub_pattern->type) {
        case PATTERN_TOKEN:
            {
                // For a token pattern, check if the first state matches
                if (states[0]->type == sub_pattern->token_type) {
                    // If token value is specified, check it
                    if (sub_pattern->token_value && states[0]->data) {
                        const char* state_value = (const char*)states[0]->data;
                        if (strcmp(state_value, sub_pattern->token_value) == 0) {
                            return true;
                        }
                    } else {
                        return true;
                    }
                }
            }
            break;
            
        case PATTERN_SEQUENCE:
            {
                // For a sequence pattern, check if the states match the sequence
                if (validate_sequence(states, state_count, 
                                    sub_pattern->sub_patterns, sub_pattern->sub_pattern_count)) {
                    return true;
                }
            }
            break;
            
        // Handle other pattern types similarly...
        
        default:
            // For other pattern types, we'd need specific validation logic
            break;
    }
    
    // If the sub-pattern doesn't match, but the optional pattern is still valid (omitted)
    return true;
}

static bool validate_repeat(State** states, size_t state_count, Pattern* sub_pattern, bool require_one) {
    // A repeat pattern is valid if:
    // 1. require_one is false and there are no states (zero repetitions), or
    // 2. The states can be partitioned into one or more valid matches of the sub-pattern
    
    if (state_count == 0) {
        return !require_one; // Valid only if zero repetitions are allowed
    }
    
    // Check if the first state matches the sub-pattern
    bool first_match = false;
    size_t states_consumed = 0;
    
    switch (sub_pattern->type) {
        case PATTERN_TOKEN:
            {
                // For a token pattern, check if the first state matches
                if (states[0]->type == sub_pattern->token_type) {
                    // If token value is specified, check it
                    if (sub_pattern->token_value && states[0]->data) {
                        const char* state_value = (const char*)states[0]->data;
                        if (strcmp(state_value, sub_pattern->token_value) == 0) {
                            first_match = true;
                            states_consumed = 1;
                        }
                    } else {
                        first_match = true;
                        states_consumed = 1;
                    }
                }
            }
            break;
            
        case PATTERN_SEQUENCE:
            {
                // For a sequence pattern, check if the states match the sequence
                // This is a simplified approach that assumes sequences consume a fixed number of states
                if (validate_sequence(states, state_count, 
                                    sub_pattern->sub_patterns, sub_pattern->sub_pattern_count)) {
                    first_match = true;
                    states_consumed = sub_pattern->sub_pattern_count;
                }
            }
            break;
            
        // Handle other pattern types similarly...
        
        default:
            // For other pattern types, we'd need specific validation logic
            break;
    }
    
    if (!first_match) {
        return !require_one; // Valid only if zero repetitions are allowed
    }
    
    // If there are more states, recursively check if they form valid repetitions
    if (states_consumed < state_count) {
        return validate_repeat(&states[states_consumed], state_count - states_consumed, 
                             sub_pattern, false);
    }
    
    // If we've consumed all states with valid matches, the repeat pattern is valid
    return true;
}

static bool validate_token(State** states, size_t state_count, TokenType token_type, const char* token_value) {
    // A token pattern is valid if there is exactly one state that matches the token type and value
    
    if (state_count != 1) {
        return false; // Must have exactly one state
    }
    
    State* state = states[0];
    
    // Check token type
    if (state->type != token_type) {
        return false;
    }
    
    // If token value is specified, check it
    if (token_value && state->data) {
        const char* state_value = (const char*)state->data;
        return (strcmp(state_value, token_value) == 0);
    }
    
    // If no token value specified, any token of the right type is valid
    return true;
}

// Validator function for different pattern types
bool braggi_pattern_validate(Pattern* pattern, State** states, size_t state_count, ConstraintPatternLibrary* library) {
    if (!pattern || !states || state_count == 0) {
        return false;
    }
    
    switch (pattern->type) {
        case PATTERN_SEQUENCE:
            return validate_sequence(states, state_count, 
                                   pattern->sub_patterns, pattern->sub_pattern_count);
            
        case PATTERN_SUPERPOSITION:
            return validate_superposition(states, state_count, 
                                       pattern->sub_patterns, pattern->sub_pattern_count);
            
        case PATTERN_OPTIONAL:
            return validate_optional(states, state_count, pattern->sub_patterns[0]);
            
        case PATTERN_REPEAT:
            return validate_repeat(states, state_count, pattern->sub_patterns[0], false);
            
        case PATTERN_REPEAT_ONE:
            return validate_repeat(states, state_count, pattern->sub_patterns[0], true);
            
        case PATTERN_TOKEN:
            return validate_token(states, state_count, pattern->token_type, pattern->token_value);
            
        case PATTERN_REFERENCE:
            {
                // For references, look up the referenced pattern in the library
                if (!library || !pattern->reference_name) {
                    return false;
                }
                
                Pattern* referenced = braggi_constraint_pattern_library_get_pattern(
                    library, pattern->reference_name);
                    
                if (!referenced) {
                    return false;
                }
                
                // Recursively validate with the referenced pattern
                return braggi_pattern_validate(referenced, states, state_count, library);
            }
            
        default:
            return false; // Unknown pattern type
    }
}

// Create a validator function that binds to a specific pattern
ConstraintValidator braggi_pattern_create_validator(Pattern* pattern, ConstraintPatternLibrary* library) {
    // We create a static validator function that will be called by the constraint system
    return (ConstraintValidator)braggi_pattern_validate;
} 