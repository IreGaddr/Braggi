/*
 * Braggi - Tokenizer Implementation
 * 
 * "Splitting up the source code into tokens is like separating
 * the cattle from the herd - you need a keen eye and steady hand!"
 * - Texas programming proverb
 */

#include "braggi/token.h"
#include "braggi/source.h"
#include "braggi/util/vector.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>  /* For int64_t */

// Helper functions for character classification
static bool is_identifier_start(char c) {
    return (isalpha(c) || c == '_');
}

static bool is_identifier_part(char c) {
    return (isalnum(c) || c == '_');
}

static bool is_operator_char(char c) {
    return (c == '+' || c == '-' || c == '*' || c == '/' || c == '%' ||
            c == '=' || c == '<' || c == '>' || c == '!' || c == '&' || 
            c == '|' || c == '^' || c == '~' || c == '.' || c == '?' ||
            c == ':');
}

static bool is_punctuation(char c) {
    return (c == '(' || c == ')' || c == '{' || c == '}' || c == '[' || 
            c == ']' || c == ';' || c == ',' || c == '@');
}

// Table of Braggi language keywords
static const char* keywords[] = {
    "region", "regime", "func", "var", "if", "else", "while",
    "for", "return", "collapse", "superpose", "periscope",
    "fifo", "filo", "seq", "rand", "in", "out", "true", 
    "false", "null", NULL
};

// Check if a string is a keyword
static bool is_keyword(const char* text, size_t length) {
    for (int i = 0; keywords[i] != NULL; i++) {
        size_t keyword_len = strlen(keywords[i]);
        if (length == keyword_len && strncmp(text, keywords[i], length) == 0) {
            return true;
        }
    }
    return false;
}

// Create a new tokenizer for a source file
Tokenizer* braggi_tokenizer_create(SourceFile* source) {
    if (!source) return NULL;
    
    Tokenizer* tokenizer = (Tokenizer*)malloc(sizeof(Tokenizer));
    if (!tokenizer) return NULL;
    
    tokenizer->source = source;
    tokenizer->position = 0;
    tokenizer->has_next = false;
    
    // Initialize token structures
    memset(&tokenizer->current_token, 0, sizeof(Token));
    memset(&tokenizer->next_token, 0, sizeof(Token));
    
    // We don't allocate a vector by default to save memory
    // Vector will be created on-demand if token storage is needed
    tokenizer->tokens = NULL;
    
    // Prime the tokenizer by reading the first token
    if (!braggi_tokenizer_next(tokenizer)) {
        braggi_tokenizer_destroy(tokenizer);
        return NULL;
    }
    
    return tokenizer;
}

// Clean up tokenizer resources
void braggi_tokenizer_destroy(Tokenizer* tokenizer) {
    if (!tokenizer) return;
    
    // Free token text for current token
    if (tokenizer->current_token.text) {
        free(tokenizer->current_token.text);
    }
    
    // Free token text for next token if available
    if (tokenizer->has_next && tokenizer->next_token.text) {
        free(tokenizer->next_token.text);
    }
    
    // If we have a string value in the union, free it too
    if (tokenizer->current_token.type == TOKEN_LITERAL_STRING || 
        tokenizer->current_token.type == TOKEN_LITERAL_CHAR) {
        if (tokenizer->current_token.string_value) {
            free(tokenizer->current_token.string_value);
        }
    }
    
    if (tokenizer->has_next && 
        (tokenizer->next_token.type == TOKEN_LITERAL_STRING || 
         tokenizer->next_token.type == TOKEN_LITERAL_CHAR)) {
        if (tokenizer->next_token.string_value) {
            free(tokenizer->next_token.string_value);
        }
    }
    
    // Free token vector if we have one
    if (tokenizer->tokens) {
        // If the vector owns the tokens, it will free them
        braggi_vector_destroy(tokenizer->tokens);
    }
    
    // Free the tokenizer itself
    free(tokenizer);
}

// Helper: Read a character from the source at the current position
static char read_char(Tokenizer* tokenizer) {
    if (tokenizer->position >= tokenizer->source->length) {
        return '\0';
    }
    return tokenizer->source->content[tokenizer->position];
}

// Helper: Peek at a character ahead without consuming it
static char peek_char(Tokenizer* tokenizer, size_t offset) {
    if (tokenizer->position + offset >= tokenizer->source->length) {
        return '\0';
    }
    return tokenizer->source->content[tokenizer->position + offset];
}

// Helper: Consume the current character and advance
static char consume_char(Tokenizer* tokenizer) {
    if (tokenizer->position >= tokenizer->source->length) {
        return '\0';
    }
    return tokenizer->source->content[tokenizer->position++];
}

// Helper functions for token scanning
static size_t scan_identifier(Tokenizer* tokenizer, Token* token) {
    size_t start = tokenizer->position;
    size_t length = 0;
    
    // First character must be a letter or underscore
    char c = read_char(tokenizer);
    if (!is_identifier_start(c)) {
        return 0;
    }
    
    // Consume the first character
    consume_char(tokenizer);
    length++;
    
    // Read remaining characters
    while (is_identifier_part(peek_char(tokenizer, 0))) {
        consume_char(tokenizer);
        length++;
    }
    
    // Create token
    token->type = TOKEN_IDENTIFIER;
    token->position.offset = start;
    token->position.length = length;
    token->text = strndup(tokenizer->source->content + start, length);
    token->length = length;
    
    // Check if it's a keyword
    if (is_keyword(token->text, length)) {
        token->type = TOKEN_KEYWORD;
    }
    
    return length;
}

static size_t scan_number(Tokenizer* tokenizer, Token* token) {
    size_t start = tokenizer->position;
    size_t length = 0;
    bool is_float = false;
    
    // Must start with a digit
    char c = read_char(tokenizer);
    if (!isdigit(c)) {
        return 0;
    }
    
    // Consume the first digit
    consume_char(tokenizer);
    length++;
    
    // Read integer part
    while (isdigit(peek_char(tokenizer, 0))) {
        consume_char(tokenizer);
        length++;
    }
    
    // Check for decimal point
    if (peek_char(tokenizer, 0) == '.') {
        // Look ahead to make sure it's followed by a digit
        if (isdigit(peek_char(tokenizer, 1))) {
            is_float = true;
            consume_char(tokenizer); // Consume the '.'
            length++;
            
            // Read fractional part
            while (isdigit(peek_char(tokenizer, 0))) {
                consume_char(tokenizer);
                length++;
            }
        }
    }
    
    // Check for exponent notation
    if (peek_char(tokenizer, 0) == 'e' || peek_char(tokenizer, 0) == 'E') {
        // Look ahead to check for sign or digit
        char next = peek_char(tokenizer, 1);
        if (isdigit(next) || next == '+' || next == '-') {
            is_float = true;
            consume_char(tokenizer); // Consume 'e' or 'E'
            length++;
            
            // Consume optional sign
            if (peek_char(tokenizer, 0) == '+' || peek_char(tokenizer, 0) == '-') {
                consume_char(tokenizer);
                length++;
            }
            
            // Ensure there's at least one digit in the exponent
            if (!isdigit(peek_char(tokenizer, 0))) {
                // Invalid exponent, rewind and treat as simple int/float
                tokenizer->position = start + length;
            } else {
                // Read exponent digits
                while (isdigit(peek_char(tokenizer, 0))) {
                    consume_char(tokenizer);
                    length++;
                }
            }
        }
    }
    
    // Create token
    token->type = is_float ? TOKEN_LITERAL_FLOAT : TOKEN_LITERAL_INT;
    token->position.offset = start;
    token->position.length = length;
    token->text = strndup(tokenizer->source->content + start, length);
    token->length = length;
    
    // Parse the value
    if (is_float) {
        token->float_value = atof(token->text);
    } else {
        token->int_value = atoll(token->text);
    }
    
    return length;
}

static size_t scan_string_literal(Tokenizer* tokenizer, Token* token) {
    size_t start = tokenizer->position;
    size_t length = 0;
    
    // Must start with a double quote
    char c = read_char(tokenizer);
    if (c != '"') {
        return 0;
    }
    
    // Consume the opening quote
    consume_char(tokenizer);
    length++;
    
    // Read until closing quote or end of file
    bool escaped = false;
    char buffer[1024]; // For storing the processed string
    size_t buf_pos = 0;
    
    while (true) {
        char next = peek_char(tokenizer, 0);
        
        // End of file before closing quote
        if (next == '\0') {
            // Error: unclosed string
            return 0;
        }
        
        // Consume the character
        consume_char(tokenizer);
        length++;
        
        // Process the character
        if (escaped) {
            // Handle escape sequence
            switch (next) {
                case '"': buffer[buf_pos++] = '"'; break;
                case '\\': buffer[buf_pos++] = '\\'; break;
                case 'n': buffer[buf_pos++] = '\n'; break;
                case 't': buffer[buf_pos++] = '\t'; break;
                case 'r': buffer[buf_pos++] = '\r'; break;
                // Add more escape sequences as needed
                default: buffer[buf_pos++] = next; break;
            }
            escaped = false;
        } else {
            // Regular character processing
            if (next == '\\') {
                escaped = true;
            } else if (next == '"') {
                // End of string
                break;
            } else {
                buffer[buf_pos++] = next;
            }
        }
        
        // Safety check to prevent buffer overflow
        if (buf_pos >= sizeof(buffer) - 1) {
            // String is too long
            return 0;
        }
    }
    
    // Null-terminate the string
    buffer[buf_pos] = '\0';
    
    // Create token
    token->type = TOKEN_LITERAL_STRING;
    token->position.offset = start;
    token->position.length = length;
    token->text = strndup(tokenizer->source->content + start, length);
    token->length = length;
    token->string_value = strdup(buffer);
    
    return length;
}

static size_t scan_char_literal(Tokenizer* tokenizer, Token* token) {
    size_t start = tokenizer->position;
    size_t length = 0;
    
    // Must start with a single quote
    char c = read_char(tokenizer);
    if (c != '\'') {
        return 0;
    }
    
    // Consume the opening quote
    consume_char(tokenizer);
    length++;
    
    // Read the character (handling escape sequences)
    char value;
    bool escaped = false;
    
    char next = peek_char(tokenizer, 0);
    if (next == '\\') {
        // Escape sequence
        consume_char(tokenizer);
        length++;
        escaped = true;
        
        next = peek_char(tokenizer, 0);
        if (next == '\0') {
            // Unexpected end of file
            return 0;
        }
        
        consume_char(tokenizer);
        length++;
        
        // Process escape sequence
        switch (next) {
            case '\'': value = '\''; break;
            case '\\': value = '\\'; break;
            case 'n': value = '\n'; break;
            case 't': value = '\t'; break;
            case 'r': value = '\r'; break;
            // Add more escape sequences as needed
            default: value = next; break;
        }
    } else if (next == '\'' || next == '\0') {
        // Empty char literal or unexpected EOF
        return 0;
    } else {
        // Regular character
        value = next;
        consume_char(tokenizer);
        length++;
    }
    
    // Expect closing quote
    if (peek_char(tokenizer, 0) != '\'') {
        return 0;
    }
    
    // Consume closing quote
    consume_char(tokenizer);
    length++;
    
    // Create token
    token->type = TOKEN_LITERAL_CHAR;
    token->position.offset = start;
    token->position.length = length;
    token->text = strndup(tokenizer->source->content + start, length);
    token->length = length;
    
    // Store the character value
    char* char_str = (char*)malloc(2);
    if (char_str) {
        char_str[0] = value;
        char_str[1] = '\0';
        token->string_value = char_str;
    }
    
    return length;
}

static size_t scan_operator(Tokenizer* tokenizer, Token* token) {
    size_t start = tokenizer->position;
    size_t length = 0;
    
    // Check if current character is an operator
    char c = read_char(tokenizer);
    if (!is_operator_char(c)) {
        return 0;
    }
    
    // Consume the first character
    consume_char(tokenizer);
    length++;
    
    // Try to form multi-character operators
    // This is a simplified version - you'd have a more comprehensive check
    // for all valid multi-character operators in a real implementation
    if (c == '=' && peek_char(tokenizer, 0) == '=') {
        // == equality operator
        consume_char(tokenizer);
        length++;
    } else if (c == '!' && peek_char(tokenizer, 0) == '=') {
        // != inequality operator
        consume_char(tokenizer);
        length++;
    } else if (c == '<' && peek_char(tokenizer, 0) == '=') {
        // <= less than or equal
        consume_char(tokenizer);
        length++;
    } else if (c == '>' && peek_char(tokenizer, 0) == '=') {
        // >= greater than or equal
        consume_char(tokenizer);
        length++;
    } else if (c == '&' && peek_char(tokenizer, 0) == '&') {
        // && logical and
        consume_char(tokenizer);
        length++;
    } else if (c == '|' && peek_char(tokenizer, 0) == '|') {
        // || logical or
        consume_char(tokenizer);
        length++;
    } else if (c == '-' && peek_char(tokenizer, 0) == '>') {
        // -> arrow operator
        consume_char(tokenizer);
        length++;
    } else if (c == '=' && peek_char(tokenizer, 0) == '>') {
        // => fat arrow operator
        consume_char(tokenizer);
        length++;
    }
    
    // Create token
    token->type = TOKEN_OPERATOR;
    token->position.offset = start;
    token->position.length = length;
    token->text = strndup(tokenizer->source->content + start, length);
    token->length = length;
    
    return length;
}

static size_t scan_punctuation(Tokenizer* tokenizer, Token* token) {
    size_t start = tokenizer->position;
    
    // Check if current character is punctuation
    char c = read_char(tokenizer);
    if (!is_punctuation(c)) {
        return 0;
    }
    
    // Consume the punctuation
    consume_char(tokenizer);
    
    // Create token
    token->type = TOKEN_PUNCTUATION;
    token->position.offset = start;
    token->position.length = 1;
    token->text = strndup(tokenizer->source->content + start, 1);
    token->length = 1;
    
    return 1;
}

static size_t scan_whitespace(Tokenizer* tokenizer, Token* token) {
    size_t start = tokenizer->position;
    size_t length = 0;
    
    // Check if current character is whitespace
    char c = read_char(tokenizer);
    if (!isspace(c)) {
        return 0;
    }
    
    // Consume all consecutive whitespace
    while (isspace(peek_char(tokenizer, 0))) {
        consume_char(tokenizer);
        length++;
    }
    
    // Consume the first whitespace character too
    consume_char(tokenizer);
    length++;
    
    // Create token
    token->type = TOKEN_WHITESPACE;
    token->position.offset = start;
    token->position.length = length;
    token->text = strndup(tokenizer->source->content + start, length);
    token->length = length;
    
    return length;
}

static size_t scan_comment(Tokenizer* tokenizer, Token* token) {
    size_t start = tokenizer->position;
    size_t length = 0;
    
    // Check for comment start: // or /*
    char c = read_char(tokenizer);
    if (c != '/' || (peek_char(tokenizer, 0) != '/' && peek_char(tokenizer, 0) != '*')) {
        return 0;
    }
    
    // Consume the first '/'
    consume_char(tokenizer);
    length++;
    
    // Determine comment type and consume accordingly
    char next = consume_char(tokenizer);
    length++;
    
    if (next == '/') {
        // Line comment: //
        // Read until end of line or end of file
        while (peek_char(tokenizer, 0) != '\n' && peek_char(tokenizer, 0) != '\0') {
            consume_char(tokenizer);
            length++;
        }
    } else if (next == '*') {
        // Block comment: /* ... */
        // Read until */ or end of file
        bool found_end = false;
        
        while (!found_end && peek_char(tokenizer, 0) != '\0') {
            if (peek_char(tokenizer, 0) == '*' && peek_char(tokenizer, 1) == '/') {
                // Found comment end
                consume_char(tokenizer); // Consume *
                consume_char(tokenizer); // Consume /
                length += 2;
                found_end = true;
            } else {
                consume_char(tokenizer);
                length++;
            }
        }
        
        if (!found_end) {
            // Error: unclosed block comment
            // But we'll handle it gracefully by returning what we have
        }
    }
    
    // Create token
    token->type = TOKEN_COMMENT;
    token->position.offset = start;
    token->position.length = length;
    token->text = strndup(tokenizer->source->content + start, length);
    token->length = length;
    
    return length;
}

// Get the next token from the source
bool braggi_tokenizer_next(Tokenizer* tokenizer) {
    if (!tokenizer) return false;
    
    // If we have a prepared next token, use it
    if (tokenizer->has_next) {
        // Free current token resources if needed
        if (tokenizer->current_token.text) {
            free(tokenizer->current_token.text);
        }
        
        if (tokenizer->current_token.type == TOKEN_LITERAL_STRING || 
            tokenizer->current_token.type == TOKEN_LITERAL_CHAR) {
            if (tokenizer->current_token.string_value) {
                free(tokenizer->current_token.string_value);
            }
        }
        
        // Move next token to current
        tokenizer->current_token = tokenizer->next_token;
        // Clear next token to prevent double-free
        memset(&tokenizer->next_token, 0, sizeof(Token));
        tokenizer->has_next = false;
        
        return true;
    }
    
    // Free current token resources if needed
    if (tokenizer->current_token.text) {
        free(tokenizer->current_token.text);
    }
    
    if (tokenizer->current_token.type == TOKEN_LITERAL_STRING || 
        tokenizer->current_token.type == TOKEN_LITERAL_CHAR) {
        if (tokenizer->current_token.string_value) {
            free(tokenizer->current_token.string_value);
        }
    }
    
    // Reset current token
    memset(&tokenizer->current_token, 0, sizeof(Token));
    
    // Check if we're at the end of the file
    if (tokenizer->position >= tokenizer->source->length) {
        tokenizer->current_token.type = TOKEN_EOF;
        tokenizer->current_token.position.offset = tokenizer->position;
        tokenizer->current_token.position.length = 0;
        tokenizer->current_token.text = NULL;
        tokenizer->current_token.length = 0;
        return true;
    }
    
    // Try to scan for different token types
    size_t length = 0;
    
    // Set file_id for position
    tokenizer->current_token.position.file_id = 0; // Assume first file for now
    
    // Try each token type scanner
    if ((length = scan_whitespace(tokenizer, &tokenizer->current_token)) > 0 ||
        (length = scan_comment(tokenizer, &tokenizer->current_token)) > 0 ||
        (length = scan_identifier(tokenizer, &tokenizer->current_token)) > 0 ||
        (length = scan_number(tokenizer, &tokenizer->current_token)) > 0 ||
        (length = scan_string_literal(tokenizer, &tokenizer->current_token)) > 0 ||
        (length = scan_char_literal(tokenizer, &tokenizer->current_token)) > 0 ||
        (length = scan_operator(tokenizer, &tokenizer->current_token)) > 0 ||
        (length = scan_punctuation(tokenizer, &tokenizer->current_token))) {
        
        // Successfully scanned a token
        
        // Store in vector if available
        if (tokenizer->tokens) {
            // Duplicate the token for storage
            Token* stored_token = (Token*)malloc(sizeof(Token));
            if (stored_token) {
                memcpy(stored_token, &tokenizer->current_token, sizeof(Token));
                
                // Duplicate the text field since the original will be freed
                if (stored_token->text) {
                    stored_token->text = strdup(stored_token->text);
                }
                
                // Duplicate string value if present
                if ((stored_token->type == TOKEN_LITERAL_STRING || 
                     stored_token->type == TOKEN_LITERAL_CHAR) && 
                    stored_token->string_value) {
                    stored_token->string_value = strdup(stored_token->string_value);
                }
                
                braggi_vector_push_back(tokenizer->tokens, stored_token);
            }
        }
        
        return true;
    }
    
    // If we get here, we encountered an invalid token
    tokenizer->current_token.type = TOKEN_INVALID;
    tokenizer->current_token.position.offset = tokenizer->position;
    tokenizer->current_token.position.length = 1;
    if (tokenizer->position < tokenizer->source->length) {
        tokenizer->current_token.text = strndup(tokenizer->source->content + tokenizer->position, 1);
        tokenizer->current_token.length = 1;
        tokenizer->position++; // Skip the invalid character
    } else {
        tokenizer->current_token.text = NULL;
        tokenizer->current_token.length = 0;
    }
    
    return false; // Failed to scan a valid token
}

// Peek at the next token without consuming it
Token* braggi_tokenizer_peek(Tokenizer* tokenizer) {
    if (!tokenizer) return NULL;
    
    // If we already have a next token prepared, return it
    if (tokenizer->has_next) {
        return &tokenizer->next_token;
    }
    
    // Save current state
    size_t saved_position = tokenizer->position;
    Token saved_token = tokenizer->current_token;
    
    // Clear the text pointer in saved_token to prevent it from being freed
    saved_token.text = NULL;
    saved_token.string_value = NULL;
    
    // Get next token
    braggi_tokenizer_next(tokenizer);
    
    // Copy it to next_token
    tokenizer->next_token = tokenizer->current_token;
    
    // Clear text pointer in current_token to prevent double-free
    tokenizer->current_token.text = NULL;
    tokenizer->current_token.string_value = NULL;
    
    // Restore state
    tokenizer->position = saved_position;
    tokenizer->current_token = saved_token;
    
    // Set has_next flag
    tokenizer->has_next = true;
    
    return &tokenizer->next_token;
}

// Get the current token
Token* braggi_tokenizer_current(Tokenizer* tokenizer) {
    if (!tokenizer) return NULL;
    return &tokenizer->current_token;
}

// Check if current token is a specific keyword
bool braggi_tokenizer_is_keyword(Tokenizer* tokenizer, const char* keyword) {
    if (!tokenizer || !keyword) return false;
    
    Token* token = braggi_tokenizer_current(tokenizer);
    return (token->type == TOKEN_KEYWORD && 
            strcmp(token->text, keyword) == 0);
}

// Check if current token is a specific operator
bool braggi_tokenizer_is_operator(Tokenizer* tokenizer, const char* op) {
    if (!tokenizer || !op) return false;
    
    Token* token = braggi_tokenizer_current(tokenizer);
    return (token->type == TOKEN_OPERATOR && 
            strcmp(token->text, op) == 0);
}

// Expect a specific token type, consuming it if found
bool braggi_tokenizer_expect(Tokenizer* tokenizer, TokenType type) {
    if (!tokenizer) return false;
    
    Token* token = braggi_tokenizer_current(tokenizer);
    if (token->type == type) {
        braggi_tokenizer_next(tokenizer);
        return true;
    }
    
    return false;
}

// Expect a specific keyword, consuming it if found
bool braggi_tokenizer_expect_keyword(Tokenizer* tokenizer, const char* keyword) {
    if (!tokenizer || !keyword) return false;
    
    if (braggi_tokenizer_is_keyword(tokenizer, keyword)) {
        braggi_tokenizer_next(tokenizer);
        return true;
    }
    
    return false;
}

// Expect a specific operator, consuming it if found
bool braggi_tokenizer_expect_operator(Tokenizer* tokenizer, const char* op) {
    if (!tokenizer || !op) return false;
    
    if (braggi_tokenizer_is_operator(tokenizer, op)) {
        braggi_tokenizer_next(tokenizer);
        return true;
    }
    
    return false;
}

// Convert a token type to string for debugging
const char* braggi_token_type_string(TokenType type) {
    switch (type) {
        case TOKEN_INVALID: return "INVALID";
        case TOKEN_IDENTIFIER: return "IDENTIFIER";
        case TOKEN_KEYWORD: return "KEYWORD";
        case TOKEN_LITERAL_INT: return "LITERAL_INT";
        case TOKEN_LITERAL_FLOAT: return "LITERAL_FLOAT";
        case TOKEN_LITERAL_STRING: return "LITERAL_STRING";
        case TOKEN_LITERAL_CHAR: return "LITERAL_CHAR";
        case TOKEN_OPERATOR: return "OPERATOR";
        case TOKEN_PUNCTUATION: return "PUNCTUATION";
        case TOKEN_COMMENT: return "COMMENT";
        case TOKEN_WHITESPACE: return "WHITESPACE";
        case TOKEN_EOF: return "EOF";
        default: return "UNKNOWN";
    }
}

/*
 * The following function is commented out as it requires the State type
 * which is not properly defined yet. This will be implemented later.
 *
// Convert a token to a state for the entropy field
State* braggi_token_to_state(Token* token) {
    if (!token) return NULL;
    
    // Allocate a new state
    State* state = (State*)malloc(sizeof(State));
    if (!state) return NULL;
    
    // Initialize state fields
    state->id = 0;      // This will be set by the entropy field
    state->type = token->type;
    
    // Copy token data to state data
    switch (token->type) {
        case TOKEN_IDENTIFIER:
            state->data = strdup(token->text);
            break;
        case TOKEN_KEYWORD:
            state->data = strdup(token->text);
            break;
        case TOKEN_LITERAL_INT:
            {
                int64_t* value = (int64_t*)malloc(sizeof(int64_t));
                if (value) {
                    *value = token->int_value;
                    state->data = value;
                } else {
                    state->data = NULL;
                }
            }
            break;
        case TOKEN_LITERAL_FLOAT:
            {
                double* value = (double*)malloc(sizeof(double));
                if (value) {
                    *value = token->float_value;
                    state->data = value;
                } else {
                    state->data = NULL;
                }
            }
            break;
        case TOKEN_LITERAL_STRING:
        case TOKEN_LITERAL_CHAR:
            state->data = strdup(token->string_value);
            break;
        case TOKEN_OPERATOR:
        case TOKEN_PUNCTUATION:
            state->data = strdup(token->text);
            break;
        default:
            state->data = NULL;
            break;
    }
    
    return state;
}
*/ 