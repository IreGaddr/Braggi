/*
 * Braggi - Tokenizer Implementation
 * 
 * "Words are just fancy air until you tokenize 'em - then they become cattle
 * you can round up and drive to market!" - Irish-Texan Programming Proverb
 */

#include "braggi/tokenizer.h"
#include "braggi/allocation.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

// Helper function to check if a character is a valid start of an identifier
static bool is_identifier_start(char c) {
    return isalpha(c) || c == '_';
}

// Helper function to check if a character is a valid part of an identifier
static bool is_identifier_part(char c) {
    return isalnum(c) || c == '_';
}

// Helper function to check if a string is a keyword
static bool is_keyword(const char* text, size_t length) {
    // List of keywords - expand as needed
    static const char* keywords[] = {
        "if", "else", "while", "for", "return", "break", "continue",
        "struct", "enum", "union", "typedef", "const", "static", "void",
        "int", "float", "double", "char", "bool", "unsigned", "signed"
    };
    static const size_t keyword_count = sizeof(keywords) / sizeof(keywords[0]);
    
    for (size_t i = 0; i < keyword_count; i++) {
        if (strlen(keywords[i]) == length && strncmp(text, keywords[i], length) == 0) {
            return true;
        }
    }
    
    return false;
}

// Helper function to check if a character is a valid operator start
static bool is_operator_char(char c) {
    return c == '+' || c == '-' || c == '*' || c == '/' || c == '%' ||
           c == '=' || c == '<' || c == '>' || c == '!' || c == '&' ||
           c == '|' || c == '^' || c == '~' || c == '.' || c == '?' ||
           c == ':';
}

// Helper function to check if a character is a punctuation
static bool is_punctuation(char c) {
    return c == '(' || c == ')' || c == '[' || c == ']' || c == '{' ||
           c == '}' || c == ';' || c == ',' || c == '@';
}

// Helper function to read a character from source
static char read_char(Source* source, size_t position) {
    if (position >= braggi_source_get_length(source)) {
        return '\0';
    }
    return braggi_source_get_char(source, position);
}

// Create a tokenizer
Tokenizer* braggi_tokenizer_create(Source* source) {
    if (!source) return NULL;
    
    Tokenizer* tokenizer = (Tokenizer*)malloc(sizeof(Tokenizer));
    if (!tokenizer) return NULL;
    
    tokenizer->source = source;
    tokenizer->position = 0;
    tokenizer->has_token = false;
    
    // Initialize the current token
    memset(&tokenizer->current_token, 0, sizeof(Token));
    
    return tokenizer;
}

// Destroy a tokenizer
void braggi_tokenizer_destroy(Tokenizer* tokenizer) {
    if (!tokenizer) return;
    free(tokenizer);
}

// Helper function to scan a token
static Token scan_token(Tokenizer* tokenizer) {
    Token token;
    memset(&token, 0, sizeof(Token));
    
    Source* source = tokenizer->source;
    size_t position = tokenizer->position;
    char c = read_char(source, position);
    
    token.position = braggi_source_get_position(source, position);
    
    // Skip whitespace
    while (isspace(c)) {
        position++;
        c = read_char(source, position);
    }
    
    if (c == '\0') {
        // End of file
        token.type = TOKEN_EOF;
        token.text = "";
        token.length = 0;
        tokenizer->position = position;
        return token;
    }
    
    // Remember token start position
    size_t start_position = position;
    token.position = braggi_source_get_position(source, start_position);
    
    // Scan based on first character
    if (is_identifier_start(c)) {
        // Identifier or keyword
        position++;
        c = read_char(source, position);
        
        while (is_identifier_part(c)) {
            position++;
            c = read_char(source, position);
        }
        
        size_t length = position - start_position;
        const char* text = braggi_source_get_range(source, start_position, length);
        
        if (is_keyword(text, length)) {
            token.type = TOKEN_KEYWORD;
        } else {
            token.type = TOKEN_IDENTIFIER;
        }
        
        token.text = text;
        token.length = length;
        tokenizer->position = position;
        return token;
        
    } else if (isdigit(c) || (c == '.' && isdigit(read_char(source, position + 1)))) {
        // Numeric literal
        bool has_decimal = (c == '.');
        bool has_exponent = false;
        
        position++;
        c = read_char(source, position);
        
        while (isdigit(c) || 
               (c == '.' && !has_decimal) || 
               ((c == 'e' || c == 'E') && !has_exponent) ||
               ((c == '+' || c == '-') && (read_char(source, position - 1) == 'e' || 
                                          read_char(source, position - 1) == 'E'))) {
            
            if (c == '.') has_decimal = true;
            if (c == 'e' || c == 'E') has_exponent = true;
            
            position++;
            c = read_char(source, position);
        }
        
        size_t length = position - start_position;
        const char* text = braggi_source_get_range(source, start_position, length);
        
        if (has_decimal || has_exponent) {
            token.type = TOKEN_LITERAL_FLOAT;
            token.float_value = atof(text);
        } else {
            token.type = TOKEN_LITERAL_INT;
            token.int_value = atol(text);
        }
        
        token.text = text;
        token.length = length;
        tokenizer->position = position;
        return token;
        
    } else if (c == '"') {
        // String literal
        position++; // Skip opening quote
        
        size_t string_start = position;
        c = read_char(source, position);
        
        while (c != '"' && c != '\0') {
            if (c == '\\') {
                // Skip escape sequence
                position++;
            }
            position++;
            c = read_char(source, position);
        }
        
        size_t string_length = position - string_start;
        const char* string_value = braggi_source_get_range(source, string_start, string_length);
        
        if (c == '"') {
            position++; // Skip closing quote
        }
        
        size_t token_length = position - start_position;
        
        token.type = TOKEN_LITERAL_STRING;
        token.text = braggi_source_get_range(source, start_position, token_length);
        token.string_value = string_value;
        token.length = token_length;
        tokenizer->position = position;
        return token;
        
    } else if (c == '\'') {
        // Character literal
        position++; // Skip opening quote
        
        size_t char_start = position;
        c = read_char(source, position);
        
        if (c == '\\') {
            // Escape sequence
            position++;
            c = read_char(source, position);
        }
        
        position++;
        c = read_char(source, position);
        
        if (c == '\'') {
            position++; // Skip closing quote
        }
        
        size_t token_length = position - start_position;
        const char* char_value = braggi_source_get_range(source, char_start, 1);
        
        token.type = TOKEN_LITERAL_CHAR;
        token.text = braggi_source_get_range(source, start_position, token_length);
        token.string_value = char_value;
        token.length = token_length;
        tokenizer->position = position;
        return token;
        
    } else if (c == '/' && read_char(source, position + 1) == '/') {
        // Line comment
        position += 2; // Skip //
        
        c = read_char(source, position);
        while (c != '\n' && c != '\0') {
            position++;
            c = read_char(source, position);
        }
        
        size_t length = position - start_position;
        
        token.type = TOKEN_COMMENT;
        token.text = braggi_source_get_range(source, start_position, length);
        token.length = length;
        tokenizer->position = position;
        return token;
        
    } else if (c == '/' && read_char(source, position + 1) == '*') {
        // Block comment
        position += 2; // Skip /*
        
        c = read_char(source, position);
        while (!(c == '*' && read_char(source, position + 1) == '/') && c != '\0') {
            position++;
            c = read_char(source, position);
        }
        
        if (c == '*') {
            position += 2; // Skip */
        }
        
        size_t length = position - start_position;
        
        token.type = TOKEN_COMMENT;
        token.text = braggi_source_get_range(source, start_position, length);
        token.length = length;
        tokenizer->position = position;
        return token;
        
    } else if (is_operator_char(c)) {
        // Operator
        position++;
        char next_c = read_char(source, position);
        
        // Handle multi-character operators
        if ((c == '+' && (next_c == '+' || next_c == '=')) ||
            (c == '-' && (next_c == '-' || next_c == '=' || next_c == '>')) ||
            (c == '*' && next_c == '=') ||
            (c == '/' && next_c == '=') ||
            (c == '%' && next_c == '=') ||
            (c == '=' && next_c == '=') ||
            (c == '!' && next_c == '=') ||
            (c == '<' && (next_c == '=' || next_c == '<')) ||
            (c == '>' && (next_c == '=' || next_c == '>')) ||
            (c == '&' && (next_c == '&' || next_c == '=')) ||
            (c == '|' && (next_c == '|' || next_c == '=')) ||
            (c == '^' && next_c == '=') ||
            (c == '.' && next_c == '.')) {
            
            position++;
            
            // Handle three-character operators
            if (((c == '<' && next_c == '<') || (c == '>' && next_c == '>')) && 
                read_char(source, position) == '=') {
                position++;
            } else if (c == '.' && next_c == '.' && read_char(source, position) == '.') {
                position++;
            }
        }
        
        size_t length = position - start_position;
        
        token.type = TOKEN_OPERATOR;
        token.text = braggi_source_get_range(source, start_position, length);
        token.length = length;
        tokenizer->position = position;
        return token;
        
    } else if (is_punctuation(c)) {
        // Punctuation
        position++;
        
        token.type = TOKEN_PUNCTUATION;
        token.text = braggi_source_get_range(source, start_position, 1);
        token.length = 1;
        tokenizer->position = position;
        return token;
        
    } else if (isspace(c)) {
        // Whitespace
        while (isspace(c)) {
            position++;
            c = read_char(source, position);
        }
        
        size_t length = position - start_position;
        
        token.type = TOKEN_WHITESPACE;
        token.text = braggi_source_get_range(source, start_position, length);
        token.length = length;
        tokenizer->position = position;
        return token;
        
    } else {
        // Invalid token
        position++;
        
        token.type = TOKEN_INVALID;
        token.text = braggi_source_get_range(source, start_position, 1);
        token.length = 1;
        tokenizer->position = position;
        return token;
    }
}

// Get the current token without advancing
Token* braggi_tokenizer_current(Tokenizer* tokenizer) {
    if (!tokenizer) return NULL;
    
    if (!tokenizer->has_token) {
        tokenizer->current_token = scan_token(tokenizer);
        tokenizer->has_token = true;
    }
    
    return &tokenizer->current_token;
}

// Advance to the next token and return it
Token* braggi_tokenizer_next(Tokenizer* tokenizer) {
    if (!tokenizer) return NULL;
    
    tokenizer->current_token = scan_token(tokenizer);
    tokenizer->has_token = true;
    
    return &tokenizer->current_token;
}

// Peek at the next token without advancing
Token* braggi_tokenizer_peek(Tokenizer* tokenizer) {
    if (!tokenizer) return NULL;
    
    // Save current state
    size_t old_position = tokenizer->position;
    bool old_has_token = tokenizer->has_token;
    Token old_token = tokenizer->current_token;
    
    // Scan next token
    Token next_token = scan_token(tokenizer);
    
    // Restore state
    tokenizer->position = old_position;
    tokenizer->has_token = old_has_token;
    tokenizer->current_token = old_token;
    
    // We need to return a pointer to the next token, but we can't use a local variable
    // In a real implementation, this would need to be handled differently
    // For now, we're just copying the token to the current token and returning that
    tokenizer->current_token = next_token;
    return &tokenizer->current_token;
}

// Check if the tokenizer has reached the end of the source
bool braggi_tokenizer_is_eof(const Tokenizer* tokenizer) {
    if (!tokenizer) return true;
    
    // Peek at the current token (without actually changing the tokenizer state)
    Tokenizer* non_const_tokenizer = (Tokenizer*)tokenizer;
    Token* current = braggi_tokenizer_current(non_const_tokenizer);
    
    return current && current->type == TOKEN_EOF;
}

// Get the current position in the source
SourcePosition braggi_tokenizer_get_position(const Tokenizer* tokenizer) {
    if (!tokenizer || !tokenizer->source) {
        SourcePosition pos = {0, 0};
        return pos;
    }
    
    return braggi_source_get_position(tokenizer->source, tokenizer->position);
}

// Skip tokens until a specific token type is found
bool braggi_tokenizer_skip_until(Tokenizer* tokenizer, TokenType type) {
    if (!tokenizer) return false;
    
    Token* token;
    while ((token = braggi_tokenizer_current(tokenizer)) != NULL && token->type != TOKEN_EOF) {
        if (token->type == type) {
            return true;
        }
        braggi_tokenizer_next(tokenizer);
    }
    
    return false;
}

// Expect a specific token type, return false if not found
bool braggi_tokenizer_expect(Tokenizer* tokenizer, TokenType type) {
    if (!tokenizer) return false;
    
    Token* token = braggi_tokenizer_current(tokenizer);
    if (!token || token->type != type) {
        return false;
    }
    
    braggi_tokenizer_next(tokenizer);
    return true;
}

// Expect a specific keyword, return false if not found
bool braggi_tokenizer_expect_keyword(Tokenizer* tokenizer, const char* keyword) {
    if (!tokenizer || !keyword) return false;
    
    Token* token = braggi_tokenizer_current(tokenizer);
    if (!token || token->type != TOKEN_KEYWORD) {
        return false;
    }
    
    if (strcmp(token->text, keyword) != 0) {
        return false;
    }
    
    braggi_tokenizer_next(tokenizer);
    return true;
}

// Expect a specific operator, return false if not found
bool braggi_tokenizer_expect_operator(Tokenizer* tokenizer, const char* operator) {
    if (!tokenizer || !operator) return false;
    
    Token* token = braggi_tokenizer_current(tokenizer);
    if (!token || token->type != TOKEN_OPERATOR) {
        return false;
    }
    
    if (strcmp(token->text, operator) != 0) {
        return false;
    }
    
    braggi_tokenizer_next(tokenizer);
    return true;
}

// Expect a specific punctuation, return false if not found
bool braggi_tokenizer_expect_punctuation(Tokenizer* tokenizer, const char* punctuation) {
    if (!tokenizer || !punctuation) return false;
    
    Token* token = braggi_tokenizer_current(tokenizer);
    if (!token || token->type != TOKEN_PUNCTUATION) {
        return false;
    }
    
    if (strcmp(token->text, punctuation) != 0) {
        return false;
    }
    
    braggi_tokenizer_next(tokenizer);
    return true;
} 