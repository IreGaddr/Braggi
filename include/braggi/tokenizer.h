/*
 * Braggi - Tokenizer
 * 
 * "Breaking code into tokens is like separating cattle at a roundup - 
 * you gotta know what's what, where it is, and where it's going!" - Texas Programming Wisdom
 */

#ifndef BRAGGI_TOKENIZER_H
#define BRAGGI_TOKENIZER_H

#include "braggi/source.h"
#include <stdbool.h>
#include <stdint.h>

/*
 * Token types
 */
typedef enum TokenType {
    TOKEN_INVALID = 0,
    TOKEN_EOF,
    TOKEN_IDENTIFIER,
    TOKEN_KEYWORD,
    TOKEN_LITERAL_INT,
    TOKEN_LITERAL_FLOAT,
    TOKEN_LITERAL_STRING,
    TOKEN_LITERAL_CHAR,
    TOKEN_OPERATOR,
    TOKEN_PUNCTUATION,
    TOKEN_COMMENT,
    TOKEN_WHITESPACE,
    TOKEN_NEWLINE,
    // Add more token types as needed
} TokenType;

/*
 * Token structure
 */
typedef struct Token {
    TokenType type;             /* Token type */
    const char* text;           /* Token text */
    SourcePosition position;    /* Position in source */
    
    /* Union for token values */
    union {
        int64_t int_value;      /* Integer literal value */
        double float_value;     /* Float literal value */
        const char* string_value; /* String literal value */
    };
    
    size_t length;              /* Token length in bytes */
} Token;

/*
 * Tokenizer structure
 */
typedef struct Tokenizer {
    Source* source;             /* Source being tokenized */
    size_t position;            /* Current position in source */
    Token current_token;        /* Current token */
    bool has_token;             /* Whether current_token is valid */
} Tokenizer;

/* Create a tokenizer */
Tokenizer* braggi_tokenizer_create(Source* source);

/* Destroy a tokenizer */
void braggi_tokenizer_destroy(Tokenizer* tokenizer);

/* Get the current token without advancing */
Token* braggi_tokenizer_current(Tokenizer* tokenizer);

/* Advance to the next token and return it */
Token* braggi_tokenizer_next(Tokenizer* tokenizer);

/* Peek at the next token without advancing */
Token* braggi_tokenizer_peek(Tokenizer* tokenizer);

/* Check if the tokenizer has reached the end of the source */
bool braggi_tokenizer_is_eof(const Tokenizer* tokenizer);

/* Get the current position in the source */
SourcePosition braggi_tokenizer_get_position(const Tokenizer* tokenizer);

/* Skip tokens until a specific token type is found */
bool braggi_tokenizer_skip_until(Tokenizer* tokenizer, TokenType type);

/* Expect a specific token type, return false if not found */
bool braggi_tokenizer_expect(Tokenizer* tokenizer, TokenType type);

/* Expect a specific keyword, return false if not found */
bool braggi_tokenizer_expect_keyword(Tokenizer* tokenizer, const char* keyword);

/* Expect a specific operator, return false if not found */
bool braggi_tokenizer_expect_operator(Tokenizer* tokenizer, const char* operator);

/* Expect a specific punctuation, return false if not found */
bool braggi_tokenizer_expect_punctuation(Tokenizer* tokenizer, const char* punctuation);

#endif /* BRAGGI_TOKENIZER_H */ 