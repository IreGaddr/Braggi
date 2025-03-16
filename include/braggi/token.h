/*
 * Braggi - Token Definitions
 * 
 * "Tokens are like words in a song - ya gotta get 'em right
 * or the whole tune falls apart!" - Texas Compiler Philosophy
 */

#ifndef BRAGGI_TOKEN_H
#define BRAGGI_TOKEN_H

#include "braggi/source.h"
#include "braggi/util/vector.h"
#include <stdbool.h>
#include <stdint.h>

/*
 * Token types - representing different elements in the source code
 */
typedef enum {
    TOKEN_INVALID,         /* Invalid token (error) */
    TOKEN_IDENTIFIER,      /* Identifier */
    TOKEN_KEYWORD,         /* Language keyword */
    TOKEN_LITERAL_INT,     /* Integer literal */
    TOKEN_LITERAL_FLOAT,   /* Floating point literal */
    TOKEN_LITERAL_STRING,  /* String literal */
    TOKEN_LITERAL_CHAR,    /* Character literal */
    TOKEN_OPERATOR,        /* Operator (e.g., +, -, *, /) */
    TOKEN_PUNCTUATION,     /* Punctuation (e.g., {, }, (, )) */
    TOKEN_COMMENT,         /* Comment */
    TOKEN_WHITESPACE,      /* Whitespace */
    TOKEN_EOF              /* End of file */
} TokenType;

/*
 * Token - Represents a single token in the source code
 */
typedef struct {
    TokenType type;             /* Type of the token */
    SourcePosition position;    /* Position in the source file */
    char* text;                 /* Text representation of the token */
    size_t length;              /* Length of the token text */
    
    /* Value union - used for literals */
    union {
        int64_t int_value;      /* Integer value */
        double float_value;     /* Float value */
        char* string_value;     /* String value */
    };
} Token;

/*
 * Tokenizer - Converts source code into tokens
 */
typedef struct {
    SourceFile* source;        /* Source file being tokenized */
    size_t position;           /* Current position in the source */
    Token current_token;       /* Current token */
    Token next_token;          /* Next token (for lookahead) */
    bool has_next;             /* Whether the next token is valid */
    Vector* tokens;            /* Optional: Vector of all tokens (if storing) */
} Tokenizer;

/* Function prototypes */

// Create a new tokenizer for a source file
Tokenizer* braggi_tokenizer_create(SourceFile* source);

// Destroy a tokenizer and free all resources
void braggi_tokenizer_destroy(Tokenizer* tokenizer);

// Get the next token from the source
bool braggi_tokenizer_next(Tokenizer* tokenizer);

// Peek at the next token without consuming it
Token* braggi_tokenizer_peek(Tokenizer* tokenizer);

// Get the current token
Token* braggi_tokenizer_current(Tokenizer* tokenizer);

// Check if current token is a specific keyword
bool braggi_tokenizer_is_keyword(Tokenizer* tokenizer, const char* keyword);

// Check if current token is a specific operator
bool braggi_tokenizer_is_operator(Tokenizer* tokenizer, const char* op);

// Expect a specific token type, consuming it if found
bool braggi_tokenizer_expect(Tokenizer* tokenizer, TokenType type);

// Expect a specific keyword, consuming it if found
bool braggi_tokenizer_expect_keyword(Tokenizer* tokenizer, const char* keyword);

// Expect a specific operator, consuming it if found
bool braggi_tokenizer_expect_operator(Tokenizer* tokenizer, const char* op);

// Convert a token type to string for debugging
const char* braggi_token_type_string(TokenType type);

/*
 * The following function should be defined elsewhere when State is properly defined
 */
// Convert a token to a state for the entropy field
// State* braggi_token_to_state(Token* token);

#endif /* BRAGGI_TOKEN_H */ 