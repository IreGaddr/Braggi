/*
 * Braggi - Constraint Pattern Library Implementation
 * 
 * "Good patterns are like Texas BBQ - simple at heart, but layered with complexity
 * that makes your head spin like a Dublin jig!"
 * - Compiler Cowboy
 */

#include <stdlib.h>
#include <string.h>
#include "braggi/constraint_pattern.h"
#include "braggi/pattern.h"
#include "braggi/util/vector.h"

// Create a new constraint pattern library
ConstraintPatternLibrary* braggi_constraint_pattern_library_create(const char* start_pattern) {
    if (!start_pattern) return NULL;
    
    ConstraintPatternLibrary* library = malloc(sizeof(ConstraintPatternLibrary));
    if (!library) return NULL;
    
    // Initialize with defaults
    library->patterns = NULL;
    library->pattern_count = 0;
    library->start_pattern = strdup(start_pattern);
    
    // Allocate initial pattern array
    library->patterns = malloc(sizeof(Pattern*) * 32); // Initial capacity
    if (!library->patterns || !library->start_pattern) {
        braggi_constraint_pattern_library_destroy(library);
        return NULL;
    }
    
    return library;
}

// Destroy a constraint pattern library
void braggi_constraint_pattern_library_destroy(ConstraintPatternLibrary* library) {
    if (!library) return;
    
    // Free patterns (if we own them)
    if (library->patterns) {
        for (size_t i = 0; i < library->pattern_count; i++) {
            if (library->patterns[i]) {
                braggi_pattern_destroy(library->patterns[i]);
            }
        }
        free(library->patterns);
    }
    
    // Free the start pattern name
    if (library->start_pattern) {
        free((void*)library->start_pattern);
    }
    
    free(library);
}

// Add a pattern to the library
bool braggi_constraint_pattern_library_add_pattern(ConstraintPatternLibrary* library, Pattern* pattern) {
    if (!library || !pattern || !library->patterns) return false;
    
    // Check if we need to resize the patterns array
    if (library->pattern_count % 32 == 0 && library->pattern_count > 0) {
        Pattern** new_patterns = realloc(library->patterns, sizeof(Pattern*) * (library->pattern_count + 32));
        if (!new_patterns) return false;
        library->patterns = new_patterns;
    }
    
    // Add the pattern
    library->patterns[library->pattern_count++] = pattern;
    
    return true;
}

// Get a pattern from the library by name
Pattern* braggi_constraint_pattern_library_get_pattern(ConstraintPatternLibrary* library, const char* name) {
    if (!library || !name || !library->patterns) return NULL;
    
    for (size_t i = 0; i < library->pattern_count; i++) {
        if (library->patterns[i]) {
            const char* pattern_name = braggi_pattern_get_name(library->patterns[i]);
            if (pattern_name && strcmp(pattern_name, name) == 0) {
                return library->patterns[i];
            }
        }
    }
    
    return NULL;
}

// PATTERN CREATION IMPLEMENTATIONS

// Create a token pattern
Pattern* braggi_pattern_create_token(const char* name, TokenType type, const char* value) {
    if (!name) return NULL;
    
    Pattern* pattern = malloc(sizeof(Pattern));
    if (!pattern) return NULL;
    
    memset(pattern, 0, sizeof(Pattern));  // Zero out all fields
    
    pattern->type = PATTERN_TOKEN;
    pattern->name = strdup(name);
    pattern->token_type = type;
    pattern->token_value = value ? strdup(value) : NULL;
    
    if (!pattern->name || (value && !pattern->token_value)) {
        braggi_pattern_destroy(pattern);
        return NULL;
    }
    
    return pattern;
}

// Create a reference pattern
Pattern* braggi_pattern_create_reference(const char* name, const char* reference) {
    if (!name || !reference) return NULL;
    
    Pattern* pattern = malloc(sizeof(Pattern));
    if (!pattern) return NULL;
    
    memset(pattern, 0, sizeof(Pattern));  // Zero out all fields
    
    pattern->type = PATTERN_REFERENCE;
    pattern->name = strdup(name);
    pattern->reference_name = strdup(reference);
    
    if (!pattern->name || !pattern->reference_name) {
        braggi_pattern_destroy(pattern);
        return NULL;
    }
    
    return pattern;
}

// Create a repetition pattern
Pattern* braggi_pattern_create_repetition(const char* name, Pattern* pattern_to_repeat) {
    if (!name || !pattern_to_repeat) return NULL;
    
    Pattern* pattern = malloc(sizeof(Pattern));
    if (!pattern) return NULL;
    
    memset(pattern, 0, sizeof(Pattern));  // Zero out all fields
    
    pattern->type = PATTERN_REPETITION;
    pattern->name = strdup(name);
    
    // Set up sub-patterns array with just one pattern
    pattern->sub_patterns = malloc(sizeof(Pattern*));
    if (!pattern->sub_patterns) {
        braggi_pattern_destroy(pattern);
        return NULL;
    }
    
    pattern->sub_patterns[0] = pattern_to_repeat;
    pattern->sub_pattern_count = 1;
    
    if (!pattern->name) {
        braggi_pattern_destroy(pattern);
        return NULL;
    }
    
    return pattern;
}

// Create a sequence pattern
Pattern* braggi_pattern_create_sequence(const char* name, Pattern** patterns, size_t pattern_count) {
    if (!name || !patterns || pattern_count == 0) return NULL;
    
    Pattern* pattern = malloc(sizeof(Pattern));
    if (!pattern) return NULL;
    
    memset(pattern, 0, sizeof(Pattern));  // Zero out all fields
    
    pattern->type = PATTERN_SEQUENCE;
    pattern->name = strdup(name);
    pattern->sub_patterns = malloc(sizeof(Pattern*) * pattern_count);
    pattern->sub_pattern_count = pattern_count;
    
    if (!pattern->name || !pattern->sub_patterns) {
        braggi_pattern_destroy(pattern);
        return NULL;
    }
    
    // Copy the pattern pointers
    for (size_t i = 0; i < pattern_count; i++) {
        pattern->sub_patterns[i] = patterns[i];
    }
    
    return pattern;
}

// Create a superposition pattern
Pattern* braggi_pattern_create_superposition(const char* name, Pattern** patterns, size_t pattern_count) {
    if (!name || !patterns || pattern_count == 0) return NULL;
    
    Pattern* pattern = malloc(sizeof(Pattern));
    if (!pattern) return NULL;
    
    memset(pattern, 0, sizeof(Pattern));  // Zero out all fields
    
    pattern->type = PATTERN_SUPERPOSITION;
    pattern->name = strdup(name);
    pattern->sub_patterns = malloc(sizeof(Pattern*) * pattern_count);
    pattern->sub_pattern_count = pattern_count;
    
    if (!pattern->name || !pattern->sub_patterns) {
        braggi_pattern_destroy(pattern);
        return NULL;
    }
    
    // Copy the pattern pointers
    for (size_t i = 0; i < pattern_count; i++) {
        pattern->sub_patterns[i] = patterns[i];
    }
    
    return pattern;
}

// Create an optional pattern
Pattern* braggi_pattern_create_optional(const char* name, Pattern* optional_pattern) {
    if (!name || !optional_pattern) return NULL;
    
    Pattern* pattern = malloc(sizeof(Pattern));
    if (!pattern) return NULL;
    
    memset(pattern, 0, sizeof(Pattern));  // Zero out all fields
    
    pattern->type = PATTERN_OPTIONAL;
    pattern->name = strdup(name);
    
    // Set up sub-patterns array with just one pattern
    pattern->sub_patterns = malloc(sizeof(Pattern*));
    if (!pattern->sub_patterns) {
        braggi_pattern_destroy(pattern);
        return NULL;
    }
    
    pattern->sub_patterns[0] = optional_pattern;
    pattern->sub_pattern_count = 1;
    
    if (!pattern->name) {
        braggi_pattern_destroy(pattern);
        return NULL;
    }
    
    return pattern;
}

// Pattern management implementations

// Destroy a pattern
void braggi_pattern_destroy(Pattern* pattern) {
    if (!pattern) return;
    
    // Free the name
    if (pattern->name) {
        free((void*)pattern->name);
    }
    
    // Free the reference name if it's a reference pattern
    if (pattern->type == PATTERN_REFERENCE && pattern->reference_name) {
        free((void*)pattern->reference_name);
    }
    
    // Free the token value if it's a token pattern
    if (pattern->type == PATTERN_TOKEN && pattern->token_value) {
        free((void*)pattern->token_value);
    }
    
    // Free the sub-patterns array (but not the patterns themselves, which may be shared)
    if (pattern->sub_patterns) {
        free(pattern->sub_patterns);
    }
    
    // Finally, free the pattern itself
    free(pattern);
}

// Get the name of a pattern
const char* braggi_pattern_get_name(const Pattern* pattern) {
    return pattern ? pattern->name : NULL;
}

// Check if a pattern matches a token
bool braggi_pattern_matches(const Pattern* pattern, const Token* token) {
    if (!pattern || !token) return false;
    
    // Only token patterns can directly match tokens
    if (pattern->type != PATTERN_TOKEN) return false;
    
    // Check if the token type matches
    if (pattern->token_type != token->type) return false;
    
    // If the pattern specifies a token value, check if it matches
    if (pattern->token_value) {
        return token->text && strcmp(pattern->token_value, token->text) == 0;
    }
    
    // If no specific value is required, any token of the right type matches
    return true;
} 