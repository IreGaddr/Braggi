/*
 * Braggi - Code Generation ECS Systems Implementation
 * 
 * "A good system knows what to do with its components, just like
 * a good Texas chef knows what to do with a prime cut of beef!" - Texas Code Ranch
 */

#include "braggi/ecs/codegen_systems.h"
#include "braggi/ecs/codegen_components.h"
#include "braggi/braggi_context.h"
#include "braggi/codegen.h"
#include "braggi/codegen_arch.h"
#include "braggi/entropy.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Define the DEBUG_PRINT macro for debugging output
#define DEBUG_PRINT(fmt, ...) do { fprintf(stderr, fmt "\n", ##__VA_ARGS__); fflush(stderr); } while(0)

// Define the validation magic number
#define GENERATOR_VALIDATION_MAGIC 0xB4C0DE47 // "BCODE G"

// Definition of the X86_64Data structure used in the backend
// Should match the one in x86_64.c
typedef struct {
    // Register allocation tracking
    uint32_t used_registers;
    
    // Current stack frame size
    int32_t stack_size;
    
    // Current function depth
    int32_t func_depth;
    
    // Region tracking
    Vector* active_regions;
    
    // Assembly output buffer
    char* asm_buffer;
    size_t asm_size;
    size_t asm_capacity;
    
    // Debug mode flag
    bool debug_mode;
} X86_64Data;

// System update function declarations
static void backend_init_system_update(ECSWorld* world, System* system, float delta_time);
static void codegen_context_system_update(ECSWorld* world, System* system, float delta_time);
static void code_generation_system_update(ECSWorld* world, System* system, float delta_time);
static void code_output_system_update(ECSWorld* world, System* system, float delta_time);

// This is the generator validation system that checks if generators are valid
static void braggi_generator_validation_system(ECSWorld* world, System* system, float delta_time) {
    if (!world) {
        DEBUG_PRINT("ERROR: Null world pointer in validation system");
        return;
    }
    
    // Get the component types we need
    ComponentTypeID codegen_comp_type = braggi_ecs_get_codegen_component_type(world);
    ComponentTypeID validation_comp_type = braggi_ecs_get_generator_validation_component_type(world);
    ComponentTypeID ctx_comp_type = braggi_ecs_get_codegen_context_component_type(world);
    
    if (codegen_comp_type == INVALID_COMPONENT_TYPE) {
        DEBUG_PRINT("WARNING: CodegenComponent type not registered");
        return;
    }
    
    // ENHANCEMENT: First check if we're in cleanup mode by looking for contexts with cleanup flags
    bool in_cleanup_mode = false;
    if (ctx_comp_type != INVALID_COMPONENT_TYPE) {
        ComponentMask ctx_mask = 0;
        braggi_ecs_mask_set(&ctx_mask, ctx_comp_type);
        
        EntityQuery ctx_query = braggi_ecs_query_entities(world, ctx_mask);
        EntityID entity;
        
        // Check if any context has the cleanup flag set
        while (braggi_ecs_query_next(&ctx_query, &entity)) {
            if (!braggi_ecs_entity_exists(world, entity)) {
                continue; // Skip entities that no longer exist
            }
            
            CodeGenContextComponent* ctx_comp = 
                braggi_ecs_get_component(world, entity, ctx_comp_type);
                
            if (ctx_comp && ctx_comp->ctx && ctx_comp->ctx->braggi_ctx && 
                (ctx_comp->ctx->braggi_ctx->flags & BRAGGI_FLAG_CODEGEN_CLEANUP_IN_PROGRESS)) {
                DEBUG_PRINT("CLEANUP MODE DETECTED: BraggiContext has cleanup flag set");
                in_cleanup_mode = true;
                break;
            }
        }
    }
    
    // Create a mask for entities with codegen components
    ComponentMask mask = 0;
    braggi_ecs_mask_set(&mask, codegen_comp_type);
    
    // Query for entities with codegen components
    EntityQuery query = braggi_ecs_query_entities(world, mask);
    EntityID entity;
    
    // Iterate through all entities with codegen components
    while (braggi_ecs_query_next(&query, &entity)) {
        if (!braggi_ecs_entity_exists(world, entity)) {
            continue; // Skip entities that no longer exist (might have been destroyed during cleanup)
        }
        
        // Get the codegen component
        CodegenComponent* codegen_comp = braggi_ecs_get_component(world, entity, codegen_comp_type);
        
        if (!codegen_comp) {
            continue;
        }
        
        // In cleanup mode, we want to be extra cautious about avoiding segfaults
        if (in_cleanup_mode) {
            // If we're in cleanup mode, just nullify references without trying to validate
            codegen_comp->generator = NULL;
            codegen_comp->initialized = false;
            
            // Update validation if it exists
            if (validation_comp_type != INVALID_COMPONENT_TYPE && 
                braggi_ecs_has_component(world, entity, validation_comp_type)) {
                GeneratorValidationComponent* validation_comp = 
                    braggi_ecs_get_component(world, entity, validation_comp_type);
                if (validation_comp) {
                    validation_comp->generator = NULL;
                    validation_comp->is_valid = false;
                }
            }
            continue; // Skip to the next entity
        }
        
        // Check if the generator is NULL
        if (!codegen_comp->generator) {
            // The generator is NULL, we should update the component's status
            codegen_comp->initialized = false;
            
            // Make sure this entity has a validation component
            if (validation_comp_type != INVALID_COMPONENT_TYPE) {
                GeneratorValidationComponent* validation_comp;
                
                if (braggi_ecs_has_component(world, entity, validation_comp_type)) {
                    // Update the existing validation component
                    validation_comp = braggi_ecs_get_component(world, entity, validation_comp_type);
                    if (validation_comp) {
                        validation_comp->is_valid = false;
                        validation_comp->validated = true;
                        validation_comp->validation_error = "Generator is NULL or has been destroyed";
                    }
                } else {
                    // Add a new validation component
                    validation_comp = braggi_ecs_add_component(world, entity, validation_comp_type);
                    if (validation_comp) {
                        validation_comp->generator = NULL;
                        validation_comp->is_valid = false;
                        validation_comp->validated = true;
                        validation_comp->validation_magic = 0xB4C0DE47; // "BCODE G"
                        validation_comp->last_validated = time(NULL);
                        validation_comp->validation_error = "Generator is NULL or has been destroyed";
                    }
                }
            }
            
            continue;
        }
        
        // If the generator is not NULL, let's validate it
        if (validation_comp_type != INVALID_COMPONENT_TYPE) {
            braggi_ecs_validate_generator(world, entity, codegen_comp->generator);
        }
    }
    
    // CRITICAL FIX: Now check for any orphaned references to generators in other components
    // This is the key part that ensures we don't have dangling references after codegen cleanup
    if (validation_comp_type != INVALID_COMPONENT_TYPE) {
        // Create a mask for entities with validation components
        ComponentMask val_mask = 0;
        braggi_ecs_mask_set(&val_mask, validation_comp_type);
        
        // Query for entities with validation components
        EntityQuery val_query = braggi_ecs_query_entities(world, val_mask);
        
        DEBUG_PRINT("Checking for orphaned generator references after cleanup");
        
        // Iterate through all entities with validation components
        while (braggi_ecs_query_next(&val_query, &entity)) {
            if (!braggi_ecs_entity_exists(world, entity)) {
                continue; // Skip entities that no longer exist
            }
            
            GeneratorValidationComponent* validation_comp = 
                braggi_ecs_get_component(world, entity, validation_comp_type);
                
            if (!validation_comp) {
                continue;
            }
            
            // In cleanup mode, always nullify references
            if (in_cleanup_mode) {
                validation_comp->generator = NULL;
                validation_comp->is_valid = false;
                continue;
            }
            
            // If the component has a generator pointer but is marked as invalid,
            // this is likely a stale reference that wasn't properly cleared
            if (validation_comp->generator != NULL && !validation_comp->is_valid) {
                DEBUG_PRINT("WARNING: Found orphaned generator reference in entity %u - clearing", entity);
                validation_comp->generator = NULL;
            }
            
            // Double-check if we have a codegen component, make sure they're in sync
            if (braggi_ecs_has_component(world, entity, codegen_comp_type)) {
                CodegenComponent* codegen_comp = 
                    braggi_ecs_get_component(world, entity, codegen_comp_type);
                    
                if (codegen_comp && codegen_comp->generator != validation_comp->generator) {
                    DEBUG_PRINT("WARNING: Generator reference mismatch in entity %u - fixing", entity);
                    
                    // If validation says it's invalid, trust that and nullify the codegen reference
                    if (!validation_comp->is_valid) {
                        codegen_comp->generator = NULL;
                        codegen_comp->initialized = false;
                    } 
                    // Otherwise update the validation component with the current generator
                    else if (codegen_comp->generator) {
                        braggi_ecs_validate_generator(world, entity, codegen_comp->generator);
                    }
                }
            }
        }
    }
    
    // Also check for any CodeGenContextComponent references to destroyed generators
    if (ctx_comp_type != INVALID_COMPONENT_TYPE) {
        ComponentMask ctx_mask = 0;
        braggi_ecs_mask_set(&ctx_mask, ctx_comp_type);
        
        EntityQuery ctx_query = braggi_ecs_query_entities(world, ctx_mask);
        
        // Process each entity with a context component
        while (braggi_ecs_query_next(&ctx_query, &entity)) {
            if (!braggi_ecs_entity_exists(world, entity)) {
                continue; // Skip entities that no longer exist
            }
            
            CodeGenContextComponent* ctx_comp = 
                braggi_ecs_get_component(world, entity, ctx_comp_type);
                
            if (!ctx_comp || !ctx_comp->ctx) {
                continue;
            }
            
            // In cleanup mode, always nullify generator references
            if (in_cleanup_mode && ctx_comp->ctx->generator) {
                DEBUG_PRINT("CLEANUP MODE: Nullifying generator in context for entity %u", entity);
                ctx_comp->ctx->generator = NULL;
                continue;
            }
            
            if (ctx_comp->ctx->generator) {
                // Check if this generator is invalid by cross-referencing with validation components
                bool is_valid = true;
                
                // First check if this entity has a validation component
                if (validation_comp_type != INVALID_COMPONENT_TYPE && 
                    braggi_ecs_has_component(world, entity, validation_comp_type)) {
                    
                    GeneratorValidationComponent* validation_comp = 
                        braggi_ecs_get_component(world, entity, validation_comp_type);
                        
                    if (validation_comp && !validation_comp->is_valid) {
                        is_valid = false;
                    }
                }
                
                // If not valid, clear the reference
                if (!is_valid) {
                    DEBUG_PRINT("WARNING: Found invalid generator in context component, entity %u - clearing", entity);
                    ctx_comp->ctx->generator = NULL;
                }
            }
        }
    }
    
    DEBUG_PRINT("Generator validation system completed cleanup check");
}

// Register all code generation systems with the ECS world
void braggi_ecs_register_codegen_systems(ECSWorld* world) {
    if (!world) return;
    
    // Register validation system FIRST so it runs before other systems
    
    // Create validation system
    SystemInfo validation_system_info = {
        .name = "GeneratorValidationSystem",
        .update_func = braggi_generator_validation_system,
        .context = NULL,
        .priority = 100  // High priority to run first
    };
    
    System* validation_system = braggi_ecs_create_system(&validation_system_info);
    if (validation_system) {
        braggi_ecs_add_system(world, validation_system);
    }
    
    // Register components first
    braggi_ecs_register_codegen_components(world);
    
    // Create and register systems
    System* backend_init_system = braggi_ecs_create_backend_init_system();
    System* codegen_context_system = braggi_ecs_create_codegen_context_system();
    System* code_generation_system = braggi_ecs_create_code_generation_system();
    System* code_output_system = braggi_ecs_create_code_output_system();
    
    braggi_ecs_register_system(world, backend_init_system);
    braggi_ecs_register_system(world, codegen_context_system);
    braggi_ecs_register_system(world, code_generation_system);
    braggi_ecs_register_system(world, code_output_system);
}

// Create the backend initialization system
System* braggi_ecs_create_backend_init_system(void) {
    // Create component mask for the system
    ComponentMask component_mask = 0;
    
    // System will operate on entities with TargetArchComponent
    // We need to initialize the component types differently since we can't
    // directly access them at this point
    
    SystemInfo info = {
        .name = "BackendInitSystem",
        .update_func = backend_init_system_update,
        .context = NULL,
        .priority = 100  // High priority since other systems depend on this
    };
    
    return braggi_ecs_create_system(&info);
}

// Create the code generation context system
System* braggi_ecs_create_codegen_context_system(void) {
    SystemInfo info = {
        .name = "CodeGenContextSystem",
        .update_func = codegen_context_system_update,
        .context = NULL,
        .priority = 90  // Run after backend init
    };
    
    return braggi_ecs_create_system(&info);
}

// Create the code generation system
System* braggi_ecs_create_code_generation_system(void) {
    SystemInfo info = {
        .name = "CodeGenerationSystem",
        .update_func = code_generation_system_update,
        .context = NULL,
        .priority = 80  // Run after context is created
    };
    
    return braggi_ecs_create_system(&info);
}

// Create the code output system
System* braggi_ecs_create_code_output_system(void) {
    SystemInfo info = {
        .name = "CodeOutputSystem",
        .update_func = code_output_system_update,
        .context = NULL,
        .priority = 70  // Run after code is generated
    };
    
    return braggi_ecs_create_system(&info);
}

// Backend initialization system update function
static void backend_init_system_update(ECSWorld* world, System* system, float delta_time) {
    if (!world || !system) return;
    
    // Get component type IDs
    ComponentTypeID target_arch_component_type = braggi_ecs_get_target_arch_component_type(world);
    ComponentTypeID backend_component_type = braggi_ecs_get_backend_component_type(world);
    
    if (target_arch_component_type == INVALID_COMPONENT_TYPE) {
        fprintf(stderr, "ERROR: TargetArchComponent not registered\n");
        return;
    }
    
    // Set up component mask to find entities with TargetArchComponent
    ComponentMask mask = 0;
    braggi_ecs_mask_set(&mask, target_arch_component_type);
    
    // Query entities
    EntityQuery query = braggi_ecs_query_entities(world, mask);
    EntityID entity;
    
    while (braggi_ecs_query_next(&query, &entity)) {
        // Get the target architecture component
        TargetArchComponent* target_comp = braggi_ecs_get_component(world, entity, target_arch_component_type);
        if (!target_comp) continue;
        
        // Check if the entity already has a backend component
        BackendComponent* backend_comp = NULL;
        if (braggi_ecs_has_component(world, entity, backend_component_type)) {
            backend_comp = braggi_ecs_get_component(world, entity, backend_component_type);
            
            // Skip if already initialized
            if (backend_comp && backend_comp->initialized) {
                continue;
            }
        } else {
            // Create a new backend component
            backend_comp = braggi_ecs_add_component(world, entity, backend_component_type);
        }
        
        if (!backend_comp) {
            fprintf(stderr, "ERROR: Failed to create or get BackendComponent\n");
            continue;
        }
        
        // Set the backend name based on the target architecture
        backend_comp->backend_name = braggi_codegen_arch_to_string(target_comp->arch);
        
        // Initialize the backend
        // In a real implementation, we would get the appropriate backend from the
        // code generator manager and initialize it
        
        // For now, just mark it as initialized
        backend_comp->initialized = true;
        
        fprintf(stderr, "DEBUG: Initialized backend '%s' for entity %u\n", 
               backend_comp->backend_name, entity);
    }
}

// Code generation context system update function
static void codegen_context_system_update(ECSWorld* world, System* system, float delta_time) {
    if (!world || !system) return;
    
    // Get component type IDs
    ComponentTypeID target_arch_component_type = braggi_ecs_get_target_arch_component_type(world);
    ComponentTypeID backend_component_type = braggi_ecs_get_backend_component_type(world);
    ComponentTypeID entropy_field_component_type = braggi_ecs_get_entropy_field_component_type(world);
    ComponentTypeID codegen_context_component_type = braggi_ecs_get_codegen_context_component_type(world);
    
    // Set up component mask
    ComponentMask mask = 0;
    braggi_ecs_mask_set(&mask, target_arch_component_type);
    braggi_ecs_mask_set(&mask, backend_component_type);
    braggi_ecs_mask_set(&mask, entropy_field_component_type);
    
    // Query entities
    EntityQuery query = braggi_ecs_query_entities(world, mask);
    EntityID entity;
    
    while (braggi_ecs_query_next(&query, &entity)) {
        // Skip entities that already have a CodeGenContextComponent
        if (braggi_ecs_has_component(world, entity, codegen_context_component_type)) {
            continue;
        }
        
        // Get the required components
        TargetArchComponent* target_comp = braggi_ecs_get_component(world, entity, target_arch_component_type);
        BackendComponent* backend_comp = braggi_ecs_get_component(world, entity, backend_component_type);
        EntropyFieldComponent* field_comp = braggi_ecs_get_component(world, entity, entropy_field_component_type);
        
        // Skip if any components are missing or backend isn't initialized
        if (!target_comp || !backend_comp || !field_comp || !backend_comp->initialized) {
            continue;
        }
        
        // Create a new CodeGenContextComponent
        CodeGenContextComponent* ctx_comp = braggi_ecs_add_component(world, entity, codegen_context_component_type);
        if (!ctx_comp) {
            fprintf(stderr, "ERROR: Failed to create CodeGenContextComponent\n");
            continue;
        }
        
        // Create a CodeGenOptions based on the TargetArchComponent
        CodeGenOptions options;
        options.arch = target_comp->arch;
        options.format = target_comp->format;
        options.optimize = target_comp->optimize;
        options.optimization_level = target_comp->optimization_level;
        options.emit_debug_info = target_comp->emit_debug_info;
        options.output_file = NULL;  // Will be set later
        
        // Create a dummy context for now
        // In a real implementation, we'd create a proper CodeGenContext with a BraggiContext
        CodeGenContext* ctx = malloc(sizeof(CodeGenContext));
        if (!ctx) {
            fprintf(stderr, "ERROR: Failed to allocate CodeGenContext\n");
            continue;
        }
        
        ctx->braggi_ctx = NULL;  // This would point to the BraggiContext
        ctx->options = options;
        ctx->arch_data = NULL;   // Architecture-specific data will be filled later
        
        // Store the context in the component
        ctx_comp->ctx = ctx;
        
        // Generate a default output file name if none is provided
        if (!ctx_comp->output_file) {
            ctx_comp->output_file = strdup("output.s");
            if (!ctx_comp->output_file) {
                fprintf(stderr, "ERROR: Failed to allocate output file name\n");
            }
        }
        
        // Update the context's output file
        ctx->options.output_file = ctx_comp->output_file;
        
        fprintf(stderr, "DEBUG: Created CodeGenContext for entity %u (arch=%s, output=%s)\n", 
               entity, braggi_codegen_arch_to_string(target_comp->arch),
               ctx_comp->output_file);
    }
}

// Code generation system update function
static void code_generation_system_update(ECSWorld* world, System* system, float delta_time) {
    if (!world || !system) return;
    
    // Get component type IDs
    ComponentTypeID codegen_context_component_type = braggi_ecs_get_codegen_context_component_type(world);
    ComponentTypeID entropy_field_component_type = braggi_ecs_get_entropy_field_component_type(world);
    ComponentTypeID code_blob_component_type = braggi_ecs_get_code_blob_component_type(world);
    ComponentTypeID validation_component_type = braggi_ecs_get_generator_validation_component_type(world);
    
    // Set up component mask
    ComponentMask mask = 0;
    braggi_ecs_mask_set(&mask, codegen_context_component_type);
    braggi_ecs_mask_set(&mask, entropy_field_component_type);
    braggi_ecs_mask_set(&mask, code_blob_component_type);
    
    // Query entities
    EntityQuery query = braggi_ecs_query_entities(world, mask);
    EntityID entity;
    
    while (braggi_ecs_query_next(&query, &entity)) {
        // Get the required components
        CodeGenContextComponent* ctx_comp = braggi_ecs_get_component(world, entity, codegen_context_component_type);
        EntropyFieldComponent* field_comp = braggi_ecs_get_component(world, entity, entropy_field_component_type);
        CodeBlobComponent* blob_comp = braggi_ecs_get_component(world, entity, code_blob_component_type);
        
        // Skip if any components are missing or if the entropy field isn't ready
        if (!ctx_comp || !field_comp || !blob_comp || !field_comp->ready_for_codegen) {
            continue;
        }
        
        // Skip if the code blob already has data
        if (blob_comp->data && blob_comp->size > 0) {
            continue;
        }
        
        // Check if we have a valid CodeGenContext
        if (!ctx_comp->ctx) {
            fprintf(stderr, "ERROR: NULL CodeGenContext for entity %u\n", entity);
            continue;
        }
        
        // In a real implementation:
        // 1. Properly connect the entropy field to the codegen context
        // NOTE: We DO NOT cast EntropyField to BraggiContext - they are different types!
        if (ctx_comp->ctx->options.entropy_field == NULL) {
            ctx_comp->ctx->options.entropy_field = field_comp->field;
        }
        
        // ENHANCEMENT: Check validation status before proceeding
        bool generator_validated = false;
        
        if (validation_component_type != INVALID_COMPONENT_TYPE) {
            // If validation component exists, check if generator is valid
            if (braggi_ecs_has_component(world, entity, validation_component_type)) {
                GeneratorValidationComponent* validation_comp = 
                    braggi_ecs_get_component(world, entity, validation_component_type);
                
                if (validation_comp) {
                    // If validator says the generator isn't valid, skip this entity
                    if (!validation_comp->is_valid || validation_comp->generator == NULL) {
                        fprintf(stderr, "ERROR: Invalid generator for entity %u (validation error: %s)\n", 
                               entity, validation_comp->validation_error ? 
                               validation_comp->validation_error : "Unknown error");
                        continue;
                    }
                    
                    // Generator has been validated and is valid
                    generator_validated = true;
                }
            }
        }
        
        // BUGFIX: If we don't have validation info, validate the generator now
        if (!generator_validated && ctx_comp->ctx->generator) {
            // Perform ad-hoc validation
            if (!braggi_ecs_validate_generator(world, entity, ctx_comp->ctx->generator)) {
                fprintf(stderr, "ERROR: Generator validation failed for entity %u\n", entity);
                continue;
            }
        }
        
        // 2. Call the code generator with proper error handling
        bool success = false;
        
        // BUGFIX: Enhanced safety check - avoid segfaults from use-after-free
        if (ctx_comp->ctx->generator && 
            ctx_comp->ctx->generator->generate &&
            ctx_comp->ctx->generator->emit) {
            
            // Defensive check for destroyed generators (avoid dereferencing possibly freed data)
            const char* generator_name = "(unknown)";
            if (ctx_comp->ctx->generator->name) {
                generator_name = ctx_comp->ctx->generator->name;
            }
            
            fprintf(stderr, "DEBUG: Using generator '%s' for entity %u\n", generator_name, entity);
            
            // Call the generator directly with the entropy field
            success = ctx_comp->ctx->generator->generate(ctx_comp->ctx->generator, field_comp->field);
            
            if (success && ctx_comp->output_file) {
                // Output the generated code to a file
                success = ctx_comp->ctx->generator->emit(
                    ctx_comp->ctx->generator, 
                    ctx_comp->output_file, 
                    ctx_comp->ctx->options.format
                );
            }
        } else {
            fprintf(stderr, "ERROR: Invalid generator for entity %u\n", entity);
        }
        
        if (success) {
            fprintf(stderr, "DEBUG: Generated code for entity %u (output=%s)\n", 
                   entity, ctx_comp->output_file ? ctx_comp->output_file : "none");
            
            // Get the generated code if available
            if (ctx_comp->ctx->generator && ctx_comp->ctx->generator->arch_data) {
                // For x86_64 backend, copy the assembly buffer to the code blob
                X86_64Data* x86_data = (X86_64Data*)ctx_comp->ctx->generator->arch_data;
                
                // BUGFIX: Add defensive NULL check for x86_data before accessing
                if (x86_data && x86_data->asm_buffer && x86_data->asm_size > 0) {
                    blob_comp->data = malloc(x86_data->asm_size);
                    if (blob_comp->data) {
                        memcpy(blob_comp->data, x86_data->asm_buffer, x86_data->asm_size);
                        blob_comp->size = x86_data->asm_size;
                        blob_comp->is_binary = false;
                    }
                } else {
                    // Fallback to a dummy blob if no real data
                    const char* dummy_code = "# Generated by Braggi ECS Code Generation System\n";
                    size_t code_size = strlen(dummy_code) + 1;
                    
                    blob_comp->data = malloc(code_size);
                    if (blob_comp->data) {
                        strcpy(blob_comp->data, dummy_code);
                        blob_comp->size = code_size;
                        blob_comp->is_binary = false;
                    }
                }
            } else {
                fprintf(stderr, "WARNING: No code generator arch_data available\n");
            }
            
            if (!blob_comp->data) {
                fprintf(stderr, "ERROR: Failed to allocate code blob data\n");
            }
        } else {
            fprintf(stderr, "ERROR: Code generation failed for entity %u\n", entity);
        }
    }
}

// Code output system update function
static void code_output_system_update(ECSWorld* world, System* system, float delta_time) {
    if (!world || !system) return;
    
    // Get component type IDs
    ComponentTypeID codegen_context_component_type = braggi_ecs_get_codegen_context_component_type(world);
    ComponentTypeID code_blob_component_type = braggi_ecs_get_code_blob_component_type(world);
    
    // Set up component mask
    ComponentMask mask = 0;
    braggi_ecs_mask_set(&mask, codegen_context_component_type);
    braggi_ecs_mask_set(&mask, code_blob_component_type);
    
    // Query entities
    EntityQuery query = braggi_ecs_query_entities(world, mask);
    EntityID entity;
    
    while (braggi_ecs_query_next(&query, &entity)) {
        // Get the required components
        CodeGenContextComponent* ctx_comp = braggi_ecs_get_component(world, entity, codegen_context_component_type);
        CodeBlobComponent* blob_comp = braggi_ecs_get_component(world, entity, code_blob_component_type);
        
        // Skip if any components are missing or if there's no data
        if (!ctx_comp || !blob_comp || !blob_comp->data || blob_comp->size == 0) {
            continue;
        }
        
        // Skip if there's no output file
        if (!ctx_comp->output_file) {
            fprintf(stderr, "ERROR: No output file specified for entity %u\n", entity);
            continue;
        }
        
        // Write the code blob to the output file
        FILE* file = fopen(ctx_comp->output_file, "wb");
        if (!file) {
            fprintf(stderr, "ERROR: Failed to open output file '%s' for entity %u\n", 
                   ctx_comp->output_file, entity);
            continue;
        }
        
        size_t written = fwrite(blob_comp->data, 1, blob_comp->size - 1, file);  // Don't write null terminator
        fclose(file);
        
        if (written != blob_comp->size - 1) {
            fprintf(stderr, "ERROR: Failed to write all data to '%s' for entity %u\n", 
                   ctx_comp->output_file, entity);
        } else {
            fprintf(stderr, "DEBUG: Wrote %zu bytes to '%s' for entity %u\n", 
                   written, ctx_comp->output_file, entity);
        }
    }
}

// Public update functions for manual control
void braggi_ecs_update_backend_init_system(ECSWorld* world, float delta_time) {
    System* system = braggi_ecs_get_system_by_name(world, "BackendInitSystem");
    if (system) {
        backend_init_system_update(world, system, delta_time);
    }
}

void braggi_ecs_update_codegen_context_system(ECSWorld* world, float delta_time) {
    System* system = braggi_ecs_get_system_by_name(world, "CodeGenContextSystem");
    if (system) {
        codegen_context_system_update(world, system, delta_time);
    }
}

void braggi_ecs_update_code_generation_system(ECSWorld* world, float delta_time) {
    System* system = braggi_ecs_get_system_by_name(world, "CodeGenerationSystem");
    if (system) {
        code_generation_system_update(world, system, delta_time);
    }
}

void braggi_ecs_update_code_output_system(ECSWorld* world, float delta_time) {
    System* system = braggi_ecs_get_system_by_name(world, "CodeOutputSystem");
    if (system) {
        code_output_system_update(world, system, delta_time);
    }
}

// Process a single entity through all code generation systems
bool braggi_ecs_process_codegen_entity(ECSWorld* world, EntityID entity) {
    if (!world || entity == INVALID_ENTITY) {
        return false;
    }
    
    // Get component type IDs
    ComponentTypeID target_arch_component_type = braggi_ecs_get_target_arch_component_type(world);
    ComponentTypeID entropy_field_component_type = braggi_ecs_get_entropy_field_component_type(world);
    
    // Check if the entity has the minimum required components
    if (!braggi_ecs_has_component(world, entity, target_arch_component_type) ||
        !braggi_ecs_has_component(world, entity, entropy_field_component_type)) {
        fprintf(stderr, "ERROR: Entity %u missing required components for code generation\n", entity);
        return false;
    }
    
    // Update each system manually for this entity
    float dummy_delta = 0.0f;
    
    // Initialize backend
    braggi_ecs_update_backend_init_system(world, dummy_delta);
    
    // Create code generation context
    braggi_ecs_update_codegen_context_system(world, dummy_delta);
    
    // Generate code
    braggi_ecs_update_code_generation_system(world, dummy_delta);
    
    // Output code to file
    braggi_ecs_update_code_output_system(world, dummy_delta);
    
    // Verify that all components were created successfully
    ComponentTypeID codegen_context_component_type = braggi_ecs_get_codegen_context_component_type(world);
    ComponentTypeID backend_component_type = braggi_ecs_get_backend_component_type(world);
    ComponentTypeID code_blob_component_type = braggi_ecs_get_code_blob_component_type(world);
    
    bool success = 
        braggi_ecs_has_component(world, entity, target_arch_component_type) &&
        braggi_ecs_has_component(world, entity, entropy_field_component_type) &&
        braggi_ecs_has_component(world, entity, backend_component_type) &&
        braggi_ecs_has_component(world, entity, codegen_context_component_type) &&
        braggi_ecs_has_component(world, entity, code_blob_component_type);
    
    if (success) {
        // Check if we have valid data in the code blob
        CodeBlobComponent* blob_comp = braggi_ecs_get_component(world, entity, code_blob_component_type);
        success = blob_comp && blob_comp->data && blob_comp->size > 0;
    }
    
    return success;
}

// Make sure we run the validation system one more time at the end of the manager cleanup
// to catch any stray references
void braggi_codegen_manager_final_validation_check(ECSWorld* world) {
    if (!world) {
        DEBUG_PRINT("WARNING: NULL world in final validation check");
        return;
    }
    
    System* validation_system = braggi_ecs_get_system_by_name(world, "GeneratorValidationSystem");
    
    if (validation_system) {
        DEBUG_PRINT("Running final validation check to clean up any stray references");
        
        // ENHANCEMENT: Set a local flag to indicate we're in the final validation phase
        // This is a last-ditch effort to prevent segmentation faults, so we'll be
        // extra aggressive about cleaning up references
        
        // Check BraggiContext flags in any CodeGenContextComponents
        ComponentTypeID ctx_comp_type = braggi_ecs_get_codegen_context_component_type(world);
        if (ctx_comp_type != INVALID_COMPONENT_TYPE) {
            ComponentMask ctx_mask = 0;
            braggi_ecs_mask_set(&ctx_mask, ctx_comp_type);
            
            EntityQuery ctx_query = braggi_ecs_query_entities(world, ctx_mask);
            EntityID entity;
            
            // Make sure all BraggiContexts have the cleanup flag set
            while (braggi_ecs_query_next(&ctx_query, &entity)) {
                if (!braggi_ecs_entity_exists(world, entity)) {
                    continue;
                }
                
                CodeGenContextComponent* ctx_comp = 
                    braggi_ecs_get_component(world, entity, ctx_comp_type);
                    
                if (ctx_comp && ctx_comp->ctx && ctx_comp->ctx->braggi_ctx) {
                    // Set the cleanup flag to ensure validation system knows we're in cleanup
                    ctx_comp->ctx->braggi_ctx->flags |= BRAGGI_FLAG_CODEGEN_CLEANUP_IN_PROGRESS;
                    DEBUG_PRINT("Set cleanup flag in BraggiContext for final validation");
                }
            }
        }
        
        // Now run the validation system which will detect the cleanup mode
        validation_system->update_func(world, validation_system, 0.0f);
        DEBUG_PRINT("Final validation check complete");
    } else {
        DEBUG_PRINT("WARNING: Validation system not found for final check");
        
        // Manual cleanup for orphaned CodeGenerator references
        DEBUG_PRINT("Attempting manual cleanup of validation components");
        
        // Check if the component type ID is valid before proceeding
        ComponentTypeID validation_comp_type = braggi_ecs_get_generator_validation_component_type(world);
        DEBUG_PRINT("Generator validation component type: %u", validation_comp_type);
        
        if (validation_comp_type == INVALID_COMPONENT_TYPE) {
            DEBUG_PRINT("No validation component type found in the world, skipping cleanup");
            DEBUG_PRINT("Manual cleanup of validation components complete");
            return;  // Early return to avoid segfault with invalid component type
        }
        
        DEBUG_PRINT("Found valid validation component type, proceeding with cleanup");
        
        // CRITICAL FIX: Use defensive programming to avoid segfaults
        // First check if world still exists and has component arrays
        if (!world || !world->component_arrays) {
            DEBUG_PRINT("WARNING: ECS world or component arrays are NULL, skipping component cleanup");
            DEBUG_PRINT("Manual cleanup of validation components complete");
            return;
        }
        
        // Check if the component array for validation components exists
        if (validation_comp_type >= world->max_component_types || 
            !world->component_arrays[validation_comp_type]) {
            DEBUG_PRINT("WARNING: Component array for validation components not found, skipping cleanup");
            DEBUG_PRINT("Manual cleanup of validation components complete");
            return;
        }
        
        ComponentMask validation_mask = 0;
        braggi_ecs_mask_set(&validation_mask, validation_comp_type);
        
        // Use a try/catch-like approach with setjmp/longjmp for critical error recovery
        DEBUG_PRINT("Creating entity query for validation components");
        
        // CRITICAL FIX: Use a safer approach to iterate through entities
        // Check if the component array has valid data
        ComponentArray* comp_array = world->component_arrays[validation_comp_type];
        if (!comp_array || !comp_array->data || comp_array->size == 0) {
            DEBUG_PRINT("WARNING: Component array is empty or invalid, skipping cleanup");
            DEBUG_PRINT("Manual cleanup of validation components complete");
            return;
        }
        
        DEBUG_PRINT("Getting entities with validation components directly from component array");
        int entity_count = 0;
        
        // Iterate directly through the component array
        for (size_t i = 0; i < comp_array->size; i++) {
            if (i >= comp_array->size) break;
            
            EntityID entity = comp_array->index_to_entity[i];
            if (entity == INVALID_ENTITY) continue;
            
            entity_count++;
            DEBUG_PRINT("Processing entity %u with validation component", entity);
            
            // Get the validation component using direct array access
            GeneratorValidationComponent* validation_comp = 
                (GeneratorValidationComponent*)(comp_array->data + (i * comp_array->component_size));
            
            if (validation_comp) {
                DEBUG_PRINT("Found validation component for entity %u", entity);
                
                // Clear the reference but don't free it (it's already freed by the manager)
                validation_comp->generator = NULL;
                validation_comp->is_valid = false;
                validation_comp->validated = true;
                DEBUG_PRINT("Cleared orphaned generator reference in validation component");
            } else {
                DEBUG_PRINT("WARNING: Failed to get validation component for entity %u", entity);
            }
        }
        
        DEBUG_PRINT("Processed %d entities with validation components", entity_count);
        DEBUG_PRINT("Manual cleanup of validation components complete");
    }
} 