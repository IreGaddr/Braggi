/* 
 * Braggi - Constraint System
 * 
 * "Constraints ain't just for rodeo bulls - they're what keep our entropy 
 * from goin' wild like a stallion at a square dance!" - County wisdom
 */

#ifndef BRAGGI_CONSTRAINT_H
#define BRAGGI_CONSTRAINT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>  // Added for size_t

// Include entropy.h to get the core constraint types
// The following types come from entropy.h and should NOT be redefined:
// - EntropyState
// - EntropyConstraint (used instead of Constraint)
// - EntropyConstraintType (used instead of ConstraintType)
// - EntropyCell
// - EntropyField
#include "braggi/entropy.h"
#include "braggi/state.h"
#include "braggi/source_position.h"
#include "braggi/util/vector.h"

// Type alias for readability in this context
typedef EntropyConstraint Constraint;  
typedef EntropyConstraintType ConstraintType;

// Constraint validation function type
typedef bool (*ConstraintValidator)(EntropyState** states, size_t state_count, void* context);

// IMPORTANT: These functions are declared in entropy.h - we're just providing
// additional documentation here, not redefining them

// Check if a constraint is satisfied by a set of states
bool braggi_constraint_check(EntropyConstraint* constraint, EntropyState** states, size_t state_count);

// Built-in constraints
EntropyConstraint* braggi_constraint_syntax_rule(const char* rule_name);
EntropyConstraint* braggi_constraint_region_lifetime(void);
EntropyConstraint* braggi_constraint_regime_compatibility(void);
EntropyConstraint* braggi_constraint_type_check(void);

// Get the affected entropy cells from a field
EntropyCell** braggi_constraint_get_entropy_cells(EntropyConstraint* constraint, EntropyField* field, size_t* count);

// Create a syntax constraint for a grammar rule
EntropyConstraint* braggi_constraint_create_syntax(const char* rule_name,
                                            bool (*validator)(EntropyConstraint*, EntropyField*),
                                            void* context);

// Create a semantic constraint for type checking, etc.
EntropyConstraint* braggi_constraint_create_semantic(const char* rule_name,
                                              bool (*validator)(EntropyConstraint*, EntropyField*),
                                              void* context);

// Create a region safety constraint
EntropyConstraint* braggi_constraint_create_region(const char* rule_name,
                                            bool (*validator)(EntropyConstraint*, EntropyField*),
                                            void* context);

// Create a regime compatibility constraint
EntropyConstraint* braggi_constraint_create_regime(const char* rule_name,
                                            bool (*validator)(EntropyConstraint*, EntropyField*),
                                            void* context);

// Create a periscope validity constraint
EntropyConstraint* braggi_constraint_create_periscope(const char* rule_name,
                                               bool (*validator)(EntropyConstraint*, EntropyField*),
                                               void* context);

// Create a custom constraint
EntropyConstraint* braggi_constraint_create_custom(const char* rule_name,
                                            bool (*validator)(EntropyConstraint*, EntropyField*),
                                            void* context);

// Get a string description of a constraint
char* braggi_constraint_get_description(const EntropyConstraint* constraint);

// Get the type of a constraint as a string
const char* braggi_constraint_type_to_string(EntropyConstraintType type);

#endif /* BRAGGI_CONSTRAINT_H */ 