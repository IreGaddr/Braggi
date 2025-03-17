/*
 * Braggi - Periscope System
 * 
 * "A good periscope lets ya see what's comin' before it hits ya,
 * just like a good validator warns ya 'bout constraint trouble!" - Irish-Texan Wisdom
 */

#ifndef BRAGGI_PERISCOPE_H
#define BRAGGI_PERISCOPE_H

#include "braggi/ecs.h"
#include "braggi/entropy.h"
#include "braggi/token_propagator.h"
#include "braggi/constraint.h"
#include <stdint.h>
#include <stdbool.h>

// Forward declarations
typedef struct Periscope Periscope;
typedef struct PeriscopeView PeriscopeView;
typedef struct RegionLifetimeContract RegionLifetimeContract;

// Periscope system tracks token-to-cell mappings and region lifetime contracts
struct Periscope {
    ECSWorld* ecs_world;             // Reference to the ECS world
    ComponentTypeID token_component;  // Component type for tokens
    ComponentTypeID cell_component;   // Component type for cells
    ComponentTypeID validator_component; // Component type for validators
    Vector* token_to_cell_mappings;  // Mapping from tokens to cell entities
    Vector* active_contracts;        // Active region lifetime contracts
    bool (*validator)(struct EntropyConstraint*, struct EntropyField*);  // Validator function
};

// View into a specific region through the periscope
struct PeriscopeView {
    Periscope* periscope;            // Reference to parent periscope
    EntityID region_entity;          // Entity representing the region
    Vector* tokens_in_view;          // Tokens visible in this view
    Vector* cells_in_view;           // Cells visible in this view
};

// Contract ensuring region lifetime guarantees for validators
struct RegionLifetimeContract {
    EntityID region_entity;           // Entity representing the region
    EntityID validator_entity;        // Entity representing the validator
    uint32_t guarantee_flags;         // Flags for specific guarantees
    bool is_valid;                    // Whether contract is currently valid
};

// Create a periscope
Periscope* braggi_periscope_create(ECSWorld* ecs_world);

// Destroy a periscope
void braggi_periscope_destroy(Periscope* periscope);

// Initialize the periscope system, registering necessary ECS components
bool braggi_periscope_initialize(Periscope* periscope);

// Register a token with the periscope
bool braggi_periscope_register_token(Periscope* periscope, void* token, uint32_t cell_id);

// Register multiple tokens with the periscope in a batch operation
bool braggi_periscope_register_tokens_batch(Periscope* periscope, Vector* tokens);

// Get cell ID for a token
uint32_t braggi_periscope_get_cell_id_for_token(Periscope* periscope, void* token, EntropyField* field);

// Create a view for a specific region
PeriscopeView* braggi_periscope_create_view(Periscope* periscope, EntityID region_entity);

// Destroy a periscope view
void braggi_periscope_destroy_view(PeriscopeView* view);

// Create a contract for a validator to ensure region lifetime guarantees
RegionLifetimeContract* braggi_periscope_create_contract(
    Periscope* periscope,
    EntityID region_entity,
    EntityID validator_entity,
    uint32_t guarantee_flags);

// Validate constraints against region lifetime contracts
bool braggi_periscope_validate_constraints(
    Periscope* periscope,
    EntropyConstraint* constraint,
    EntropyField* field);

// Track a token-to-cell mapping
bool braggi_periscope_track_token_cell_mapping(
    Periscope* periscope,
    Token* token,
    uint32_t cell_id);

// Get validator result based on region lifetime contracts
bool braggi_periscope_check_validator(
    Periscope* periscope,
    EntropyConstraint* constraint,
    EntropyField* field);

// ECS system update function for periscope updates
void braggi_periscope_system_update(ECSWorld* world, System* system, float delta_time);

// Register a contract with the periscope
bool braggi_periscope_register_contract(Periscope* periscope, void* contract);

#endif /* BRAGGI_PERISCOPE_H */ 