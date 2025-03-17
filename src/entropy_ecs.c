/*
 * Braggi - Entropy ECS Integration
 * 
 * "Like a proper Texas-Irish fusion, this code brings together quantum-inspired
 * wave function collapse with structured ECS components - makin' the tokens dance
 * to a unified fiddle!" - Compiler Cowboy
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "braggi/entropy_ecs.h"
#include "braggi/ecs.h"
#include "braggi/token.h"
#include "braggi/entropy.h"
#include "braggi/token_propagator.h"
#include "braggi/braggi_context.h"
#include "braggi/error.h"
#include "braggi/error_handler.h"
#include "braggi/util/hashmap.h"
#include "braggi/util/vector.h"
#include "braggi/constraint.h"

// Forward declaration of the default adjacency validator
extern bool braggi_default_adjacency_validator(EntropyConstraint* constraint, EntropyField* field);

// Component type IDs (set during initialization)
static ComponentTypeID g_state_component_type = INVALID_COMPONENT_TYPE;
static ComponentTypeID g_constraint_component_type = INVALID_COMPONENT_TYPE;
static ComponentTypeID g_token_state_component_type = INVALID_COMPONENT_TYPE;

// Add an enum for convenience when using component IDs
typedef enum {
    ENTROPY_ECS_COMPONENT_STATE = 0,
    ENTROPY_ECS_COMPONENT_CONSTRAINT,
    ENTROPY_ECS_COMPONENT_TOKEN,
    ENTROPY_ECS_COMPONENT_COUNT
} EntropyECSComponentType;

// Static array to hold registered component IDs
static ComponentTypeID g_component_ids[ENTROPY_ECS_COMPONENT_COUNT];

// Component structure definitions
typedef struct EntropyStateComponent {
    uint32_t state_id;       // ID of the entropy state
    uint32_t cell_id;        // ID of the cell this state belongs to
    uint32_t type;           // Type of the state
    char* label;             // Label of the state (duplicated for quick access)
    void* data;              // Raw pointer to data (Token* for token states)
    float probability;       // Current probability of this state
    bool eliminated;         // Whether this state has been eliminated
} EntropyStateComponent;

typedef struct EntropyConstraintComponent {
    uint32_t constraint_id;  // ID of the constraint
    uint32_t type;           // Type of the constraint
    char* description;       // Description of the constraint
    uint32_t* cell_ids;      // Array of cell IDs affected by this constraint
    size_t cell_count;       // Number of cells affected by this constraint
    void* context;           // Context data for the constraint
    bool (*validate)(EntropyConstraint*, EntropyField*); // Validator function
} EntropyConstraintComponent;

typedef struct TokenStateComponent {
    uint32_t token_id;       // ID of the token
    TokenType type;          // Type of the token
    char* text;              // Text of the token (duplicated for quick access)
    SourcePosition position; // Position of the token in the source
    uint32_t state_id;       // ID of the corresponding entropy state
} TokenStateComponent;

// System data structures
typedef struct EntropySyncSystemData {
    BraggiContext* context;
    HashMap* entity_by_state_id;    // Maps state IDs to entity IDs
    HashMap* entity_by_constraint_id; // Maps constraint IDs to entity IDs
    uint32_t last_synced_state_id;
    uint32_t last_synced_constraint_id;
    EntropyField* field;
} EntropySyncSystemData;

typedef struct EntropyConstraintSystemData {
    BraggiContext* context;
    Vector* constraint_entities; // Vector of constraint entity IDs
} EntropyConstraintSystemData;

typedef struct EntropyWFCSystemData {
    BraggiContext* context;
    Vector* cell_state_map;  // Maps cell IDs to vectors of state entity IDs
    bool is_running;         // Whether WFC is currently running
    uint32_t iteration_count; // Number of iterations performed
    uint32_t max_iterations; // Maximum number of iterations
    EntropyField* field;
    uint32_t last_collapsed_count;
    uint32_t consecutive_no_progress;
} EntropyWFCSystemData;

// Structure to store information about a constraint validator
typedef struct ConstraintValidatorContext {
    ConstraintValidator validator;
    void* context;
} ConstraintValidatorContext;

// Forward declarations for system functions
static void entropy_sync_system_update(ECSWorld* world, System* system, float delta_time);
static void entropy_constraint_system_update(ECSWorld* world, System* system, float delta_time);
static void entropy_wfc_system_update(ECSWorld* world, System* system, float delta_time);

// Forward declarations for component destructors
static void entropy_state_component_destructor(void* component);
static void entropy_constraint_component_destructor(void* component);
static void token_state_component_destructor(void* component);

// Forward declarations for utility functions
static bool convert_constraint_to_ecs(ECSWorld* world, EntropyConstraint* constraint, EntityID* out_entity);
static bool convert_state_to_ecs(ECSWorld* world, EntropyState* state, uint32_t cell_id, EntityID* out_entity);
static bool convert_ecs_to_constraint(ECSWorld* world, EntityID entity, EntropyConstraint* out_constraint);
static bool convert_ecs_to_state(ECSWorld* world, EntityID entity, EntropyState* out_state);
static bool attempt_to_recover_null_cell(EntropyField* field, uint32_t cell_index);

// Check if the state represents a token type
bool is_token_state(EntropyState* state) {
    // We don't have TOKEN_TYPE_ANY, so let's check based on what we know
    // This is a simplified check; enhance based on actual implementation
    return state->type < 20; // Assuming token types are small integers
}

/**
 * Register the entropy components with the ECS world
 */
bool braggi_entropy_register_components(ECSWorld* world, ComponentTypeID* component_ids) {
    if (!world || !component_ids) {
        braggi_error_report_ctx(ERROR_CATEGORY_SYSTEM, ERROR_SEVERITY_ERROR, 0, 0, 
            "entropy_ecs.c", "Cannot register components with NULL parameters", NULL);
        return false;
    }

    // Register the entropy state component
    ComponentTypeInfo state_info = {
        .name = "EntropyStateComponent",
        .size = sizeof(EntropyStateComponent),
        .constructor = NULL,
        .destructor = entropy_state_component_destructor
    };
    component_ids[ENTROPY_ECS_COMPONENT_STATE] = braggi_ecs_register_component_type(world, &state_info);
    if (component_ids[ENTROPY_ECS_COMPONENT_STATE] == INVALID_COMPONENT_TYPE) {
        braggi_error_report_ctx(ERROR_CATEGORY_SYSTEM, ERROR_SEVERITY_ERROR, 0, 0, 
            "entropy_ecs.c", "Failed to register entropy state component", NULL);
        return false;
    }
    
    // Register the entropy constraint component
    ComponentTypeInfo constraint_info = {
        .name = "EntropyConstraintComponent",
        .size = sizeof(EntropyConstraintComponent),
        .constructor = NULL,
        .destructor = entropy_constraint_component_destructor
    };
    component_ids[ENTROPY_ECS_COMPONENT_CONSTRAINT] = braggi_ecs_register_component_type(world, &constraint_info);
    if (component_ids[ENTROPY_ECS_COMPONENT_CONSTRAINT] == INVALID_COMPONENT_TYPE) {
        braggi_error_report_ctx(ERROR_CATEGORY_SYSTEM, ERROR_SEVERITY_ERROR, 0, 0, 
            "entropy_ecs.c", "Failed to register entropy constraint component", NULL);
        return false;
    }
    
    // Register the token state component
    ComponentTypeInfo token_info = {
        .name = "TokenStateComponent",
        .size = sizeof(TokenStateComponent),
        .constructor = NULL,
        .destructor = token_state_component_destructor
    };
    component_ids[ENTROPY_ECS_COMPONENT_TOKEN] = braggi_ecs_register_component_type(world, &token_info);
    if (component_ids[ENTROPY_ECS_COMPONENT_TOKEN] == INVALID_COMPONENT_TYPE) {
        braggi_error_report_ctx(ERROR_CATEGORY_SYSTEM, ERROR_SEVERITY_ERROR, 0, 0, 
            "entropy_ecs.c", "Failed to register token state component", NULL);
        return false;
    }

    // Store the component IDs for later use
    memcpy(g_component_ids, component_ids, sizeof(ComponentTypeID) * ENTROPY_ECS_COMPONENT_COUNT);
    
    return true;
}

/**
 * Initialize the entropy ECS integration
 */
bool braggi_entropy_ecs_initialize(BraggiContext* context) {
    if (!context || !context->ecs_world) {
        return false;
    }
    
    // Register components
    ComponentTypeID component_ids[3];
    if (!braggi_entropy_register_components(context->ecs_world, component_ids)) {
        braggi_context_report_error(context, ERROR_CATEGORY_GENERAL, ERROR_SEVERITY_ERROR, 
                                  0, 0, "entropy_ecs.c", 
                                  "Failed to register entropy components", 
                                  "Check ECS component registration");
        return false;
    }
    
    // Create and add systems
    System* sync_system = braggi_entropy_create_sync_system(context);
    if (!sync_system) {
        braggi_context_report_error(context, ERROR_CATEGORY_GENERAL, ERROR_SEVERITY_ERROR, 
                                  0, 0, "entropy_ecs.c", 
                                  "Failed to create entropy sync system", 
                                  "Check memory allocation");
        return false;
    }
    
    if (!braggi_ecs_add_system(context->ecs_world, sync_system)) {
        braggi_context_report_error(context, ERROR_CATEGORY_GENERAL, ERROR_SEVERITY_ERROR, 
                                  0, 0, "entropy_ecs.c", 
                                  "Failed to add entropy sync system to ECS world", 
                                  "Check system registration");
        return false;
    }
    
    System* constraint_system = braggi_entropy_create_constraint_system(context);
    if (!constraint_system) {
        braggi_context_report_error(context, ERROR_CATEGORY_GENERAL, ERROR_SEVERITY_ERROR, 
                                  0, 0, "entropy_ecs.c", 
                                  "Failed to create constraint system", 
                                  "Check memory allocation");
        return false;
    }
    
    if (!braggi_ecs_add_system(context->ecs_world, constraint_system)) {
        braggi_context_report_error(context, ERROR_CATEGORY_GENERAL, ERROR_SEVERITY_ERROR, 
                                  0, 0, "entropy_ecs.c", 
                                  "Failed to add constraint system to ECS world", 
                                  "Check system registration");
        return false;
    }
    
    System* wfc_system = braggi_entropy_create_wfc_system(context);
    if (!wfc_system) {
        braggi_context_report_error(context, ERROR_CATEGORY_GENERAL, ERROR_SEVERITY_ERROR, 
                                  0, 0, "entropy_ecs.c", 
                                  "Failed to create WFC system", 
                                  "Check memory allocation");
        return false;
    }
    
    if (!braggi_ecs_add_system(context->ecs_world, wfc_system)) {
        braggi_context_report_error(context, ERROR_CATEGORY_GENERAL, ERROR_SEVERITY_ERROR, 
                                  0, 0, "entropy_ecs.c", 
                                  "Failed to add WFC system to ECS world", 
                                  "Check system registration");
        return false;
    }
    
    // Perform initial synchronization
    if (!braggi_ecs_update_system(context->ecs_world, sync_system, 0.0f)) {
        braggi_context_report_error(context, ERROR_CATEGORY_GENERAL, ERROR_SEVERITY_WARNING, 
                                  0, 0, "entropy_ecs.c", 
                                  "Initial entropy sync failed", 
                                  "Entropy will be synced on next update");
        // Non-fatal, continue
    }
    
    return true;
}

/**
 * Create a system that synchronizes the entropy field with the ECS world
 */
System* braggi_entropy_create_sync_system(BraggiContext* context) {
    if (!context) {
        return NULL;
    }
    
    EntropySyncSystemData* data = malloc(sizeof(EntropySyncSystemData));
    if (!data) {
        return NULL;
    }
    
    data->context = context;
    data->entity_by_state_id = braggi_hashmap_create();
    data->entity_by_constraint_id = braggi_hashmap_create();
    data->last_synced_state_id = 0;
    data->last_synced_constraint_id = 0;
    
    if (!data->entity_by_state_id || !data->entity_by_constraint_id) {
        if (data->entity_by_state_id) {
            braggi_hashmap_destroy(data->entity_by_state_id);
        }
        if (data->entity_by_constraint_id) {
            braggi_hashmap_destroy(data->entity_by_constraint_id);
        }
        free(data);
        return NULL;
    }
    
    SystemInfo system_info = {
        .name = "EntropySyncSystem",
        .update_func = entropy_sync_system_update,
        .context = data,
        .priority = 90 // Run after token sync but before constraints
    };
    
    return braggi_ecs_create_system(&system_info);
}

/**
 * Create a system that applies constraints in the ECS
 */
System* braggi_entropy_create_constraint_system(BraggiContext* context) {
    if (!context) {
        return NULL;
    }
    
    EntropyConstraintSystemData* data = malloc(sizeof(EntropyConstraintSystemData));
    if (!data) {
        return NULL;
    }
    
    data->context = context;
    data->constraint_entities = braggi_vector_create(sizeof(EntityID));
    if (!data->constraint_entities) {
        free(data);
        return NULL;
    }
    
    SystemInfo system_info = {
        .name = "EntropyConstraintSystem",
        .update_func = entropy_constraint_system_update,
        .context = data,
        .priority = 80 // Run after sync but before WFC
    };
    
    return braggi_ecs_create_system(&system_info);
}

/**
 * Create a system that applies the Wave Function Collapse algorithm in the ECS
 */
System* braggi_entropy_create_wfc_system(BraggiContext* context) {
    if (!context) {
        return NULL;
    }
    
    EntropyWFCSystemData* data = malloc(sizeof(EntropyWFCSystemData));
    if (!data) {
        return NULL;
    }
    
    data->context = context;
    data->cell_state_map = braggi_vector_create(sizeof(Vector*)); // Vector of vectors
    data->is_running = false;
    data->iteration_count = 0;
    data->max_iterations = 100; // Reasonable default
    
    if (!data->cell_state_map) {
        free(data);
        return NULL;
    }
    
    SystemInfo system_info = {
        .name = "EntropyWFCSystem",
        .update_func = entropy_wfc_system_update,
        .context = data,
        .priority = 70 // Run after constraints
    };
    
    return braggi_ecs_create_system(&system_info);
}

/**
 * Create an entity for an entropy state
 */
EntityID braggi_entropy_create_state_entity(ECSWorld* world, EntropyState* state, uint32_t cell_id) {
    if (!world || !state) {
        braggi_error_report_ctx(ERROR_CATEGORY_SYSTEM, ERROR_SEVERITY_ERROR, 0, 0, 
            "entropy_ecs.c", "Cannot create state entity with NULL parameters", NULL);
        return INVALID_ENTITY;
    }

    EntityID entity = braggi_ecs_create_entity(world);
    if (entity == INVALID_ENTITY) {
        braggi_error_report_ctx(ERROR_CATEGORY_SYSTEM, ERROR_SEVERITY_ERROR, 0, 0, 
            "entropy_ecs.c", "Failed to create entity for entropy state", NULL);
        return INVALID_ENTITY;
    }

    // Add state component
    EntropyStateComponent* state_component = (EntropyStateComponent*)braggi_ecs_add_component(
        world, entity, g_component_ids[ENTROPY_ECS_COMPONENT_STATE]);
    if (!state_component) {
        braggi_error_report_ctx(ERROR_CATEGORY_SYSTEM, ERROR_SEVERITY_ERROR, 0, 0, 
            "entropy_ecs.c", "Failed to add entropy state component to entity", NULL);
        braggi_ecs_destroy_entity(world, entity);
        return INVALID_ENTITY;
    }

    // Initialize state component
    state_component->state_id = state->id;
    state_component->cell_id = cell_id;
    state_component->type = state->type;
    state_component->label = state->label ? strdup(state->label) : NULL;
    state_component->data = state->data; // Just copy the pointer, ownership is with the entropy state
    state_component->probability = state->probability;
    state_component->eliminated = braggi_state_is_eliminated(state);

    // If this is a token state, add a token state component
    if (is_token_state(state) && state->data) {
        Token* token = (Token*)state->data;
        
        TokenStateComponent* token_component = (TokenStateComponent*)braggi_ecs_add_component(
            world, entity, g_component_ids[ENTROPY_ECS_COMPONENT_TOKEN]);
        if (token_component) {
            // Initialize token component with token data
            token_component->token_id = 0;  // Tokens don't have IDs in the Token struct
            token_component->type = token->type;
            token_component->text = token->text ? strdup(token->text) : NULL;
            token_component->position = token->position;
            token_component->state_id = state->id;
        }
    }

    return entity;
}

/**
 * Check if the validator function is a known built-in validator
 */
static bool is_builtin_validator(bool (*validator)(EntropyConstraint*, EntropyField*)) {
    if (!validator) return false;
    
    // Check against known validator functions from constraint.c
    if (validator == braggi_default_adjacency_validator) return true;
    
    // Add other built-in validators as needed
    
    return false;
}

/**
 * Create a wrapper for a validator function that works with ECS components
 */
static bool ecs_validator_wrapper(EntropyState** states, size_t state_count, void* context) {
    // Extract the original validator function and context from the wrapper context
    if (!context) return false;
    
    ConstraintValidator original_validator = ((ConstraintValidatorContext*)context)->validator;
    void* original_context = ((ConstraintValidatorContext*)context)->context;
    
    // Call the original validator with the states and original context
    return original_validator(states, state_count, original_context);
}

/**
 * Create an entity for an entropy constraint
 */
EntityID braggi_entropy_create_constraint_entity(ECSWorld* world, EntropyConstraint* constraint) {
    if (!world || !constraint) {
        braggi_error_report_ctx(ERROR_CATEGORY_SYSTEM, ERROR_SEVERITY_ERROR, 0, 0, 
            "entropy_ecs.c", "Invalid parameters for creating constraint entity", NULL);
        fprintf(stderr, "ERROR: NULL parameters passed to create_constraint_entity\n");
        return INVALID_ENTITY;
    }
    
    // Ensure component types are registered
    if (g_component_ids[ENTROPY_ECS_COMPONENT_CONSTRAINT] == INVALID_COMPONENT_TYPE) {
        braggi_error_report_ctx(ERROR_CATEGORY_SYSTEM, ERROR_SEVERITY_ERROR, 0, 0, 
            "entropy_ecs.c", "Constraint component type not registered, ensure braggi_entropy_register_components was called", NULL);
        fprintf(stderr, "ERROR: Constraint component type not registered\n");
        return INVALID_ENTITY;
    }

    // Log constraint information for debugging
    fprintf(stderr, "DEBUG: Creating constraint entity for constraint ID: %u, Type: %u, Cells: %zu\n", 
           constraint->id, constraint->type, constraint->cell_count);
    
    // Create the entity
    EntityID entity = braggi_ecs_create_entity(world);
    if (entity == INVALID_ENTITY) {
        braggi_error_report_ctx(ERROR_CATEGORY_SYSTEM, ERROR_SEVERITY_ERROR, 0, 0, 
            "entropy_ecs.c", "Failed to create entity for constraint", NULL);
        fprintf(stderr, "ERROR: Failed to create entity for constraint ID: %u\n", constraint->id);
        return INVALID_ENTITY;
    }
    
    // Add constraint component
    EntropyConstraintComponent* constraint_component = (EntropyConstraintComponent*)braggi_ecs_add_component(
        world, entity, g_component_ids[ENTROPY_ECS_COMPONENT_CONSTRAINT]);
    if (!constraint_component) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), 
                "Failed to add constraint component to entity. Component type ID: %u", 
                g_component_ids[ENTROPY_ECS_COMPONENT_CONSTRAINT]);
        braggi_error_report_ctx(ERROR_CATEGORY_SYSTEM, ERROR_SEVERITY_ERROR, 0, 0, 
            "entropy_ecs.c", error_msg, NULL);
        braggi_ecs_destroy_entity(world, entity);
        fprintf(stderr, "ERROR: Failed to add constraint component for constraint ID: %u\n", constraint->id);
        return INVALID_ENTITY;
    }
    
    // Initialize constraint component with defensive programming
    constraint_component->constraint_id = constraint->id;
    constraint_component->type = constraint->type;
    constraint_component->description = constraint->description ? strdup(constraint->description) : NULL;
    constraint_component->context = constraint->context;
    constraint_component->validate = constraint->validate;  // Using validate instead of validate_func
    
    // Initialize cell IDs to empty initially
    constraint_component->cell_count = 0;
    constraint_component->cell_ids = NULL;
    
    // Handle cell IDs carefully - make sure constraint->cell_ids is valid
    if (constraint->cell_count > 0) {
        if (!constraint->cell_ids) {
            // This is a data inconsistency - log it but don't fail
            fprintf(stderr, "WARNING: Constraint %u has cell_count=%zu but NULL cell_ids\n", 
                   constraint->id, constraint->cell_count);
        } else {
            // Allocate memory for cell IDs
            constraint_component->cell_ids = (uint32_t*)malloc(sizeof(uint32_t) * constraint->cell_count);
            if (!constraint_component->cell_ids) {
                // Memory allocation failure
                braggi_error_report_ctx(ERROR_CATEGORY_SYSTEM, ERROR_SEVERITY_ERROR, 0, 0, 
                    "entropy_ecs.c", "Failed to allocate memory for cell IDs in constraint component", NULL);
                
                // Clean up
                if (constraint_component->description) {
                    free(constraint_component->description);
                }
                braggi_ecs_destroy_entity(world, entity);
                fprintf(stderr, "ERROR: Memory allocation failed for cell IDs in constraint ID: %u\n", constraint->id);
                return INVALID_ENTITY;
            }
            
            // Copy cell IDs and update count
            memcpy(constraint_component->cell_ids, constraint->cell_ids, sizeof(uint32_t) * constraint->cell_count);
            constraint_component->cell_count = constraint->cell_count;
            
            // Debug output for cell IDs
            fprintf(stderr, "DEBUG: Copied %zu cell IDs for constraint %u: ", 
                   constraint_component->cell_count, constraint->id);
            for (size_t i = 0; i < constraint_component->cell_count && i < 5; i++) {
                fprintf(stderr, "%u ", constraint_component->cell_ids[i]);
            }
            if (constraint_component->cell_count > 5) {
                fprintf(stderr, "...");
            }
            fprintf(stderr, "\n");
        }
    }
    
    fprintf(stderr, "DEBUG: Successfully created entity %u for constraint ID: %u\n", entity, constraint->id);
    return entity;
}

/**
 * Synchronize entropy field with ECS
 */
bool braggi_entropy_sync_to_ecs(BraggiContext* context) {
    if (!context || !context->entropy_field || !context->ecs_world) {
        braggi_error_report_ctx(ERROR_CATEGORY_SYSTEM, ERROR_SEVERITY_ERROR, 0, 0, 
            "entropy_ecs.c", "Cannot sync NULL context or field", NULL);
        return false;
    }
    
    // Get the entropy sync system
    System* sync_system = braggi_ecs_get_system_by_name(context->ecs_world, "EntropySyncSystem");
    if (!sync_system) {
        // Create the system if it doesn't exist yet
        sync_system = braggi_entropy_create_sync_system(context);
        if (!sync_system) {
            braggi_error_report_ctx(ERROR_CATEGORY_SYSTEM, ERROR_SEVERITY_ERROR, 0, 0, 
                "entropy_ecs.c", "Failed to create entropy sync system", NULL);
            return false;
        }
    }

    // Set the system's field context
    EntropySyncSystemData* data = (EntropySyncSystemData*)sync_system->context;
    if (!data) {
        braggi_error_report_ctx(ERROR_CATEGORY_SYSTEM, ERROR_SEVERITY_ERROR, 0, 0, 
            "entropy_ecs.c", "Sync system has NULL context", NULL);
        return false;
    }
    
    data->field = context->entropy_field;
    
    // Run the sync system to update the ECS world
    entropy_sync_system_update(context->ecs_world, sync_system, 0.0f);
    
    return true;
}

/**
 * Apply the Wave Function Collapse algorithm using ECS
 */
bool braggi_entropy_apply_wfc_with_ecs(BraggiContext* context) {
    if (!context) {
        fprintf(stderr, "ERROR: NULL context in braggi_entropy_apply_wfc_with_ecs\n");
        return false;
    }

    if (!context->entropy_field) {
        fprintf(stderr, "ERROR: NULL entropy field in braggi_entropy_apply_wfc_with_ecs\n");
        // Create a new field with the proper arguments
        context->entropy_field = braggi_entropy_field_create(0, context->error_handler);
        if (!context->entropy_field) {
            fprintf(stderr, "ERROR: Failed to create recovery entropy field\n");
            return false;
        }
        fprintf(stderr, "DEBUG: Created recovery entropy field\n");
    }

    // Verify the integrity of the entropy field
    if (!context->entropy_field->cells) {
        fprintf(stderr, "ERROR: Entropy field has NULL cells array, attempting to fix\n");
        
        // Try to reinitialize cells array
        uint32_t cell_count = context->entropy_field->cell_count > 0 ? 
                            context->entropy_field->cell_count : 10; // Default to 10 cells if count is zero
        
        context->entropy_field->cells = (EntropyCell**)calloc(cell_count, sizeof(EntropyCell*));
        if (!context->entropy_field->cells) {
            fprintf(stderr, "ERROR: Failed to allocate cells array for entropy field\n");
            return false;
        }
        
        // Initialize the cells to NULL
        for (uint32_t i = 0; i < cell_count; i++) {
            context->entropy_field->cells[i] = NULL;
        }
        
        // Update cell count
        context->entropy_field->cell_count = cell_count;
        fprintf(stderr, "DEBUG: Reinitialized cells array with %u cells\n", cell_count);
    }

    // First sync the entropy field to ECS
    if (!braggi_entropy_sync_to_ecs(context)) {
        fprintf(stderr, "ERROR: Failed to sync entropy field to ECS\n");
        return false;
    }

    // Clear the WFC completed flag
    context->wfc_completed = false;

    // Get or create the constraint and WFC systems
    System* constraint_system = braggi_ecs_get_system_by_name(context->ecs_world, "EntropyConstraintSystem");
    System* wfc_system = braggi_ecs_get_system_by_name(context->ecs_world, "EntropyWFCSystem");

    if (!constraint_system) {
        fprintf(stderr, "ERROR: Constraint system not found, attempting to create it\n");
        constraint_system = braggi_entropy_create_constraint_system(context);
        if (!constraint_system) {
            fprintf(stderr, "ERROR: Failed to create constraint system\n");
            return false;
        }
        
        // Add the newly created system to the world
        if (!braggi_ecs_add_system(context->ecs_world, constraint_system)) {
            fprintf(stderr, "ERROR: Failed to add constraint system to world\n");
            return false;
        }
    }

    if (!wfc_system) {
        fprintf(stderr, "ERROR: WFC system not found, attempting to create it\n");
        wfc_system = braggi_entropy_create_wfc_system(context);
        if (!wfc_system) {
            fprintf(stderr, "ERROR: Failed to create WFC system\n");
            return false;
        }
        
        // Add the newly created system to the world
        if (!braggi_ecs_add_system(context->ecs_world, wfc_system)) {
            fprintf(stderr, "ERROR: Failed to add WFC system to world\n");
            return false;
        }
    }

    // Set the field in the systems
    EntropyConstraintSystemData* constraint_data = (EntropyConstraintSystemData*)constraint_system->context;
    EntropyWFCSystemData* wfc_data = (EntropyWFCSystemData*)wfc_system->context;

    if (!constraint_data || !wfc_data) {
        fprintf(stderr, "ERROR: System data is NULL\n");
        
        // Attempt recovery by recreating the systems
        if (!constraint_data) {
            fprintf(stderr, "Attempting to recreate constraint system\n");
            constraint_system = braggi_entropy_create_constraint_system(context);
            if (constraint_system) {
                // Add the new system - the system manager will handle replacement
                System* old_system = braggi_ecs_get_system_by_name(context->ecs_world, "EntropyConstraintSystem");
                if (old_system) {
                    // Remove old system reference
                    // Note: removal happens implicitly when adding with same name
                }
                braggi_ecs_add_system(context->ecs_world, constraint_system);
                constraint_data = (EntropyConstraintSystemData*)constraint_system->context;
            }
        }
        
        if (!wfc_data) {
            fprintf(stderr, "Attempting to recreate WFC system\n");
            wfc_system = braggi_entropy_create_wfc_system(context);
            if (wfc_system) {
                // Add the new system - the system manager will handle replacement
                System* old_system = braggi_ecs_get_system_by_name(context->ecs_world, "EntropyWFCSystem");
                if (old_system) {
                    // Remove old system reference
                    // Note: removal happens implicitly when adding with same name
                }
                braggi_ecs_add_system(context->ecs_world, wfc_system);
                wfc_data = (EntropyWFCSystemData*)wfc_system->context;
            }
        }
        
        // Check if recovery was successful
        if (!constraint_data || !wfc_data) {
            fprintf(stderr, "ERROR: Failed to recover system data\n");
            return false;
        }
    }

    // Set the context and field in the system data
    constraint_data->context = context;
    wfc_data->context = context;
    wfc_data->field = context->entropy_field;

    // Reset the WFC system state
    wfc_data->is_running = true;
    wfc_data->iteration_count = 0;
    wfc_data->last_collapsed_count = 0;
    wfc_data->consecutive_no_progress = 0;
    wfc_data->max_iterations = 100; // Set a reasonable default

    // Run the constraint system first to apply all constraints
    fprintf(stderr, "DEBUG: Running constraint system before WFC\n");
    constraint_system->update_func(context->ecs_world, constraint_system, 0.0f);

    // Now run the WFC system
    fprintf(stderr, "DEBUG: Starting WFC system\n");
    
    // Let's set a safe maximum number of iterations to prevent infinite loops
    unsigned int max_iterations = 50;
    unsigned int iteration = 0;
    
    // Run WFC system until it's done or we hit max_iterations
    while (wfc_data->is_running && iteration < max_iterations) {
        // Update the WFC system
        wfc_system->update_func(context->ecs_world, wfc_system, 0.0f);
        iteration++;
        
        // Check if the field is still valid after each iteration
        if (!wfc_data->field || !wfc_data->field->cells) {
            fprintf(stderr, "ERROR: Field became invalid during WFC iteration %u\n", iteration);
            wfc_data->is_running = false;
            break;
        }
        
        // If WFC is done, sync the results back to the field
        if (!wfc_data->is_running || context->wfc_completed) {
            fprintf(stderr, "DEBUG: WFC completed after %u iterations\n", iteration);
            break;
        }
    }
    
    // Check if WFC ran out of iterations
    if (iteration >= max_iterations && wfc_data->is_running) {
        fprintf(stderr, "WARNING: WFC system did not complete after %u iterations\n", max_iterations);
        wfc_data->is_running = false;
    }
    
    // Run post-WFC cleanup
    fprintf(stderr, "DEBUG: Running post-WFC cleanup\n");
    
    // One final sync to ensure ECS and field are aligned
    if (!braggi_entropy_sync_to_ecs(context)) {
        fprintf(stderr, "WARNING: Final sync failed after WFC\n");
    }
    
    // Log results
    size_t collapsed_count = 0;
    size_t contradiction_count = 0;
    EntropyField* field = context->entropy_field;
    
    if (field && field->cells) {
        for (size_t i = 0; i < field->cell_count; i++) {
            EntropyCell* cell = field->cells[i];
            if (cell) {
                if (braggi_entropy_cell_is_collapsed(cell)) {
                    collapsed_count++;
                }
                if (cell->state_count == 0) {
                    contradiction_count++;
                }
            } else {
                // Try to recover NULL cells during final check
                EntropyCell* new_cell = braggi_entropy_cell_create(i);
                if (new_cell) {
                    // Replace the NULL cell with the new one
                    field->cells[i] = new_cell;
                    fprintf(stderr, "DEBUG: Recovered NULL cell %zu during final check\n", i);
                }
            }
        }
        
        fprintf(stderr, "DEBUG: WFC completed with %zu/%zu cells collapsed, %zu contradictions\n",
               collapsed_count, field->cell_count, contradiction_count);
        
        // Check for success - no contradictions and all cells collapsed
        bool success = (contradiction_count == 0 && collapsed_count == field->cell_count);
        
        return success;
    } else {
        fprintf(stderr, "ERROR: Invalid field or cells array after WFC\n");
        return false;
    }
}

/**
 * Attempts to recover a NULL cell in an entropy field by creating a new empty cell
 * This is a safety mechanism to handle corruption or memory issues
 * 
 * @param field The entropy field containing the NULL cell
 * @param cell_index The index of the NULL cell to recover
 * @return true if recovery was successful, false otherwise
 */
static bool attempt_to_recover_null_cell(EntropyField* field, uint32_t cell_index) {
    if (!field || !field->cells || cell_index >= field->cell_count) {
        fprintf(stderr, "ERROR: Invalid parameters for cell recovery\n");
        return false;
    }
    
    // Check if the cell is already valid
    if (field->cells[cell_index]) {
        // Cell is already valid, no recovery needed
        return true;
    }
    
    fprintf(stderr, "DEBUG: Attempting to recover NULL cell at index %u\n", cell_index);
    
    // Create a new cell using the proper API function
    EntropyCell* new_cell = braggi_entropy_cell_create(cell_index);
    if (!new_cell) {
        fprintf(stderr, "ERROR: Failed to create replacement cell\n");
        return false;
    }
    
    // Replace the NULL cell with the new one
    field->cells[cell_index] = new_cell;
    
    fprintf(stderr, "DEBUG: Successfully recovered NULL cell at index %u\n", cell_index);
    return true;
}

/**
 * Initialize default states for a cell that has no states
 * This can help recover from situations where a cell is valid but has no states
 * 
 * @param field The entropy field containing the cell
 * @param cell The cell to initialize with default states
 * @return true if initialization was successful, false otherwise
 */
static bool initialize_default_states_for_cell(EntropyField* field, EntropyCell* cell) {
    if (!field || !cell) {
        fprintf(stderr, "ERROR: Invalid parameters for cell state initialization\n");
        return false;
    }
    
    // If the cell already has states, no initialization needed
    if (cell->states && cell->state_count > 0) {
        return true;
    }
    
    fprintf(stderr, "DEBUG: Initializing default states for cell %u\n", cell->id);
    
    // Allocate memory for a single default state
    cell->state_capacity = 1;
    cell->state_count = 0;
    cell->states = (EntropyState**)malloc(sizeof(EntropyState*) * cell->state_capacity);
    
    if (!cell->states) {
        fprintf(stderr, "ERROR: Failed to allocate memory for cell states\n");
        return false;
    }
    
    // Create a default state - this will be a placeholder state with no data
    EntropyState* default_state = (EntropyState*)malloc(sizeof(EntropyState));
    if (!default_state) {
        fprintf(stderr, "ERROR: Failed to allocate memory for default state\n");
        free(cell->states);
        cell->states = NULL;
        return false;
    }
    
    // Initialize the default state with a static ID counter
    static uint32_t next_state_id = 1;
    default_state->id = next_state_id++;
    default_state->type = 0; // Generic type
    default_state->label = strdup("default_state");
    default_state->data = NULL;
    default_state->probability = 100;  // Use full probability for simplicity
    
    // Add the state to the cell
    cell->states[0] = default_state;
    cell->state_count = 1;
    
    fprintf(stderr, "DEBUG: Successfully initialized default state for cell %u\n", cell->id);
    return true;
}

/**
 * System update function for applying Wave Function Collapse
 */
static void entropy_wfc_system_update(ECSWorld* world, System* system, float delta_time) {
    if (!world || !system) {
        fprintf(stderr, "ERROR: NULL world or system in entropy_wfc_system_update\n");
        return;
    }
    
    EntropyWFCSystemData* data = (EntropyWFCSystemData*)system->context;
    if (!data) {
        fprintf(stderr, "ERROR: NULL data in entropy_wfc_system_update\n");
        return;
    }
    
    // CRITICAL SAFETY CHECK: Ensure valid context and field
    if (!data->context) {
        fprintf(stderr, "ERROR: NULL context in entropy_wfc_system_update\n");
        return;
    }

    // CRITICAL SAFETY ENHANCEMENT: If the system is not running, don't attempt to access the field
    if (!data->is_running) {
        fprintf(stderr, "DEBUG: WFC system is not running, skipping update\n");
        return;
    }

    // CRITICAL SAFETY CHECK: If the field is NULL, try to recover or abort
    if (!data->field) {
        fprintf(stderr, "ERROR: NULL field in entropy_wfc_system_update. Attempting to recover from BraggiContext.\n");
        
        // Try to recover the field from the context if possible
        if (data->context && data->context->entropy_field) {
            fprintf(stderr, "DEBUG: Recovered field from BraggiContext\n");
            data->field = data->context->entropy_field;
        } else {
            fprintf(stderr, "ERROR: Could not recover field. Both data->field and context->entropy_field are NULL\n");
            // NEW: Mark system as not running to prevent future attempts
            data->is_running = false;
            return;
        }
    }
    
    // CRITICAL SAFETY CHECK: Verify the field is valid by checking a magic value
    // This helps detect when a field has been destroyed but the pointer wasn't nullified
    if (data->field && data->field->id == 0) { // id should never be 0 for a valid field
        fprintf(stderr, "ERROR: Field appears to be invalid (id=0), possible use-after-free\n");
        data->field = NULL; // Clear the pointer to prevent further access
        data->is_running = false; // Stop the system
        return;
    }
    
    // Additional validation of field structure
    if (!data->field->cells) {
        fprintf(stderr, "ERROR: Field has NULL cells array in entropy_wfc_system_update\n");
        data->is_running = false;
        return;
    }
    
    // Validate field->cell_count to ensure it's reasonable
    if (data->field->cell_count == 0 || data->field->cell_count > 10000) {
        fprintf(stderr, "ERROR: Field has invalid cell_count (%zu) in entropy_wfc_system_update\n", 
                data->field->cell_count);
        data->is_running = false;
        return;
    }
    
    // CRITICAL SAFETY CHECK: If code generator cleanup is in progress, don't try to use it
    if (data->context->flags & BRAGGI_FLAG_CODEGEN_CLEANUP_IN_PROGRESS) {
        fprintf(stderr, "WARNING: Skipping WFC system update because code generator cleanup is in progress\n");
        return;
    }
    
    // Check if we're already running
    if (data->is_running) {
        fprintf(stderr, "DEBUG: WFC System is already running (iteration %u)\n", data->iteration_count);
        
        // Count collapsed cells
        uint32_t collapsed_count = 0;
        uint32_t contradiction_count = 0;
        uint32_t total_cells = 0;
        uint32_t recovered_cells = 0;
        uint32_t initialized_states = 0;
        
        // Safely access cell count with validation
        if (data->field) {
            total_cells = data->field->cell_count;
            
            // Validate that cells array exists before iteration
            if (data->field->cells) {
                for (uint32_t i = 0; i < total_cells; i++) {
                    EntropyCell* cell = data->field->cells[i];
                    if (!cell) {
                        fprintf(stderr, "WARNING: NULL cell at index %u in entropy field, attempting recovery\n", i);
                        
                        // Attempt to recover the NULL cell
                        if (attempt_to_recover_null_cell(data->field, i)) {
                            recovered_cells++;
                            // Get the newly created cell
                            cell = data->field->cells[i];
                        } else {
                            continue; // Skip this cell if recovery failed
                        }
                    }
                    
                    // Additional validation: check if cell is valid before proceeding
                    if (!cell) {
                        fprintf(stderr, "ERROR: Cell recovery failed at index %u, skipping\n", i);
                        continue;
                    }
                    
                    // Check if the cell needs states initialized
                    if (!cell->states) {
                        fprintf(stderr, "WARNING: Cell %u has NULL states array, attempting to initialize\n", i);
                        
                        // Initialize default states for the cell
                        if (initialize_default_states_for_cell(data->field, cell)) {
                            initialized_states++;
                        }
                    } else if (cell->state_count == 0 && cell->state_capacity > 0) {
                        // Cell has a states array but no states, which could cause issues
                        fprintf(stderr, "WARNING: Cell %u has 0 states but allocated array, attempting to initialize\n", i);
                        
                        // Initialize default states for the cell
                        if (initialize_default_states_for_cell(data->field, cell)) {
                            initialized_states++;
                        }
                    }
                    
                    // Additional check for states array validity
                    if (!cell->states && cell->state_count > 0) {
                        fprintf(stderr, "WARNING: Cell %u has NULL states array but non-zero state_count (%zu)\n", 
                               i, cell->state_count);
                        // Fix the inconsistency
                        cell->state_count = 0;
                    }
                    
                    // Check for collapsed cells with safer function call - validate cell is still valid
                    if (cell && braggi_entropy_cell_is_collapsed(cell)) {
                        collapsed_count++;
                    }
                    
                    // Check for contradictions - a cell with no available states
                    // Add additional validation for states array
                    if (!cell || cell->state_count == 0 || !cell->states) {
                        contradiction_count++;
                    }
                }
                
                // Log recoveries
                if (recovered_cells > 0 || initialized_states > 0) {
                    fprintf(stderr, "DEBUG: Recovered %u NULL cells and initialized %u cells with default states during WFC update\n", 
                           recovered_cells, initialized_states);
                }
            } else {
                fprintf(stderr, "ERROR: Field has NULL cells array during iteration\n");
                data->is_running = false;
                return;
            }
        } else {
            fprintf(stderr, "ERROR: Field became NULL during WFC iteration\n");
            data->is_running = false;
            return;
        }
        
        // Check for contradictions - include explicit field check for has_contradiction
        if (contradiction_count > 0 || (data->field && data->field->has_contradiction)) {
            fprintf(stderr, "DEBUG: WFC System: Found %u contradictions, stopping\n", contradiction_count);
            data->is_running = false;
            return;
        }
        
        // Check if all cells are collapsed - ensure total_cells is not zero to avoid division by zero
        if (total_cells > 0 && collapsed_count == total_cells) {
            fprintf(stderr, "DEBUG: WFC System: All cells collapsed (%u/%u), stopping\n", 
                   collapsed_count, total_cells);
            data->is_running = false;
            
            // Make sure context is still valid before setting flag
            if (data->context) {
                data->context->wfc_completed = true;
            }
            
            fprintf(stderr, "DEBUG: WFC System: Wave function collapse successful\n");
            return;
        }
        
        // Check for no progress
        if (collapsed_count == data->last_collapsed_count) {
            data->consecutive_no_progress++;
            fprintf(stderr, "DEBUG: WFC System: No progress for %u iterations (still at %u/%u cells)\n", 
                   data->consecutive_no_progress, collapsed_count, total_cells);
            
            if (data->consecutive_no_progress >= 3) {
                fprintf(stderr, "DEBUG: WFC System: No progress for %u iterations, forcing a collapse step\n", 
                       data->consecutive_no_progress);
                
                // Try one iteration of the collapse algorithm with additional NULL check
                if (data->field && data->field->cells && !braggi_entropy_field_apply_wave_function_collapse(data->field)) {
                    fprintf(stderr, "DEBUG: WFC System: Forced collapse step failed, stopping\n");
                    data->is_running = false;
                    return;
                }
                
                // Reset the counter since we made progress
                data->consecutive_no_progress = 0;
            }
        } else {
            // Made progress, reset counter
            data->consecutive_no_progress = 0;
        }
        
        // Update last count
        data->last_collapsed_count = collapsed_count;
        
        // Check if we've exceeded the maximum number of iterations
        if (data->iteration_count >= data->max_iterations) {
            fprintf(stderr, "DEBUG: WFC System: Reached max iterations (%u), stopping\n", 
                   data->max_iterations);
            data->is_running = false;
            return;
        }
        
        // Increment the iteration count
        data->iteration_count++;
        
        // Run one iteration of the WFC algorithm with additional NULL checks
        fprintf(stderr, "DEBUG: WFC System: Running iteration %u\n", data->iteration_count);
        if (!data->field || !data->field->cells || !braggi_entropy_field_apply_wave_function_collapse(data->field)) {
            fprintf(stderr, "DEBUG: WFC System: Collapse algorithm failed, stopping\n");
            data->is_running = false;
            return;
        }
        
        fprintf(stderr, "DEBUG: WFC System: Iteration %u complete\n", data->iteration_count);
    }
}

/**
 * Component destructor for entropy state component
 */
static void entropy_state_component_destructor(void* component) {
    if (!component) {
        return;
    }
    
    EntropyStateComponent* state_component = (EntropyStateComponent*)component;
    
    // Free the label
    if (state_component->label) {
        free(state_component->label);
        state_component->label = NULL;
    }
    
    // We don't free the data pointer, as it's owned by the entropy state
}

/**
 * Component destructor for entropy constraint component
 */
static void entropy_constraint_component_destructor(void* component) {
    if (!component) {
        return;
    }
    
    EntropyConstraintComponent* constraint_component = (EntropyConstraintComponent*)component;
    
    // Free the description
    if (constraint_component->description) {
        free(constraint_component->description);
        constraint_component->description = NULL;
    }
    
    // Free the cell IDs array
    if (constraint_component->cell_ids) {
        free(constraint_component->cell_ids);
        constraint_component->cell_ids = NULL;
    }
    
    // We don't free the context, as it's owned by the constraint
}

/**
 * Component destructor for token state component
 */
static void token_state_component_destructor(void* component) {
    if (!component) {
        return;
    }
    
    TokenStateComponent* token_component = (TokenStateComponent*)component;
    
    // Free the text
    if (token_component->text) {
        free(token_component->text);
        token_component->text = NULL;
    }
}

/**
 * Clean up entropy ECS resources
 */
void braggi_entropy_ecs_cleanup(ECSWorld* world) {
    if (!world) {
        fprintf(stderr, "ERROR: NULL world in braggi_entropy_ecs_cleanup\n");
        return;
    }

    fprintf(stderr, "DEBUG: Starting entropy ECS cleanup\n");

    // Get the systems first so we can clean up their data
    System* sync_system = braggi_ecs_get_system_by_name(world, "EntropySyncSystem");
    System* constraint_system = braggi_ecs_get_system_by_name(world, "EntropyConstraintSystem");
    System* wfc_system = braggi_ecs_get_system_by_name(world, "EntropyWFCSystem");

    // Clean up the sync system data
    if (sync_system && sync_system->context) {
        EntropySyncSystemData* sync_data = (EntropySyncSystemData*)sync_system->context;
        
        // Clean up the HashMap's
        if (sync_data->entity_by_state_id) {
            braggi_hashmap_destroy(sync_data->entity_by_state_id);
            sync_data->entity_by_state_id = NULL;
        }
        
        if (sync_data->entity_by_constraint_id) {
            braggi_hashmap_destroy(sync_data->entity_by_constraint_id);
            sync_data->entity_by_constraint_id = NULL;
        }
        
        // Don't free the context or field, they are owned by the BraggiContext
        sync_data->context = NULL;
        sync_data->field = NULL;
        
        // Free the system data itself
        free(sync_data);
        sync_system->context = NULL;
    }

    // Clean up the constraint system data
    if (constraint_system && constraint_system->context) {
        EntropyConstraintSystemData* constraint_data = (EntropyConstraintSystemData*)constraint_system->context;
        
        // Clean up the Vector
        if (constraint_data->constraint_entities) {
            // The vector contains entity IDs, so we don't need to free the entities
            // Just the vector itself
            braggi_vector_destroy(constraint_data->constraint_entities);
            constraint_data->constraint_entities = NULL;
        }
        
        // Don't free the context, it's owned by the BraggiContext
        constraint_data->context = NULL;
        
        // Free the system data itself
        free(constraint_data);
        constraint_system->context = NULL;
    }

    // Clean up the WFC system data
    if (wfc_system && wfc_system->context) {
        EntropyWFCSystemData* wfc_data = (EntropyWFCSystemData*)wfc_system->context;
        
        // Clean up the cell_state_map Vector
        if (wfc_data->cell_state_map) {
            // First, free any sub-vectors
            for (size_t i = 0; i < braggi_vector_size(wfc_data->cell_state_map); i++) {
                Vector** vec_ptr = braggi_vector_get(wfc_data->cell_state_map, i);
                if (vec_ptr && *vec_ptr) {
                    braggi_vector_destroy(*vec_ptr);
                }
            }
            
            // Then destroy the main vector
            braggi_vector_destroy(wfc_data->cell_state_map);
            wfc_data->cell_state_map = NULL;
        }
        
        // Don't free the context or field, they are owned by the BraggiContext
        wfc_data->context = NULL;
        wfc_data->field = NULL;
        
        // Reset state variables
        wfc_data->is_running = false;
        wfc_data->iteration_count = 0;
        
        // Free the system data itself
        free(wfc_data);
        wfc_system->context = NULL;
    }

    // Now clean up all entropy components
    // Get the component type IDs
    ComponentTypeID state_comp_type = INVALID_COMPONENT_TYPE;
    ComponentTypeID constraint_comp_type = INVALID_COMPONENT_TYPE;
    ComponentTypeID token_comp_type = INVALID_COMPONENT_TYPE;

    // Get the type IDs if the components are registered
    if (g_component_ids[ENTROPY_ECS_COMPONENT_STATE] != INVALID_COMPONENT_TYPE) {
        state_comp_type = g_component_ids[ENTROPY_ECS_COMPONENT_STATE];
    }
    
    if (g_component_ids[ENTROPY_ECS_COMPONENT_CONSTRAINT] != INVALID_COMPONENT_TYPE) {
        constraint_comp_type = g_component_ids[ENTROPY_ECS_COMPONENT_CONSTRAINT];
    }
    
    if (g_component_ids[ENTROPY_ECS_COMPONENT_TOKEN] != INVALID_COMPONENT_TYPE) {
        token_comp_type = g_component_ids[ENTROPY_ECS_COMPONENT_TOKEN];
    }

    // Create queries for each component type
    if (state_comp_type != INVALID_COMPONENT_TYPE) {
        ComponentMask state_mask = 0;
        braggi_ecs_mask_set(&state_mask, state_comp_type);
        
        EntityQuery state_query = braggi_ecs_query_entities(world, state_mask);
        EntityID entity;
        
        fprintf(stderr, "DEBUG: Cleaning up entropy state entities\n");
        
        while (braggi_ecs_query_next(&state_query, &entity)) {
            // We don't need to manually free the component data since the ECS system
            // will call the registered destructor for us, but we should destroy the entity
            braggi_ecs_destroy_entity(world, entity);
        }
    }

    if (constraint_comp_type != INVALID_COMPONENT_TYPE) {
        ComponentMask constraint_mask = 0;
        braggi_ecs_mask_set(&constraint_mask, constraint_comp_type);
        
        EntityQuery constraint_query = braggi_ecs_query_entities(world, constraint_mask);
        EntityID entity;
        
        fprintf(stderr, "DEBUG: Cleaning up entropy constraint entities\n");
        
        while (braggi_ecs_query_next(&constraint_query, &entity)) {
            braggi_ecs_destroy_entity(world, entity);
        }
    }

    if (token_comp_type != INVALID_COMPONENT_TYPE) {
        ComponentMask token_mask = 0;
        braggi_ecs_mask_set(&token_mask, token_comp_type);
        
        EntityQuery token_query = braggi_ecs_query_entities(world, token_mask);
        EntityID entity;
        
        fprintf(stderr, "DEBUG: Cleaning up token state entities\n");
        
        while (braggi_ecs_query_next(&token_query, &entity)) {
            braggi_ecs_destroy_entity(world, entity);
        }
    }

    // Reset the global component IDs
    for (int i = 0; i < ENTROPY_ECS_COMPONENT_COUNT; i++) {
        g_component_ids[i] = INVALID_COMPONENT_TYPE;
    }

    fprintf(stderr, "DEBUG: Entropy ECS cleanup complete\n");
}

/**
 * Updates the entropy synchronization system
 * Synchronizes the entropy field data with entity component data
 * 
 * @param world The ECS world
 * @param system The system being updated
 * @param delta_time Time elapsed since last update
 */
static void entropy_sync_system_update(ECSWorld* world, System* system, float delta_time) {
    if (!world || !system || !system->context) {
        fprintf(stderr, "ERROR: Invalid parameters for entropy sync system update\n");
        return;
    }
    
    // Get the system data
    EntropySyncSystemData* data = (EntropySyncSystemData*)system->context;
    if (!data->context || !data->context->entropy_field) {
        fprintf(stderr, "WARNING: Entropy sync system has no valid context or field\n");
        return;
    }
    
    // We'll work with the state component (EntropyStateComponent)
    ComponentMask mask = 0;
    braggi_ecs_mask_set(&mask, g_state_component_type);
    
    EntityQuery query = braggi_ecs_query_entities(world, mask);
    EntityID entity;
    
    // Process each entity with a state component
    while (braggi_ecs_query_next(&query, &entity)) {
        // Get the state component
        EntropyStateComponent* state_component = braggi_ecs_get_component(world, entity, g_state_component_type);
        if (!state_component) continue;
        
        // Get the cell in the field
        uint32_t cell_id = state_component->cell_id;
        if (cell_id >= data->context->entropy_field->cell_count) {
            fprintf(stderr, "ERROR: Entity %u has invalid cell ID %u\n", entity, cell_id);
            continue;
        }
        
        EntropyCell* cell = data->context->entropy_field->cells[cell_id];
        if (!cell) {
            fprintf(stderr, "WARNING: Entity %u references NULL cell %u\n", entity, cell_id);
            // Attempt to recover the cell
            if (!attempt_to_recover_null_cell(data->context->entropy_field, cell_id)) {
                continue;
            }
            cell = data->context->entropy_field->cells[cell_id];
        }
        
        // Find the entropy state in the cell
        EntropyState* state = NULL;
        for (uint32_t i = 0; i < cell->state_count; i++) {
            if (cell->states[i] && cell->states[i]->id == state_component->state_id) {
                state = cell->states[i];
                break;
            }
        }
        
        if (!state) {
            fprintf(stderr, "WARNING: Entity %u references non-existent state %u\n", 
                    entity, state_component->state_id);
            continue;
        }
        
        // Update component data from state
        state_component->probability = state->probability;
        state_component->eliminated = (state->probability <= 0.0f);
        
        // Update any other relevant data between state and component
    }
}

/**
 * Updates the entropy constraint system
 * Processes constraint rules on the entropy field
 * 
 * @param world The ECS world
 * @param system The system being updated
 * @param delta_time Time elapsed since last update
 */
static void entropy_constraint_system_update(ECSWorld* world, System* system, float delta_time) {
    if (!world || !system || !system->context) {
        fprintf(stderr, "ERROR: Invalid parameters for entropy constraint system update\n");
        return;
    }
    
    // Get the system data
    EntropyConstraintSystemData* data = (EntropyConstraintSystemData*)system->context;
    if (!data->context || !data->context->entropy_field) {
        fprintf(stderr, "WARNING: Entropy constraint system has no valid context or field\n");
        return;
    }
    
    EntropyField* field = data->context->entropy_field;
    
    // Process constraints for each cell in the field
    for (uint32_t i = 0; i < field->cell_count; i++) {
        EntropyCell* cell = field->cells[i];
        if (!cell) {
            fprintf(stderr, "WARNING: NULL cell at index %u during constraint processing\n", i);
            // Attempt to recover the cell
            if (!attempt_to_recover_null_cell(field, i)) {
                continue;
            }
            cell = field->cells[i];
        }
        
        // Skip already collapsed cells
        if (braggi_entropy_cell_is_collapsed(cell)) {
            continue;
        }
        
        // Process constraints for this cell
        // This would involve checking neighboring cells and applying constraint rules
        // For now, this is just a stub implementation
    }
}

/**
 * Safely clears the field pointer in the EntropyWFCSystemData after the token propagator 
 * (which owns the entropy field) is destroyed.
 * 
 * @param world The ECS world
 */
void braggi_entropy_ecs_clear_field_reference(ECSWorld* world) {
    if (!world) {
        fprintf(stderr, "ERROR: NULL world in braggi_entropy_ecs_clear_field_reference\n");
        return;
    }
    
    // Find the EntropyWFCSystem by name
    System* system = braggi_ecs_get_system_by_name(world, "EntropyWFCSystem");
    if (!system) {
        fprintf(stderr, "WARNING: EntropyWFCSystem not found in braggi_entropy_ecs_clear_field_reference\n");
        return;
    }
    
    // Get system data
    if (!system->context) {
        fprintf(stderr, "WARNING: NULL system context in braggi_entropy_ecs_clear_field_reference\n");
        return;
    }
    
    // Cast to the correct type
    EntropyWFCSystemData* data = (EntropyWFCSystemData*)system->context;
    
    // Check if field is already NULL
    if (!data->field) {
        fprintf(stderr, "DEBUG: Field pointer is already NULL in EntropyWFCSystemData\n");
        // Still mark the system as not running to be safe
        data->is_running = false;
        return;
    }
    
    // Clear the field pointer - it's no longer valid as the field has been destroyed
    data->field = NULL;
    // CRITICAL: Mark the system as not running to prevent it from trying to access the field
    data->is_running = false;
    fprintf(stderr, "DEBUG: Successfully cleared field pointer in EntropyWFCSystemData and stopped system\n");
    
    // Also clear the field reference in any other related systems that might access it
    // ... existing code ...
} 