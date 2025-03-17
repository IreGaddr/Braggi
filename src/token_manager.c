/*
 * Braggi - Token Manager Implementation
 * 
 * "Tokens are like cattle - you need a good system to keep track of 'em all,
 * or they'll wander off into the wrong pasture!" - Texas Token Wisdom
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "braggi/token.h"
#include "braggi/util/vector.h"
#include "braggi/util/hashmap.h"

// Structure for token ID mapping
typedef struct TokenIDMapping {
    Token* token;
    uint32_t id;
} TokenIDMapping;

// Structure for the token manager
typedef struct TokenManager {
    Vector* tokens;                // Vector of Token* (all tokens)
    HashMap* token_by_id;          // Map from uint32_t ID to Token*
    HashMap* token_by_position;    // Map from source position to Token*
    Vector* token_id_mappings;     // Vector of TokenIDMapping for reverse lookups
    size_t next_token_id;          // Next available token ID
    bool initialized;              // Whether the manager is initialized
} TokenManager;

/*
 * Create a new token manager
 * 
 * "Starting up a new token manager is like opening the gates on roundup day -
 * gotta make sure you've got room for all them little tokens!"
 */
TokenManager* braggi_token_manager_create(void) {
    TokenManager* manager = (TokenManager*)malloc(sizeof(TokenManager));
    if (!manager) {
        return NULL;
    }
    
    // Initialize manager
    manager->tokens = braggi_vector_create(sizeof(Token*));
    if (!manager->tokens) {
        free(manager);
        return NULL;
    }
    
    manager->token_by_id = braggi_hashmap_create();
    if (!manager->token_by_id) {
        braggi_vector_destroy(manager->tokens);
        free(manager);
        return NULL;
    }
    
    manager->token_by_position = braggi_hashmap_create();
    if (!manager->token_by_position) {
        braggi_hashmap_destroy(manager->token_by_id);
        braggi_vector_destroy(manager->tokens);
        free(manager);
        return NULL;
    }
    
    manager->token_id_mappings = braggi_vector_create(sizeof(TokenIDMapping));
    if (!manager->token_id_mappings) {
        braggi_hashmap_destroy(manager->token_by_position);
        braggi_hashmap_destroy(manager->token_by_id);
        braggi_vector_destroy(manager->tokens);
        free(manager);
        return NULL;
    }
    
    manager->next_token_id = 1;  // Start from 1, reserve 0 as invalid
    manager->initialized = true;
    
    return manager;
}

/*
 * Destroy a token manager and all its managed tokens
 * 
 * "When the roundup's over, make sure all your tokens
 * get properly put away - no stragglers allowed!"
 */
void braggi_token_manager_destroy(TokenManager* manager) {
    if (!manager) {
        return;
    }
    
    // Clean up all managed tokens
    if (manager->tokens) {
        for (size_t i = 0; i < braggi_vector_size(manager->tokens); i++) {
            Token* token = *(Token**)braggi_vector_get(manager->tokens, i);
            if (token) {
                braggi_token_destroy(token);
            }
        }
        braggi_vector_destroy(manager->tokens);
    }
    
    // Clean up hashmaps
    if (manager->token_by_id) {
        braggi_hashmap_destroy(manager->token_by_id);
    }
    
    if (manager->token_by_position) {
        braggi_hashmap_destroy(manager->token_by_position);
    }
    
    // Clean up token ID mappings
    if (manager->token_id_mappings) {
        braggi_vector_destroy(manager->token_id_mappings);
    }
    
    // Free the manager itself
    free(manager);
}

/*
 * Get token ID for a token
 * 
 * "Every token needs a unique brand, and this helper function
 * lets us find that brand when we need it!"
 */
static uint32_t get_token_id(TokenManager* manager, Token* token) {
    if (!manager || !token || !manager->token_id_mappings) {
        return 0;
    }
    
    // Search for the token in our mappings
    for (size_t i = 0; i < braggi_vector_size(manager->token_id_mappings); i++) {
        TokenIDMapping* mapping = (TokenIDMapping*)braggi_vector_get(manager->token_id_mappings, i);
        if (mapping && mapping->token == token) {
            return mapping->id;
        }
    }
    
    return 0; // Not found
}

/*
 * Add a token to the manager
 * 
 * "Adding a token to the manager is like branding a new calf -
 * gotta make sure it's got the right ID and knows where it belongs!"
 */
bool braggi_token_manager_add_token(TokenManager* manager, Token* token) {
    if (!manager || !token) {
        return false;
    }
    
    // Check if we already have this token
    uint32_t existing_id = get_token_id(manager, token);
    if (existing_id != 0) {
        return true; // Already added
    }
    
    // Assign an ID
    uint32_t token_id = manager->next_token_id++;
    
    // Add to token ID mappings
    TokenIDMapping mapping;
    mapping.token = token;
    mapping.id = token_id;
    
    if (!braggi_vector_push(manager->token_id_mappings, &mapping)) {
        return false;
    }
    
    // Add to vector
    if (!braggi_vector_push(manager->tokens, &token)) {
        // Remove from mappings
        braggi_vector_pop(manager->token_id_mappings);
        return false;
    }
    
    // Add to hashmaps
    char id_key[32];
    snprintf(id_key, sizeof(id_key), "%u", token_id);
    braggi_hashmap_put(manager->token_by_id, strdup(id_key), token);
    
    // Add by position if token has a position
    if (token->position.line > 0) {
        char pos_key[64];
        snprintf(pos_key, sizeof(pos_key), "%u:%u:%u", 
                 token->position.file_id, 
                 token->position.line, 
                 token->position.column);
        braggi_hashmap_put(manager->token_by_position, strdup(pos_key), token);
    }
    
    return true;
}

/*
 * Get a token by ID
 * 
 * "Looking up a token by ID is like finding the right steer by its brand -
 * when you've got the right system in place, it's a piece of cake!"
 */
Token* braggi_token_manager_get_token(TokenManager* manager, uint32_t token_id) {
    if (!manager || token_id == 0) {
        return NULL;
    }
    
    char id_key[32];
    snprintf(id_key, sizeof(id_key), "%u", token_id);
    return (Token*)braggi_hashmap_get(manager->token_by_id, id_key);
}

/*
 * Get a token by source position
 * 
 * "Finding a token by its position is like spotting a longhorn
 * in a particular meadow - you just need to know where to look!"
 */
Token* braggi_token_manager_get_token_at_position(TokenManager* manager, 
                                                uint32_t file_id, 
                                                uint32_t line, 
                                                uint32_t column) {
    if (!manager) {
        return NULL;
    }
    
    char pos_key[64];
    snprintf(pos_key, sizeof(pos_key), "%u:%u:%u", file_id, line, column);
    return (Token*)braggi_hashmap_get(manager->token_by_position, pos_key);
} 