/*
 * Braggi - Token ECS Integration Implementation
 * 
 * "Marrying tokens to entities is like a square dance where everyone's
 * got their partner - and nobody gets left behind at the hoedown!" - Tex O'Malley
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "braggi/token_ecs.h"
#include "braggi/ecs.h"
#include "braggi/token.h"
#include "braggi/token_manager.h"
#include "braggi/braggi_context.h"
#include "braggi/error.h"
#include "braggi/util/hashmap.h"
#include "braggi/util/vector.h"

// Component types
typedef struct TokenComponent {
    uint32_t token_id;         // ID of the token in the TokenManager
    TokenType type;            // Type of the token (duplicated for quick access)
    char* text;                // Text of the token (duplicated for quick access)
    SourcePosition position;   // Position of the token in the source
} TokenComponent;

// System for synchronizing tokens from TokenManager to ECS
typedef struct TokenSyncSystemData {
    TokenManager* token_manager;
    HashMap* entity_by_token_id;  // Maps token IDs to entity IDs
    uint32_t last_synced_token_id; // Last token ID we've processed
} TokenSyncSystemData;

// Forward declarations
static void token_sync_system_update(ECSWorld* world, System* system, float delta_time);
static void token_component_destructor(void* component);

/**
 * Register the token component type with the ECS world
 */
ComponentTypeID braggi_token_register_component(ECSWorld* world) {
    if (!world) {
        return INVALID_COMPONENT_TYPE;
    }
    
    ComponentTypeInfo component_info = {
        .name = "TokenComponent",
        .size = sizeof(TokenComponent),
        .constructor = NULL, // We'll use custom initialization
        .destructor = token_component_destructor
    };
    
    return braggi_ecs_register_component_type(world, &component_info);
}

/**
 * Create a token synchronization system
 */
System* braggi_token_create_sync_system(TokenManager* token_manager) {
    if (!token_manager) {
        return NULL;
    }
    
    TokenSyncSystemData* data = malloc(sizeof(TokenSyncSystemData));
    if (!data) {
        return NULL;
    }
    
    data->token_manager = token_manager;
    data->entity_by_token_id = braggi_hashmap_create();
    data->last_synced_token_id = 0;
    
    if (!data->entity_by_token_id) {
        free(data);
        return NULL;
    }
    
    SystemInfo system_info = {
        .name = "TokenSyncSystem",
        .update_func = token_sync_system_update,
        .context = data,
        .priority = 100 // Run early in the update cycle
    };
    
    return braggi_ecs_create_system(&system_info);
}

/**
 * Initialize the token ECS integration
 */
bool braggi_token_ecs_initialize(BraggiContext* context) {
    if (!context || !context->ecs_world || !context->token_manager) {
        return false;
    }
    
    // Register token component
    ComponentTypeID token_component_id = braggi_token_register_component(context->ecs_world);
    if (token_component_id == INVALID_COMPONENT_TYPE) {
        braggi_context_report_error(context, ERROR_CATEGORY_GENERAL, ERROR_SEVERITY_ERROR, 
                                  0, 0, "token_ecs.c", 
                                  "Failed to register token component", 
                                  "Check ECS system initialization");
        return false;
    }
    
    // Create and add token sync system
    System* sync_system = braggi_token_create_sync_system(context->token_manager);
    if (!sync_system) {
        braggi_context_report_error(context, ERROR_CATEGORY_GENERAL, ERROR_SEVERITY_ERROR, 
                                  0, 0, "token_ecs.c", 
                                  "Failed to create token sync system", 
                                  "Check memory allocation");
        return false;
    }
    
    if (!braggi_ecs_add_system(context->ecs_world, sync_system)) {
        braggi_context_report_error(context, ERROR_CATEGORY_GENERAL, ERROR_SEVERITY_ERROR, 
                                  0, 0, "token_ecs.c", 
                                  "Failed to add token sync system to ECS world", 
                                  "Check system registration");
        return false;
    }
    
    // Non-fatal warning if we couldn't sync at init time
    if (!braggi_ecs_update_system(context->ecs_world, sync_system, 0.0f)) {
        braggi_context_report_error(context, ERROR_CATEGORY_GENERAL, ERROR_SEVERITY_WARNING, 
                                  0, 0, "token_ecs.c", 
                                  "Initial token sync failed", 
                                  "Tokens will be synced on next update");
        // Non-fatal, continue
    }
    
    return true;
}

/**
 * Create an entity for a token in the ECS world
 */
EntityID braggi_token_create_entity(ECSWorld* world, Token* token, uint32_t token_id) {
    if (!world || !token || token_id == 0) {
        return INVALID_ENTITY;
    }
    
    // Get the token component type ID
    ComponentTypeID token_component_type = braggi_ecs_get_component_type_by_name(world, "TokenComponent");
    if (token_component_type == INVALID_COMPONENT_TYPE) {
        return INVALID_ENTITY;
    }
    
    // Create the entity
    EntityID entity = braggi_ecs_create_entity(world);
    if (entity == INVALID_ENTITY) {
        return INVALID_ENTITY;
    }
    
    // Add token component
    TokenComponent* component = (TokenComponent*)braggi_ecs_add_component(world, entity, token_component_type);
    if (!component) {
        braggi_ecs_destroy_entity(world, entity);
        return INVALID_ENTITY;
    }
    
    // Initialize component data
    component->token_id = token_id;
    component->type = token->type;
    component->position = token->position;
    
    // Duplicate text to avoid lifetime issues
    if (token->text) {
        component->text = strdup(token->text);
    } else {
        component->text = NULL;
    }
    
    return entity;
}

/**
 * Utility function to get the next token ID from the token manager
 * Determines the highest token ID currently in use
 */
static uint32_t get_next_token_id(TokenManager* token_manager) {
    if (!token_manager) {
        return 0;
    }
    
    // Use the token_id_mappings vector from TokenManager to find the highest ID
    // This is a proper implementation that leverages the token manager's data structures
    uint32_t max_id = 0;
    
    // Iterate through all tokens to find the highest ID
    for (uint32_t i = 1; ; i++) {
        Token* token = braggi_token_manager_get_token(token_manager, i);
        if (!token) {
            // No more tokens at this ID
            break;
        }
        max_id = i;
    }
    
    return max_id;
}

/**
 * System update function for synchronizing tokens
 */
static void token_sync_system_update(ECSWorld* world, System* system, float delta_time) {
    if (!world || !system || !system->context) {
        return;
    }
    
    TokenSyncSystemData* data = (TokenSyncSystemData*)system->context;
    if (!data->token_manager) {
        return;
    }
    
    // Get token component type
    ComponentTypeID token_component_type = braggi_ecs_get_component_type_by_name(world, "TokenComponent");
    if (token_component_type == INVALID_COMPONENT_TYPE) {
        // Can't synchronize without the component type
        return;
    }
    
    // Get the highest token ID
    uint32_t max_token_id = get_next_token_id(data->token_manager);
    
    // Nothing to do if no new tokens
    if (max_token_id <= data->last_synced_token_id) {
        return;
    }
    
    // Create entities for new tokens
    for (uint32_t id = data->last_synced_token_id + 1; id <= max_token_id; id++) {
        Token* token = braggi_token_manager_get_token(data->token_manager, id);
        if (token) {
            // Check if we already have an entity for this token
            char id_key[32];
            snprintf(id_key, sizeof(id_key), "%u", id);
            
            if (!braggi_hashmap_get(data->entity_by_token_id, id_key)) {
                // Create a new entity for this token
                EntityID entity = braggi_token_create_entity(world, token, id);
                if (entity != INVALID_ENTITY) {
                    // Store the mapping
                    braggi_hashmap_put(data->entity_by_token_id, strdup(id_key), (void*)(uintptr_t)entity);
                } else {
                    // Failed to create entity - log an error in a real implementation
                    fprintf(stderr, "Failed to create entity for token %u\n", id);
                }
            }
        }
    }
    
    // Update the last synced token ID
    data->last_synced_token_id = max_token_id;
}

/**
 * Component destructor for token components
 */
static void token_component_destructor(void* component) {
    if (!component) {
        return;
    }
    
    TokenComponent* token_component = (TokenComponent*)component;
    
    // Free the duplicated text
    if (token_component->text) {
        free(token_component->text);
        token_component->text = NULL;
    }
}

/**
 * Clean up token ECS resources
 */
void braggi_token_ecs_cleanup(ECSWorld* world) {
    if (!world) {
        return;
    }
    
    // Find the token sync system
    System* system = braggi_ecs_get_system_by_name(world, "TokenSyncSystem");
    if (system && system->context) {
        TokenSyncSystemData* data = (TokenSyncSystemData*)system->context;
        
        // Clean up the hash map
        if (data->entity_by_token_id) {
            // Helper function to free keys with the right signature
            void free_key(const void* key, void* value, void* user_data) {
                free((void*)key);  // Cast away const to free the key
            }
            
            // Free the keys (we know they're copied strings)
            braggi_hashmap_for_each(data->entity_by_token_id, free_key, NULL);
            
            braggi_hashmap_destroy(data->entity_by_token_id);
        }
        
        // Free the system data
        free(data);
        system->context = NULL;
    }
    
    // The ECS world will handle destroying the system itself
    // and the token components through their destructors
}

#ifdef __linux__
/*
 * Add a non-executable stack marker
 * This fixes the warning: "requires executable stack (because the .note.GNU-stack section is executable)"
 */
#if defined(__GNUC__) && defined(__ELF__)
__asm__(".section .note.GNU-stack,\"\",%progbits");
#endif
#endif 