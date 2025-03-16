/*
 * Braggi - Grammar Patterns
 * 
 * "Grammar patterns are like the Texas two-step - simple to learn, 
 * but mighty impressive when you get good at 'em!"
 * - Parsing Philosopher
 */

#ifndef BRAGGI_GRAMMAR_PATTERNS_H
#define BRAGGI_GRAMMAR_PATTERNS_H

#include <stdint.h>
#include <stdbool.h>
#include "braggi/token.h"
#include "braggi/constraint_pattern.h"  // Include the new header

// Main function to build all language patterns
ConstraintPatternLibrary* braggi_build_language_patterns(void);

// Individual pattern creation functions
Pattern* braggi_pattern_program(void);
Pattern* braggi_pattern_declaration(void);
Pattern* braggi_pattern_region_decl(void);
Pattern* braggi_pattern_regime_decl(void);
Pattern* braggi_pattern_func_decl(void);
Pattern* braggi_pattern_var_decl(void);
Pattern* braggi_pattern_statement(void);
Pattern* braggi_pattern_block(void);
Pattern* braggi_pattern_expression(void);
Pattern* braggi_pattern_assignment(void);
Pattern* braggi_pattern_if_stmt(void);
Pattern* braggi_pattern_while_stmt(void);
Pattern* braggi_pattern_for_stmt(void);
Pattern* braggi_pattern_return_stmt(void);
Pattern* braggi_pattern_collapse_stmt(void);
Pattern* braggi_pattern_superpose_stmt(void);
Pattern* braggi_pattern_periscope_stmt(void);
Pattern* braggi_pattern_type(void);
Pattern* braggi_pattern_parameter_list(void);
Pattern* braggi_pattern_argument_list(void);
Pattern* braggi_pattern_binary_expr(void);
Pattern* braggi_pattern_unary_expr(void);
Pattern* braggi_pattern_primary_expr(void);
Pattern* braggi_pattern_literal(void);

#endif /* BRAGGI_GRAMMAR_PATTERNS_H */ 