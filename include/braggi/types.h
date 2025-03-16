/*
 * Braggi - Common Type Definitions
 * 
 * "You can tell a lot about a programmer by their types, 
 * just like you can tell a lot about a cowboy by their boots!" 
 * - Texas Computing Wisdom
 */

#ifndef BRAGGI_TYPES_H
#define BRAGGI_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Forward declarations of structures to avoid circular includes
typedef struct BraggiContext BraggiContext;
typedef struct EntropyField EntropyField;
typedef struct Cell Cell;
typedef struct State State;
typedef struct Region Region;
typedef struct SourceFile SourceFile;
typedef struct SourcePosition SourcePosition;
typedef struct ErrorHandler ErrorHandler;
typedef struct ErrorContext ErrorContext;

// Version information
#define BRAGGI_VERSION_MAJOR 0
#define BRAGGI_VERSION_MINOR 1
#define BRAGGI_VERSION_PATCH 0

// Compiler state
typedef enum {
    COMPILER_STATE_INIT,
    COMPILER_STATE_PARSING,
    COMPILER_STATE_ANALYSIS,
    COMPILER_STATE_GENERATION,
    COMPILER_STATE_COMPLETE,
    COMPILER_STATE_ERROR
} CompilerState;

// Type system foundational types
typedef enum {
    TYPE_VOID,
    TYPE_BOOL,
    TYPE_INT,
    TYPE_UINT,
    TYPE_FLOAT,
    TYPE_STRING,
    TYPE_ARRAY,
    TYPE_STRUCT,
    TYPE_ENUM,
    TYPE_FUNCTION,
    TYPE_REGION,
    TYPE_REFERENCE
} BaseType;

// Memory regime types
typedef enum {
    REGIME_FIFO,    // First-In-First-Out
    REGIME_FILO,    // First-In-Last-Out (stack-like)
    REGIME_SEQ,     // Sequential access
    REGIME_RAND     // Random access
} RegimeType;

// State flags
typedef enum {
    STATE_FLAG_NONE       = 0,
    STATE_FLAG_TERMINAL   = 1 << 0,  // Terminal symbol
    STATE_FLAG_NONTERMINAL = 1 << 1,  // Non-terminal symbol
    STATE_FLAG_KEYWORD    = 1 << 2,  // Language keyword
    STATE_FLAG_OPERATOR   = 1 << 3,  // Operator
    STATE_FLAG_IDENTIFIER = 1 << 4,  // Identifier
    STATE_FLAG_LITERAL    = 1 << 5,  // Literal value
    STATE_FLAG_TYPE       = 1 << 6,  // Type name
    STATE_FLAG_VARIABLE   = 1 << 7,  // Variable
    STATE_FLAG_FUNCTION   = 1 << 8,  // Function
    STATE_FLAG_PARAMETER  = 1 << 9,  // Function parameter
    STATE_FLAG_STATEMENT  = 1 << 10, // Statement
    STATE_FLAG_EXPRESSION = 1 << 11  // Expression
} StateFlags;

#endif /* BRAGGI_TYPES_H */ 