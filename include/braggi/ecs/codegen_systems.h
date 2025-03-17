/*
 * Braggi - Code Generation ECS Systems
 * 
 * "Systems are like cowboys - they work the herd of entities and
 * do the heavy liftin' so the folks don't have to!" - Texas System Rancher
 */

#ifndef BRAGGI_ECS_CODEGEN_SYSTEMS_H
#define BRAGGI_ECS_CODEGEN_SYSTEMS_H

#include "braggi/ecs.h"
#include "braggi/codegen.h"
#include "braggi/codegen_arch.h"

/*
 * System initialization and registration
 */

// Register all code generation systems with the ECS world
void braggi_ecs_register_codegen_systems(ECSWorld* world);

/*
 * BackendInitSystem - Initializes the appropriate code generator backend for entities
 * 
 * Required components:
 * - TargetArchComponent
 * - BackendComponent (created if not present)
 */
System* braggi_ecs_create_backend_init_system(void);

/*
 * CodeGenContextSystem - Creates CodeGenContext structures for entities
 * 
 * Required components:
 * - TargetArchComponent
 * - EntropyFieldComponent
 * - BackendComponent (must be initialized)
 * 
 * Created components:
 * - CodeGenContextComponent
 */
System* braggi_ecs_create_codegen_context_system(void);

/*
 * CodeGenerationSystem - Performs the actual code generation
 * 
 * Required components:
 * - CodeGenContextComponent
 * - EntropyFieldComponent (with ready_for_codegen=true)
 * 
 * Updated components:
 * - CodeBlobComponent (filled with generated code)
 */
System* braggi_ecs_create_code_generation_system(void);

/*
 * CodeOutputSystem - Writes generated code to files
 * 
 * Required components:
 * - CodeGenContextComponent (with valid output_file)
 * - CodeBlobComponent (with valid data)
 */
System* braggi_ecs_create_code_output_system(void);

/*
 * System update functions - for manual control
 */

// Initialize backends for entities that need them
void braggi_ecs_update_backend_init_system(ECSWorld* world, float delta_time);

// Create code generation contexts for entities that need them
void braggi_ecs_update_codegen_context_system(ECSWorld* world, float delta_time);

// Generate code for entities with ready entropy fields
void braggi_ecs_update_code_generation_system(ECSWorld* world, float delta_time);

// Write generated code to output files
void braggi_ecs_update_code_output_system(ECSWorld* world, float delta_time);

// Process a single entity through all code generation systems
bool braggi_ecs_process_codegen_entity(ECSWorld* world, EntityID entity);

#endif /* BRAGGI_ECS_CODEGEN_SYSTEMS_H */ 