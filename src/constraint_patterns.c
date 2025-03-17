/*
 * Braggi - Constraint Pattern Validators Implementation
 * 
 * "In the quantum world of parsing, the constraints don't just check what's right -
 * they collapse all wrong possibilities until only the correct ones remain!"
 * - Texan quantum computing wisdom
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "braggi/token_propagator.h"
#include "braggi/constraint.h"
#include "braggi/entropy.h"
#include "braggi/periscope.h"
#include "braggi/util/vector.h"

// External function declarations
extern bool braggi_default_adjacency_validator(EntropyConstraint* constraint, EntropyField* field);

// Global periscope reference
static Periscope* g_periscope = NULL;

// Forward declarations for all functions
static uint32_t braggi_entropy_field_get_cell_id_for_token(EntropyField* field, Token* token);
static bool variable_pattern_impl(EntropyField* field, Vector* tokens, void* data);
static bool function_pattern_impl(EntropyField* field, Vector* tokens, void* data);
static bool type_pattern_impl(EntropyField* field, Vector* tokens, void* data);
static bool control_flow_pattern_impl(EntropyField* field, Vector* tokens, void* data);

// Function prototypes - these are the public functions that need to be declared first
bool adjacency_pattern(EntropyField* field, Vector* tokens, void* data);
bool sequence_pattern(EntropyField* field, Vector* tokens, void* data);
bool grammar_pattern(EntropyField* field, Vector* tokens, void* data);
bool variable_pattern(EntropyField* field, Vector* tokens, void* data);
bool function_pattern(EntropyField* field, Vector* tokens, void* data);
bool type_pattern(EntropyField* field, Vector* tokens, void* data);
bool control_flow_pattern(EntropyField* field, Vector* tokens, void* data);
bool braggi_default_adjacency_validator(EntropyConstraint* constraint, EntropyField* field);
bool braggi_default_sequence_validator(EntropyConstraint* constraint, EntropyField* field);
uint32_t braggi_normalize_field_cell_id(EntropyField* field, uint32_t cell_id);

// Function prototypes for internal registration functions
bool register_pattern_type(uint32_t type, const char* name);
bool register_pattern_function(const char* name, bool (*func)(EntropyField*, Vector*, void*), void* data);
bool (*get_pattern_function(const char* name))(EntropyField*, Vector*, void*);

// Pattern function registry
typedef struct {
    const char* name;
    bool (*func)(EntropyField*, Vector*, void*);
    void* data;
} PatternFunctionEntry;

static Vector* pattern_functions = NULL;
static Vector* pattern_types = NULL;

// Pattern type registry entry
typedef struct {
    uint32_t type;
    const char* name;
} PatternTypeEntry;

// Register a pattern type
bool register_pattern_type(uint32_t type, const char* name) {
    if (!pattern_types) return false;
    
    PatternTypeEntry entry = { type, name };
    return braggi_vector_push_back(pattern_types, &entry);
}

// Register a pattern function
bool register_pattern_function(const char* name, bool (*func)(EntropyField*, Vector*, void*), void* data) {
    if (!pattern_functions) return false;
    
    PatternFunctionEntry entry = { name, func, data };
    return braggi_vector_push_back(pattern_functions, &entry);
}

// Get a pattern function by name
bool (*get_pattern_function(const char* name))(EntropyField*, Vector*, void*) {
    if (!pattern_functions) {
        fprintf(stderr, "ERROR: pattern_functions vector is NULL in get_pattern_function\n");
        return NULL;
    }
    
    if (!name) {
        fprintf(stderr, "ERROR: name is NULL in get_pattern_function\n");
        return NULL;
    }
    
    size_t function_count = braggi_vector_size(pattern_functions);
    fprintf(stderr, "DEBUG: Searching for pattern function '%s' among %zu registered functions\n", 
            name, function_count);
    
    for (size_t i = 0; i < function_count; i++) {
        PatternFunctionEntry* entry = braggi_vector_get(pattern_functions, i);
        if (!entry) {
            fprintf(stderr, "WARNING: NULL entry at index %zu in pattern_functions\n", i);
            continue;
        }
        
        if (!entry->name) {
            fprintf(stderr, "WARNING: NULL name in entry at index %zu\n", i);
            continue;
        }
        
        fprintf(stderr, "DEBUG: Comparing '%s' with registered pattern '%s'\n", name, entry->name);
        
        if (strcmp(entry->name, name) == 0) {
            if (!entry->func) {
                fprintf(stderr, "ERROR: Found '%s' but function pointer is NULL\n", name);
                return NULL;
            }
            
            fprintf(stderr, "DEBUG: Found pattern function '%s' at index %zu\n", name, i);
            return entry->func;
        }
    }
    
    fprintf(stderr, "DEBUG: Pattern function '%s' not found\n", name);
    return NULL;
}

// Initialize the constraint patterns system
bool braggi_constraint_patterns_initialize(void) {
    // Safety: if already initialized, just return success
    if (pattern_types && pattern_functions && braggi_vector_size(pattern_functions) > 0) {
        fprintf(stderr, "DEBUG: Constraint patterns system already initialized with %zu functions\n", 
                braggi_vector_size(pattern_functions));
        return true;
    }
    
    fprintf(stderr, "DEBUG: Initializing constraint patterns system from scratch\n");
    
    // Clean up any existing vectors to avoid memory leaks
    if (pattern_types) {
        fprintf(stderr, "DEBUG: Destroying existing pattern_types vector\n");
        braggi_vector_destroy(pattern_types);
        pattern_types = NULL;
    }
    
    if (pattern_functions) {
        fprintf(stderr, "DEBUG: Destroying existing pattern_functions vector\n");
        braggi_vector_destroy(pattern_functions);
        pattern_functions = NULL;
    }
    
    // Create pattern type and function registries
    pattern_types = braggi_vector_create(sizeof(PatternTypeEntry));
    if (!pattern_types) {
        fprintf(stderr, "ERROR: Failed to create pattern_types vector\n");
        return false;
    }
    
    pattern_functions = braggi_vector_create(sizeof(PatternFunctionEntry));
    if (!pattern_functions) {
        fprintf(stderr, "ERROR: Failed to create pattern_functions vector\n");
        braggi_vector_destroy(pattern_types);
        pattern_types = NULL;
        return false;
    }
    
    // Register pattern types with safety checks
    PatternTypeEntry syntax_type = { 1, "Syntax pattern" };
    PatternTypeEntry semantic_type = { 2, "Semantic pattern" };
    PatternTypeEntry grammar_type = { 3, "Grammar pattern" };
    PatternTypeEntry user_type = { 4, "User-defined pattern" };
    
    if (!braggi_vector_push_back(pattern_types, &syntax_type)) {
        fprintf(stderr, "ERROR: Failed to register syntax pattern type\n");
        goto cleanup_and_fail;
    }
    
    if (!braggi_vector_push_back(pattern_types, &semantic_type)) {
        fprintf(stderr, "ERROR: Failed to register semantic pattern type\n");
        goto cleanup_and_fail;
    }
    
    if (!braggi_vector_push_back(pattern_types, &grammar_type)) {
        fprintf(stderr, "ERROR: Failed to register grammar pattern type\n");
        goto cleanup_and_fail;
    }
    
    if (!braggi_vector_push_back(pattern_types, &user_type)) {
        fprintf(stderr, "ERROR: Failed to register user-defined pattern type\n");
        goto cleanup_and_fail;
    }
    
    // Verify functions exist before registering them
    if (!adjacency_pattern) {
        fprintf(stderr, "ERROR: adjacency_pattern function is NULL\n");
        goto cleanup_and_fail;
    }
    
    if (!sequence_pattern) {
        fprintf(stderr, "ERROR: sequence_pattern function is NULL\n");
        goto cleanup_and_fail;
    }
    
    if (!grammar_pattern) {
        fprintf(stderr, "ERROR: grammar_pattern function is NULL\n");
        goto cleanup_and_fail;
    }
    
    fprintf(stderr, "DEBUG: All core pattern functions exist, proceeding with registration\n");
    
    // Register built-in patterns with EXACT names matching what token_propagator.c looks for
    // Each registration has its own safety check
    if (!register_pattern_function("adjacency_pattern", adjacency_pattern, NULL)) {
        fprintf(stderr, "ERROR: Failed to register adjacency_pattern\n");
        goto cleanup_and_fail;
    }
    fprintf(stderr, "DEBUG: Successfully registered adjacency_pattern\n");
    
    if (!register_pattern_function("sequence_pattern", sequence_pattern, NULL)) {
        fprintf(stderr, "ERROR: Failed to register sequence_pattern\n");
        goto cleanup_and_fail;
    }
    fprintf(stderr, "DEBUG: Successfully registered sequence_pattern\n");
    
    if (!register_pattern_function("grammar_pattern", grammar_pattern, NULL)) {
        fprintf(stderr, "ERROR: Failed to register grammar_pattern\n");
        goto cleanup_and_fail;
    }
    fprintf(stderr, "DEBUG: Successfully registered grammar_pattern\n");
    
    // These pattern functions are optional - register them if they exist
    if (variable_pattern && !register_pattern_function("variable_pattern", variable_pattern, NULL)) {
        fprintf(stderr, "WARNING: Failed to register variable_pattern\n");
    }
    
    if (function_pattern && !register_pattern_function("function_pattern", function_pattern, NULL)) {
        fprintf(stderr, "WARNING: Failed to register function_pattern\n");
    }
    
    if (type_pattern && !register_pattern_function("type_pattern", type_pattern, NULL)) {
        fprintf(stderr, "WARNING: Failed to register type_pattern\n");
    }
    
    if (control_flow_pattern && !register_pattern_function("control_flow_pattern", control_flow_pattern, NULL)) {
        fprintf(stderr, "WARNING: Failed to register control_flow_pattern\n");
    }
    
    // Also register with shorter names for backward compatibility - optional
    register_pattern_function("adjacency", adjacency_pattern, NULL);
    register_pattern_function("sequence", sequence_pattern, NULL);
    register_pattern_function("grammar", grammar_pattern, NULL);
    
    if (variable_pattern) register_pattern_function("variables", variable_pattern, NULL);
    if (function_pattern) register_pattern_function("functions", function_pattern, NULL);
    if (type_pattern) register_pattern_function("types", type_pattern, NULL);
    if (control_flow_pattern) register_pattern_function("control_flow", control_flow_pattern, NULL);
    
    // Verify that the registration worked
    size_t count = braggi_vector_size(pattern_functions);
    fprintf(stderr, "DEBUG: Registered %zu pattern functions in total\n", count);
    
    // Test lookup of key functions to ensure they're registered correctly
    if (!get_pattern_function("adjacency_pattern")) {
        fprintf(stderr, "ERROR: adjacency_pattern seems registered but can't be retrieved\n");
        goto cleanup_and_fail;
    }
    
    if (!get_pattern_function("sequence_pattern")) {
        fprintf(stderr, "ERROR: sequence_pattern seems registered but can't be retrieved\n");
        goto cleanup_and_fail;
    }
    
    if (!get_pattern_function("grammar_pattern")) {
        fprintf(stderr, "ERROR: grammar_pattern seems registered but can't be retrieved\n");
        goto cleanup_and_fail;
    }
    
    fprintf(stderr, "DEBUG: Successfully registered all pattern functions and verified retrieval\n");
    return true;

cleanup_and_fail:
    if (pattern_types) {
        braggi_vector_destroy(pattern_types);
        pattern_types = NULL;
    }
    
    if (pattern_functions) {
        braggi_vector_destroy(pattern_functions);
        pattern_functions = NULL;
    }
    
    fprintf(stderr, "ERROR: Failed to initialize constraint patterns system\n");
    return false;
}

// Create a constraint from a pattern
EntropyConstraint* braggi_constraint_from_pattern(const char* pattern_name, EntropyField* field, Vector* cells) {
    if (!pattern_name || !field || !cells) return NULL;
    
    bool (*pattern_func)(EntropyField*, Vector*, void*) = get_pattern_function(pattern_name);
    if (!pattern_func) return NULL;
    
    // Create a constraint using the pattern function
    // This is a simplified implementation
    EntropyConstraint* constraint = braggi_constraint_create(
        CONSTRAINT_SYNTAX,  // Default to syntax constraint
        NULL,               // No validator function
        NULL,               // No context data
        pattern_name        // Use pattern name as description
    );
    
    if (!constraint) return NULL;
    
    // Add all cells to the constraint
    for (size_t i = 0; i < braggi_vector_size(cells); i++) {
        uint32_t* cell_id = braggi_vector_get(cells, i);
        if (cell_id) {
            braggi_constraint_add_cell(constraint, *cell_id);
        }
    }
    
    return constraint;
}

// Helper to check if a token pointer is valid (not a suspicious value)
static bool is_valid_token_pointer(Token* token) {
    if (!token) return false;
    
    uintptr_t token_addr = (uintptr_t)token;
    // Check if the address is suspicious (too small or too large)
    if (token_addr < 0x1000000 || token_addr > 0x7FFFFFFFFFFF) {
        fprintf(stderr, "WARNING: Suspicious token address 0x%lx\n", token_addr);
        return false;
    }
    
    // Try to safely access basic token fields to detect if it's a valid structure
    // This is not foolproof but can catch some cases of invalid pointers
    if (token->type < 0 || token->type > 20) { // Assume valid token types are limited
        fprintf(stderr, "WARNING: Token has invalid type %d at address 0x%lx\n", token->type, token_addr);
        return false;
    }
    
    // Check if position fields look reasonable
    if (token->position.line < 0 || token->position.line > 100000 ||
        token->position.column < 0 || token->position.column > 10000) {
        fprintf(stderr, "WARNING: Token has suspicious position (line %d, column %d) at address 0x%lx\n", 
                token->position.line, token->position.column, token_addr);
        return false;
    }
    
    return true;
}

// Adjacency pattern - creates constraints between adjacent tokens
bool adjacency_pattern(EntropyField* field, Vector* tokens, void* data) {
    // Super cautious parameter validation
    if (!field) {
        fprintf(stderr, "ERROR: NULL field in adjacency_pattern\n");
        return false;
    }
    
    if (!tokens) {
        fprintf(stderr, "ERROR: NULL tokens in adjacency_pattern\n");
        return false;
    }
    
    if (!field->cells || field->cell_count == 0) {
        fprintf(stderr, "ERROR: Field has no cells in adjacency_pattern\n");
        return false;
    }
    
    // Check that the vector is properly initialized and has enough elements
    size_t token_count = braggi_vector_size(tokens);
    if (token_count < 2) {
        fprintf(stderr, "DEBUG: Not enough tokens (%zu) for adjacency pattern\n", token_count);
        return true; // Not an error, just nothing to do
    }
    
    fprintf(stderr, "DEBUG: Processing adjacency pattern with %zu tokens\n", token_count);
    
    // Log vector details for debugging
    fprintf(stderr, "DEBUG: Token vector at %p, elem_size=%zu\n", 
            (void*)tokens, tokens->elem_size);
    
    bool success = true;
    
    // Safer iteration through the tokens
    for (size_t i = 0; i < token_count - 1; i++) {
        fprintf(stderr, "DEBUG: Processing token pair at indices %zu and %zu\n", i, i+1);
        
        // Get pointers with cautious validation and safe casting
        void* current_ptr_void = braggi_vector_get(tokens, i);
        void* next_ptr_void = braggi_vector_get(tokens, i+1);
        
        if (!current_ptr_void) {
            fprintf(stderr, "WARNING: NULL vector entry at index %zu\n", i);
            continue;
        }
        
        if (!next_ptr_void) {
            fprintf(stderr, "WARNING: NULL vector entry at index %zu\n", i+1);
            continue;
        }
        
        // Carefully cast to Token* depending on vector's element size
        Token* current = NULL;
        Token* next = NULL;
        
        // Handle based on vector element size - pointer vs direct token
        if (tokens->elem_size == sizeof(void*) || tokens->elem_size == sizeof(Token*)) {
            // Vector contains pointers to tokens
            current = *(Token**)current_ptr_void;
            next = *(Token**)next_ptr_void;
            fprintf(stderr, "DEBUG: Vector contains pointers to tokens\n");
        } else if (tokens->elem_size == sizeof(Token)) {
            // Vector contains tokens directly
            current = (Token*)current_ptr_void;
            next = (Token*)next_ptr_void;
            fprintf(stderr, "DEBUG: Vector contains tokens directly\n");
        } else {
            fprintf(stderr, "ERROR: Unexpected vector element size %zu, expected %zu or %zu\n",
                    tokens->elem_size, sizeof(void*), sizeof(Token));
            continue;
        }
        
        // Check for NULL pointers
        if (!current) {
            fprintf(stderr, "WARNING: NULL current token at index %zu\n", i);
            continue;
        }
        
        if (!next) {
            fprintf(stderr, "WARNING: NULL next token at index %zu\n", i+1);
            continue;
        }
        
        // Print debug info for these tokens 
        fprintf(stderr, "DEBUG: Processing tokens: current=%p, next=%p\n", 
                (void*)current, (void*)next);
        
        // Validate token pointers using our improved function
        if (!is_valid_token_pointer(current)) {
            fprintf(stderr, "WARNING: Invalid current token at index %zu\n", i);
            continue;
        }
        
        if (!is_valid_token_pointer(next)) {
            fprintf(stderr, "WARNING: Invalid next token at index %zu\n", i+1);
            continue;
        }
        
        // Create adjacency constraint between tokens
        EntropyConstraint* constraint = braggi_constraint_create(
            CONSTRAINT_SYNTAX,  // Adjacency is a syntax constraint
            braggi_default_adjacency_validator, // Use the default adjacency validator
            NULL,               // No context data
            "Adjacency constraint"  // Description
        );
        
        if (!constraint) {
            fprintf(stderr, "ERROR: Failed to create adjacency constraint\n");
            success = false;
            continue;
        }
        
        // Get cell IDs safely - first try periscope if available
        uint32_t current_id = 0;
        uint32_t next_id = 0;
        
        // Get the global periscope if available
        extern Periscope* braggi_constraint_patterns_get_periscope(void);
        Periscope* periscope = braggi_constraint_patterns_get_periscope();
        
        if (periscope) {
            fprintf(stderr, "DEBUG: Using periscope to get cell IDs\n");
            current_id = braggi_periscope_get_cell_id_for_token(periscope, current, field);
            next_id = braggi_periscope_get_cell_id_for_token(periscope, next, field);
        } else {
            // Fallback to direct field lookup
            fprintf(stderr, "DEBUG: No periscope available, using fallback method\n");
            current_id = braggi_entropy_field_get_cell_id_for_token(field, current);
            next_id = braggi_entropy_field_get_cell_id_for_token(field, next);
        }
        
        fprintf(stderr, "DEBUG: Got cell IDs: current_id=%u, next_id=%u\n", current_id, next_id);
        
        // Normalize cell IDs to make sure they're within bounds
        current_id = braggi_normalize_field_cell_id(field, current_id);
        next_id = braggi_normalize_field_cell_id(field, next_id);
        
        fprintf(stderr, "DEBUG: Normalized cell IDs: current_id=%u, next_id=%u\n", 
                current_id, next_id);
        
        // Add cells to the constraint
        braggi_constraint_add_cell(constraint, current_id);
        braggi_constraint_add_cell(constraint, next_id);
        
        // Add constraint to field
        if (!braggi_entropy_field_add_constraint(field, constraint)) {
            fprintf(stderr, "ERROR: Failed to add adjacency constraint to field\n");
            braggi_constraint_destroy(constraint);
            success = false;
        } else {
            fprintf(stderr, "DEBUG: Successfully added constraint between cells %u and %u\n", 
                    current_id, next_id);
        }
    }
    
    fprintf(stderr, "DEBUG: Successfully processed adjacency pattern\n");
    return success;
}

// Sequence pattern - identifies sequences of tokens and creates constraints
bool sequence_pattern(EntropyField* field, Vector* tokens, void* data) {
    // Super cautious parameter validation
    if (!field) {
        fprintf(stderr, "ERROR: NULL field in sequence_pattern\n");
        return false;
    }
    
    if (!tokens) {
        fprintf(stderr, "ERROR: NULL tokens in sequence_pattern\n");
        return false;
    }
    
    if (!field->cells || field->cell_count == 0) {
        fprintf(stderr, "ERROR: Field has no cells in sequence_pattern\n");
        return false;
    }
    
    // Check that the vector is properly initialized and has enough elements
    size_t token_count = braggi_vector_size(tokens);
    if (token_count < 3) {
        fprintf(stderr, "DEBUG: Not enough tokens (%zu) for sequence pattern\n", token_count);
        return true; // Not an error, just nothing to do
    }
    
    fprintf(stderr, "DEBUG: Processing sequence pattern with %zu tokens\n", token_count);
    
    // Log vector details for debugging
    fprintf(stderr, "DEBUG: Token vector at %p, elem_size=%zu\n", 
            (void*)tokens, tokens->elem_size);
    
    bool success = true;
    
    // Safer iteration through the tokens
    for (size_t i = 0; i < token_count - 2; i++) {
        fprintf(stderr, "DEBUG: Processing token triplet at indices %zu, %zu, and %zu\n", 
                i, i+1, i+2);
                
        // Get pointers with cautious validation and safe casting
        void* first_ptr_void = braggi_vector_get(tokens, i);
        void* second_ptr_void = braggi_vector_get(tokens, i+1);
        void* third_ptr_void = braggi_vector_get(tokens, i+2);
        
        if (!first_ptr_void || !second_ptr_void || !third_ptr_void) {
            fprintf(stderr, "WARNING: NULL vector entry in sequence at indices %zu, %zu, or %zu\n", 
                    i, i+1, i+2);
            continue;
        }
        
        // Carefully cast to Token* depending on vector's element size
        Token* first = NULL;
        Token* second = NULL;
        Token* third = NULL;
        
        // Handle based on vector element size - pointer vs direct token
        if (tokens->elem_size == sizeof(void*) || tokens->elem_size == sizeof(Token*)) {
            // Vector contains pointers to tokens
            first = *(Token**)first_ptr_void;
            second = *(Token**)second_ptr_void;
            third = *(Token**)third_ptr_void;
        } else if (tokens->elem_size == sizeof(Token)) {
            // Vector contains tokens directly
            first = (Token*)first_ptr_void;
            second = (Token*)second_ptr_void;
            third = (Token*)third_ptr_void;
        } else {
            fprintf(stderr, "ERROR: Unexpected vector element size %zu, expected %zu or %zu\n",
                    tokens->elem_size, sizeof(void*), sizeof(Token));
            continue;
        }
        
        // Check for NULL pointers
        if (!first || !second || !third) {
            fprintf(stderr, "WARNING: NULL token in sequence at indices %zu, %zu, or %zu\n", 
                    i, i+1, i+2);
            continue;
        }
        
        // Print debug info for these tokens
        fprintf(stderr, "DEBUG: Processing tokens in sequence: first=%p, second=%p, third=%p\n", 
                (void*)first, (void*)second, (void*)third);
        
        // Validate token pointers using our improved function
        if (!is_valid_token_pointer(first)) {
            fprintf(stderr, "WARNING: Invalid first token at index %zu\n", i);
            continue;
        }
        
        if (!is_valid_token_pointer(second)) {
            fprintf(stderr, "WARNING: Invalid second token at index %zu\n", i+1);
            continue;
        }
        
        if (!is_valid_token_pointer(third)) {
            fprintf(stderr, "WARNING: Invalid third token at index %zu\n", i+2);
            continue;
        }
        
        // Get cell IDs safely - first try periscope if available
        uint32_t first_id = 0;
        uint32_t second_id = 0;
        uint32_t third_id = 0;
        
        // Get the global periscope if available
        extern Periscope* braggi_constraint_patterns_get_periscope(void);
        Periscope* periscope = braggi_constraint_patterns_get_periscope();
        
        if (periscope) {
            fprintf(stderr, "DEBUG: Using periscope to get cell IDs\n");
            first_id = braggi_periscope_get_cell_id_for_token(periscope, first, field);
            second_id = braggi_periscope_get_cell_id_for_token(periscope, second, field);
            third_id = braggi_periscope_get_cell_id_for_token(periscope, third, field);
        } else {
            // Fallback to direct field lookup
            fprintf(stderr, "DEBUG: No periscope available, using fallback method\n");
            first_id = braggi_entropy_field_get_cell_id_for_token(field, first);
            second_id = braggi_entropy_field_get_cell_id_for_token(field, second);
            third_id = braggi_entropy_field_get_cell_id_for_token(field, third);
        }
        
        fprintf(stderr, "DEBUG: Got cell IDs: first_id=%u, second_id=%u, third_id=%u\n", 
                first_id, second_id, third_id);
        
        // Normalize cell IDs to make sure they're within bounds
        first_id = braggi_normalize_field_cell_id(field, first_id);
        second_id = braggi_normalize_field_cell_id(field, second_id);
        third_id = braggi_normalize_field_cell_id(field, third_id);
        
        fprintf(stderr, "DEBUG: Normalized cell IDs: first_id=%u, second_id=%u, third_id=%u\n", 
                first_id, second_id, third_id);
        
        // Create sequence constraint
        EntropyConstraint* constraint = braggi_constraint_create(
            CONSTRAINT_SYNTAX,      // Sequence is a syntax constraint
            braggi_default_sequence_validator,  // Use default validator
            NULL,                   // No context data
            "Sequence constraint"   // Description
        );
        
        if (!constraint) {
            fprintf(stderr, "ERROR: Failed to create sequence constraint\n");
            success = false;
            continue;
        }
        
        // Add the cell IDs to the constraint
        braggi_constraint_add_cell(constraint, first_id);
        braggi_constraint_add_cell(constraint, second_id);
        braggi_constraint_add_cell(constraint, third_id);
        
        // Add constraint to field
        if (!braggi_entropy_field_add_constraint(field, constraint)) {
            fprintf(stderr, "ERROR: Failed to add sequence constraint to field\n");
            braggi_constraint_destroy(constraint);
            success = false;
        } else {
            fprintf(stderr, "DEBUG: Successfully added constraint between cells %u, %u, and %u\n", 
                    first_id, second_id, third_id);
        }
    }
    
    fprintf(stderr, "DEBUG: Successfully processed sequence pattern\n");
    return success;
}

// Helper function to check if two tokens form a compound operator
static bool is_compound_operator(const Token* t1, const Token* t2) {
    if (!t1 || !t2 || !t1->text || !t2->text) return false;
    
    // Check for common compound operators
    return (
        // Increment/decrement
        (strcmp(t1->text, "+") == 0 && strcmp(t2->text, "+") == 0) ||
        (strcmp(t1->text, "-") == 0 && strcmp(t2->text, "-") == 0) ||
        
        // Logical operators
        (strcmp(t1->text, "&") == 0 && strcmp(t2->text, "&") == 0) ||
        (strcmp(t1->text, "|") == 0 && strcmp(t2->text, "|") == 0) ||
        
        // Comparison operators
        (strcmp(t1->text, "=") == 0 && strcmp(t2->text, "=") == 0) ||
        (strcmp(t1->text, "<") == 0 && strcmp(t2->text, "=") == 0) ||
        (strcmp(t1->text, ">") == 0 && strcmp(t2->text, "=") == 0) ||
        (strcmp(t1->text, "!") == 0 && strcmp(t2->text, "=") == 0) ||
        
        // Assignment operators
        (strcmp(t1->text, "+") == 0 && strcmp(t2->text, "=") == 0) ||
        (strcmp(t1->text, "-") == 0 && strcmp(t2->text, "=") == 0) ||
        (strcmp(t1->text, "*") == 0 && strcmp(t2->text, "=") == 0) ||
        (strcmp(t1->text, "/") == 0 && strcmp(t2->text, "=") == 0) ||
        (strcmp(t1->text, "%") == 0 && strcmp(t2->text, "=") == 0) ||
        (strcmp(t1->text, "&") == 0 && strcmp(t2->text, "=") == 0) ||
        (strcmp(t1->text, "|") == 0 && strcmp(t2->text, "=") == 0) ||
        (strcmp(t1->text, "^") == 0 && strcmp(t2->text, "=") == 0) ||
        
        // Shift operators
        (strcmp(t1->text, "<") == 0 && strcmp(t2->text, "<") == 0) ||
        (strcmp(t1->text, ">") == 0 && strcmp(t2->text, ">") == 0)
    );
}

// Grammar pattern - enforces grammar rules
bool grammar_pattern(EntropyField* field, Vector* tokens, void* data) {
    // Super cautious parameter validation
    if (!field) {
        fprintf(stderr, "ERROR: NULL field in grammar_pattern\n");
        return false;
    }
    
    if (!tokens) {
        fprintf(stderr, "ERROR: NULL tokens in grammar_pattern\n");
        return false;
    }
    
    if (!field->cells || field->cell_count == 0) {
        fprintf(stderr, "ERROR: Field has no cells in grammar_pattern\n");
        return false;
    }
    
    // Check that the vector is properly initialized and has enough elements
    size_t token_count = braggi_vector_size(tokens);
    if (token_count < 2) {
        fprintf(stderr, "DEBUG: Not enough tokens (%zu) for grammar pattern\n", token_count);
        return true; // Not an error, just nothing to do
    }
    
    fprintf(stderr, "DEBUG: Processing grammar pattern with %zu tokens\n", token_count);
    
    // Log vector details for debugging
    fprintf(stderr, "DEBUG: Token vector at %p, elem_size=%zu\n", 
            (void*)tokens, tokens->elem_size);
    
    bool success = true;
    
    // Iterate through tokens safely
    for (size_t i = 0; i < token_count - 1; i++) {
        fprintf(stderr, "DEBUG: Processing token at index %zu\n", i);
        
        // Get pointers with cautious validation and safe casting
        void* current_ptr_void = braggi_vector_get(tokens, i);
        if (!current_ptr_void) {
            fprintf(stderr, "WARNING: NULL vector entry at index %zu\n", i);
            continue;
        }
        
        // Carefully cast to Token* depending on vector's element size
        Token* token = NULL;
        
        // Handle based on vector element size - pointer vs direct token
        if (tokens->elem_size == sizeof(void*) || tokens->elem_size == sizeof(Token*)) {
            // Vector contains pointers to tokens
            token = *(Token**)current_ptr_void;
            fprintf(stderr, "DEBUG: Vector contains pointers to tokens\n");
        } else if (tokens->elem_size == sizeof(Token)) {
            // Vector contains tokens directly
            token = (Token*)current_ptr_void;
            fprintf(stderr, "DEBUG: Vector contains tokens directly\n");
        } else {
            fprintf(stderr, "ERROR: Unexpected vector element size %zu, expected %zu or %zu\n",
                    tokens->elem_size, sizeof(void*), sizeof(Token));
            continue;
        }
        
        // Check for NULL pointers
        if (!token) {
            fprintf(stderr, "WARNING: NULL token at index %zu\n", i);
            continue;
        }
        
        // Print debug info for this token
        fprintf(stderr, "DEBUG: Processing token in grammar pattern: token=%p\n", (void*)token);
        
        // Validate token pointers using our improved function
        if (!is_valid_token_pointer(token)) {
            fprintf(stderr, "WARNING: Invalid token at index %zu\n", i);
            continue;
        }
        
        // Check if next token is valid
        if (i + 1 < token_count) {
            void* next_ptr_void = braggi_vector_get(tokens, i + 1);
            if (!next_ptr_void) continue;
            
            Token* next_token = NULL;
            if (tokens->elem_size == sizeof(void*) || tokens->elem_size == sizeof(Token*)) {
                next_token = *(Token**)next_ptr_void;
            } else if (tokens->elem_size == sizeof(Token)) {
                next_token = (Token*)next_ptr_void;
            }
            
            if (!next_token || !is_valid_token_pointer(next_token)) continue;
            
            // Check for compound operators
            if (is_compound_operator(token, next_token)) {
                // For compound operators, use stronger constraints
                fprintf(stderr, "DEBUG: Detected compound operator: %s%s\n", 
                        token->text ? token->text : "(null)", 
                        next_token->text ? next_token->text : "(null)");
                
                // Check if tokens are directly adjacent in source
                bool are_adjacent = token->position.offset + strlen(token->text) == next_token->position.offset;
                
                if (are_adjacent) {
                    // Get the global periscope if available for cell IDs
                    extern Periscope* braggi_constraint_patterns_get_periscope(void);
                    Periscope* periscope = braggi_constraint_patterns_get_periscope();
                    
                    uint32_t token_id = 0;
                    uint32_t next_id = 0;
                    
                    if (periscope) {
                        token_id = braggi_periscope_get_cell_id_for_token(periscope, token, field);
                        next_id = braggi_periscope_get_cell_id_for_token(periscope, next_token, field);
                    } else {
                        token_id = braggi_entropy_field_get_cell_id_for_token(field, token);
                        next_id = braggi_entropy_field_get_cell_id_for_token(field, next_token);
                    }
                    
                    // Normalize IDs
                    token_id = braggi_normalize_field_cell_id(field, token_id);
                    next_id = braggi_normalize_field_cell_id(field, next_id);
                    
                    // Create a special grammar constraint for compound operators
                    EntropyConstraint* compound_constraint = braggi_constraint_create(
                        CONSTRAINT_SEMANTIC,  // Semantic constraint type (stronger)
                        braggi_default_adjacency_validator,
                        NULL,
                        "Compound operator grammar constraint"
                    );
                    
                    if (compound_constraint) {
                        braggi_constraint_add_cell(compound_constraint, token_id);
                        braggi_constraint_add_cell(compound_constraint, next_id);
                        
                        // Add to field
                        if (!braggi_entropy_field_add_constraint(field, compound_constraint)) {
                            fprintf(stderr, "ERROR: Failed to add compound operator grammar constraint to field\n");
                            braggi_constraint_destroy(compound_constraint);
                            success = false;
                        } else {
                            fprintf(stderr, "DEBUG: Successfully added compound operator grammar constraint\n");
                            // Skip the next token since we've handled it as part of compound operator
                            i++;
                            continue;
                        }
                    }
                }
            }
        }
        
        // Get cell ID safely - first try periscope if available
        uint32_t token_id = 0;
        
        // Get the global periscope if available
        extern Periscope* braggi_constraint_patterns_get_periscope(void);
        Periscope* periscope = braggi_constraint_patterns_get_periscope();
        
        if (periscope) {
            fprintf(stderr, "DEBUG: Using periscope to get cell ID\n");
            token_id = braggi_periscope_get_cell_id_for_token(periscope, token, field);
        } else {
            // Fallback to direct field lookup
            fprintf(stderr, "DEBUG: No periscope available, using fallback method\n");
            token_id = braggi_entropy_field_get_cell_id_for_token(field, token);
        }
        
        fprintf(stderr, "DEBUG: Got cell ID: token_id=%u\n", token_id);
        
        // Normalize cell ID to make sure it's within bounds
        token_id = braggi_normalize_field_cell_id(field, token_id);
        
        fprintf(stderr, "DEBUG: Normalized cell ID: token_id=%u\n", token_id);
        
        // Look ahead for other tokens to relate to this one
        bool found_relation = false;
        
        // Skip looking ahead if this token is part of a compound operator (it will be handled separately)
        bool skip_look_ahead = false;
        if (i + 1 < token_count) {
            void* next_ptr_void = braggi_vector_get(tokens, i + 1);
            if (next_ptr_void) {
                Token* next_token = NULL;
                if (tokens->elem_size == sizeof(void*) || tokens->elem_size == sizeof(Token*)) {
                    next_token = *(Token**)next_ptr_void;
                } else if (tokens->elem_size == sizeof(Token)) {
                    next_token = (Token*)next_ptr_void;
                }
                
                if (next_token && is_valid_token_pointer(next_token) && 
                    is_compound_operator(token, next_token)) {
                    skip_look_ahead = true;
                }
            }
        }
        
        if (skip_look_ahead) {
            fprintf(stderr, "DEBUG: Skipping look-ahead for token at index %zu (part of compound operator)\n", i);
            continue;
        }
        
        for (size_t j = i + 1; j < token_count && j < i + 5; j++) { // Look ahead up to 4 tokens
            fprintf(stderr, "DEBUG: Looking at potential related token at index %zu\n", j);
            
            // Get pointers safely
            void* next_ptr_void = braggi_vector_get(tokens, j);
            if (!next_ptr_void) {
                fprintf(stderr, "WARNING: NULL vector entry at index %zu\n", j);
                continue;
            }
            
            // Carefully cast to Token* depending on vector's element size
            Token* next = NULL;
            
            // Handle based on vector element size - pointer vs direct token
            if (tokens->elem_size == sizeof(void*) || tokens->elem_size == sizeof(Token*)) {
                // Vector contains pointers to tokens
                next = *(Token**)next_ptr_void;
            } else if (tokens->elem_size == sizeof(Token)) {
                // Vector contains tokens directly
                next = (Token*)next_ptr_void;
            } else {
                // Already logged this error above, so just skip
                continue;
            }
            
            // Check for NULL pointers
            if (!next) {
                fprintf(stderr, "WARNING: NULL next token at index %zu\n", j);
                continue;
            }
            
            // Print debug info for this related token
            fprintf(stderr, "DEBUG: Checking relation with token: next=%p\n", (void*)next);
            
            // Validate token pointers using our improved function
            if (!is_valid_token_pointer(next)) {
                fprintf(stderr, "WARNING: Invalid related token at index %zu\n", j);
                continue;
            }
            
            // Check if this would create a compound operator with a later token, if so skip it
            bool is_part_of_compound = false;
            if (j + 1 < token_count) {
                void* next_next_ptr_void = braggi_vector_get(tokens, j + 1);
                if (next_next_ptr_void) {
                    Token* next_next = NULL;
                    if (tokens->elem_size == sizeof(void*) || tokens->elem_size == sizeof(Token*)) {
                        next_next = *(Token**)next_next_ptr_void;
                    } else if (tokens->elem_size == sizeof(Token)) {
                        next_next = (Token*)next_next_ptr_void;
                    }
                    
                    if (next_next && is_valid_token_pointer(next_next) && 
                        is_compound_operator(next, next_next)) {
                        is_part_of_compound = true;
                    }
                }
            }
            
            if (is_part_of_compound) {
                fprintf(stderr, "DEBUG: Skipping token at index %zu (part of compound operator)\n", j);
                continue;
            }
            
            // Get next token's cell ID - first try periscope if available
            uint32_t next_id = 0;
            
            if (periscope) {
                fprintf(stderr, "DEBUG: Using periscope to get cell ID for related token\n");
                next_id = braggi_periscope_get_cell_id_for_token(periscope, next, field);
            } else {
                // Fallback to direct field lookup
                fprintf(stderr, "DEBUG: No periscope available, using fallback method\n");
                next_id = braggi_entropy_field_get_cell_id_for_token(field, next);
            }
            
            fprintf(stderr, "DEBUG: Got related token cell ID: next_id=%u\n", next_id);
            
            // Normalize cell ID to make sure it's within bounds
            next_id = braggi_normalize_field_cell_id(field, next_id);
            
            fprintf(stderr, "DEBUG: Normalized related token cell ID: next_id=%u\n", next_id);
            
            // Create grammar constraint between these tokens
            EntropyConstraint* constraint = braggi_constraint_create(
                CONSTRAINT_SYNTAX,         // Grammar is a syntax constraint
                braggi_default_adjacency_validator,  // Use default validator
                NULL,                     // No context data
                "Grammar constraint"      // Description
            );
            
            if (!constraint) {
                fprintf(stderr, "ERROR: Failed to create grammar constraint\n");
                success = false;
                continue;
            }
            
            // Add the cell IDs to the constraint
            braggi_constraint_add_cell(constraint, token_id);
            braggi_constraint_add_cell(constraint, next_id);
            
            // Add constraint to field
            if (!braggi_entropy_field_add_constraint(field, constraint)) {
                fprintf(stderr, "ERROR: Failed to add grammar constraint to field\n");
                braggi_constraint_destroy(constraint);
                success = false;
            } else {
                fprintf(stderr, "DEBUG: Successfully added grammar constraint between cells %u and %u\n", 
                        token_id, next_id);
                found_relation = true;
                break; // Successfully created one constraint, move on
            }
        }
        
        if (!found_relation) {
            fprintf(stderr, "DEBUG: No grammar relation found for token at index %zu\n", i);
        }
    }
    
    fprintf(stderr, "DEBUG: Successfully processed grammar pattern\n");
    return success;
}

// Variable pattern - enforces variable declaration and usage constraints
// This is the public interface to the more detailed implementation
bool variable_pattern(EntropyField* field, Vector* tokens, void* data) {
    return variable_pattern_impl(field, tokens, data);
}

// Function pattern - enforces function declaration and call constraints
// This is the public interface to the more detailed implementation
bool function_pattern(EntropyField* field, Vector* tokens, void* data) {
    return function_pattern_impl(field, tokens, data);
}

// Type pattern - enforces type constraints
// This is the public interface to the more detailed implementation
bool type_pattern(EntropyField* field, Vector* tokens, void* data) {
    return type_pattern_impl(field, tokens, data);
}

// Control flow pattern - enforces control flow constraints
// This is the public interface to the more detailed implementation
bool control_flow_pattern(EntropyField* field, Vector* tokens, void* data) {
    return control_flow_pattern_impl(field, tokens, data);
}

// Implementation of variable_pattern (simplified version)
static bool variable_pattern_impl(EntropyField* field, Vector* tokens, void* data) {
    // Super cautious parameter validation
    if (!field) {
        fprintf(stderr, "ERROR: NULL field in variable_pattern\n");
        return false;
    }
    
    if (!tokens) {
        fprintf(stderr, "ERROR: NULL tokens in variable_pattern\n");
        return false;
    }
    
    if (!field->cells || field->cell_count == 0) {
        fprintf(stderr, "ERROR: Field has no cells in variable_pattern\n");
        return false;
    }
    
    size_t token_count = braggi_vector_size(tokens);
    if (token_count == 0) {
        fprintf(stderr, "DEBUG: No tokens provided for variable pattern\n");
        return true; // Not an error, just nothing to do
    }
    
    bool success = true;
    bool found_variables = false;
    
    // Track variable declarations and usage
    for (size_t i = 0; i < token_count; i++) {
        // Get token pointer safely
        Token** token_ptr = (Token**)braggi_vector_get(tokens, i);
        if (!token_ptr) {
            fprintf(stderr, "WARNING: Invalid vector entry at index %zu\n", i);
            continue;
        }
        
        Token* token = *token_ptr;
        
        // Use our improved token validation
        if (!is_valid_token_pointer(token)) {
            fprintf(stderr, "WARNING: Invalid token at index %zu\n", i);
            continue;
        }
        
        // Get cell ID safely
        uint32_t token_id = braggi_entropy_field_get_cell_id_for_token(field, token);
        
        // A sophisticated implementation would identify variable declarations and usages
        // and create constraints between them. For this simplified version, we'll just
        // check if the token might be a variable (based on type) and log it.
        
        // For demonstration, assume token type 3 is variables (adjust based on actual type values)
        if (token->type == 3) {
            found_variables = true;
            fprintf(stderr, "DEBUG: Found potential variable token at index %zu, cell ID %u\n", 
                    i, token_id);
            
            // In a real implementation, we'd create constraints here
        }
    }
    
    if (!found_variables) {
        fprintf(stderr, "DEBUG: No variable tokens identified in token stream\n");
    } else {
        fprintf(stderr, "DEBUG: Successfully processed variable pattern\n");
    }
    
    return success;
}

// Implementation of function_pattern (simplified version)
static bool function_pattern_impl(EntropyField* field, Vector* tokens, void* data) {
    // Super cautious parameter validation
    if (!field) {
        fprintf(stderr, "ERROR: NULL field in function_pattern\n");
        return false;
    }
    
    if (!tokens) {
        fprintf(stderr, "ERROR: NULL tokens in function_pattern\n");
        return false;
    }
    
    if (!field->cells || field->cell_count == 0) {
        fprintf(stderr, "ERROR: Field has no cells in function_pattern\n");
        return false;
    }
    
    size_t token_count = braggi_vector_size(tokens);
    if (token_count == 0) {
        fprintf(stderr, "DEBUG: No tokens provided for function pattern\n");
        return true; // Not an error, just nothing to do
    }
    
    bool success = true;
    bool found_functions = false;
    
    // Track function declarations and calls
    for (size_t i = 0; i < token_count; i++) {
        // Get token pointer safely
        Token** token_ptr = (Token**)braggi_vector_get(tokens, i);
        if (!token_ptr) {
            fprintf(stderr, "WARNING: Invalid vector entry at index %zu\n", i);
            continue;
        }
        
        Token* token = *token_ptr;
        
        // Use our improved token validation
        if (!is_valid_token_pointer(token)) {
            fprintf(stderr, "WARNING: Invalid token at index %zu\n", i);
            continue;
        }
        
        // Get cell ID safely
        uint32_t token_id = braggi_entropy_field_get_cell_id_for_token(field, token);
        
        // A sophisticated implementation would identify function declarations and calls
        // and create constraints between them. For this simplified version, we'll just
        // check if the token might be a function (based on type) and log it.
        
        // For demonstration, assume token type 4 is functions (adjust based on actual type values)
        if (token->type == 4) {
            found_functions = true;
            fprintf(stderr, "DEBUG: Found potential function token at index %zu, cell ID %u\n", 
                    i, token_id);
            
            // In a real implementation, we'd create constraints here connecting
            // function declarations with their calls
        }
    }
    
    if (!found_functions) {
        fprintf(stderr, "DEBUG: No function tokens identified in token stream\n");
    } else {
        fprintf(stderr, "DEBUG: Successfully processed function pattern\n");
    }
    
    return success;
}

// Implementation of type_pattern (simplified version)
static bool type_pattern_impl(EntropyField* field, Vector* tokens, void* data) {
    // Super cautious parameter validation
    if (!field) {
        fprintf(stderr, "ERROR: NULL field in type_pattern\n");
        return false;
    }
    
    if (!tokens) {
        fprintf(stderr, "ERROR: NULL tokens in type_pattern\n");
        return false;
    }
    
    if (!field->cells || field->cell_count == 0) {
        fprintf(stderr, "ERROR: Field has no cells in type_pattern\n");
        return false;
    }
    
    size_t token_count = braggi_vector_size(tokens);
    if (token_count == 0) {
        fprintf(stderr, "DEBUG: No tokens provided for type pattern\n");
        return true; // Not an error, just nothing to do
    }
    
    bool success = true;
    bool found_types = false;
    
    // Track type declarations and usage
    for (size_t i = 0; i < token_count; i++) {
        // Get token pointer safely
        Token** token_ptr = (Token**)braggi_vector_get(tokens, i);
        if (!token_ptr) {
            fprintf(stderr, "WARNING: Invalid vector entry at index %zu\n", i);
            continue;
        }
        
        Token* token = *token_ptr;
        
        // Use our improved token validation
        if (!is_valid_token_pointer(token)) {
            fprintf(stderr, "WARNING: Invalid token at index %zu\n", i);
            continue;
        }
        
        // Get cell ID safely
        uint32_t token_id = braggi_entropy_field_get_cell_id_for_token(field, token);
        
        // A sophisticated implementation would identify type declarations and usages
        // and create constraints between them. For this simplified version, we'll just
        // check if the token might be a type (based on token type) and log it.
        
        // For demonstration, assume token type 5 is types (adjust based on actual type values)
        if (token->type == 5) {
            found_types = true;
            fprintf(stderr, "DEBUG: Found potential type token at index %zu, cell ID %u\n", 
                    i, token_id);
            
            // In a real implementation, we'd create constraints here
            // connecting type declarations with their usage
        }
    }
    
    if (!found_types) {
        fprintf(stderr, "DEBUG: No type tokens identified in token stream\n");
    } else {
        fprintf(stderr, "DEBUG: Successfully processed type pattern\n");
    }
    
    return success;
}

// Implementation of control_flow_pattern (simplified version)
static bool control_flow_pattern_impl(EntropyField* field, Vector* tokens, void* data) {
    // Super cautious parameter validation
    if (!field) {
        fprintf(stderr, "ERROR: NULL field in control_flow_pattern\n");
        return false;
    }
    
    if (!tokens) {
        fprintf(stderr, "ERROR: NULL tokens in control_flow_pattern\n");
        return false;
    }
    
    if (!field->cells || field->cell_count == 0) {
        fprintf(stderr, "ERROR: Field has no cells in control_flow_pattern\n");
        return false;
    }
    
    size_t token_count = braggi_vector_size(tokens);
    if (token_count == 0) {
        fprintf(stderr, "DEBUG: No tokens provided for control flow pattern\n");
        return true; // Not an error, just nothing to do
    }
    
    bool success = true;
    bool found_control_flow = false;
    
    // Track control flow constructs
    for (size_t i = 0; i < token_count; i++) {
        // Get token pointer safely
        Token** token_ptr = (Token**)braggi_vector_get(tokens, i);
        if (!token_ptr) {
            fprintf(stderr, "WARNING: Invalid vector entry at index %zu\n", i);
            continue;
        }
        
        Token* token = *token_ptr;
        
        // Use our improved token validation
        if (!is_valid_token_pointer(token)) {
            fprintf(stderr, "WARNING: Invalid token at index %zu\n", i);
            continue;
        }
        
        // Get cell ID safely
        uint32_t token_id = braggi_entropy_field_get_cell_id_for_token(field, token);
        
        // A sophisticated implementation would identify control flow tokens (if, else, for, while, etc.)
        // and create constraints for their related blocks. For this simplified version, we'll just
        // check if the token might be a control flow token (based on type) and log it.
        
        // For demonstration, assume token type 6 is control flow (adjust based on actual type values)
        if (token->type == 6) {
            found_control_flow = true;
            fprintf(stderr, "DEBUG: Found potential control flow token at index %zu, cell ID %u\n", 
                    i, token_id);
            
            // In a real implementation, we'd create constraints here
            // connecting control flow constructs with their blocks
            
            // For example, for an 'if' token, we'd want to create constraints with
            // its condition expression and both the 'then' and optional 'else' blocks
        }
    }
    
    if (!found_control_flow) {
        fprintf(stderr, "DEBUG: No control flow tokens identified in token stream\n");
    } else {
        fprintf(stderr, "DEBUG: Successfully processed control flow pattern\n");
    }
    
    return success;
}

// Function to get cell ID for a token, leveraging periscope if available
uint32_t braggi_entropy_field_get_cell_id_for_token(EntropyField* field, Token* token) {
    if (!field || !token) {
        fprintf(stderr, "ERROR: Invalid parameters for get_cell_id_for_token\n");
        return 0;
    }
    
    // Check if the token pointer looks valid (not a small/suspicious value)
    uintptr_t token_addr = (uintptr_t)token;
    if (token_addr < 0x1000000 || token_addr > 0x7FFFFFFFFFFF) {
        fprintf(stderr, "WARNING: Suspicious token address 0x%lx, using fallback cell ID 0\n", token_addr);
        return 0;
    }
    
    if (g_periscope) {
        uint32_t cell_id = braggi_periscope_get_cell_id_for_token(g_periscope, token, field);
        if (cell_id != UINT32_MAX) {
            return cell_id;
        }
        // If periscope doesn't have this token, fall back to other methods
    }
    
    // Fallback: use the token's line number as the cell ID
    // This assumes that cells are mapped to lines in a 1:1 fashion
    if (token->position.line > 0) {
        uint32_t cell_id = token->position.line - 1; // Convert 1-based to 0-based
        
        // Make sure it's within the valid range
        if (cell_id < field->cell_count) {
            return cell_id;
        }
    }
    
    // Last resort: just return the first cell
    fprintf(stderr, "WARNING: Using fallback cell ID 0 for token\n");
    return 0;
}

// Set the global periscope for constraint patterns
bool braggi_constraint_patterns_set_periscope(Periscope* periscope) {
    // Store the old periscope for logging
    Periscope* old_periscope = g_periscope;
    
    // Allow NULL periscope during cleanup
    if (!periscope) {
        fprintf(stderr, "DEBUG: Setting constraint patterns periscope to NULL (cleanup phase)\n");
        g_periscope = NULL;
        return true;
    }
    
    // Set the new periscope
    g_periscope = periscope;
    
    if (old_periscope != g_periscope) {
        fprintf(stderr, "DEBUG: Constraint patterns periscope updated from %p to %p\n", 
                (void*)old_periscope, (void*)g_periscope);
    } else {
        fprintf(stderr, "DEBUG: Constraint patterns periscope reaffirmed to %p\n", (void*)g_periscope);
    }
    
    return true;
}

// Get the global periscope
Periscope* braggi_constraint_patterns_get_periscope(void) {
    if (!g_periscope) {
        fprintf(stderr, "WARNING: Requested constraint patterns periscope is NULL\n");
    }
    return g_periscope;
}

// Forward declarations for validator functions
bool braggi_default_adjacency_validator(EntropyConstraint* constraint, EntropyField* field);
bool braggi_default_sequence_validator(EntropyConstraint* constraint, EntropyField* field);

// Helper function to ensure cell IDs are within bounds for the field
uint32_t braggi_normalize_field_cell_id(EntropyField* field, uint32_t cell_id) {
    if (!field) {
        fprintf(stderr, "ERROR: NULL field in normalize_field_cell_id\n");
        return 0;
    }
    
    // Immediately return if field has no cells
    if (!field->cells || field->cell_count == 0) {
        fprintf(stderr, "ERROR: Field has no cells in normalize_field_cell_id\n");
        return 0;
    }
    
    uint32_t max_cell_id = field->cell_count - 1;
    
    // Cell ID is already valid, no need to normalize
    if (cell_id <= max_cell_id) {
        return cell_id;
    }
    
    // Cell ID is out of bounds - normalize it
    uint32_t normalized_id;
    
    // If the cell ID is way out of bounds (more than 2x max), 
    // it's likely a completely invalid ID - use a more restrictive modulo
    if (cell_id > max_cell_id * 2) {
        // If max_cell_id is 0, we'd get division by zero, so handle that case
        if (max_cell_id == 0) {
            fprintf(stderr, "WARNING: Max cell ID is 0, normalizing to 0\n");
            return 0;
        }
        
        // Use modulo to get a value in range [0, max]
        normalized_id = cell_id % (max_cell_id + 1);
        fprintf(stderr, "WARNING: Cell ID %u is significantly out of bounds (max: %u), using modulo to get %u\n", 
                cell_id, max_cell_id, normalized_id);
    } else {
        // For values that are just a little out of bounds, clamp to max
        normalized_id = max_cell_id;
        fprintf(stderr, "WARNING: Cell ID %u is out of bounds (max: %u), clamping to valid range\n", 
                cell_id, max_cell_id);
    }
    
    // Final sanity check - make absolutely sure we're within bounds
    if (normalized_id > max_cell_id) {
        fprintf(stderr, "ERROR: Normalization failed! Forced to use cell ID 0\n");
        return 0;
    }
    
    return normalized_id;
}

/*
 * Default validator function for adjacency constraints
 * 
 * "When tokens are neighbors, they need to get along like two ranchers 
 * sharing a fence line - respecting each other's boundaries but still connected!"
 */
bool braggi_default_adjacency_validator(EntropyConstraint* constraint, EntropyField* field) {
    // Basic validation checks
    if (!constraint || !field) {
        fprintf(stderr, "Warning: NULL constraint or field in adjacency validator\n");
        return false;
    }
    
    // Check if we have at least two cells in the constraint
    if (constraint->cell_count < 2) {
        fprintf(stderr, "Warning: Adjacency constraint needs at least 2 cells, got %zu\n", 
               constraint->cell_count);
        return true; // Return true to avoid blocking propagation
    }
    
    // Get the cells from the constraint
    uint32_t cell1_id = constraint->cell_ids[0];
    uint32_t cell2_id = constraint->cell_ids[1];
    
    // Validate cell IDs before access
    if (cell1_id >= field->cell_count || cell2_id >= field->cell_count) {
        fprintf(stderr, "Warning: Cell ID out of bounds in adjacency constraint (%u or %u >= %zu)\n", 
                cell1_id, cell2_id, field->cell_count);
        return true; // Return true to avoid blocking propagation
    }
    
    // Get the cells from the field
    EntropyCell* cell1 = braggi_entropy_field_get_cell(field, cell1_id);
    EntropyCell* cell2 = braggi_entropy_field_get_cell(field, cell2_id);
    
    if (!cell1 || !cell2) {
        fprintf(stderr, "Warning: Could not get cells for adjacency constraint\n");
        return true; // Return true to avoid blocking propagation
    }
    
    // Validate state arrays
    if (!cell1->states || !cell2->states) {
        fprintf(stderr, "Warning: Null state array in cell for adjacency constraint\n");
        return true; // Return true to avoid blocking propagation  
    }
    
    // Track compatibility statistics for better debugging
    size_t compatible_pairs = 0;
    size_t total_valid_pairs = 0;
    size_t eliminated_states = 0;
    
    // For each possible state in the first cell, check if it's compatible with 
    // the states in the second cell
    for (size_t i = 0; i < cell1->state_count; i++) {
        EntropyState* state1 = cell1->states[i];
        if (!state1 || braggi_state_is_eliminated(state1)) continue;
        
        Token* token1 = (Token*)state1->data;
        if (!token1) continue;
        
        bool has_compatible_next = false;
        
        // Check all states in the second cell
        for (size_t j = 0; j < cell2->state_count; j++) {
            EntropyState* state2 = cell2->states[j];
            if (!state2 || braggi_state_is_eliminated(state2)) continue;
            
            Token* token2 = (Token*)state2->data;
            if (!token2) continue;
            
            total_valid_pairs++;
            
            // More lenient adjacency validation for syntax
            // For example, tokens can have larger gaps, especially for tokens like ";" or "}"
            
            // Special cases for common tokens that can have bigger gaps
            bool is_special_token1 = (token1->text && (
                strcmp(token1->text, ";") == 0 || 
                strcmp(token1->text, "}") == 0 || 
                strcmp(token1->text, "{") == 0 ||
                strcmp(token1->text, ")") == 0));
                
            bool is_special_token2 = (token2->text && (
                strcmp(token2->text, ";") == 0 || 
                strcmp(token2->text, "}") == 0 || 
                strcmp(token2->text, "{") == 0 ||
                strcmp(token2->text, ")") == 0));
            
            // Increased MAX_GAP to handle whitespace and comments better
            const size_t NORMAL_MAX_GAP = 200;  // Regular tokens can be this far apart
            const size_t SPECIAL_MAX_GAP = 500; // Special tokens can be farther apart
            
            size_t token1_length = token1->text ? strlen(token1->text) : 0;
            
            // Check position compatibility with larger tolerance for certain tokens
            if (token1->position.offset + token1_length <= token2->position.offset) {
                size_t gap = token2->position.offset - (token1->position.offset + token1_length);
                size_t max_allowed_gap = (is_special_token1 || is_special_token2) ? 
                                        SPECIAL_MAX_GAP : NORMAL_MAX_GAP;
                
                if (gap <= max_allowed_gap) {
                    // These tokens can be adjacent
                    has_compatible_next = true;
                    compatible_pairs++;
                    break;
                }
            }
        }
        
        // If this state has no compatible next state, log it but be lenient
        if (!has_compatible_next) {
            // Log the issue but don't eliminate the state in all cases
            fprintf(stderr, "Debug: Token '%s' at position %d:%d has no compatible next token\n", 
                   token1->text ? token1->text : "(null)", 
                   token1->position.line, token1->position.column);
            
            // Only eliminate if not at end of file
            // Tokens at the end of a file might legitimately have no "next" token
            bool is_last_token = true;
            for (size_t j = 0; j < cell2->state_count; j++) {
                EntropyState* state2 = cell2->states[j];
                if (!state2 || braggi_state_is_eliminated(state2)) continue;
                
                Token* token2 = (Token*)state2->data;
                if (!token2) continue;
                
                // If any token2 has a higher line number, token1 is not the last token
                if (token2->position.line > token1->position.line) {
                    is_last_token = false;
                    break;
                }
            }
            
            // Only eliminate non-last tokens
            if (!is_last_token) {
                braggi_state_set_eliminated(state1, true);
                eliminated_states++;
            }
        }
    }
    
    // Log useful statistics
    fprintf(stderr, "Debug: Adjacency constraint: %zu compatible pairs out of %zu valid pairs, eliminated %zu states\n",
            compatible_pairs, total_valid_pairs, eliminated_states);
    
    // If we have any compatible pairs, the constraint is satisfied
    return compatible_pairs > 0 || total_valid_pairs == 0;
}

/*
 * Default validator function for sequence constraints
 * 
 * "A sequence of tokens should be as harmonious as a bluegrass band - 
 * each player knows when to come in, and they all follow the same tune!"
 */
bool braggi_default_sequence_validator(EntropyConstraint* constraint, EntropyField* field) {
    // Basic validation checks
    if (!constraint || !field) {
        fprintf(stderr, "Warning: NULL constraint or field in sequence validator\n");
        return false;
    }
    
    // Check if we have enough cells in the constraint
    if (constraint->cell_count < 3) {
        fprintf(stderr, "Warning: Sequence constraint needs at least 3 cells, got %zu\n", 
               constraint->cell_count);
        return true; // Return true to avoid blocking propagation
    }
    
    // In a sequence, all three tokens should appear in order in the source
    // Get the cells for each position in the sequence
    uint32_t first_cell_id = constraint->cell_ids[0];
    uint32_t middle_cell_id = constraint->cell_ids[1];
    uint32_t last_cell_id = constraint->cell_ids[2];
    
    // Validate cell IDs
    if (first_cell_id >= field->cell_count || 
        middle_cell_id >= field->cell_count || 
        last_cell_id >= field->cell_count) {
        fprintf(stderr, "Warning: Cell ID out of bounds in sequence constraint\n");
        return true; // Return true to avoid blocking propagation
    }
    
    // Get the cells from the field
    EntropyCell* first_cell = braggi_entropy_field_get_cell(field, first_cell_id);
    EntropyCell* middle_cell = braggi_entropy_field_get_cell(field, middle_cell_id);
    EntropyCell* last_cell = braggi_entropy_field_get_cell(field, last_cell_id);
    
    if (!first_cell || !middle_cell || !last_cell) {
        fprintf(stderr, "Warning: Could not get cells for sequence constraint\n");
        return true; // Return true to avoid blocking propagation
    }
    
    // For collapsed cells, verify ordering
    if (braggi_entropy_cell_is_collapsed(first_cell) && 
        braggi_entropy_cell_is_collapsed(middle_cell) && 
        braggi_entropy_cell_is_collapsed(last_cell)) {
        
        EntropyState* first_state = braggi_entropy_cell_get_collapsed_state(first_cell);
        EntropyState* middle_state = braggi_entropy_cell_get_collapsed_state(middle_cell);
        EntropyState* last_state = braggi_entropy_cell_get_collapsed_state(last_cell);
        
        if (!first_state || !middle_state || !last_state) {
            fprintf(stderr, "Warning: Could not get collapsed states for sequence constraint\n");
            return true; // Return true to avoid blocking propagation
        }
        
        Token* first_token = (Token*)first_state->data;
        Token* middle_token = (Token*)middle_state->data;
        Token* last_token = (Token*)last_state->data;
        
        if (!first_token || !middle_token || !last_token) {
            fprintf(stderr, "Warning: Could not get tokens from states for sequence constraint\n");
            return true; // Return true to avoid blocking propagation
        }
        
        // Check if the tokens are in the correct order
        if (first_token->position.offset >= middle_token->position.offset ||
            middle_token->position.offset >= last_token->position.offset) {
            fprintf(stderr, "Warning: Tokens are out of order in sequence constraint\n");
            fprintf(stderr, "  First token: %s at position %d:%d (offset %u)\n",
                    first_token->text ? first_token->text : "(null)",
                    first_token->position.line, first_token->position.column,
                    first_token->position.offset);
            fprintf(stderr, "  Middle token: %s at position %d:%d (offset %u)\n",
                    middle_token->text ? middle_token->text : "(null)",
                    middle_token->position.line, middle_token->position.column,
                    middle_token->position.offset);
            fprintf(stderr, "  Last token: %s at position %d:%d (offset %u)\n",
                    last_token->text ? last_token->text : "(null)",
                    last_token->position.line, last_token->position.column,
                    last_token->position.offset);
            
            // Be more lenient - only enforce line-based ordering, not absolute offset
            if (first_token->position.line > middle_token->position.line ||
                middle_token->position.line > last_token->position.line) {
                // This is a serious error - lines are out of order
                return false;
            }
            else {
                // Tokens are on the same line or neighboring lines - assume this is ok
                fprintf(stderr, "  Tokens are on same or neighboring lines - accepting sequence\n");
            }
        }
    }
    
    // For uncollapsed cells, check that there are valid combinations that preserve order
    bool found_valid_sequence = false;
    
    // Check each possible combination of states
    for (size_t i = 0; i < first_cell->state_count && !found_valid_sequence; i++) {
        EntropyState* first_state = first_cell->states[i];
        if (!first_state || braggi_state_is_eliminated(first_state)) continue;
        
        Token* first_token = (Token*)first_state->data;
        if (!first_token) continue;
        
        for (size_t j = 0; j < middle_cell->state_count && !found_valid_sequence; j++) {
            EntropyState* middle_state = middle_cell->states[j];
            if (!middle_state || braggi_state_is_eliminated(middle_state)) continue;
            
            Token* middle_token = (Token*)middle_state->data;
            if (!middle_token) continue;
            
            // Check if first and middle tokens are in order
            if (first_token->position.line > middle_token->position.line) continue;
            if (first_token->position.line == middle_token->position.line && 
                first_token->position.offset >= middle_token->position.offset) continue;
            
            for (size_t k = 0; k < last_cell->state_count; k++) {
                EntropyState* last_state = last_cell->states[k];
                if (!last_state || braggi_state_is_eliminated(last_state)) continue;
                
                Token* last_token = (Token*)last_state->data;
                if (!last_token) continue;
                
                // Check if middle and last tokens are in order
                if (middle_token->position.line > last_token->position.line) continue;
                if (middle_token->position.line == last_token->position.line && 
                    middle_token->position.offset >= last_token->position.offset) continue;
                
                // Found a valid sequence!
                found_valid_sequence = true;
                break;
            }
        }
    }
    
    // Be lenient - if no valid sequence, log warning but don't fail
    if (!found_valid_sequence && !braggi_entropy_cell_is_collapsed(first_cell) && 
        !braggi_entropy_cell_is_collapsed(middle_cell) && 
        !braggi_entropy_cell_is_collapsed(last_cell)) {
        fprintf(stderr, "Warning: No valid sequence found, but cells are not collapsed\n");
        // Don't return false - be lenient
    }
    
    return true;
} 