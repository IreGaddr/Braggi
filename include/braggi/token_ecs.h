/*
 * Braggi - Token ECS Integration Header
 * 
 * "Bridgin' the gap between token management and ECS worlds
 * is like building a dance floor where Irish and Texan styles can mingle!"
 */

#ifndef BRAGGI_TOKEN_ECS_H
#define BRAGGI_TOKEN_ECS_H

#include "braggi/ecs.h"
#include "braggi/token.h"
#include "braggi/token_manager.h"

// Forward declarations
typedef struct BraggiContext BraggiContext;

// Token component type (defined in token_ecs.c)
typedef struct TokenComponent TokenComponent;

/**
 * Initialize the token ECS integration
 * 
 * @param context The Braggi context containing the token manager
 * @return true if the integration was successful, false otherwise
 */
bool braggi_token_ecs_initialize(BraggiContext* context);

/**
 * Register the token component type with the ECS world
 * 
 * @param world The ECS world to register the component with
 * @return The component type ID for the token component
 */
ComponentTypeID braggi_token_register_component(ECSWorld* world);

/**
 * Create a token synchronization system that keeps tokens in sync between
 * the TokenManager and the ECS world.
 * 
 * @param token_manager The token manager to synchronize with
 * @return A newly created system, or NULL on failure
 */
System* braggi_token_create_sync_system(TokenManager* token_manager);

/**
 * Create an entity for a token in the ECS world
 * 
 * @param world The ECS world to create the entity in
 * @param token The token to create an entity for
 * @param token_id The ID of the token in the token manager
 * @return The entity ID, or INVALID_ENTITY on failure
 */
EntityID braggi_token_create_entity(ECSWorld* world, Token* token, uint32_t token_id);

/**
 * Clean up token resources in the ECS
 * 
 * @param world The ECS world to clean up
 */
void braggi_token_ecs_cleanup(ECSWorld* world);

#endif /* BRAGGI_TOKEN_ECS_H */ 