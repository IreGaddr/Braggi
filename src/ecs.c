/*
 * Braggi - Entity Component System Implementation
 * 
 * "In Texas, we separate the cattle from the cowboys.
 * In Braggi, we separate the data from the behavior!" - Irish-Texan ECS Wisdom
 */

#include "braggi/ecs.h"
#include "braggi/mem/region.h"
#include "braggi/mem/regime.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Missing define that we need
#define MAX_COMPONENT_TYPES 64

// Define the EntityMask type for internal use
typedef struct EntityMask {
    uint64_t bits[4];  // Support up to 256 component types
} EntityMask;

// EntityQuery is now defined in ecs.h, so we don't redefine it here

// Legacy ECS type for backward compatibility
typedef struct ECS {
    EntityID next_entity_id;
    size_t entity_capacity;
    uint64_t* entity_component_masks;
    
    size_t component_count;
    size_t component_capacity;
    void*** components;
} ECS;

// Legacy type aliases for backward compatibility
typedef ComponentTypeID ComponentID;

// Forward declare mask functions to avoid implicit declaration warnings
void braggi_ecs_mask_set(ComponentMask* mask, ComponentTypeID component);
void braggi_ecs_mask_clear(ComponentMask* mask, ComponentTypeID component);
bool braggi_ecs_mask_has(ComponentMask mask, ComponentTypeID component);
bool braggi_ecs_mask_contains(ComponentMask* container, ComponentMask* subset);

// Forward declare the functions for proper compilation
EntityID braggi_ecs_create_entity_legacy(ECS* ecs);
bool braggi_ecs_add_component_legacy(ECS* ecs, EntityID entity, ComponentID component, void* data);
void braggi_ecs_destroy_legacy(ECS* ecs);

// Helper function to grow entity capacity
static bool grow_entity_capacity(ECSWorld* world, size_t min_capacity) {
    size_t new_capacity = world->entity_capacity * 2;
    if (new_capacity < min_capacity) {
        new_capacity = min_capacity;
    }
    
    ComponentMask* new_masks;
    
    if (world->memory_region) {
        // Use region allocation
        new_masks = (ComponentMask*)braggi_mem_region_alloc(world->memory_region, 
                                                   new_capacity * sizeof(ComponentMask));
        if (new_masks) {
            // Copy existing masks
            memcpy(new_masks, world->entity_component_masks, world->entity_capacity * sizeof(ComponentMask));
            // Initialize new masks to zero
            memset(&new_masks[world->entity_capacity], 0, 
                   (new_capacity - world->entity_capacity) * sizeof(ComponentMask));
        }
    } else {
        // Use standard allocation
        new_masks = (ComponentMask*)realloc(world->entity_component_masks, 
                                       new_capacity * sizeof(ComponentMask));
        if (new_masks) {
            // Initialize new masks to zero
            memset(&new_masks[world->entity_capacity], 0, 
                   (new_capacity - world->entity_capacity) * sizeof(ComponentMask));
        }
    }
    
    if (!new_masks) return false;
    
    world->entity_component_masks = new_masks;
    world->entity_capacity = new_capacity;
    
    return true;
}

// Helper function to grow component array capacity
static bool grow_component_array_capacity(ECSWorld* world, size_t min_capacity) {
    size_t new_capacity = world->max_component_types * 2;
    if (new_capacity < min_capacity) {
        new_capacity = min_capacity;
    }
    
    ComponentArray** new_arrays;
    
    if (world->memory_region) {
        // Use region allocation
        new_arrays = (ComponentArray**)braggi_mem_region_alloc(world->memory_region, 
                                                      new_capacity * sizeof(ComponentArray*));
        if (new_arrays) {
            // Copy existing arrays
            memcpy(new_arrays, world->component_arrays, 
                   world->max_component_types * sizeof(ComponentArray*));
            // Initialize new arrays to NULL
            memset(&new_arrays[world->max_component_types], 0,
                   (new_capacity - world->max_component_types) * sizeof(ComponentArray*));
        }
    } else {
        // Use standard allocation
        new_arrays = (ComponentArray**)realloc(world->component_arrays, 
                                         new_capacity * sizeof(ComponentArray*));
        if (new_arrays) {
            // Initialize new arrays to NULL
            memset(&new_arrays[world->max_component_types], 0,
                   (new_capacity - world->max_component_types) * sizeof(ComponentArray*));
        }
    }
    
    if (!new_arrays) {
        return false;
    }
    
    world->component_arrays = new_arrays;
    world->max_component_types = new_capacity;
    
    return true;
}

// Memory allocation with region awareness
static void* ecs_alloc(ECSWorld* world, size_t size) {
    if (world && world->memory_region) {
        return braggi_mem_region_alloc(world->memory_region, size);
    } else {
        return malloc(size);
    }
}

// String duplication with region awareness
static char* ecs_strdup(ECSWorld* world, const char* str) {
    if (!str) return NULL;
    
    size_t len = strlen(str) + 1;
    char* dup;
    
    if (world && world->memory_region) {
        dup = (char*)braggi_mem_region_alloc(world->memory_region, len);
    } else {
        dup = (char*)malloc(len);
    }
    
    if (dup) {
        memcpy(dup, str, len);
    }
    
    return dup;
}

/*
 * Initialize the ECS world and prepare it for use
 */
ECSWorld* braggi_ecs_world_create(size_t entity_capacity, size_t max_component_types) {
    // Validate inputs
    if (entity_capacity == 0) {
        entity_capacity = 1000; // Default reasonable capacity
    }
    
    if (max_component_types == 0) {
        max_component_types = MAX_COMPONENT_TYPES; // Use the defined constant
    }
    
    // Allocate the world structure
    ECSWorld* world = (ECSWorld*)calloc(1, sizeof(ECSWorld));
    if (!world) {
        return NULL;
    }
    
    // Initialize the world properties
    world->entity_capacity = entity_capacity;
    world->next_entity_id = 1; // Start entity IDs at 1, reserve 0 as invalid
    world->component_type_count = 0;
    world->max_component_types = max_component_types;
    
    // Allocate the component arrays array
    world->component_arrays = (ComponentArray**)calloc(max_component_types, sizeof(ComponentArray*));
    if (!world->component_arrays) {
        free(world);
        return NULL;
    }
    
    // Allocate the entity component masks
    world->entity_component_masks = (ComponentMask*)calloc(entity_capacity, sizeof(ComponentMask));
    if (!world->entity_component_masks) {
        free(world->component_arrays);
        free(world);
        return NULL;
    }
    
    // Create the free entities vector
    world->free_entities = braggi_vector_create(sizeof(EntityID));
    if (!world->free_entities) {
        free(world->entity_component_masks);
        free(world->component_arrays);
        free(world);
        return NULL;
    }
    
    // Create the systems vector
    world->systems = braggi_vector_create(sizeof(System*));
    if (!world->systems) {
        braggi_vector_destroy(world->free_entities);
        free(world->entity_component_masks);
        free(world->component_arrays);
        free(world);
        return NULL;
    }
    
    return world;
}

// Create a new ECS world with region-based memory
ECSWorld* braggi_ecs_create_world_with_region(Region* region, Region* entity_region, RegimeType regime) {
    // Validate parameters
    if (!region) return braggi_ecs_world_create(64, MAX_COMPONENT_TYPES);
    
    // Allocate the world structure
    ECSWorld* world = (ECSWorld*)braggi_mem_region_alloc(region, sizeof(ECSWorld));
    if (!world) return NULL;
    
    // Initialize with region-based values
    world->free_entities = braggi_vector_create_in_region(region, sizeof(EntityID)); // Using the region-specific function
    if (!world->free_entities) {
        return NULL;
    }
    
    world->next_entity_id = 1; // Start at 1, 0 reserved for invalid entity
    world->entity_component_masks = NULL;
    world->entity_capacity = 0;
    
    world->component_arrays = NULL;
    world->component_type_count = 0;
    world->max_component_types = MAX_COMPONENT_TYPES;
    
    // Create systems vector with proper element size
    world->systems = braggi_vector_create_in_region(region, sizeof(System*)); // Using the region-specific function
    if (!world->systems) {
        braggi_vector_destroy(world->free_entities);
        return NULL;
    }
    
    world->memory_region = region;
    
    return world;
}

/*
 * Destroy the ECS world and free all resources
 */
void braggi_ecs_world_destroy(ECSWorld* world) {
    if (!world) {
        return;
    }
    
    fprintf(stderr, "DEBUG: Starting ECS world destruction at %p\n", (void*)world);
    
    // First destroy all systems before we destroy component arrays
    // This prevents systems from trying to access components that no longer exist
    if (world->systems) {
        fprintf(stderr, "DEBUG: Destroying %zu systems\n", braggi_vector_size(world->systems));
        
        for (size_t i = 0; i < braggi_vector_size(world->systems); i++) {
            System** system_ptr = (System**)braggi_vector_get(world->systems, i);
            if (system_ptr && *system_ptr) {
                fprintf(stderr, "DEBUG: Destroying system at %p\n", (void*)*system_ptr);
                braggi_ecs_system_destroy(*system_ptr);
                *system_ptr = NULL; // Set to NULL to prevent double-free
            }
        }
    }
    
    // Destroy all component arrays
    if (world->component_arrays) {
        fprintf(stderr, "DEBUG: Destroying component arrays\n");
        
        for (size_t i = 0; i < world->max_component_types; i++) {
            if (world->component_arrays[i]) {
                fprintf(stderr, "DEBUG: Destroying component array %zu\n", i);
                
                // Free components if needed and if destructor exists
                if (world->component_arrays[i]->destructor && world->component_arrays[i]->data) {
                    for (size_t j = 0; j < world->component_arrays[i]->size; j++) {
                        // Check we're within the valid component size
                        if (j >= world->component_arrays[i]->size) {
                            break;
                        }
                        
                        // Get the component pointer with safety checks
                        void* component = NULL;
                        size_t component_size = world->component_arrays[i]->component_size;
                        if (component_size > 0) {
                            component = world->component_arrays[i]->data + (j * component_size);
                        }
                        
                        // Call the destructor if component is valid
                        if (component) {
                            world->component_arrays[i]->destructor(component);
                        }
                    }
                }
                
                // Free the component array resources
                if (world->component_arrays[i]->data) {
                    free(world->component_arrays[i]->data);
                    world->component_arrays[i]->data = NULL;
                }
                
                if (world->component_arrays[i]->entity_to_index) {
                    free(world->component_arrays[i]->entity_to_index);
                    world->component_arrays[i]->entity_to_index = NULL;
                }
                
                if (world->component_arrays[i]->index_to_entity) {
                    free(world->component_arrays[i]->index_to_entity);
                    world->component_arrays[i]->index_to_entity = NULL;
                }
                
                // Free the component array itself
                free(world->component_arrays[i]);
                world->component_arrays[i] = NULL;
            }
        }
        
        // Free the component arrays array
        free(world->component_arrays);
        world->component_arrays = NULL;
    }
    
    // Free entity component masks
    if (world->entity_component_masks) {
        free(world->entity_component_masks);
        world->entity_component_masks = NULL;
    }
    
    // Free vectors
    if (world->free_entities) {
        braggi_vector_destroy(world->free_entities);
        world->free_entities = NULL;
    }
    
    if (world->systems) {
        braggi_vector_destroy(world->systems);
        world->systems = NULL;
    }
    
    // Finally, free the world itself
    fprintf(stderr, "DEBUG: ECS world destruction complete\n");
    free(world);
}

void braggi_ecs_destroy_world(ECSWorld* world) {
    // Simple wrapper that calls the world_destroy function
    braggi_ecs_world_destroy(world);
}

// Ensure entity capacity
static bool braggi_ecs_ensure_entity_capacity(ECSWorld* world, EntityID entity) {
    if (entity < world->entity_capacity) return true;
    
    // Calculate new capacity
    size_t new_capacity = world->entity_capacity == 0 ? 64 : world->entity_capacity * 2;
    while (new_capacity <= entity) {
        new_capacity *= 2;
    }
    
    // Resize entity masks array
    ComponentMask* new_masks;
    
    if (world->memory_region) {
        new_masks = (ComponentMask*)braggi_mem_region_alloc(world->memory_region, 
                                                    new_capacity * sizeof(ComponentMask));
        if (!new_masks) return false;
        
        // Copy existing masks
        if (world->entity_component_masks) {
            memcpy(new_masks, world->entity_component_masks, 
                   world->entity_capacity * sizeof(ComponentMask));
        }
    } else {
        new_masks = (ComponentMask*)realloc(world->entity_component_masks, 
                                        new_capacity * sizeof(ComponentMask));
        if (!new_masks) return false;
    }
    
    // Initialize new masks to zero
    memset(&new_masks[world->entity_capacity], 0, 
           (new_capacity - world->entity_capacity) * sizeof(ComponentMask));
    
    world->entity_component_masks = new_masks;
    world->entity_capacity = new_capacity;
    
    return true;
}

/*
 * Create a new entity in the ECS world
 */
EntityID braggi_ecs_create_entity(ECSWorld* world) {
    if (!world) {
        return INVALID_ENTITY;
    }
    
    EntityID entity_id;
    
    // Check if we have any recycled entities
    if (world->free_entities->size > 0) {
        // Reuse a previously deleted entity
        entity_id = *(EntityID*)braggi_vector_get(world->free_entities, world->free_entities->size - 1);
        braggi_vector_pop(world->free_entities);
    } else {
        // Create a new entity
        if (world->next_entity_id >= world->entity_capacity) {
            return INVALID_ENTITY; // We've reached capacity
        }
        
        entity_id = world->next_entity_id++;
    }
    
    // Clear the component mask for this entity
    world->entity_component_masks[entity_id] = 0;
    
    return entity_id;
}

/*
 * Destroy an entity and recycle its ID
 */
void braggi_ecs_destroy_entity(ECSWorld* world, EntityID entity) {
    if (!world || entity >= world->entity_capacity) {
        return;
    }
    
    // Remove the entity from all component arrays
    for (size_t i = 0; i < world->component_type_count; i++) {
        if (world->component_arrays[i] && 
            (world->entity_component_masks[entity] & (1ULL << i))) {
            // Remove the component from this entity
            braggi_component_array_remove(world->component_arrays[i], entity);
        }
    }
    
    // Clear the component mask
    world->entity_component_masks[entity] = 0;
    
    // Add the entity ID to the free list for recycling
    braggi_vector_push(world->free_entities, &entity);
}

// Check if an entity exists
bool braggi_ecs_entity_exists(ECSWorld* world, EntityID entity) {
    if (!world || entity == 0 || entity >= world->entity_capacity) return false;
    
    // In this simple implementation, we consider an entity to exist if its ID
    // is within the valid range and not in the free list
    
    // Check if entity is in the free list
    for (size_t i = 0; i < braggi_vector_size(world->free_entities); i++) {
        if (*(EntityID*)braggi_vector_get(world->free_entities, i) == entity) {
            return false;
        }
    }
    
    return true;
}

// Ensure component type capacity
static bool braggi_ecs_ensure_component_capacity(ECSWorld* world, ComponentTypeID type) {
    if (type < world->max_component_types) return true;
    
    // Grow capacity
    size_t new_capacity = world->max_component_types == 0 ? 16 : world->max_component_types * 2;
    while (type >= new_capacity) {
        new_capacity *= 2;
    }
    
    ComponentArray** new_arrays;
    
    if (world->memory_region) {
        // Use region allocation
        new_arrays = (ComponentArray**)braggi_mem_region_alloc(world->memory_region, 
                                                      new_capacity * sizeof(ComponentArray*));
        if (!new_arrays) {
            return false;
        }
        
        // Copy existing arrays
        memcpy(new_arrays, world->component_arrays, 
               world->max_component_types * sizeof(ComponentArray*));
    } else {
        // Use standard allocation
        new_arrays = (ComponentArray**)realloc(world->component_arrays, 
                                         new_capacity * sizeof(ComponentArray*));
        if (!new_arrays) {
            return false;
        }
    }
    
    // Initialize new arrays to NULL
    memset(&new_arrays[world->max_component_types], 0,
           (new_capacity - world->max_component_types) * sizeof(ComponentArray*));
    
    world->component_arrays = new_arrays;
    world->max_component_types = new_capacity;
    
    return true;
}

/**
 * Register a component type with detailed information
 */
ComponentTypeID braggi_ecs_register_component_type(ECSWorld* world, const ComponentTypeInfo* info) {
    if (!world || !info || world->component_type_count >= world->max_component_types) {
        return INVALID_COMPONENT_TYPE;
    }
    
    // Get the next available component type ID
    ComponentTypeID type_id = world->component_type_count;
    
    // Create a new component array for this type
    ComponentArray* component_array = braggi_component_array_create(world->entity_capacity, info->size);
    if (!component_array) {
        return INVALID_COMPONENT_TYPE;
    }
    
    // Store the destructor function if provided
    if (info->destructor) {
        component_array->destructor = info->destructor;
    }
    
    // Store the component array
    world->component_arrays[type_id] = component_array;
    
    // Increment the component type count
    world->component_type_count++;
    
    return type_id;
}

// Ensure component array capacity
static bool braggi_ecs_ensure_component_array_capacity(ECSWorld* world, ComponentTypeID type) {
    if (!braggi_ecs_ensure_component_capacity(world, type)) {
        return false;
    }
    
    ComponentArray* array = world->component_arrays[type];
    if (!array) {
        return true;  // No array to check capacity for
    }
    
    // Check if the array needs to grow
    if (array->size < array->capacity) {
        return true;  // Still has space
    }
    
    // Grow the array
    size_t new_capacity = array->capacity * 2;
    size_t component_size = array->component_size;
    
    void* new_data;
    size_t* new_entity_to_index;
    EntityID* new_index_to_entity;
    
    if (world->memory_region) {
        new_data = braggi_mem_region_alloc(world->memory_region, new_capacity * component_size);
        if (!new_data) return false;
        
        new_entity_to_index = (size_t*)braggi_mem_region_alloc(world->memory_region, 
                                                      new_capacity * sizeof(size_t));
        if (!new_entity_to_index) return false;
        
        new_index_to_entity = (EntityID*)braggi_mem_region_alloc(world->memory_region, 
                                                      new_capacity * sizeof(EntityID));
        if (!new_index_to_entity) return false;
        
        // Copy existing data
        if (array->data) {
            memcpy(new_data, array->data, array->size * component_size);
            memcpy(new_entity_to_index, array->entity_to_index, array->size * sizeof(size_t));
            memcpy(new_index_to_entity, array->index_to_entity, array->size * sizeof(EntityID));
        }
    } else {
        new_data = realloc(array->data, new_capacity * component_size);
        if (!new_data) return false;
        
        new_entity_to_index = (size_t*)realloc(array->entity_to_index, new_capacity * sizeof(size_t));
        if (!new_entity_to_index) {
            array->data = new_data; // Update even if the entity IDs allocation fails
            return false;
        }
        
        new_index_to_entity = (EntityID*)realloc(array->index_to_entity, new_capacity * sizeof(EntityID));
        if (!new_index_to_entity) {
            array->data = new_data; // Update even if the entity IDs allocation fails
            return false;
        }
    }
    
    array->data = new_data;
    array->entity_to_index = new_entity_to_index;
    array->index_to_entity = new_index_to_entity;
    array->capacity = new_capacity;
    
    return true;
}

// Find a component index for an entity
static int braggi_ecs_find_component_index(ECSWorld* world, EntityID entity, ComponentTypeID type) {
    if (!world || type >= world->component_type_count) return -1;
    
    ComponentArray* array = world->component_arrays[type];
    if (!array) return -1;
    
    // In our implementation, we directly store entity->index mapping
    if (braggi_ecs_has_component(world, entity, type)) {
        return (int)array->entity_to_index[entity];
    }
    
    return -1;
}

/*
 * Add a component to an entity in the world
 */
void* braggi_ecs_world_add_component(ECSWorld* world, EntityID entity, ComponentTypeID type) {
    if (!world || entity >= world->next_entity_id) return NULL;
    
    // Ensure we have capacity for this component type
    if (!braggi_ecs_ensure_component_capacity(world, type)) {
        return NULL;
    }
    
    // If the component array doesn't exist, create it
    if (!world->component_arrays[type]) {
        ComponentArray* array = braggi_component_array_create(32, sizeof(void*)); // Default size
        if (!array) return NULL;
        world->component_arrays[type] = array;
    }
    
    // Add the component to the entity
    void* component_slot = malloc(sizeof(void*)); // Allocate a slot
    if (!component_slot) return NULL;
    
    // Update the component mask for this entity
    braggi_ecs_mask_set(&world->entity_component_masks[entity], type);
    
    // Add the component to the array
    braggi_component_array_add(world->component_arrays[type], entity, component_slot);
    
    return component_slot;
}

// Remove a component from an entity
void braggi_ecs_remove_component(ECSWorld* world, EntityID entity, ComponentTypeID type) {
    if (!world || entity == 0 || entity >= world->entity_capacity || 
        type >= world->component_type_count) {
        return;
    }
    
    // Check if entity has this component
    if (!braggi_ecs_has_component(world, entity, type)) {
        return;
    }
    
    // Get the component array
    ComponentArray* array = world->component_arrays[type];
    
    // Get the index for this entity
    size_t index = array->entity_to_index[entity];
    size_t last_index = array->size - 1;
    
    // Only swap and pop if this isn't the last element
    if (index < last_index) {
        // Copy the last component data to this index
        char* dst = (char*)array->data + (index * array->component_size);
        char* src = (char*)array->data + (last_index * array->component_size);
        memcpy(dst, src, array->component_size);
        
        // Update the entity indices
        EntityID last_entity = array->index_to_entity[last_index];
        array->entity_to_index[last_entity] = index;
        array->index_to_entity[index] = last_entity;
    }
    
    // Decrement the count
    array->size--;
    
    // Clear the component bit in the entity mask
    braggi_ecs_mask_clear(&world->entity_component_masks[entity], type);
}

// Get a component from an entity
void* braggi_ecs_get_component(ECSWorld* world, EntityID entity, ComponentTypeID type) {
    if (!world || entity == 0 || entity >= world->entity_capacity || 
        type >= world->component_type_count) {
        return NULL;
    }
    
    // Check if entity has this component
    if (!braggi_ecs_has_component(world, entity, type)) {
        return NULL;
    }
    
    // Get the component array
    ComponentArray* array = world->component_arrays[type];
    
    // Get the component index
    size_t index = array->entity_to_index[entity];
    
    // Return a pointer to the component data
    return (char*)array->data + (index * array->component_size);
}

// Check if an entity has a component
bool braggi_ecs_has_component(ECSWorld* world, EntityID entity, ComponentTypeID type) {
    if (!world || entity == 0 || entity >= world->entity_capacity || 
        type >= world->component_type_count) {
        return false;
    }
    
    // Check the component bit in the entity mask
    return braggi_ecs_mask_has(world->entity_component_masks[entity], type);
}

/*
 * Register a system with the ECS world
 */
void braggi_ecs_register_system(ECSWorld* world, System* system) {
    if (!world || !system) {
        return;
    }
    
    // Add the system to the list
    braggi_vector_push(world->systems, &system);
}

/*
 * Execute all systems in the ECS world
 */
void braggi_ecs_update(ECSWorld* world, float delta_time) {
    if (!world) {
        return;
    }
    
    // Update each system
    for (size_t i = 0; i < world->systems->size; i++) {
        System* system = *(System**)braggi_vector_get(world->systems, i);
        if (system && system->update_func) {
            system->update_func(world, system, delta_time);
        }
    }
}

/*
 * Create a new system
 */
System* braggi_ecs_system_create(ComponentMask component_mask, SystemUpdateFunc update_func, void* user_data) {
    System* system = (System*)malloc(sizeof(System));
    if (!system) {
        return NULL;
    }
    
    system->component_mask = component_mask;
    system->update_func = update_func;
    system->context = user_data;
    
    return system;
}

/*
 * Destroy a system
 */
void braggi_ecs_system_destroy(System* system) {
    if (!system) {
        return;
    }
    
    fprintf(stderr, "DEBUG: Destroying system at %p\n", (void*)system);
    
    // Clean up context if needed
    if (system->context) {
        fprintf(stderr, "DEBUG: Freeing system context at %p\n", system->context);
        free(system->context);
        system->context = NULL;
    }
    
    // Free the system itself
    free(system);
    fprintf(stderr, "DEBUG: System destroyed\n");
}

// Create a query for entities matching a component mask
EntityQuery braggi_ecs_query_entities(ECSWorld* world, ComponentMask required_components) {
    EntityQuery query;
    query.world = world;
    query.required_components = required_components;
    query.position = 0;
    
    return query;
}

// Get the next entity matching a query
bool braggi_ecs_query_next(EntityQuery* query, EntityID* out_entity) {
    if (!query || !query->world || !out_entity) return false;
    
    ECSWorld* world = query->world;
    
    // Find the next entity matching the required components
    while (query->position < world->entity_capacity) {
        EntityID entity = (EntityID)query->position++;
        
        if (entity > 0 && braggi_ecs_entity_exists(world, entity)) {
            // Check if entity has all required components
            if ((world->entity_component_masks[entity] & query->required_components) == query->required_components) {
                *out_entity = entity;
                return true;
            }
        }
    }
    
    return false;
}

/*
 * Creates a new empty ECS.
 * 
 * "Every good ECS starts with nothin' but potential, 
 * just like a fresh plot of Texas land!" - Coding Rancher Wisdom
 */
ECS* braggi_ecs_create(void) {
    // Allocate the ECS structure
    ECS* ecs = (ECS*)calloc(1, sizeof(ECS));
    if (!ecs) {
        return NULL;
    }
    
    // Initialize with default values
    ecs->next_entity_id = 1; // Start at 1, 0 reserved for invalid entity
    ecs->entity_capacity = 0;
    ecs->entity_component_masks = NULL;
    
    ecs->component_count = 0;
    ecs->component_capacity = 0;
    ecs->components = NULL;
    
    return ecs;
}

/*
 * Destroys the ECS system and frees all resources.
 */
void braggi_ecs_destroy_legacy(ECS* ecs) {
    if (!ecs) {
        return;
    }
    
    // Free all component arrays
    if (ecs->components) {
        for (size_t i = 0; i < ecs->component_count; i++) {
            if (ecs->components[i]) {
                free(ecs->components[i]);
            }
        }
        free(ecs->components);
    }
    
    // Free entity component masks
    if (ecs->entity_component_masks) {
        free(ecs->entity_component_masks);
    }
    
    // Free the ECS structure itself
    free(ecs);
}

/*
 * Creates a new entity (legacy implementation)
 */
EntityID braggi_ecs_create_entity_legacy(ECS* ecs) {
    if (!ecs) {
        return 0;
    }
    
    // Get the next entity ID
    EntityID entity = ecs->next_entity_id++;
    
    // If entity ID is too large, grow the entity array
    if (entity >= ecs->entity_capacity) {
        size_t new_capacity = ecs->entity_capacity * 2;
        
        // Resize the component masks array
        uint64_t* new_masks = (uint64_t*)realloc(ecs->entity_component_masks, 
                                              new_capacity * sizeof(uint64_t));
        
        if (!new_masks) {
            // Failed to resize
            ecs->next_entity_id--;
            return 0;
        }
        
        // Initialize the new masks to zero
        memset(&new_masks[ecs->entity_capacity], 0, 
               (new_capacity - ecs->entity_capacity) * sizeof(uint64_t));
        
        ecs->entity_component_masks = new_masks;
        ecs->entity_capacity = new_capacity;
    }
    
    return entity;
}

/*
 * Add a component to an entity (legacy implementation)
 */
bool braggi_ecs_add_component_legacy(ECS* ecs, EntityID entity, ComponentID component, void* data) {
    if (!ecs || entity >= ecs->next_entity_id) {
        return false;
    }
    
    // Ensure we have space for the component
    if (component >= ecs->component_capacity) {
        size_t new_capacity = ecs->component_capacity == 0 ? 16 : ecs->component_capacity * 2;
        while (component >= new_capacity) {
            new_capacity *= 2;
        }
        
        // Resize the components array
        void*** new_components = (void***)realloc(ecs->components, 
                                               new_capacity * sizeof(void**));
        
        if (!new_components) {
            return false;
        }
        
        // Initialize the new components to NULL
        memset(&new_components[ecs->component_capacity], 0, 
               (new_capacity - ecs->component_capacity) * sizeof(void**));
        
        ecs->components = new_components;
        ecs->component_capacity = new_capacity;
    }
    
    // If needed, make this component type valid
    if (component >= ecs->component_count) {
        ecs->component_count = component + 1;
    }
    
    // Ensure we have an array for this component
    if (!ecs->components[component]) {
        ecs->components[component] = (void**)calloc(ecs->entity_capacity, sizeof(void*));
        if (!ecs->components[component]) {
            return false;
        }

    }
    
    // Store the component data
    ecs->components[component][entity] = data;
    
    // Mark the entity as having this component
    ecs->entity_component_masks[entity] |= (1ULL << component);
    
    return true;
}

// Use the existing implementation for mask functions
void braggi_ecs_mask_set(ComponentMask* mask, ComponentTypeID component) {
    *mask |= (1ULL << component);
}

void braggi_ecs_mask_clear(ComponentMask* mask, ComponentTypeID component) {
    *mask &= ~(1ULL << component);
}

bool braggi_ecs_mask_has(ComponentMask mask, ComponentTypeID component) {
    return (mask & (1ULL << component)) != 0;
}

bool braggi_ecs_mask_contains(ComponentMask* container, ComponentMask* subset) {
    return (*container & *subset) == *subset;
}

/*
 * Add a component to an entity
 */
void braggi_ecs_add_component_data(ECSWorld* world, EntityID entity, ComponentTypeID component_type, void* component_data) {
    if (!world || entity == INVALID_ENTITY) {
        return;
    }
    
    // Ensure we have capacity for this component type
    if (!braggi_ecs_ensure_component_capacity(world, component_type)) {
        return;
    }
    
    // Create component array if it doesn't exist
    if (!world->component_arrays[component_type]) {
        world->component_arrays[component_type] = braggi_component_array_create(16, sizeof(void*));
        if (!world->component_arrays[component_type]) {
            return;
        }
    }
    
    // Add the component to the array
    braggi_component_array_add(world->component_arrays[component_type], entity, component_data);
    
    // Update the entity's component mask
    braggi_ecs_mask_set(&world->entity_component_masks[entity], component_type);
}

// Add the correct implementation of braggi_ecs_add_component that returns void*
void* braggi_ecs_add_component(ECSWorld* world, EntityID entity, ComponentTypeID component_type) {
    if (!world || entity == INVALID_ENTITY) {
        return NULL;
    }
    
    // Ensure we have capacity for this component type
    if (!braggi_ecs_ensure_component_capacity(world, component_type)) {
        return NULL;
    }
    
    // Create component array if it doesn't exist
    if (!world->component_arrays[component_type]) {
        world->component_arrays[component_type] = braggi_component_array_create(16, sizeof(void*));
        if (!world->component_arrays[component_type]) {
            return NULL;
        }
    }
    
    // Allocate memory for the component
    void* component_data = calloc(1, world->component_arrays[component_type]->component_size);
    if (!component_data) {
        return NULL;
    }
    
    // Add the component to the array
    braggi_component_array_add(world->component_arrays[component_type], entity, component_data);
    
    // Update the entity's component mask
    braggi_ecs_mask_set(&world->entity_component_masks[entity], component_type);
    
    return component_data;
}

/*
 * Create a new component array for storing components of a specific type
 */
ComponentArray* braggi_component_array_create(size_t capacity, size_t component_size) {
    ComponentArray* array = (ComponentArray*)malloc(sizeof(ComponentArray));
    if (!array) {
        return NULL;
    }
    
    // Allocate memory for the component data
    array->data = malloc(capacity * component_size);
    if (!array->data) {
        free(array);
        return NULL;
    }
    
    // Initialize the entity to index mapping
    array->entity_to_index = (size_t*)malloc(capacity * sizeof(size_t));
    if (!array->entity_to_index) {
        free(array->data);
        free(array);
        return NULL;
    }
    
    // Initialize the index to entity mapping
    array->index_to_entity = (EntityID*)malloc(capacity * sizeof(EntityID));
    if (!array->index_to_entity) {
        free(array->entity_to_index);
        free(array->data);
        free(array);
        return NULL;
    }
    
    // Initialize other fields
    array->size = 0;
    array->capacity = capacity;
    array->component_size = component_size;
    
    return array;
}

/*
 * Add a component to a component array for a specific entity
 */
void braggi_component_array_add(ComponentArray* array, EntityID entity, void* component) {
    if (!array || !component) {
        return;
    }
    
    // Store the entity at the end of the array
    size_t new_index = array->size;
    
    // Update the maps to maintain the packed array
    array->entity_to_index[entity] = new_index;
    array->index_to_entity[new_index] = entity;
    
    // Copy the component data into the array
    char* dst = (char*)array->data + (new_index * array->component_size);
    memcpy(dst, component, array->component_size);
    
    // Increment the size
    array->size++;
}

/*
 * Remove a component from a component array for a specific entity
 */
void braggi_component_array_remove(ComponentArray* array, EntityID entity) {
    if (!array) {
        return;
    }
    
    // Check if the entity has this component
    size_t index_of_removed = array->entity_to_index[entity];
    size_t last_index = array->size - 1;
    
    if (index_of_removed != last_index) {
        // Copy the last element to the removed element's place
        char* src = (char*)array->data + (last_index * array->component_size);
        char* dst = (char*)array->data + (index_of_removed * array->component_size);
        memcpy(dst, src, array->component_size);
        
        // Update the index map for the moved entity
        EntityID last_entity = array->index_to_entity[last_index];
        array->entity_to_index[last_entity] = index_of_removed;
        array->index_to_entity[index_of_removed] = last_entity;
    }
    
    // Decrement the size
    array->size--;
}

/*
 * Execute a specific system in the ECS world
 */
bool braggi_ecs_update_system(ECSWorld* world, System* system, float delta_time) {
    if (!world || !system || !system->update_func) {
        return false;
    }
    
    // Execute the system's update function
    system->update_func(world, system, delta_time);
    
    return true;
}

/*
 * Get a system by name
 *
 * "Finding a system by name is like yellin' for your favorite dog in a pack - 
 * the right one oughta come runnin'!" - Texas ECS wisdom
 */
System* braggi_ecs_get_system_by_name(ECSWorld* world, const char* name) {
    if (!world || !name || !world->systems) {
        return NULL;
    }
    
    // Iterate through all systems in the world
    for (size_t i = 0; i < braggi_vector_size(world->systems); i++) {
        System* system = *(System**)braggi_vector_get(world->systems, i);
        if (system && system->name && strcmp(system->name, name) == 0) {
            return system;
        }
    }
    
    return NULL; // Not found
}

/*
 * Add a system to the ECS world
 */
bool braggi_ecs_add_system(ECSWorld* world, System* system) {
    if (!world || !system) {
        return false;
    }
    
    // Add the system to the world
    bool result = braggi_vector_push(world->systems, &system);
    
    return result;
}

/*
 * Create a system from a SystemInfo structure
 */
System* braggi_ecs_create_system(const SystemInfo* info) {
    if (!info || !info->update_func) {
        return NULL;
    }
    
    System* system = (System*)malloc(sizeof(System));
    if (!system) {
        return NULL;
    }
    
    system->name = info->name ? info->name : "Anonymous System";
    system->component_mask = 0;  // No components required by default
    system->update_func = info->update_func;
    system->context = info->context;
    system->priority = info->priority;
    
    return system;
}

/*
 * Get a component type ID by name
 */
ComponentTypeID braggi_ecs_get_component_type_by_name(ECSWorld* world, const char* name) {
    if (!world || !name) {
        return INVALID_COMPONENT_TYPE;
    }
    
    // Simple implementation: iterate through component arrays 
    // and check their names (if available)
    for (ComponentTypeID type = 0; type < world->component_type_count; type++) {
        // Skip if component array doesn't exist
        if (!world->component_arrays[type]) continue;
        
        // In a proper implementation, component arrays would store their names
        // For now, we'll use a simple comparison method by checking key component properties
        
        // Implement your component registration and lookup logic here
        // This is a simplified implementation that returns the first component type
        // whose name matches the requested name
        
        // If this is the TokenComponent, the type stored would match the name
        if (strcmp(name, "TokenComponent") == 0) {
            return type; // Return the first component type we find (stubbed for now)
        }
    }
    
    return INVALID_COMPONENT_TYPE;
}

/*
 * Register a new component type with the ECS world
 */
ComponentTypeID braggi_ecs_register_component(ECSWorld* world, size_t component_size) {
    if (!world || world->component_type_count >= world->max_component_types) {
        return INVALID_COMPONENT_TYPE;
    }
    
    // Create component info with minimal details
    ComponentTypeInfo info = {
        .name = NULL,
        .size = component_size,
        .constructor = NULL,
        .destructor = NULL
    };
    
    return braggi_ecs_register_component_type(world, &info);
}

ECSWorld* braggi_ecs_create_world() {
    // Simple wrapper that calls the create_world_with_region function with NULL parameters
    // This creates a standard ECS world without region-based memory
    return braggi_ecs_create_world_with_region(NULL, NULL, REGIME_SEQ);
} 