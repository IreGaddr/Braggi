/*
 * Braggi - Entropy ECS Integration
 * 
 * "Like a Texas two-step with Irish footwork, we're joining two powerful
 * systems - the quantum-inspired Wave Function Collapse with the structured
 * Entity Component System!" - Irish-Texan Architecture Philosophy
 */

#ifndef BRAGGI_ENTROPY_ECS_H
#define BRAGGI_ENTROPY_ECS_H

#include "braggi/ecs.h"
#include "braggi/entropy.h"
#include "braggi/token.h"
#include "braggi/token_propagator.h"

// Forward declarations
typedef struct BraggiContext BraggiContext;

// Component types for entropy integration
typedef struct EntropyStateComponent EntropyStateComponent;
typedef struct EntropyConstraintComponent EntropyConstraintComponent;
typedef struct TokenStateComponent TokenStateComponent;

/**
 * Initialize the entropy ECS integration
 * 
 * @param context The Braggi context
 * @return true if initialization was successful, false otherwise
 */
bool braggi_entropy_ecs_initialize(BraggiContext* context);

/**
 * Register the entropy components with the ECS world
 * 
 * @param world The ECS world to register components with
 * @param component_ids Array to store the component type IDs
 * @return true if registration was successful, false otherwise
 */
bool braggi_entropy_register_components(ECSWorld* world, ComponentTypeID* component_ids);

/**
 * Create a system that synchronizes the entropy field with the ECS world
 * 
 * @param context The Braggi context
 * @return A newly created system, or NULL on failure
 */
System* braggi_entropy_create_sync_system(BraggiContext* context);

/**
 * Create a system that applies constraints in the ECS
 * 
 * @param context The Braggi context
 * @return A newly created system, or NULL on failure
 */
System* braggi_entropy_create_constraint_system(BraggiContext* context);

/**
 * Create a system that applies the Wave Function Collapse algorithm in the ECS
 * 
 * @param context The Braggi context
 * @return A newly created system, or NULL on failure
 */
System* braggi_entropy_create_wfc_system(BraggiContext* context);

/**
 * Clean up entropy ECS resources
 * 
 * @param world The ECS world to clean up
 */
void braggi_entropy_ecs_cleanup(ECSWorld* world);

/**
 * Safely clears the field pointer in the EntropyWFCSystemData
 * Should be called after the token propagator is destroyed
 * 
 * @param world The ECS world
 */
void braggi_entropy_ecs_clear_field_reference(ECSWorld* world);

/**
 * Create an entity for an entropy state
 * 
 * @param world The ECS world
 * @param state The entropy state
 * @param cell_id The ID of the cell this state belongs to
 * @return The entity ID, or INVALID_ENTITY on failure
 */
EntityID braggi_entropy_create_state_entity(ECSWorld* world, EntropyState* state, uint32_t cell_id);

/**
 * Create an entity for an entropy constraint
 * 
 * @param world The ECS world
 * @param constraint The entropy constraint
 * @return The entity ID, or INVALID_ENTITY on failure
 */
EntityID braggi_entropy_create_constraint_entity(ECSWorld* world, EntropyConstraint* constraint);

/**
 * Synchronize entities between the entropy field and the ECS
 * This should be called after changes to the entropy field
 * 
 * @param context The Braggi context
 * @return true if synchronization was successful, false otherwise
 */
bool braggi_entropy_sync_to_ecs(BraggiContext* context);

/**
 * Apply the Wave Function Collapse algorithm using the ECS
 * This uses the ECS systems to perform the algorithm
 * 
 * @param context The Braggi context
 * @return true if the algorithm was successful, false otherwise
 */
bool braggi_entropy_apply_wfc_with_ecs(BraggiContext* context);

#endif /* BRAGGI_ENTROPY_ECS_H */ 