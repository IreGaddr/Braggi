/*
 * Braggi - Entity Component System
 * 
 * "A good ECS is like a well-organized ranch - data in one pasture,
 * behavior in another, and the entities roamin' freely!" - Irish-Texan System Design
 */

#ifndef BRAGGI_ECS_H
#define BRAGGI_ECS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "braggi/util/vector.h"
#include "braggi/mem/region.h"

/*
 * "The Entity Component System - where your code becomes more organized
 * than your sock drawer. Unless you're a chaos wizard, then it's a toss-up."
 * - Ye Olde Texan-Irish Programming Proverb
 */

// Type definitions
typedef uint32_t EntityID;
typedef uint32_t ComponentTypeID;
typedef uint64_t ComponentMask;

// Constants
#define INVALID_ENTITY UINT32_MAX
#define INVALID_COMPONENT_TYPE UINT32_MAX

// Forward declarations
typedef struct ECSWorld ECSWorld;
typedef struct ComponentArray ComponentArray;
typedef struct System System;

// Entity query structure
typedef struct EntityQuery {
    ECSWorld* world;
    ComponentMask required_components;
    size_t position;
} EntityQuery;

// Function types
typedef void (*SystemUpdateFunc)(ECSWorld* world, System* system, float delta_time);
typedef void (*ComponentDestructorFunc)(void* component);

// Component type information
typedef struct ComponentTypeInfo {
    const char* name;                     // Name of the component type
    size_t size;                          // Size of each component in bytes
    void (*constructor)(void* component); // Optional constructor function
    void (*destructor)(void* component);  // Optional destructor function
} ComponentTypeInfo;

// System information structure
typedef struct SystemInfo {
    const char* name;                  // Name of the system
    SystemUpdateFunc update_func;      // Function to update entities
    void* context;                     // Custom data for the system
    int priority;                      // Priority (higher = runs earlier)
} SystemInfo;

// Component array structure
struct ComponentArray {
    void* data;                  // Packed array of component data
    size_t* entity_to_index;     // Maps entity IDs to indices in the data array
    EntityID* index_to_entity;   // Maps indices to entity IDs
    size_t size;                 // Current number of components
    size_t capacity;             // Maximum number of components
    size_t component_size;       // Size of each component in bytes
    ComponentDestructorFunc destructor; // Optional destructor function
};

// System structure
struct System {
    const char* name;              // System name
    ComponentMask component_mask;  // Components required by this system
    SystemUpdateFunc update_func;  // Function to update entities
    void* context;                 // Custom data for the system
    int priority;                  // Priority (higher = runs earlier)
};

// ECS World structure
struct ECSWorld {
    size_t entity_capacity;           // Maximum number of entities
    size_t next_entity_id;            // Next entity ID to assign
    size_t component_type_count;      // Number of registered component types
    size_t max_component_types;       // Maximum number of component types
    Vector* free_entities;            // Recycled entity IDs
    ComponentArray** component_arrays; // Array of component arrays
    ComponentMask* entity_component_masks; // Component masks for each entity
    Vector* systems;                  // Registered systems
    Region* memory_region;            // Optional memory region for allocations
};

// ECS World functions
ECSWorld* braggi_ecs_world_create(size_t entity_capacity, size_t max_component_types);
ECSWorld* braggi_ecs_create_world(void);
ECSWorld* braggi_ecs_create_world_with_region(Region* region, Region* entity_region, RegimeType regime);
void braggi_ecs_world_destroy(ECSWorld* world);
void braggi_ecs_destroy_world(ECSWorld* world);

// Entity functions
EntityID braggi_ecs_create_entity(ECSWorld* world);
void braggi_ecs_destroy_entity(ECSWorld* world, EntityID entity);
bool braggi_ecs_entity_exists(ECSWorld* world, EntityID entity);

// Component functions
ComponentTypeID braggi_ecs_register_component(ECSWorld* world, size_t component_size);

/**
 * Register a component type with detailed information
 * 
 * @param world The ECS world
 * @param info Component type information including size, constructor and destructor
 * @return The newly registered component type ID, or INVALID_COMPONENT_TYPE on failure
 */
ComponentTypeID braggi_ecs_register_component_type(ECSWorld* world, const ComponentTypeInfo* info);

void* braggi_ecs_add_component(ECSWorld* world, EntityID entity, ComponentTypeID component_type);
void braggi_ecs_add_component_data(ECSWorld* world, EntityID entity, ComponentTypeID component_type, void* component_data);
void braggi_ecs_remove_component(ECSWorld* world, EntityID entity, ComponentTypeID component_type);
void* braggi_ecs_get_component(ECSWorld* world, EntityID entity, ComponentTypeID component_type);
bool braggi_ecs_has_component(ECSWorld* world, EntityID entity, ComponentTypeID component_type);
ComponentTypeID braggi_ecs_get_component_type_by_name(ECSWorld* world, const char* name);

// Component mask manipulation
void braggi_ecs_mask_set(ComponentMask* mask, ComponentTypeID component_type);
void braggi_ecs_mask_clear(ComponentMask* mask, ComponentTypeID component_type);
bool braggi_ecs_mask_has(ComponentMask mask, ComponentTypeID component_type);

// System functions
System* braggi_ecs_system_create(ComponentMask component_mask, SystemUpdateFunc update_func, void* user_data);
System* braggi_ecs_create_system(const SystemInfo* info);
void braggi_ecs_system_destroy(System* system);
void braggi_ecs_register_system(ECSWorld* world, System* system);
bool braggi_ecs_add_system(ECSWorld* world, System* system);
System* braggi_ecs_get_system_by_name(ECSWorld* world, const char* name);
bool braggi_ecs_update_system(ECSWorld* world, System* system, float delta_time);
void braggi_ecs_update(ECSWorld* world, float delta_time);

// Query functions
Vector* braggi_ecs_get_entities_with_components(ECSWorld* world, ComponentMask component_mask);

// Entity query functions
EntityQuery braggi_ecs_query_entities(ECSWorld* world, ComponentMask required_components);
bool braggi_ecs_query_next(EntityQuery* query, EntityID* out_entity);

// ComponentArray functions (internal use)
ComponentArray* braggi_component_array_create(size_t capacity, size_t component_size);
void braggi_component_array_add(ComponentArray* array, EntityID entity, void* component);
void braggi_component_array_remove(ComponentArray* array, EntityID entity);
void* braggi_component_array_get(ComponentArray* array, EntityID entity);

#endif /* BRAGGI_ECS_H */ 