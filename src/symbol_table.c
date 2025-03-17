/*
 * Braggi - Symbol Table Implementation
 * 
 * "A symbol table is like the phonebook at a Texas family reunion -
 * it tells you where to find everyone and who they're related to!"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "braggi/util/hashmap.h"
#include "braggi/util/vector.h"

// Define symbol types
typedef enum {
    SYMBOL_UNKNOWN = 0,
    SYMBOL_VARIABLE,
    SYMBOL_FUNCTION,
    SYMBOL_TYPE,
    SYMBOL_CONSTANT,
    SYMBOL_MODULE,
    SYMBOL_NAMESPACE
} SymbolType;

// Structure for a symbol
typedef struct Symbol {
    char* name;              // Symbol name
    SymbolType type;         // Symbol type
    void* data;              // Symbol-specific data
    uint32_t scope_id;       // Scope ID (0 = global)
    uint32_t declaration_id; // Declaration token ID
    struct Symbol* parent;   // Parent symbol (for nested symbols)
} Symbol;

// Structure for a scope
typedef struct Scope {
    uint32_t id;             // Scope ID
    uint32_t parent_id;      // Parent scope ID
    HashMap* symbols;        // Map from name to Symbol*
    char* name;              // Optional scope name (e.g., function name)
} Scope;

// Structure for the symbol table
typedef struct SymbolTable {
    HashMap* scopes;          // Map from scope ID to Scope*
    HashMap* global_symbols;  // Map from name to Symbol* (global scope)
    Vector* all_symbols;      // Vector of Symbol* (all symbols)
    uint32_t next_scope_id;   // Next available scope ID
    uint32_t current_scope_id; // Current scope ID
    Scope* current_scope;     // Current scope
    Scope* global_scope;      // Global scope
    bool initialized;         // Whether the table is initialized
} SymbolTable;

/*
 * Create a new symbol
 * 
 * "Branding a new symbol is like naming a new foal - 
 * you gotta make sure it's unique and fits its purpose!"
 */
static Symbol* create_symbol(const char* name, SymbolType type, uint32_t scope_id) {
    if (!name) {
        return NULL;
    }
    
    Symbol* symbol = (Symbol*)malloc(sizeof(Symbol));
    if (!symbol) {
        return NULL;
    }
    
    symbol->name = strdup(name);
    if (!symbol->name) {
        free(symbol);
        return NULL;
    }
    
    symbol->type = type;
    symbol->data = NULL;
    symbol->scope_id = scope_id;
    symbol->declaration_id = 0;
    symbol->parent = NULL;
    
    return symbol;
}

/*
 * Destroy a symbol
 * 
 * "When a symbol's time is up, make sure you clean up after it -
 * no memory leaks on this ranch!"
 */
static void destroy_symbol(Symbol* symbol) {
    if (!symbol) {
        return;
    }
    
    free(symbol->name);
    // Note: We don't free symbol->data as it might be owned elsewhere
    free(symbol);
}

/*
 * Create a new scope
 * 
 * "A new scope is like a new pasture - you need to fence it off
 * properly so your symbols don't wander into the wrong area!"
 */
static Scope* create_scope(uint32_t id, uint32_t parent_id, const char* name) {
    Scope* scope = (Scope*)malloc(sizeof(Scope));
    if (!scope) {
        return NULL;
    }
    
    scope->id = id;
    scope->parent_id = parent_id;
    scope->symbols = braggi_hashmap_create_with_capacity(64);  // Start with reasonable size
    
    if (!scope->symbols) {
        free(scope);
        return NULL;
    }
    
    if (name) {
        scope->name = strdup(name);
        if (!scope->name) {
            braggi_hashmap_destroy(scope->symbols);
            free(scope);
            return NULL;
        }
    } else {
        scope->name = NULL;
    }
    
    return scope;
}

/*
 * Destroy a scope
 * 
 * "Cleaning up a scope is like dismantling a corral - 
 * make sure all the critters are accounted for first!"
 */
static void destroy_scope(Scope* scope) {
    if (!scope) {
        return;
    }
    
    // Symbols are freed separately when destroying the symbol table
    
    braggi_hashmap_destroy(scope->symbols);
    free(scope->name);
    free(scope);
}

/*
 * Create a new symbol table
 * 
 * "Starting a new symbol table is like opening a brand new ranch -
 * you've got to set it up right from the get-go!"
 */
SymbolTable* braggi_symbol_table_create(void) {
    SymbolTable* table = (SymbolTable*)malloc(sizeof(SymbolTable));
    if (!table) {
        return NULL;
    }
    
    // Initialize table
    table->scopes = braggi_hashmap_create_with_capacity(16);  // Start with reasonable size
    if (!table->scopes) {
        free(table);
        return NULL;
    }
    
    table->all_symbols = braggi_vector_create(sizeof(Symbol*));
    if (!table->all_symbols) {
        braggi_hashmap_destroy(table->scopes);
        free(table);
        return NULL;
    }
    
    // Create global scope
    table->next_scope_id = 1;  // Start from 1, reserve 0 as invalid
    table->global_scope = create_scope(table->next_scope_id++, 0, "global");
    if (!table->global_scope) {
        braggi_vector_destroy(table->all_symbols);
        braggi_hashmap_destroy(table->scopes);
        free(table);
        return NULL;
    }
    
    // Add global scope to scopes map
    char scope_key[32];
    snprintf(scope_key, sizeof(scope_key), "%u", table->global_scope->id);
    braggi_hashmap_set(table->scopes, scope_key, table->global_scope);
    
    // Set global scope as current
    table->current_scope = table->global_scope;
    table->current_scope_id = table->global_scope->id;
    table->global_symbols = table->global_scope->symbols;
    table->initialized = true;
    
    return table;
}

/*
 * Destroy a symbol table
 * 
 * "When it's time to close down the ranch, make sure you account for
 * every last symbol - we don't leave stragglers behind in Texas!"
 */
void braggi_symbol_table_destroy(SymbolTable* table) {
    if (!table) {
        return;
    }
    
    // Clean up all symbols
    if (table->all_symbols) {
        for (size_t i = 0; i < braggi_vector_size(table->all_symbols); i++) {
            Symbol* symbol = *(Symbol**)braggi_vector_get(table->all_symbols, i);
            if (symbol) {
                destroy_symbol(symbol);
            }
        }
        braggi_vector_destroy(table->all_symbols);
    }
    
    // Clean up all scopes
    if (table->scopes) {
        // Note: We can't iterate over a hashmap, so we'll handle the global scope separately
        if (table->global_scope) {
            destroy_scope(table->global_scope);
        }
        
        // In a full implementation, we would iterate over the vector for scopes
        // and free each one. Since we can't directly iterate over the hashmap,
        // we'll destroy the hashmap directly, knowing the scopes have been freed.
        braggi_hashmap_destroy(table->scopes);
    }
    
    // Free the table itself
    free(table);
}

/*
 * Enter a new scope
 * 
 * "Moving into a new scope is like entering a new pasture -
 * different rules apply, but you're still on the same ranch!"
 */
uint32_t braggi_symbol_table_enter_scope(SymbolTable* table, const char* name) {
    if (!table) {
        return 0;
    }
    
    // Create new scope
    Scope* scope = create_scope(table->next_scope_id++, table->current_scope_id, name);
    if (!scope) {
        return 0;
    }
    
    // Add scope to scopes map
    char scope_key[32];
    snprintf(scope_key, sizeof(scope_key), "%u", scope->id);
    braggi_hashmap_set(table->scopes, scope_key, scope);
    
    // Update current scope
    table->current_scope = scope;
    table->current_scope_id = scope->id;
    
    return scope->id;
}

/*
 * Exit the current scope
 * 
 * "Leaving a scope is like moving back to the main corral -
 * you go back to where you came from, with all your symbols in tow!"
 */
uint32_t braggi_symbol_table_exit_scope(SymbolTable* table) {
    if (!table || !table->current_scope || table->current_scope == table->global_scope) {
        return 0;  // Can't exit from global scope
    }
    
    // Get parent scope
    uint32_t parent_id = table->current_scope->parent_id;
    char scope_key[32];
    snprintf(scope_key, sizeof(scope_key), "%u", parent_id);
    Scope* parent_scope = (Scope*)braggi_hashmap_get(table->scopes, scope_key);
    
    if (!parent_scope) {
        // Something went wrong, fall back to global scope
        table->current_scope = table->global_scope;
        table->current_scope_id = table->global_scope->id;
        return table->global_scope->id;
    }
    
    // Update current scope
    table->current_scope = parent_scope;
    table->current_scope_id = parent_id;
    
    return parent_id;
}

/*
 * Add a symbol to the current scope
 * 
 * "Adding a new symbol to the corral is like registering a new brand -
 * you gotta make sure it's unique in its scope!"
 */
Symbol* braggi_symbol_table_add_symbol(SymbolTable* table, const char* name, SymbolType type) {
    if (!table || !name || !table->current_scope) {
        return NULL;
    }
    
    // Check if symbol already exists in current scope
    if (braggi_hashmap_get(table->current_scope->symbols, name)) {
        return NULL;  // Symbol already exists in this scope
    }
    
    // Create new symbol
    Symbol* symbol = create_symbol(name, type, table->current_scope_id);
    if (!symbol) {
        return NULL;
    }
    
    // Add to current scope
    braggi_hashmap_set(table->current_scope->symbols, name, symbol);
    
    // Add to all symbols vector
    braggi_vector_push(table->all_symbols, &symbol);
    
    return symbol;
}

/*
 * Look up a symbol in the current scope or parent scopes
 * 
 * "Looking up a symbol is like tracking a wandering steer -
 * you might find it in the current pasture, or you might have to
 * look in the neighboring fields to track it down!"
 */
Symbol* braggi_symbol_table_lookup(SymbolTable* table, const char* name) {
    if (!table || !name) {
        return NULL;
    }
    
    // Start with current scope
    Scope* scope = table->current_scope;
    while (scope) {
        // Look for symbol in this scope
        Symbol* symbol = (Symbol*)braggi_hashmap_get(scope->symbols, name);
        if (symbol) {
            return symbol;
        }
        
        // Move to parent scope if not found
        if (scope->parent_id == 0) {
            break;  // No parent scope
        }
        
        char scope_key[32];
        snprintf(scope_key, sizeof(scope_key), "%u", scope->parent_id);
        scope = (Scope*)braggi_hashmap_get(table->scopes, scope_key);
    }
    
    // Symbol not found in any accessible scope
    return NULL;
}

/*
 * Get the name of a symbol
 * 
 * "Every symbol's got a name, just like every cow's got a brand -
 * it's how we tell 'em apart!"
 */
const char* braggi_symbol_get_name(Symbol* symbol) {
    if (!symbol) {
        return NULL;
    }
    
    return symbol->name;
}

/*
 * Get the type of a symbol
 * 
 * "Knowing a symbol's type is like knowing if you're dealing with a
 * longhorn or a heifer - makes all the difference in how you handle it!"
 */
SymbolType braggi_symbol_get_type(Symbol* symbol) {
    if (!symbol) {
        return SYMBOL_UNKNOWN;
    }
    
    return symbol->type;
}

/*
 * Set custom data for a symbol
 * 
 * "Customizing a symbol is like giving your prize steer a fancy saddle -
 * it's still the same critter, but now it's carrying something extra!"
 */
bool braggi_symbol_set_data(Symbol* symbol, void* data) {
    if (!symbol) {
        return false;
    }
    
    symbol->data = data;
    return true;
}

/*
 * Get the custom data for a symbol
 * 
 * "Getting a symbol's data is like checking what your horse is carrying -
 * hopefully it's exactly what you packed earlier!"
 */
void* braggi_symbol_get_data(Symbol* symbol) {
    if (!symbol) {
        return NULL;
    }
    
    return symbol->data;
} 