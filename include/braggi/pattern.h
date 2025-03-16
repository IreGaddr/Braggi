/*
 * Braggi - Pattern Definitions for WFCCC
 * 
 * "A pattern in code is like a pattern on a ranch - once ya see it,
 * you can't unsee it, and it tells ya where things oughta be!" - Irish-Texan Pattern Philosophy
 */

#ifndef BRAGGI_PATTERN_H
#define BRAGGI_PATTERN_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "braggi/token.h"
#include "braggi/entropy.h"  // For EntropyConstraintType
#include "braggi/util/vector.h"

// Forward declaration for recursive structure
typedef struct Pattern Pattern;

/*
 * Pattern types for the Wave Function Collapse Constraint system
 */
typedef enum {
    PATTERN_INVALID = 0,
    PATTERN_TOKEN,          // Exact token match
    PATTERN_SEQUENCE,       // Sequence of patterns
    PATTERN_SUPERPOSITION,  // Choice between patterns
    PATTERN_REPETITION,     // Repeated pattern (0 or more times)
    PATTERN_OPTIONAL,       // Optional pattern (0 or 1 times)
    PATTERN_GROUP,          // Named group of patterns
    PATTERN_REFERENCE,      // Reference to a named pattern
    PATTERN_CONSTRAINT      // Custom constraint function
} PatternType;

/*
 * Pattern structure for the WFCCC system
 */
struct Pattern {
    PatternType type;        /* Type of pattern */
    const char* name;        /* Name of the pattern (for reference) */
    
    /* For composite patterns (sequence, superposition, etc.) */
    Pattern** sub_patterns;  /* Array of sub-patterns */
    size_t sub_pattern_count; /* Number of sub-patterns */
    
    /* For token patterns */
    TokenType token_type;    /* Token type to match */
    const char* token_value; /* Optional specific token value to match */
    
    /* For reference patterns */
    const char* reference_name;    /* Name of referenced pattern */
    
    /* For constraint settings */
    EntropyConstraintType constraint_type;  /* Type of constraint to create */
    float entropy_bias;     /* Bias toward or away from this pattern (1.0 = neutral) */
    
    /* Additional data for pattern matching */
    void* context;           /* Custom context data */
    bool collapsed;          /* Whether this pattern has been collapsed to a specific option */
    uint32_t match_count;    /* Number of times this pattern has been matched */
};

/*
 * Library of patterns that can be referenced by name
 */
typedef struct {
    Pattern** patterns;     /* Array of patterns */
    size_t pattern_count;   /* Number of patterns */
    const char* start_pattern;    /* Name of the starting pattern */
} ConstraintPatternLibrary;

/*
 * Pattern creation and management
 */

// Create a pattern that matches a specific token
Pattern* braggi_pattern_create_token(const char* name, TokenType type, const char* value);

// Create a pattern for a sequence of patterns
Pattern* braggi_pattern_create_sequence(const char* name, Pattern** sub_patterns, size_t count);

// Create a pattern for a choice between patterns
Pattern* braggi_pattern_create_superposition(const char* name, Pattern** sub_patterns, size_t count);

// Create a pattern for a repeated pattern (0 or more times)
Pattern* braggi_pattern_create_repetition(const char* name, Pattern* sub_pattern);

// Create a pattern for an optional pattern (0 or 1 times)
Pattern* braggi_pattern_create_optional(const char* name, Pattern* sub_pattern);

// Create a named group pattern
Pattern* braggi_pattern_create_group(const char* name, Pattern* sub_pattern);

// Create a reference to a named pattern
Pattern* braggi_pattern_create_reference(const char* name, const char* reference_name);

// Create a custom constraint pattern
Pattern* braggi_pattern_create_constraint(const char* name, 
                                          EntropyConstraintType constraint_type,
                                          float entropy_bias);

// Destroy a pattern and free its resources
void braggi_pattern_destroy(Pattern* pattern);

/*
 * Pattern library management
 */

// Create a new constraint pattern library
ConstraintPatternLibrary* braggi_constraint_pattern_library_create(const char* start_pattern);

// Destroy a constraint pattern library
void braggi_constraint_pattern_library_destroy(ConstraintPatternLibrary* library);

// Add a pattern to the library
bool braggi_constraint_pattern_library_add_pattern(ConstraintPatternLibrary* library, Pattern* pattern);

// Get a pattern by name
Pattern* braggi_constraint_pattern_library_get_pattern(ConstraintPatternLibrary* library, const char* name);

// Check if a pattern matches a token
bool braggi_pattern_matches(const Pattern* pattern, const Token* token);

// Get the name of a pattern
const char* braggi_pattern_get_name(const Pattern* pattern);

#endif /* BRAGGI_PATTERN_H */ 