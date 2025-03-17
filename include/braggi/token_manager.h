/*
 * Braggi - Token Manager Header
 * 
 * "A token without a manager is like a cow without a cowboy -
 * it'll just wander around aimlessly and get lost in the syntax prairie!"
 */

#ifndef BRAGGI_TOKEN_MANAGER_H
#define BRAGGI_TOKEN_MANAGER_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "braggi/token.h"

// Forward declaration
typedef struct TokenManager TokenManager;

/**
 * Create a new token manager
 * 
 * @return A newly allocated token manager, or NULL on failure
 */
TokenManager* braggi_token_manager_create(void);

/**
 * Destroy a token manager and all its managed tokens
 * 
 * @param manager The manager to destroy
 */
void braggi_token_manager_destroy(TokenManager* manager);

/**
 * Add a token to the manager
 * 
 * @param manager The token manager
 * @param token The token to add
 * @return true if the token was added successfully, false otherwise
 */
bool braggi_token_manager_add_token(TokenManager* manager, Token* token);

/**
 * Get a token by ID
 * 
 * @param manager The token manager
 * @param token_id The ID of the token to get
 * @return The token with the specified ID, or NULL if not found
 */
Token* braggi_token_manager_get_token(TokenManager* manager, uint32_t token_id);

/**
 * Get a token by source position
 * 
 * @param manager The token manager
 * @param file_id The ID of the file containing the token
 * @param line The line number of the token
 * @param column The column number of the token
 * @return The token at the specified position, or NULL if not found
 */
Token* braggi_token_manager_get_token_at_position(TokenManager* manager, 
                                               uint32_t file_id, 
                                               uint32_t line, 
                                               uint32_t column);

#endif /* BRAGGI_TOKEN_MANAGER_H */ 