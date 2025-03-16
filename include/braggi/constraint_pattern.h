/* 
 * Braggi - Constraint Pattern Library
 * 
 * "A good pattern library is like a well-stocked Irish pub at a Texas ranch - 
 * it's got everything you need to handle any situation that comes your way!"
 * - Compiler Philosopher
 */

#ifndef BRAGGI_CONSTRAINT_PATTERN_H
#define BRAGGI_CONSTRAINT_PATTERN_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "braggi/token.h"
#include "braggi/constraint.h"
#include "braggi/util/vector.h"
#include "braggi/pattern.h"  // All core Pattern types and functions are defined here

// Pattern types for grammar (function declarations only - implementations are elsewhere)
Pattern* braggi_pattern_program(void);
Pattern* braggi_pattern_declaration(void);
Pattern* braggi_pattern_region_decl(void);
Pattern* braggi_pattern_regime_decl(void);
Pattern* braggi_pattern_func_decl(void);
Pattern* braggi_pattern_var_decl(void);
Pattern* braggi_pattern_statement(void);
Pattern* braggi_pattern_block(void);
Pattern* braggi_pattern_expression(void);
Pattern* braggi_pattern_return_stmt(void);
Pattern* braggi_pattern_if_stmt(void);
Pattern* braggi_pattern_while_stmt(void);
Pattern* braggi_pattern_for_stmt(void);
Pattern* braggi_pattern_collapse_stmt(void);
Pattern* braggi_pattern_superpose_stmt(void);
Pattern* braggi_pattern_periscope_stmt(void);
Pattern* braggi_pattern_assignment(void);
Pattern* braggi_pattern_type(void);
Pattern* braggi_pattern_parameter_list(void);
Pattern* braggi_pattern_argument_list(void);
Pattern* braggi_pattern_binary_expr(void);
Pattern* braggi_pattern_unary_expr(void);
Pattern* braggi_pattern_primary_expr(void);
Pattern* braggi_pattern_literal(void);

// Main pattern library construction function (defined in grammar_patterns.c)
ConstraintPatternLibrary* braggi_build_language_patterns(void);

#endif /* BRAGGI_CONSTRAINT_PATTERN_H */ 