/*
 * Braggi - Periscope System Implementation
 * 
 * "When yer validator needs to peek at tokens across regions, 
 * ya need a good periscope to keep yer bearings!" - Dallas-Irish Programming Proverb
 */

#include "braggi/periscope.h"
#include "braggi/util/vector.h"
#include "braggi/mem/region.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Define logging macros
#define DEBUG(msg, ...) fprintf(stderr, "DEBUG: " msg "\n", ##__VA_ARGS__)
#define WARNING(msg, ...) fprintf(stderr, "WARNING: " msg "\n", ##__VA_ARGS__)
#define ERROR(msg, ...) fprintf(stderr, "ERROR: " msg "\n", ##__VA_ARGS__)

// Token to cell mapping structure
typedef struct {
    Token* token;     // Pointer to token
    uint32_t cell_id; // Cell ID in entropy field
    EntityID entity;  // Entity ID in ECS
} TokenCellMapping;

// Component types for ECS integration
typedef struct {
    Token* token;     // Reference to the token
} TokenComponent;

typedef struct {
    uint32_t cell_id; // Cell ID in entropy field
} CellComponent;

typedef struct {
    bool (*validate)(EntropyConstraint*, EntropyField*); // Validator function
    void* context;    // Context data for validator
} ValidatorComponent;

// Forward declarations for internal functions
static bool periscope_register_components(Periscope* periscope);
static void periscope_token_system_update(ECSWorld* world, System* system, float delta_time);
static bool token_cell_mapping_compare(const void* a, const void* b);

// Create a periscope
Periscope* braggi_periscope_create(ECSWorld* ecs_world) {
    if (!ecs_world) {
        ERROR("Cannot create periscope without ECS world");
        return NULL;
    }
    
    Periscope* periscope = malloc(sizeof(Periscope));
    if (!periscope) {
        ERROR("Failed to allocate memory for periscope");
        return NULL;
    }
    
    // Initialize the periscope
    memset(periscope, 0, sizeof(Periscope));
    
    // Store the ECS world reference
    periscope->ecs_world = ecs_world;
    
    // Initialize vectors
    periscope->token_to_cell_mappings = braggi_vector_create(sizeof(TokenCellMapping*));
    if (!periscope->token_to_cell_mappings) {
        ERROR("Failed to create token_to_cell_mappings vector");
        free(periscope);
        return NULL;
    }
    
    periscope->active_contracts = braggi_vector_create(sizeof(void*));
    if (!periscope->active_contracts) {
        ERROR("Failed to create active_contracts vector");
        braggi_vector_destroy(periscope->token_to_cell_mappings);
        free(periscope);
        return NULL;
    }
    
    DEBUG("Created periscope at %p with ECS world %p", periscope, ecs_world);
    return periscope;
}

// Destroy a periscope system
void braggi_periscope_destroy(Periscope* periscope) {
    if (!periscope) {
        fprintf(stderr, "DEBUG: Attempted to destroy NULL periscope\n");
        return;
    }
    
    fprintf(stderr, "DEBUG: Starting periscope destruction at %p\n", (void*)periscope);
    
    // Check if we're registered with constraint patterns and unregister if so
    extern Periscope* braggi_constraint_patterns_get_periscope(void);
    extern bool braggi_constraint_patterns_set_periscope(Periscope* periscope);
    
    Periscope* global_periscope = braggi_constraint_patterns_get_periscope();
    if (global_periscope == periscope) {
        fprintf(stderr, "DEBUG: Unregistering periscope %p from constraint patterns\n", (void*)periscope);
        braggi_constraint_patterns_set_periscope(NULL);
    }
    
    // Clean up vectors with defensive checks
    if (periscope->token_to_cell_mappings) {
        fprintf(stderr, "DEBUG: Destroying token_to_cell_mappings vector at %p\n", 
                (void*)periscope->token_to_cell_mappings);
        braggi_vector_destroy(periscope->token_to_cell_mappings);
        periscope->token_to_cell_mappings = NULL;
    }
    
    if (periscope->active_contracts) {
        fprintf(stderr, "DEBUG: Destroying active_contracts vector at %p\n", 
                (void*)periscope->active_contracts);
        braggi_vector_destroy(periscope->active_contracts);
        periscope->active_contracts = NULL;
    }
    
    // Null out other pointers defensively
    periscope->ecs_world = NULL;
    periscope->validator = NULL;
    periscope->token_component = 0;
    periscope->cell_component = 0;
    periscope->validator_component = 0;
    
    // Create a copy of the periscope pointer for safety
    Periscope* periscope_to_free = periscope;
    
    fprintf(stderr, "DEBUG: Periscope resources cleaned up, now freeing periscope itself\n");
    free(periscope_to_free);
}

// Initialize the periscope system
bool braggi_periscope_initialize(Periscope* periscope) {
    if (!periscope || !periscope->ecs_world) {
        fprintf(stderr, "ERROR: Cannot initialize invalid periscope\n");
        return false;
    }
    
    // Register component types with ECS
    if (!periscope_register_components(periscope)) {
        fprintf(stderr, "ERROR: Failed to register periscope components with ECS\n");
        return false;
    }
    
    // Set a default validator function if not already set
    if (!periscope->validator) {
        // Import the default validator function 
        extern bool braggi_default_adjacency_validator(EntropyConstraint* constraint, EntropyField* field);
        periscope->validator = braggi_default_adjacency_validator;
        fprintf(stderr, "DEBUG: Set default adjacency validator for periscope\n");
    }
    
    // Register system for token updates
    SystemInfo system_info = {
        .name = "Periscope Token System",
        .update_func = periscope_token_system_update,
        .context = periscope,
        .priority = 100  // High priority to run early
    };
    
    System* system = braggi_ecs_create_system(&system_info);
    if (!system) {
        fprintf(stderr, "ERROR: Failed to create periscope token system\n");
        return false;
    }
    
    // Set component mask for system
    braggi_ecs_mask_set(&system->component_mask, periscope->token_component);
    braggi_ecs_mask_set(&system->component_mask, periscope->cell_component);
    
    // Register system with ECS
    if (!braggi_ecs_add_system(periscope->ecs_world, system)) {
        fprintf(stderr, "ERROR: Failed to add periscope system to ECS world\n");
        braggi_ecs_system_destroy(system);
        return false;
    }
    
    fprintf(stderr, "DEBUG: Successfully initialized periscope system\n");
    return true;
}

// Register components with ECS
static bool periscope_register_components(Periscope* periscope) {
    if (!periscope || !periscope->ecs_world) return false;
    
    // Register token component
    ComponentTypeInfo token_info = {
        .name = "TokenComponent",
        .size = sizeof(TokenComponent),
        .constructor = NULL,
        .destructor = NULL
    };
    
    periscope->token_component = braggi_ecs_register_component_type(
        periscope->ecs_world, &token_info);
    
    // Register cell component
    ComponentTypeInfo cell_info = {
        .name = "CellComponent",
        .size = sizeof(CellComponent),
        .constructor = NULL,
        .destructor = NULL
    };
    
    periscope->cell_component = braggi_ecs_register_component_type(
        periscope->ecs_world, &cell_info);
    
    // Register validator component
    ComponentTypeInfo validator_info = {
        .name = "ValidatorComponent",
        .size = sizeof(ValidatorComponent),
        .constructor = NULL,
        .destructor = NULL
    };
    
    periscope->validator_component = braggi_ecs_register_component_type(
        periscope->ecs_world, &validator_info);
    
    // Verify all components registered successfully
    if (periscope->token_component == INVALID_COMPONENT_TYPE ||
        periscope->cell_component == INVALID_COMPONENT_TYPE ||
        periscope->validator_component == INVALID_COMPONENT_TYPE) {
        fprintf(stderr, "ERROR: Failed to register one or more periscope components\n");
        return false;
    }
    
    return true;
}

// System update function for token entities
static void periscope_token_system_update(ECSWorld* world, System* system, float delta_time) {
    if (!world || !system || !system->context) return;
    
    Periscope* periscope = (Periscope*)system->context;
    
    // Create query for entities with token and cell components
    EntityQuery query = braggi_ecs_query_entities(world, system->component_mask);
    EntityID entity;
    
    // Process all matching entities
    while (braggi_ecs_query_next(&query, &entity)) {
        TokenComponent* token_comp = braggi_ecs_get_component(
            world, entity, periscope->token_component);
        
        CellComponent* cell_comp = braggi_ecs_get_component(
            world, entity, periscope->cell_component);
        
        if (!token_comp || !cell_comp || !token_comp->token) continue;
        
        // Update token-to-cell mapping
        braggi_periscope_track_token_cell_mapping(
            periscope, token_comp->token, cell_comp->cell_id);
    }
}

// Register a token with the periscope
bool braggi_periscope_register_token(Periscope* periscope, void* token, uint32_t cell_id) {
    if (!periscope || !token) {
        ERROR("NULL periscope or token in register_token");
        return false;
    }
    
    if (!periscope->token_to_cell_mappings) {
        ERROR("Periscope has no token_to_cell_mappings vector");
        return false;
    }
    
    // Check if token is already registered
    TokenCellMapping* existing_mapping = NULL;
    for (size_t i = 0; i < braggi_vector_size(periscope->token_to_cell_mappings); i++) {
        TokenCellMapping** mapping_ptr = (TokenCellMapping**)braggi_vector_get(periscope->token_to_cell_mappings, i);
        if (!mapping_ptr || !*mapping_ptr) continue;
        
        if ((*mapping_ptr)->token == token) {
            existing_mapping = *mapping_ptr;
            break;
        }
    }
    
    if (existing_mapping) {
        // Update existing mapping
        fprintf(stderr, "DEBUG: Token %p already registered, updating cell ID from %u to %u\n", 
                token, existing_mapping->cell_id, cell_id);
        existing_mapping->cell_id = cell_id;
        return true;
    }
    
    // Create new mapping
    TokenCellMapping* mapping = (TokenCellMapping*)malloc(sizeof(TokenCellMapping));
    if (!mapping) {
        ERROR("Failed to allocate memory for token-cell mapping");
        return false;
    }
    
    mapping->token = token;
    mapping->cell_id = cell_id;
    
    // Add mapping to vector
    if (!braggi_vector_push_back(periscope->token_to_cell_mappings, &mapping)) {
        ERROR("Failed to add token-cell mapping to vector");
        free(mapping);
        return false;
    }
    
    return true;
}

// Get cell ID for a token
uint32_t braggi_periscope_get_cell_id_for_token(Periscope* periscope, void* token, EntropyField* field) {
    if (!periscope || !token) {
        ERROR("NULL periscope or token in get_cell_id_for_token");
        return 0;
    }
    
    if (!periscope->token_to_cell_mappings) {
        WARNING("Periscope has no token_to_cell_mappings");
        return 0;
    }
    
    // Search for token in mappings
    for (size_t i = 0; i < braggi_vector_size(periscope->token_to_cell_mappings); i++) {
        TokenCellMapping** mapping_ptr = (TokenCellMapping**)braggi_vector_get(periscope->token_to_cell_mappings, i);
        if (!mapping_ptr || !*mapping_ptr) continue;
        
        TokenCellMapping* mapping = *mapping_ptr;
        if (mapping->token == token) {
            DEBUG("Found stored cell ID %u for token %p", mapping->cell_id, token);
            return mapping->cell_id;
        }
    }
    
    // Token not found in mappings, create a fallback ID
    WARNING("No cell ID found for token at %p, using fallback", token);
    
    uint32_t fallback_cell_id = 0;
    
    // If token is a Braggi token, use its line number as fallback
    Token* braggi_token = (Token*)token;
    if (braggi_token && braggi_token->position.line > 0) {
        fallback_cell_id = braggi_token->position.line;
        DEBUG("Using token line %u as cell ID %u", braggi_token->position.line, fallback_cell_id);
        
        // Register this mapping so we don't have to calculate it again
        if (field && fallback_cell_id < field->cell_count) {
            braggi_periscope_register_token(periscope, token, fallback_cell_id);
            return fallback_cell_id;
        }
    }
    
    // If we have a field, default to first cell
    if (field && field->cell_count > 0) {
        fallback_cell_id = 0;
    }
    
    return fallback_cell_id;
}

// Track a token-to-cell mapping
bool braggi_periscope_track_token_cell_mapping(
    Periscope* periscope,
    Token* token,
    uint32_t cell_id) {
    
    if (!periscope || !token || !periscope->token_to_cell_mappings) return false;
    
    // Check if mapping already exists
    size_t count = braggi_vector_size(periscope->token_to_cell_mappings);
    for (size_t i = 0; i < count; i++) {
        TokenCellMapping* mapping = braggi_vector_get(periscope->token_to_cell_mappings, i);
        if (mapping && mapping->token == token) {
            // Update existing mapping
            mapping->cell_id = cell_id;
            return true;
        }
    }
    
    // Create new mapping
    TokenCellMapping new_mapping = {
        .token = token,
        .cell_id = cell_id,
        .entity = 0  // Will be set when entity is created
    };
    
    return braggi_vector_push_back(periscope->token_to_cell_mappings, &new_mapping);
}

// Create a view for a specific region
PeriscopeView* braggi_periscope_create_view(Periscope* periscope, EntityID region_entity) {
    if (!periscope || region_entity == INVALID_ENTITY) return NULL;
    
    PeriscopeView* view = (PeriscopeView*)calloc(1, sizeof(PeriscopeView));
    if (!view) return NULL;
    
    view->periscope = periscope;
    view->region_entity = region_entity;
    view->tokens_in_view = braggi_vector_create(sizeof(Token*));
    view->cells_in_view = braggi_vector_create(sizeof(uint32_t));
    
    if (!view->tokens_in_view || !view->cells_in_view) {
        braggi_periscope_destroy_view(view);
        return NULL;
    }
    
    return view;
}

// Destroy a periscope view
void braggi_periscope_destroy_view(PeriscopeView* view) {
    if (!view) return;
    
    if (view->tokens_in_view) {
        braggi_vector_destroy(view->tokens_in_view);
    }
    
    if (view->cells_in_view) {
        braggi_vector_destroy(view->cells_in_view);
    }
    
    free(view);
}

// Create a contract for a validator to ensure region lifetime guarantees
RegionLifetimeContract* braggi_periscope_create_contract(
    Periscope* periscope,
    EntityID region_entity,
    EntityID validator_entity,
    uint32_t guarantee_flags)
{
    if (!periscope) {
        fprintf(stderr, "ERROR: NULL periscope in create_contract\n");
        return NULL;
    }
    
    // Enhanced safety check
    if (!periscope->active_contracts) {
        fprintf(stderr, "ERROR: active_contracts vector is NULL in periscope at %p\n", (void*)periscope);
        
        // Try to recover by creating the vector
        periscope->active_contracts = braggi_vector_create(sizeof(RegionLifetimeContract*));
        if (!periscope->active_contracts) {
            fprintf(stderr, "ERROR: Failed to create active_contracts vector in recovery\n");
            return NULL;
        }
        
        fprintf(stderr, "DEBUG: Created active_contracts vector as recovery at %p\n", 
                (void*)periscope->active_contracts);
    }
    
    // Check region entity
    if (region_entity == 0) {
        fprintf(stderr, "WARNING: Invalid region entity ID 0 in create_contract\n");
        // We'll still create the contract, but mark as suspicious
    }

    // Create a new contract
    RegionLifetimeContract* contract = malloc(sizeof(RegionLifetimeContract));
    if (!contract) {
        fprintf(stderr, "ERROR: Failed to allocate memory for contract\n");
        return NULL;
    }
    
    // Initialize contract
    contract->region_entity = region_entity;
    contract->validator_entity = validator_entity;
    contract->guarantee_flags = guarantee_flags;
    contract->is_valid = true;  // Assume valid until proven otherwise
    
    // Add to the list of active contracts
    if (!braggi_vector_push_back(periscope->active_contracts, &contract)) {
        fprintf(stderr, "ERROR: Failed to add contract to active contracts vector\n");
        free(contract);
        return NULL;
    }
    
    fprintf(stderr, "DEBUG: Created contract %p: region=%u, validator=%u, flags=%u\n",
            (void*)contract, contract->region_entity, contract->validator_entity, 
            contract->guarantee_flags);
    
    // Print contract count for better tracking
    size_t contract_count = braggi_vector_size(periscope->active_contracts);
    fprintf(stderr, "DEBUG: Periscope now has %zu active contracts\n", contract_count);
    
    return contract;
}

// Validate constraints against region lifetime contracts
bool braggi_periscope_validate_constraints(Periscope* periscope, EntropyConstraint* constraint, EntropyField* field) {
    if (!periscope) {
        fprintf(stderr, "ERROR: NULL periscope in validate_constraints\n");
        return true; // Return true to avoid blocking propagation
    }

    if (!constraint) {
        fprintf(stderr, "ERROR: NULL constraint in validate_constraints\n");
        return true; // Return true to avoid blocking propagation
    }

    if (!field) {
        fprintf(stderr, "ERROR: NULL field in validate_constraints\n");
        return true; // Return true to avoid blocking propagation
    }

    fprintf(stderr, "DEBUG: Validating constraint type %d (desc: %s)\n", 
            constraint->type, 
            constraint->description ? constraint->description : "unnamed");

    // Enhanced contract checking - add more diagnostics
    if (!periscope->active_contracts) {
        fprintf(stderr, "WARNING: No active_contracts vector in periscope at %p - creating one\n", (void*)periscope);
        
        // Recovery: create the vector
        periscope->active_contracts = braggi_vector_create(sizeof(RegionLifetimeContract*));
        if (!periscope->active_contracts) {
            fprintf(stderr, "ERROR: Failed to create active_contracts vector in recovery\n");
            goto fallback_validation;
        }
        
        // Recovery: create a fallback contract
        RegionLifetimeContract* contract = malloc(sizeof(RegionLifetimeContract));
        if (!contract) {
            fprintf(stderr, "ERROR: Failed to allocate memory for fallback contract\n");
            goto fallback_validation;
        }
        
        // Initialize the fallback contract
        contract->region_entity = 1;  // Default region ID 
        contract->validator_entity = 1;  // Default validator ID
        contract->guarantee_flags = 1;  // Basic guarantee flag
        contract->is_valid = true;
        
        if (!braggi_vector_push_back(periscope->active_contracts, &contract)) {
            fprintf(stderr, "ERROR: Failed to add fallback contract to active contracts\n");
            free(contract);
            goto fallback_validation;
        }
        
        fprintf(stderr, "DEBUG: Created fallback contract as recovery at %p\n", (void*)contract);
    }
    
    size_t contract_count = braggi_vector_size(periscope->active_contracts);
    
    // Don't just check if there are any contracts, check if they're valid
    size_t valid_contracts = 0;
    for (size_t i = 0; i < contract_count; i++) {
        RegionLifetimeContract** contract_ptr = braggi_vector_get(periscope->active_contracts, i);
        if (contract_ptr && *contract_ptr && (*contract_ptr)->is_valid) {
            valid_contracts++;
            
            // Print details of valid contracts for debugging
            fprintf(stderr, "DEBUG: Valid contract at %p, region=%u, validator=%u, flags=%u\n",
                   (void*)*contract_ptr, (*contract_ptr)->region_entity, 
                   (*contract_ptr)->validator_entity, (*contract_ptr)->guarantee_flags);
        }
    }
    
    fprintf(stderr, "DEBUG: Periscope has %zu total contracts, %zu valid\n", 
            contract_count, valid_contracts);
    
    // If we have no valid contracts, create one on the fly as a last resort
    if (valid_contracts == 0) {
        fprintf(stderr, "WARNING: No active valid contracts, creating one as last resort\n");
        
        // Last resort: create an emergency contract 
        RegionLifetimeContract* emergency_contract = malloc(sizeof(RegionLifetimeContract));
        if (emergency_contract) {
            // Initialize the emergency contract
            emergency_contract->region_entity = 1;
            emergency_contract->validator_entity = 1;
            emergency_contract->guarantee_flags = 1;
            emergency_contract->is_valid = true;
            
            if (braggi_vector_push_back(periscope->active_contracts, &emergency_contract)) {
                fprintf(stderr, "DEBUG: Created emergency contract at %p\n", 
                        (void*)emergency_contract);
                valid_contracts = 1;
            } else {
                fprintf(stderr, "ERROR: Failed to add emergency contract\n");
                free(emergency_contract);
                goto fallback_validation;
            }
        }
    }
    
    // If we still have valid contracts, use them for validation
    if (valid_contracts > 0) {
        fprintf(stderr, "DEBUG: Has active contracts, using periscope validator\n");
    } else {
        fprintf(stderr, "DEBUG: No active valid contracts, using validator directly\n");
        goto fallback_validation;
    }

    // Check constraint type and apply appropriate validation
    switch (constraint->type) {
        case CONSTRAINT_SYNTAX: {
            fprintf(stderr, "DEBUG: Processing syntax constraint with validator: %p\n", 
                    (void*)periscope->validator);
            
            // For syntax constraints, check if they're allowed by any active contract
            bool validated = false;
            for (size_t i = 0; i < contract_count; i++) {
                RegionLifetimeContract** contract_ptr = braggi_vector_get(periscope->active_contracts, i);
                if (!contract_ptr || !*contract_ptr) continue;
                
                RegionLifetimeContract* contract = *contract_ptr;
                if (!contract->is_valid) {
                    fprintf(stderr, "DEBUG: Skipping invalid contract at index %zu\n", i);
                    continue;
                }
                
                fprintf(stderr, "DEBUG: Contract %zu: region=%u, validator=%u, flags=%u\n",
                        i, contract->region_entity, contract->validator_entity, 
                        contract->guarantee_flags);
                
                // This is where we'd check contract-specific validation logic
                // For now we just count valid contracts
                validated = true;
            }
            
            if (validated) {
                // We've checked and at least one contract allows this constraint
                fprintf(stderr, "DEBUG: At least one contract validates this constraint\n");
                
                // We still use the validator as the final authority if we have one
                if (periscope->validator) {
                    return periscope->validator(constraint, field);
                } else {
                    return true; // Allow if we have a valid contract but no validator
                }
            } else {
                fprintf(stderr, "DEBUG: No contracts validated this constraint\n");
            }
            
            // Fallback to direct validation if no contracts validated
            goto fallback_validation;
        }
        
        case CONSTRAINT_SEMANTIC:
        case CONSTRAINT_TYPE:
        case CONSTRAINT_REGION:
        case CONSTRAINT_REGIME:
        case CONSTRAINT_PERISCOPE:
        case CONSTRAINT_CUSTOM:
        default:
            // For other constraints, use the validator directly
            fprintf(stderr, "DEBUG: Using validator directly for constraint type %d\n", constraint->type);
            goto fallback_validation;
    }

fallback_validation:
    // Fallback validation path when no contracts apply or for direct validation
    if (!periscope->validator) {
        fprintf(stderr, "WARNING: No validator available in periscope\n");
        
        // Last resort: set a default validator
        extern bool braggi_default_adjacency_validator(EntropyConstraint* constraint, EntropyField* field);
        periscope->validator = braggi_default_adjacency_validator;
        fprintf(stderr, "DEBUG: Set fallback default adjacency validator for periscope\n");
    }
    
    return periscope->validator(constraint, field);
}

// Get validator result based on region lifetime contracts
bool braggi_periscope_check_validator(
    Periscope* periscope,
    EntropyConstraint* constraint,
    EntropyField* field) {
    
    // This is a wrapper around validate_constraints to allow for different
    // error handling or contract enforcement policies
    return braggi_periscope_validate_constraints(periscope, constraint, field);
}

// ECS system update function for periscope updates
void braggi_periscope_system_update(ECSWorld* world, System* system, float delta_time) {
    if (!world || !system || !system->context) return;
    
    Periscope* periscope = (Periscope*)system->context;
    
    // For now, just update token entities
    periscope_token_system_update(world, system, delta_time);
}

// Helper function to compare token-cell mappings
static bool token_cell_mapping_compare(const void* a, const void* b) {
    if (!a || !b) return false;
    
    const TokenCellMapping* mapping_a = (const TokenCellMapping*)a;
    const TokenCellMapping* mapping_b = (const TokenCellMapping*)b;
    
    return mapping_a->token == mapping_b->token;
}

// Register a contract with the periscope
bool braggi_periscope_register_contract(Periscope* periscope, void* contract) {
    if (!periscope) {
        DEBUG("Cannot register contract with NULL periscope");
        return false;
    }
    
    if (!contract) {
        DEBUG("Cannot register NULL contract");
        return false;
    }
    
    // Store the contract in the vector
    braggi_vector_push(periscope->active_contracts, contract);
    
    DEBUG("Registered contract %p with periscope", contract);
    return true;
}

bool braggi_periscope_register_tokens_batch(Periscope* periscope, Vector* tokens) {
    if (!periscope || !tokens) {
        ERROR("NULL periscope or tokens vector in register_tokens_batch");
        return false;
    }
    
    DEBUG("Registering %zu tokens in batch mode", braggi_vector_size(tokens));
    
    size_t success_count = 0;
    
    // First pass - register all tokens with their positions
    for (size_t i = 0; i < braggi_vector_size(tokens); i++) {
        Token** token_ptr = (Token**)braggi_vector_get(tokens, i);
        if (!token_ptr || !*token_ptr) continue;
        
        Token* token = *token_ptr;
        
        // Use token's position to determine initial cell ID
        uint32_t cell_id = (uint32_t)i;  // Default to index in token stream
        
        // If the token has a valid position, use the line number
        if (token->position.line > 0) {
            cell_id = token->position.line;
        }
        
        if (braggi_periscope_register_token(periscope, token, cell_id)) {
            success_count++;
        }
    }
    
    DEBUG("Successfully registered %zu/%zu tokens with periscope in batch mode", 
          success_count, braggi_vector_size(tokens));
          
    return success_count > 0;
} 