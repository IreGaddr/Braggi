/*
 * Braggi - Language Grammar Constraint Patterns Implementation
 * 
 * "In the quantum world of parsing, every token is both a noun and a verb
 * until the constraints make it pick a lane!" - Irish-Texan compiler philosophy
 */

#include "braggi/grammar_patterns.h"
#include "braggi/pattern.h"
#include "braggi/token.h"
#include <stdlib.h>
#include <string.h>

// Helper to create a token pattern with a specific keyword
static Pattern* create_keyword_pattern(const char* name, const char* keyword) {
    return braggi_pattern_create_token(name, TOKEN_KEYWORD, keyword);
}

// Helper to create a token pattern with a specific operator
static Pattern* create_operator_pattern(const char* name, const char* op) {
    return braggi_pattern_create_token(name, TOKEN_OPERATOR, op);
}

// Helper to create a token pattern with a specific punctuation
static Pattern* create_punctuation_pattern(const char* name, const char* punct) {
    return braggi_pattern_create_token(name, TOKEN_PUNCTUATION, punct);
}

// Helper to create a token pattern for any identifier
static Pattern* create_identifier_pattern(const char* name) {
    return braggi_pattern_create_token(name, TOKEN_IDENTIFIER, NULL);
}

// Build the complete pattern library for Braggi language
ConstraintPatternLibrary* braggi_build_language_patterns(void) {
    // Create the pattern library with the "program" pattern as the start
    ConstraintPatternLibrary* library = braggi_constraint_pattern_library_create("program");
    if (!library) return NULL;
    
    // Add all patterns to the library
    if (!braggi_constraint_pattern_library_add_pattern(library, braggi_pattern_program()) ||
        !braggi_constraint_pattern_library_add_pattern(library, braggi_pattern_declaration()) ||
        !braggi_constraint_pattern_library_add_pattern(library, braggi_pattern_region_decl()) ||
        !braggi_constraint_pattern_library_add_pattern(library, braggi_pattern_regime_decl()) ||
        !braggi_constraint_pattern_library_add_pattern(library, braggi_pattern_func_decl()) ||
        !braggi_constraint_pattern_library_add_pattern(library, braggi_pattern_var_decl()) ||
        !braggi_constraint_pattern_library_add_pattern(library, braggi_pattern_statement()) ||
        !braggi_constraint_pattern_library_add_pattern(library, braggi_pattern_block()) ||
        !braggi_constraint_pattern_library_add_pattern(library, braggi_pattern_expression()) ||
        !braggi_constraint_pattern_library_add_pattern(library, braggi_pattern_assignment()) ||
        !braggi_constraint_pattern_library_add_pattern(library, braggi_pattern_if_stmt()) ||
        !braggi_constraint_pattern_library_add_pattern(library, braggi_pattern_while_stmt()) ||
        !braggi_constraint_pattern_library_add_pattern(library, braggi_pattern_for_stmt()) ||
        !braggi_constraint_pattern_library_add_pattern(library, braggi_pattern_return_stmt()) ||
        !braggi_constraint_pattern_library_add_pattern(library, braggi_pattern_collapse_stmt()) ||
        !braggi_constraint_pattern_library_add_pattern(library, braggi_pattern_superpose_stmt()) ||
        !braggi_constraint_pattern_library_add_pattern(library, braggi_pattern_periscope_stmt()) ||
        !braggi_constraint_pattern_library_add_pattern(library, braggi_pattern_type()) ||
        !braggi_constraint_pattern_library_add_pattern(library, braggi_pattern_parameter_list()) ||
        !braggi_constraint_pattern_library_add_pattern(library, braggi_pattern_argument_list()) ||
        !braggi_constraint_pattern_library_add_pattern(library, braggi_pattern_binary_expr()) ||
        !braggi_constraint_pattern_library_add_pattern(library, braggi_pattern_unary_expr()) ||
        !braggi_constraint_pattern_library_add_pattern(library, braggi_pattern_primary_expr()) ||
        !braggi_constraint_pattern_library_add_pattern(library, braggi_pattern_literal())) {
        
        // If any pattern fails to add, clean up and return NULL
        braggi_constraint_pattern_library_destroy(library);
        return NULL;
    }
    
    return library;
}

// Program is a sequence of declarations
Pattern* braggi_pattern_program(void) {
    Pattern* decl_ref = braggi_pattern_create_reference("declaration_ref", "declaration");
    Pattern* decl_repeat = braggi_pattern_create_repetition("declarations", decl_ref);
    
    Pattern** sub_patterns = (Pattern**)malloc(sizeof(Pattern*));
    if (!sub_patterns) {
        braggi_pattern_destroy(decl_ref);
        braggi_pattern_destroy(decl_repeat);
        return NULL;
    }
    
    sub_patterns[0] = decl_repeat;
    
    return braggi_pattern_create_sequence("program", sub_patterns, 1);
}

// Declaration can be a region, regime, function, or variable declaration
Pattern* braggi_pattern_declaration(void) {
    Pattern* region_ref = braggi_pattern_create_reference("region_decl_ref", "region_decl");
    Pattern* regime_ref = braggi_pattern_create_reference("regime_decl_ref", "regime_decl");
    Pattern* func_ref = braggi_pattern_create_reference("func_decl_ref", "func_decl");
    Pattern* var_ref = braggi_pattern_create_reference("var_decl_ref", "var_decl");
    
    Pattern** sub_patterns = (Pattern**)malloc(sizeof(Pattern*) * 4);
    if (!sub_patterns) {
        braggi_pattern_destroy(region_ref);
        braggi_pattern_destroy(regime_ref);
        braggi_pattern_destroy(func_ref);
        braggi_pattern_destroy(var_ref);
        return NULL;
    }
    
    sub_patterns[0] = region_ref;
    sub_patterns[1] = regime_ref;
    sub_patterns[2] = func_ref;
    sub_patterns[3] = var_ref;
    
    return braggi_pattern_create_superposition("declaration", sub_patterns, 4);
}

// Region declaration: region <name> <regime_type> { ... }
Pattern* braggi_pattern_region_decl(void) {
    Pattern* region_kw = create_keyword_pattern("region_keyword", "region");
    Pattern* region_name = create_identifier_pattern("region_name");
    Pattern* regime_type = braggi_pattern_create_reference("regime_type_ref", "type");
    Pattern* open_brace = create_punctuation_pattern("open_brace", "{");
    Pattern* decl_ref = braggi_pattern_create_reference("inner_decl_ref", "declaration");
    Pattern* decl_repeat = braggi_pattern_create_repetition("inner_declarations", decl_ref);
    Pattern* close_brace = create_punctuation_pattern("close_brace", "}");
    
    Pattern** sub_patterns = (Pattern**)malloc(sizeof(Pattern*) * 6);
    if (!sub_patterns) {
        braggi_pattern_destroy(region_kw);
        braggi_pattern_destroy(region_name);
        braggi_pattern_destroy(regime_type);
        braggi_pattern_destroy(open_brace);
        braggi_pattern_destroy(decl_ref);
        braggi_pattern_destroy(decl_repeat);
        braggi_pattern_destroy(close_brace);
        return NULL;
    }
    
    sub_patterns[0] = region_kw;
    sub_patterns[1] = region_name;
    sub_patterns[2] = regime_type;
    sub_patterns[3] = open_brace;
    sub_patterns[4] = decl_repeat;
    sub_patterns[5] = close_brace;
    
    return braggi_pattern_create_sequence("region_decl", sub_patterns, 6);
}

// Regime declaration: regime <name> { ... }
Pattern* braggi_pattern_regime_decl(void) {
    Pattern* regime_kw = create_keyword_pattern("regime_keyword", "regime");
    Pattern* regime_name = create_identifier_pattern("regime_name");
    Pattern* fifo_kw = create_keyword_pattern("fifo_keyword", "fifo");
    Pattern* filo_kw = create_keyword_pattern("filo_keyword", "filo");
    Pattern* seq_kw = create_keyword_pattern("seq_keyword", "seq");
    Pattern* rand_kw = create_keyword_pattern("rand_keyword", "rand");
    
    // Create a superposition of regime types
    Pattern** regime_types = (Pattern**)malloc(sizeof(Pattern*) * 4);
    if (!regime_types) {
        braggi_pattern_destroy(regime_kw);
        braggi_pattern_destroy(regime_name);
        braggi_pattern_destroy(fifo_kw);
        braggi_pattern_destroy(filo_kw);
        braggi_pattern_destroy(seq_kw);
        braggi_pattern_destroy(rand_kw);
        return NULL;
    }
    
    regime_types[0] = fifo_kw;
    regime_types[1] = filo_kw;
    regime_types[2] = seq_kw;
    regime_types[3] = rand_kw;
    
    Pattern* regime_type = braggi_pattern_create_superposition("regime_type", regime_types, 4);
    
    Pattern* open_brace = create_punctuation_pattern("open_brace", "{");
    Pattern* stmt_ref = braggi_pattern_create_reference("stmt_ref", "statement");
    Pattern* stmt_repeat = braggi_pattern_create_repetition("statements", stmt_ref);
    Pattern* close_brace = create_punctuation_pattern("close_brace", "}");
    
    Pattern** sub_patterns = (Pattern**)malloc(sizeof(Pattern*) * 5);
    if (!sub_patterns) {
        braggi_pattern_destroy(regime_kw);
        braggi_pattern_destroy(regime_name);
        braggi_pattern_destroy(regime_type);
        braggi_pattern_destroy(open_brace);
        braggi_pattern_destroy(stmt_ref);
        braggi_pattern_destroy(stmt_repeat);
        braggi_pattern_destroy(close_brace);
        return NULL;
    }
    
    sub_patterns[0] = regime_kw;
    sub_patterns[1] = regime_name;
    sub_patterns[2] = regime_type;
    sub_patterns[3] = open_brace;
    sub_patterns[4] = stmt_repeat;
    sub_patterns[5] = close_brace;
    
    return braggi_pattern_create_sequence("regime_decl", sub_patterns, 6);
}

// Function declaration: func <name>(<params>) [-> <type>] <block>
Pattern* braggi_pattern_func_decl(void) {
    Pattern* func_kw = create_keyword_pattern("func_keyword", "func");
    Pattern* func_name = create_identifier_pattern("func_name");
    Pattern* open_paren = create_punctuation_pattern("open_paren", "(");
    Pattern* params = braggi_pattern_create_reference("params_ref", "parameter_list");
    Pattern* close_paren = create_punctuation_pattern("close_paren", ")");
    
    // Return type is optional
    Pattern* arrow = create_operator_pattern("arrow", "->");
    Pattern* ret_type = braggi_pattern_create_reference("return_type", "type");
    
    Pattern** ret_seq = (Pattern**)malloc(sizeof(Pattern*) * 2);
    if (!ret_seq) {
        // Clean up all created patterns...
        return NULL;
    }
    ret_seq[0] = arrow;
    ret_seq[1] = ret_type;
    
    Pattern* ret_type_seq = braggi_pattern_create_sequence("return_type_seq", ret_seq, 2);
    Pattern* opt_ret_type = braggi_pattern_create_optional("optional_return_type", ret_type_seq);
    
    // Function body
    Pattern* block = braggi_pattern_create_reference("func_body", "block");
    
    // Full function declaration
    Pattern** sub_patterns = (Pattern**)malloc(sizeof(Pattern*) * 6);
    if (!sub_patterns) {
        // Clean up all created patterns...
        return NULL;
    }
    
    sub_patterns[0] = func_kw;
    sub_patterns[1] = func_name;
    sub_patterns[2] = open_paren;
    sub_patterns[3] = params;
    sub_patterns[4] = close_paren;
    sub_patterns[5] = opt_ret_type;
    sub_patterns[6] = block;
    
    return braggi_pattern_create_sequence("func_decl", sub_patterns, 7);
}

// Variable declaration: var <name> [: <type>] [= <expr>]
Pattern* braggi_pattern_var_decl(void) {
    Pattern* var_kw = create_keyword_pattern("var_keyword", "var");
    Pattern* var_name = create_identifier_pattern("var_name");
    
    // Type annotation is optional
    Pattern* colon = create_punctuation_pattern("colon", ":");
    Pattern* var_type = braggi_pattern_create_reference("var_type", "type");
    
    Pattern** type_seq = (Pattern**)malloc(sizeof(Pattern*) * 2);
    if (!type_seq) {
        // Clean up all created patterns...
        return NULL;
    }
    type_seq[0] = colon;
    type_seq[1] = var_type;
    
    Pattern* type_annot = braggi_pattern_create_sequence("type_annotation", type_seq, 2);
    Pattern* opt_type = braggi_pattern_create_optional("optional_type", type_annot);
    
    // Initialization is optional
    Pattern* equals = create_operator_pattern("equals", "=");
    Pattern* init_expr = braggi_pattern_create_reference("init_expr", "expression");
    
    Pattern** init_seq = (Pattern**)malloc(sizeof(Pattern*) * 2);
    if (!init_seq) {
        // Clean up all created patterns...
        return NULL;
    }
    init_seq[0] = equals;
    init_seq[1] = init_expr;
    
    Pattern* init = braggi_pattern_create_sequence("initializer", init_seq, 2);
    Pattern* opt_init = braggi_pattern_create_optional("optional_init", init);
    
    // Semicolon
    Pattern* semicolon = create_punctuation_pattern("semicolon", ";");
    
    // Full variable declaration
    Pattern** sub_patterns = (Pattern**)malloc(sizeof(Pattern*) * 5);
    if (!sub_patterns) {
        // Clean up all created patterns...
        return NULL;
    }
    
    sub_patterns[0] = var_kw;
    sub_patterns[1] = var_name;
    sub_patterns[2] = opt_type;
    sub_patterns[3] = opt_init;
    sub_patterns[4] = semicolon;
    
    return braggi_pattern_create_sequence("var_decl", sub_patterns, 5);
}

// Statement can be a block, if, while, for, return, collapse, superpose, periscope, var_decl, or expression
Pattern* braggi_pattern_statement(void) {
    Pattern* block_ref = braggi_pattern_create_reference("block_stmt", "block");
    Pattern* if_ref = braggi_pattern_create_reference("if_stmt", "if_stmt");
    Pattern* while_ref = braggi_pattern_create_reference("while_stmt", "while_stmt");
    Pattern* for_ref = braggi_pattern_create_reference("for_stmt", "for_stmt");
    Pattern* return_ref = braggi_pattern_create_reference("return_stmt", "return_stmt");
    Pattern* collapse_ref = braggi_pattern_create_reference("collapse_stmt", "collapse_stmt");
    Pattern* superpose_ref = braggi_pattern_create_reference("superpose_stmt", "superpose_stmt");
    Pattern* periscope_ref = braggi_pattern_create_reference("periscope_stmt", "periscope_stmt");
    Pattern* var_ref = braggi_pattern_create_reference("var_stmt", "var_decl");
    
    // Expression statement: expr;
    Pattern* expr_ref = braggi_pattern_create_reference("expr_ref", "expression");
    Pattern* semicolon = create_punctuation_pattern("semicolon", ";");
    
    Pattern** expr_seq = (Pattern**)malloc(sizeof(Pattern*) * 2);
    if (!expr_seq) {
        // Clean up all created patterns...
        return NULL;
    }
    expr_seq[0] = expr_ref;
    expr_seq[1] = semicolon;
    
    Pattern* expr_stmt = braggi_pattern_create_sequence("expr_stmt", expr_seq, 2);
    
    // All possible statement types
    Pattern** sub_patterns = (Pattern**)malloc(sizeof(Pattern*) * 10);
    if (!sub_patterns) {
        // Clean up all created patterns...
        return NULL;
    }
    
    sub_patterns[0] = block_ref;
    sub_patterns[1] = if_ref;
    sub_patterns[2] = while_ref;
    sub_patterns[3] = for_ref;
    sub_patterns[4] = return_ref;
    sub_patterns[5] = collapse_ref;
    sub_patterns[6] = superpose_ref;
    sub_patterns[7] = periscope_ref;
    sub_patterns[8] = var_ref;
    sub_patterns[9] = expr_stmt;
    
    return braggi_pattern_create_superposition("statement", sub_patterns, 10);
}

// Block: { <statements> }
Pattern* braggi_pattern_block(void) {
    Pattern* open_brace = create_punctuation_pattern("open_brace", "{");
    Pattern* stmt_ref = braggi_pattern_create_reference("block_stmt_ref", "statement");
    Pattern* stmt_repeat = braggi_pattern_create_repetition("block_stmts", stmt_ref);
    Pattern* close_brace = create_punctuation_pattern("close_brace", "}");
    
    Pattern** sub_patterns = (Pattern**)malloc(sizeof(Pattern*) * 3);
    if (!sub_patterns) {
        braggi_pattern_destroy(open_brace);
        braggi_pattern_destroy(stmt_ref);
        braggi_pattern_destroy(stmt_repeat);
        braggi_pattern_destroy(close_brace);
        return NULL;
    }
    
    sub_patterns[0] = open_brace;
    sub_patterns[1] = stmt_repeat;
    sub_patterns[2] = close_brace;
    
    return braggi_pattern_create_sequence("block", sub_patterns, 3);
}

// Expression can be an assignment, binary expression, unary expression, or primary expression
Pattern* braggi_pattern_expression(void) {
    Pattern* assign_ref = braggi_pattern_create_reference("assign_expr", "assignment");
    Pattern* binary_ref = braggi_pattern_create_reference("binary_expr", "binary_expr");
    Pattern* unary_ref = braggi_pattern_create_reference("unary_expr", "unary_expr");
    Pattern* primary_ref = braggi_pattern_create_reference("primary_expr", "primary_expr");
    
    Pattern** sub_patterns = (Pattern**)malloc(sizeof(Pattern*) * 4);
    if (!sub_patterns) {
        braggi_pattern_destroy(assign_ref);
        braggi_pattern_destroy(binary_ref);
        braggi_pattern_destroy(unary_ref);
        braggi_pattern_destroy(primary_ref);
        return NULL;
    }
    
    sub_patterns[0] = assign_ref;
    sub_patterns[1] = binary_ref;
    sub_patterns[2] = unary_ref;
    sub_patterns[3] = primary_ref;
    
    return braggi_pattern_create_superposition("expression", sub_patterns, 4);
}

// Add implementations for all other patterns similarly... 