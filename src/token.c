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
#include "braggi/entropy.h" /* Added to access EntropyState */

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
    // Region and regime keywords
    "region", "regime", 
    
    // Function and variable declarations
    "func", "fn", "var", "const",
    
    // Control flow
    "if", "else", "while", "for", "return", "break", "continue",
    
    // Quantum operations
    "collapse", "superpose", "periscope",
    
    // Collection types
    "fifo", "filo", "seq", "rand", 
    
    // I/O operations
    "in", "out", "print", "println",
    
    // Boolean constants
    "true", "false", 
    
    // Null value
    "null",
    
    // Type keywords
    "int", "float", "string", "char", "bool", "void",
    
    // Case related
    "switch", "case", "default",
    
    // Special operators
    "as", "in", 
    
    // End of list marker
    NULL
};

// Check if a string is a keyword
static bool is_keyword(const char* text, size_t length) {
    if (!text || length == 0) {
        return false;
    }
    
    // In a real-world implementation, a hash table or trie would be more efficient
    for (int i = 0; keywords[i] != NULL; i++) {
        size_t keyword_len = strlen(keywords[i]);
        
        // Length must match exactly
        if (length != keyword_len) {
            continue;
        }
        
        // Compare the strings (case-sensitive)
        if (strncmp(text, keywords[i], length) == 0) {
            fprintf(stderr, "DEBUG: Identified keyword: '%s'\n", keywords[i]);
            return true;
        }
    }
    
    return false;
}

// Create a new tokenizer for a source file
Tokenizer* braggi_tokenizer_create(Source* source) {
    if (!source) {
        fprintf(stderr, "DEBUG: Source is NULL in braggi_tokenizer_create\n");
        return NULL;
    }
    
    fprintf(stderr, "DEBUG: Creating tokenizer for source '%s' with %u lines\n", 
            source->filename ? source->filename : "(unnamed)", 
            source->num_lines);
    
    Tokenizer* tokenizer = (Tokenizer*)malloc(sizeof(Tokenizer));
    if (!tokenizer) {
        fprintf(stderr, "DEBUG: Failed to allocate memory for tokenizer\n");
        return NULL;
    }
    
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
        fprintf(stderr, "DEBUG: Failed to read first token\n");
        braggi_tokenizer_destroy(tokenizer);
        return NULL;
    }
    
    fprintf(stderr, "DEBUG: Successfully created and primed tokenizer\n");
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

// Helper function to read a character from a Source structure
static char read_char(Tokenizer* tokenizer) {
    if (!tokenizer || !tokenizer->source) {
        return '\0';
    }
    
    Source* source = tokenizer->source;
    size_t pos = tokenizer->position;
    
    // Calculate the line number and column
    unsigned int line = 0;
    size_t current_pos = 0;
    
    for (unsigned int i = 0; i < source->num_lines; i++) {
        const char* line_text = source->lines[i];
        if (!line_text) continue;
        
        size_t line_length = strlen(line_text);
        
        // Check if position is in this line's range
        if (pos >= current_pos && pos < current_pos + line_length) {
            // It's in this line, return the character
            return line_text[pos - current_pos];
        }
        
        // Check if position is at the newline after this line
        if (pos == current_pos + line_length) {
            return '\n';
        }
        
        // Move position counter to next line
        current_pos += line_length + 1; // +1 for newline
    }
    
    // Position is past the end of the file
    return '\0';
}

// Helper function to peek ahead a few characters
static char peek_char(Tokenizer* tokenizer, size_t offset) {
    if (!tokenizer || !tokenizer->source) {
        return '\0';
    }
    
    // Save current position
    size_t original_pos = tokenizer->position;
    
    // Move to the peeked position
    tokenizer->position = original_pos + offset;
    
    // Read the character
    char ch = read_char(tokenizer);
    
    // Restore original position
    tokenizer->position = original_pos;
    
    return ch;
}

// Helper function to consume a character and advance
static char consume_char(Tokenizer* tokenizer) {
    if (!tokenizer || !tokenizer->source) {
        return '\0';
    }
    
    char ch = read_char(tokenizer);
    tokenizer->position++;
    return ch;
}

// Helper function to get a source segment for creating token text
static char* get_source_segment(Tokenizer* tokenizer, size_t start, size_t length) {
    if (!tokenizer || !tokenizer->source || length == 0) {
        return NULL;
    }
    
    // Debug info for extraction
    fprintf(stderr, "DEBUG: Extracting segment from pos %zu, length %zu\n", start, length);
    
    char* result = (char*)malloc(length + 1);
    if (!result) {
        fprintf(stderr, "DEBUG: Failed to allocate memory for segment\n");
        return NULL;
    }
    
    // Save current position
    size_t original_pos = tokenizer->position;
    
    // Move to start position
    tokenizer->position = start;
    
    // Read characters one by one
    for (size_t i = 0; i < length; i++) {
        char c = read_char(tokenizer);
        result[i] = c;
        tokenizer->position++;
    }
    
    // Ensure null termination
    result[length] = '\0';
    
    // Restore original position
    tokenizer->position = original_pos;
    
    fprintf(stderr, "DEBUG: Extracted segment: '%s'\n", result);
    return result;
}

// Helper function to get total length of Source content
static size_t get_source_total_length(Source* source) {
    if (!source) return 0;
    
    size_t total_length = 0;
    for (unsigned int i = 0; i < source->num_lines; i++) {
        const char* line = source->lines[i];
        if (line) {
            total_length += strlen(line) + 1; // +1 for newline
        } else {
            total_length++; // Just count the newline for empty lines
        }
    }
    
    return total_length;
}

// Scan an identifier token
static size_t scan_identifier(Tokenizer* tokenizer, Token* token) {
    size_t start_position = tokenizer->position;
    char c = read_char(tokenizer);
    
    // Check if this is a valid identifier start
    if (!is_identifier_start(c)) {
        return 0;
    }
    
    // Consume all valid identifier parts
    size_t length = 1;
    while (is_identifier_part(peek_char(tokenizer, 0))) {
        consume_char(tokenizer);
        length++;
    }
    
    // Get the identifier text
    char* text = get_source_segment(tokenizer, start_position, length);
    if (!text) {
        return 0;
    }
    
    // Check if this is a keyword
    bool is_kw = is_keyword(text, length);
    
    // Set token data
    token->type = is_kw ? TOKEN_KEYWORD : TOKEN_IDENTIFIER;
    token->position.offset = start_position;
    token->position.length = length;
    token->text = text;
    token->length = length;
    
    return length;
}

// Scan a number token (integer or float)
static size_t scan_number(Tokenizer* tokenizer, Token* token) {
    size_t start_position = tokenizer->position;
    char c = read_char(tokenizer);
    
    // Check if this is a valid number start
    if (!isdigit(c) && c != '.') {
        return 0;
    }
    
    // If it starts with a dot, there must be a digit after it
    if (c == '.' && !isdigit(peek_char(tokenizer, 0))) {
        return 0;
    }
    
    // Assume integer until proven otherwise
    bool is_float = (c == '.');
    size_t length = 1;
    
    // Consume digits
    while (isdigit(peek_char(tokenizer, 0))) {
        consume_char(tokenizer);
        length++;
    }
    
    // Check for decimal point (if not already encountered)
    if (!is_float && peek_char(tokenizer, 0) == '.') {
        // We've found a decimal point
        is_float = true;
        consume_char(tokenizer);
        length++;
        
        // Consume fractional digits
        while (isdigit(peek_char(tokenizer, 0))) {
            consume_char(tokenizer);
            length++;
        }
    }
    
    // Check for exponent part
    char exp_char = peek_char(tokenizer, 0);
    if (exp_char == 'e' || exp_char == 'E') {
        is_float = true; // Numbers with exponents are considered floating point
        consume_char(tokenizer);
        length++;
        
        // Optional sign
        char sign_char = peek_char(tokenizer, 0);
        if (sign_char == '+' || sign_char == '-') {
            consume_char(tokenizer);
            length++;
        }
        
        // Must have at least one digit after exponent
        if (!isdigit(peek_char(tokenizer, 0))) {
            // Invalid exponent, backtrack
            tokenizer->position = start_position;
            return 0;
        }
        
        // Consume exponent digits
        while (isdigit(peek_char(tokenizer, 0))) {
            consume_char(tokenizer);
            length++;
        }
    }
    
    // Get the number text
    char* text = get_source_segment(tokenizer, start_position, length);
    if (!text) {
        return 0;
    }
    
    // Set token data
    token->type = is_float ? TOKEN_LITERAL_FLOAT : TOKEN_LITERAL_INT;
    token->position.offset = start_position;
    token->position.length = length;
    token->text = text;
    token->length = length;
    
    // Parse the value
    if (is_float) {
        token->float_value = atof(text);
    } else {
        token->int_value = atol(text);
    }
    
    return length;
}

// Scan a string literal token
static size_t scan_string_literal(Tokenizer* tokenizer, Token* token) {
    size_t start_position = tokenizer->position;
    char c = read_char(tokenizer);
    
    // Check if this is a string start
    if (c != '"') {
        return 0;
    }
    
    // Consume all characters until closing quote or end of input
    size_t length = 1;
    bool escaped = false;
    
    while (true) {
        char next = peek_char(tokenizer, 0);
        
        // Handle end of file without closing quote
        if (next == '\0') {
            // Invalid string literal, backtrack
            tokenizer->position = start_position;
            return 0;
        }
        
        consume_char(tokenizer);
        length++;
        
        if (escaped) {
            // This character is escaped, so it doesn't terminate the string
            escaped = false;
            continue;
        }
        
        if (next == '\\') {
            // Escape the next character
            escaped = true;
            continue;
        }
        
        if (next == '"') {
            // Found the closing quote
            break;
        }
    }
    
    // Get the string text (including quotes)
    char* full_text = get_source_segment(tokenizer, start_position, length);
    if (!full_text) {
        return 0;
    }
    
    // Set token data
    token->type = TOKEN_LITERAL_STRING;
    token->position.offset = start_position;
    token->position.length = length;
    token->text = full_text;
    token->length = length;
    
    // Extract string value (without quotes)
    // We'll handle escape sequences properly later
    token->string_value = (char*)malloc(length - 1);
    if (token->string_value) {
        strncpy(token->string_value, full_text + 1, length - 2);
        token->string_value[length - 2] = '\0';
    }
    
    return length;
}

// Scan a character literal token
static size_t scan_char_literal(Tokenizer* tokenizer, Token* token) {
    size_t start_position = tokenizer->position;
    char c = read_char(tokenizer);
    
    // Check if this is a character literal start
    if (c != '\'') {
        return 0;
    }
    
    // Consume all characters until closing quote or end of input
    size_t length = 1;
    bool escaped = false;
    
    while (true) {
        char next = peek_char(tokenizer, 0);
        
        // Handle end of file without closing quote
        if (next == '\0') {
            // Invalid character literal, backtrack
            tokenizer->position = start_position;
            return 0;
        }
        
        consume_char(tokenizer);
        length++;
        
        if (escaped) {
            // This character is escaped, so it doesn't terminate the literal
            escaped = false;
            continue;
        }
        
        if (next == '\\') {
            // Escape the next character
            escaped = true;
            continue;
        }
        
        if (next == '\'') {
            // Found the closing quote
            break;
        }
    }
    
    // Get the character literal text (including quotes)
    char* full_text = get_source_segment(tokenizer, start_position, length);
    if (!full_text) {
        return 0;
    }
    
    // Set token data
    token->type = TOKEN_LITERAL_CHAR;
    token->position.offset = start_position;
    token->position.length = length;
    token->text = full_text;
    token->length = length;
    
    // Extract character value (without quotes)
    // We'll handle escape sequences properly later
    token->string_value = (char*)malloc(2);
    if (token->string_value) {
        token->string_value[0] = full_text[1] == '\\' ? full_text[2] : full_text[1];
        token->string_value[1] = '\0';
    }
    token->int_value = (int)token->string_value[0];
    
    return length;
}

// Scan an operator token
static size_t scan_operator(Tokenizer* tokenizer, Token* token) {
    size_t start_position = tokenizer->position;
    char c = read_char(tokenizer);
    
    // Check if this is an operator character
    if (!is_operator_char(c)) {
        return 0;
    }
    
    // Handle multi-character operators
    size_t length = 1;
    char next = peek_char(tokenizer, 0);
    
    // Two-character operators
    if ((c == '+' && next == '+') || // ++
        (c == '-' && next == '-') || // --
        (c == '=' && next == '=') || // ==
        (c == '!' && next == '=') || // !=
        (c == '<' && next == '=') || // <=
        (c == '>' && next == '=') || // >=
        (c == '&' && next == '&') || // &&
        (c == '|' && next == '|') || // ||
        (c == '-' && next == '>') || // ->
        (c == '+' && next == '=') || // +=
        (c == '-' && next == '=') || // -=
        (c == '*' && next == '=') || // *=
        (c == '/' && next == '=') || // /=
        (c == '%' && next == '=') || // %=
        (c == '&' && next == '=') || // &=
        (c == '|' && next == '=') || // |=
        (c == '^' && next == '=') || // ^=
        (c == '<' && next == '<') || // <<
        (c == '>' && next == '>')) { // >>
        consume_char(tokenizer);
        length++;
        
        // Three-character operators
        char third = peek_char(tokenizer, 0);
        if (((c == '<' && next == '<') || (c == '>' && next == '>')) && third == '=') {
            // <<= or >>=
            consume_char(tokenizer);
            length++;
        }
    }
    
    // Get the operator text
    char* text = get_source_segment(tokenizer, start_position, length);
    if (!text) {
        return 0;
    }
    
    // Set token data
    token->type = TOKEN_OPERATOR;
    token->position.offset = start_position;
    token->position.length = length;
    token->text = text;
    token->length = length;
    
    return length;
}

// Scan a punctuation token
static size_t scan_punctuation(Tokenizer* tokenizer, Token* token) {
    size_t start_position = tokenizer->position;
    char c = read_char(tokenizer);
    
    // Check if this is a punctuation character
    if (!is_punctuation(c)) {
        return 0;
    }
    
    // Get the punctuation text
    char* text = get_source_segment(tokenizer, start_position, 1);
    if (!text) {
        return 0;
    }
    
    // Set token data
    token->type = TOKEN_PUNCTUATION;
    token->position.offset = start_position;
    token->position.length = 1;
    token->text = text;
    token->length = 1;
    
    return 1;
}

// Scan whitespace
static size_t scan_whitespace(Tokenizer* tokenizer, Token* token) {
    size_t start_position = tokenizer->position;
    char c = read_char(tokenizer);
    
    // Not whitespace
    if (!isspace(c)) {
        return 0;
    }
    
    // Consume all consecutive whitespace
    size_t length = 1;
    while (isspace(peek_char(tokenizer, 0))) {
        consume_char(tokenizer);
        length++;
    }
    
    // Set token data
    token->type = TOKEN_WHITESPACE;
    token->position.offset = start_position;
    token->position.length = length;
    token->text = get_source_segment(tokenizer, start_position, length);
    token->length = length;
    
    return length;
}

// Scan a comment
static size_t scan_comment(Tokenizer* tokenizer, Token* token) {
    size_t start_position = tokenizer->position;
    char c = read_char(tokenizer);
    
    // Check for comment start
    if (c != '/') {
        return 0;
    }
    
    char next = peek_char(tokenizer, 0);
    if (next != '/' && next != '*') {
        // Not a comment, backtrack
        tokenizer->position = start_position;
        return 0;
    }
    
    consume_char(tokenizer); // Consume the second character
    size_t length = 2;
    
    if (next == '/') {
        // Single-line comment, read until end of line
        while (peek_char(tokenizer, 0) != '\0' && peek_char(tokenizer, 0) != '\n') {
            consume_char(tokenizer);
            length++;
        }
    } else { // next == '*'
        // Multi-line comment, read until */
        bool end_found = false;
        while (!end_found && peek_char(tokenizer, 0) != '\0') {
            char c1 = consume_char(tokenizer);
            length++;
            
            if (c1 == '*' && peek_char(tokenizer, 0) == '/') {
                consume_char(tokenizer); // Consume the closing /
                length++;
                end_found = true;
            }
        }
        
        if (!end_found) {
            // Unterminated comment, backtrack
            tokenizer->position = start_position;
            return 0;
        }
    }
    
    // Get the comment text
    char* text = get_source_segment(tokenizer, start_position, length);
    if (!text) {
        return 0;
    }
    
    // Set token data
    token->type = TOKEN_COMMENT;
    token->position.offset = start_position;
    token->position.length = length;
    token->text = text;
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
    size_t source_length = get_source_total_length(tokenizer->source);
    if (tokenizer->position >= source_length) {
        fprintf(stderr, "DEBUG: Reached end of source at position %zu\n", tokenizer->position);
        tokenizer->current_token.type = TOKEN_EOF;
        tokenizer->current_token.position.offset = tokenizer->position;
        tokenizer->current_token.position.length = 0;
        tokenizer->current_token.text = NULL;
        tokenizer->current_token.length = 0;
        return true;
    }
    
    // Save the original position for token extraction
    size_t start_position = tokenizer->position;
    size_t line_pos = 0;
    size_t column_pos = 0;
    
    // Calculate line and column for the current position
    for (unsigned int i = 0; i < tokenizer->source->num_lines; i++) {
        const char* line_text = tokenizer->source->lines[i];
        if (!line_text) continue;
        
        size_t line_length = strlen(line_text);
        
        if (start_position >= line_pos && start_position < line_pos + line_length + 1) {
            // Position is in this line (or at the newline after it)
            line_pos = i + 1; // Line is 1-indexed
            column_pos = start_position - (line_pos - 1) + 1; // Column is 1-indexed
            break;
        }
        
        line_pos += line_length + 1; // +1 for newline
    }
    
    // Set file_id for position
    tokenizer->current_token.position.file_id = tokenizer->source->file_id;
    tokenizer->current_token.position.line = line_pos;
    tokenizer->current_token.position.column = column_pos;
    
    // Debug: Show the current character we're about to scan
    char current_char = read_char(tokenizer);
    fprintf(stderr, "DEBUG: Scanning at position %zu (%zu:%zu), char: '%c' (0x%02X)\n", 
            tokenizer->position, line_pos, column_pos, 
            isprint(current_char) ? current_char : '.', 
            (unsigned char)current_char);
    
    // Try each token type scanner
    size_t length = 0;
    
    if ((length = scan_whitespace(tokenizer, &tokenizer->current_token)) > 0) {
        fprintf(stderr, "DEBUG: Scanned whitespace token of length %zu\n", length);
    }
    else if ((length = scan_comment(tokenizer, &tokenizer->current_token)) > 0) {
        fprintf(stderr, "DEBUG: Scanned comment token of length %zu\n", length);
    }
    else if ((length = scan_identifier(tokenizer, &tokenizer->current_token)) > 0) {
        fprintf(stderr, "DEBUG: Scanned identifier token of length %zu\n", length);
    }
    else if ((length = scan_number(tokenizer, &tokenizer->current_token)) > 0) {
        fprintf(stderr, "DEBUG: Scanned number token of length %zu\n", length);
    }
    else if ((length = scan_string_literal(tokenizer, &tokenizer->current_token)) > 0) {
        fprintf(stderr, "DEBUG: Scanned string literal token of length %zu\n", length);
    }
    else if ((length = scan_char_literal(tokenizer, &tokenizer->current_token)) > 0) {
        fprintf(stderr, "DEBUG: Scanned char literal token of length %zu\n", length);
    }
    else if ((length = scan_operator(tokenizer, &tokenizer->current_token)) > 0) {
        fprintf(stderr, "DEBUG: Scanned operator token of length %zu\n", length);
    }
    else if ((length = scan_punctuation(tokenizer, &tokenizer->current_token)) > 0) {
        fprintf(stderr, "DEBUG: Scanned punctuation token of length %zu\n", length);
    }
    else {
        // If we get here, we encountered an invalid token
        fprintf(stderr, "DEBUG: Invalid token at position %zu\n", tokenizer->position);
        tokenizer->current_token.type = TOKEN_INVALID;
        tokenizer->current_token.position.offset = tokenizer->position;
        tokenizer->current_token.position.length = 1;
        
        current_char = read_char(tokenizer);
        if (current_char != '\0') {
            char invalid_char[2] = {current_char, '\0'};
            tokenizer->current_token.text = strdup(invalid_char);
            tokenizer->current_token.length = 1;
            tokenizer->position++; // Skip the invalid character
        } else {
            tokenizer->current_token.text = NULL;
            tokenizer->current_token.length = 0;
        }
        
        return false; // Failed to scan a valid token
    }
    
    // Successfully scanned a token
    fprintf(stderr, "DEBUG: Scanned token: type=%s, text='%s', length=%zu\n", 
            braggi_token_type_string(tokenizer->current_token.type),
            tokenizer->current_token.text ? tokenizer->current_token.text : "(null)",
            tokenizer->current_token.length);
    
    // Advance the position by the token length
    tokenizer->position = start_position + length;
    
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
 * Create a new token
 * 
 * "Creating a new token is like branding a newborn calf -
 * ya gotta make sure it's marked proper so folks know what it is!" - Irish-Texan Token Wisdom
 */
Token* braggi_token_create(TokenType type, char* text, SourcePosition position) {
    Token* token = (Token*)malloc(sizeof(Token));
    if (!token) {
        return NULL;
    }
    
    token->type = type;
    token->text = text;  // Note: The caller is responsible for allocating text
    token->position = position;
    token->length = text ? strlen(text) : 0;
    
    // Initialize the value union to zero
    token->int_value = 0;
    
    return token;
}

/*
 * Destroy a token and free its resources
 * 
 * "When a token's done its job, ya gotta clean up after it -
 * we don't leave memory lyin' around like trash after a rodeo!" - Texas Memory Management
 */
void braggi_token_destroy(Token* token) {
    if (!token) {
        return;
    }
    
    // Free the text if it exists
    if (token->text) {
        free(token->text);
        token->text = NULL;
    }
    
    // Free string_value if it exists and is different from text
    if (token->type == TOKEN_LITERAL_STRING || token->type == TOKEN_LITERAL_CHAR) {
        if (token->string_value && token->string_value != token->text) {
            free(token->string_value);
            token->string_value = NULL;
        }
    }
    
    // Free the token itself
    free(token);
}

// Convert a token to a state for the entropy field
EntropyState* braggi_token_to_state(Token* token) {
    if (!token) return NULL;
    
    // Create string label from token type
    char label[128];
    snprintf(label, sizeof(label), "Token(%s): %s", 
             braggi_token_type_string(token->type),
             token->text ? token->text : "(null)");
    
    // Create state data based on token type
    void* state_data = NULL;
    
    switch (token->type) {
        case TOKEN_IDENTIFIER:
        case TOKEN_KEYWORD:
        case TOKEN_OPERATOR:
        case TOKEN_PUNCTUATION:
        case TOKEN_COMMENT:
        case TOKEN_WHITESPACE:
            // For these types, just duplicate the text
            state_data = token->text ? strdup(token->text) : NULL;
            break;
            
        case TOKEN_LITERAL_INT: {
            // Create a copy of the integer value
            int64_t* value = (int64_t*)malloc(sizeof(int64_t));
            if (value) {
                *value = token->int_value;
                state_data = value;
            }
            break;
        }
            
        case TOKEN_LITERAL_FLOAT: {
            // Create a copy of the float value
            double* value = (double*)malloc(sizeof(double));
            if (value) {
                *value = token->float_value;
                state_data = value;
            }
            break;
        }
            
        case TOKEN_LITERAL_STRING:
        case TOKEN_LITERAL_CHAR:
            // For string literals, duplicate the string value
            state_data = token->string_value ? strdup(token->string_value) : NULL;
            break;
            
        default:
            state_data = NULL;
            break;
    }
    
    // Calculate default probability based on token type
    uint32_t probability = 100; // Default high probability
    
    switch (token->type) {
        case TOKEN_INVALID:
            probability = 10;   // Invalid tokens have low probability
            break;
        case TOKEN_WHITESPACE:
        case TOKEN_COMMENT:
            probability = 70;   // Whitespace and comments are somewhat arbitrary
            break;
        case TOKEN_LITERAL_INT:
        case TOKEN_LITERAL_FLOAT:
        case TOKEN_LITERAL_STRING:
        case TOKEN_LITERAL_CHAR:
            probability = 85;   // Literals are fairly certain
            break;
        case TOKEN_IDENTIFIER:
            probability = 80;   // Identifiers might be mistaken
            break;
        case TOKEN_KEYWORD:
        case TOKEN_OPERATOR:
        case TOKEN_PUNCTUATION:
            probability = 95;   // Keywords, operators, and punctuation are quite certain
            break;
        case TOKEN_EOF:
            probability = 100;  // EOF is certain
            break;
    }
    
    // Create and return state using the EntropyState constructor
    return braggi_entropy_state_create(token->type, token->type, strdup(label), state_data, probability);
}

/*
 * Tokenize an entire source file and store all tokens in a vector
 * 
 * "Roundin' up all the tokens in one go is like a cattle drive - 
 * y'all gotta get every last one of 'em into the corral!" - Texan programmer wisdom
 */
Vector* braggi_tokenize_all(Source* source, bool skip_whitespace, bool skip_comments) {
    if (!source) {
        fprintf(stderr, "ERROR: Source is NULL in braggi_tokenize_all\n");
        return NULL;
    }
    
    fprintf(stderr, "INFO: Starting tokenization of %s with %u lines\n", 
            source->filename ? source->filename : "(unnamed)", 
            source->num_lines);
    
    // Create a tokenizer
    Tokenizer* tokenizer = braggi_tokenizer_create(source);
    if (!tokenizer) {
        fprintf(stderr, "ERROR: Failed to create tokenizer in braggi_tokenize_all\n");
        return NULL;
    }
    
    // Create a vector to store tokens
    Vector* tokens = braggi_vector_create(sizeof(Token*));
    if (!tokens) {
        fprintf(stderr, "ERROR: Failed to create token vector in braggi_tokenize_all\n");
        braggi_tokenizer_destroy(tokenizer);
        return NULL;
    }
    
    // Pre-reserve capacity to avoid reallocations (optional optimization)
    if (!braggi_vector_reserve(tokens, 256)) {
        fprintf(stderr, "WARNING: Failed to reserve capacity for token vector\n");
        // Continue anyway, it will grow as needed
    }
    
    // Attach the vector to the tokenizer (for token storage in next())
    tokenizer->tokens = tokens;
    
    // Process all tokens
    size_t token_count = 0;
    size_t error_count = 0;
    size_t whitespace_count = 0;
    size_t comment_count = 0;
    
    fprintf(stderr, "DEBUG: Beginning token processing loop\n");
    do {
        Token* token = braggi_tokenizer_current(tokenizer);
        if (!token) {
            fprintf(stderr, "ERROR: Received NULL token during tokenization\n");
            continue;
        }
        
        // Count token types and possibly skip
        if (token->type == TOKEN_WHITESPACE) {
            whitespace_count++;
            if (skip_whitespace) {
                // Skip this token by removing it from the vector
                if (tokens->size > 0) {
                    tokens->size--; // Decrement the vector size to "remove" the last token
                }
            }
        } else if (token->type == TOKEN_COMMENT) {
            comment_count++;
            if (skip_comments) {
                // Skip this token by removing it from the vector
                if (tokens->size > 0) {
                    tokens->size--; // Decrement the vector size to "remove" the last token
                }
            }
        } else if (token->type == TOKEN_INVALID) {
            error_count++;
            fprintf(stderr, "WARNING: Invalid token at line %u, column %u: '%s'\n",
                    token->position.line, token->position.column,
                    token->text ? token->text : "(null)");
        }
        
        token_count++;
        
        // Check for EOF
        if (token->type == TOKEN_EOF) {
            fprintf(stderr, "DEBUG: Reached EOF after %zu tokens (%zu errors, %zu whitespace, %zu comments)\n",
                    token_count, error_count, whitespace_count, comment_count);
            break;
        }
    } while (braggi_tokenizer_next(tokenizer));
    
    // Detach the vector from the tokenizer to prevent it from being destroyed
    Vector* result = tokenizer->tokens;
    tokenizer->tokens = NULL;
    
    // Destroy the tokenizer
    braggi_tokenizer_destroy(tokenizer);
    
    // Report stats
    fprintf(stderr, "INFO: Tokenization complete: %zu tokens total (%zu in final vector, %zu errors)\n", 
            token_count, braggi_vector_size(result), error_count);
    
    // If we had errors and the result is empty, return NULL
    if (error_count > 0 && braggi_vector_size(result) == 0) {
        fprintf(stderr, "ERROR: Tokenization failed - no valid tokens found\n");
        braggi_vector_destroy(result);
        return NULL;
    }
    
    return result;
} 