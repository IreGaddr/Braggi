/*
 * Braggi - Symbol Table Header
 * 
 * "A good symbol table is like a well-organized ranch - 
 * every symbol has its place, and you can find 'em all in a jiffy!"
 */

#ifndef BRAGGI_SYMBOL_TABLE_H
#define BRAGGI_SYMBOL_TABLE_H

#include <stdint.h>
#include <stdbool.h>

// Forward declarations
typedef struct SymbolTable SymbolTable;
typedef struct Symbol Symbol;

// Symbol types (exposed for public use)
typedef enum SymbolType {
    SYMBOL_UNKNOWN = 0,
    SYMBOL_VARIABLE,
    SYMBOL_FUNCTION,
    SYMBOL_TYPE,
    SYMBOL_CONSTANT,
    SYMBOL_MODULE,
    SYMBOL_NAMESPACE
} SymbolType;

/**
 * Create a new symbol table
 * 
 * @return A newly allocated symbol table, or NULL on failure
 */
SymbolTable* braggi_symbol_table_create(void);

/**
 * Destroy a symbol table and all its contents
 * 
 * @param table The symbol table to destroy
 */
void braggi_symbol_table_destroy(SymbolTable* table);

/**
 * Enter a new scope
 * 
 * @param table The symbol table
 * @param name Optional name for the scope (can be NULL)
 * @return The ID of the new scope, or 0 on failure
 */
uint32_t braggi_symbol_table_enter_scope(SymbolTable* table, const char* name);

/**
 * Exit the current scope, returning to the parent scope
 * 
 * @param table The symbol table
 * @return The ID of the parent scope, or 0 if in global scope or on failure
 */
uint32_t braggi_symbol_table_exit_scope(SymbolTable* table);

/**
 * Add a symbol to the current scope
 * 
 * @param table The symbol table
 * @param name The name of the symbol
 * @param type The type of symbol
 * @return A pointer to the created symbol, or NULL on failure or if already exists
 */
Symbol* braggi_symbol_table_add_symbol(SymbolTable* table, const char* name, SymbolType type);

/**
 * Look up a symbol in the current scope or parent scopes
 * 
 * @param table The symbol table
 * @param name The name of the symbol to look up
 * @return A pointer to the symbol if found, or NULL if not found
 */
Symbol* braggi_symbol_table_lookup(SymbolTable* table, const char* name);

/**
 * Get the name of a symbol
 * 
 * @param symbol The symbol
 * @return The name of the symbol, or NULL if invalid
 */
const char* braggi_symbol_get_name(Symbol* symbol);

/**
 * Get the type of a symbol
 * 
 * @param symbol The symbol
 * @return The type of the symbol, or SYMBOL_UNKNOWN if invalid
 */
SymbolType braggi_symbol_get_type(Symbol* symbol);

/**
 * Set custom data for a symbol
 * 
 * @param symbol The symbol
 * @param data The data to set
 * @return true if successful, false otherwise
 */
bool braggi_symbol_set_data(Symbol* symbol, void* data);

/**
 * Get the custom data for a symbol
 * 
 * @param symbol The symbol
 * @return The custom data, or NULL if not set or symbol is invalid
 */
void* braggi_symbol_get_data(Symbol* symbol);

#endif /* BRAGGI_SYMBOL_TABLE_H */ 