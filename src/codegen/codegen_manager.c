/*
 * Braggi - Code Generation Manager
 *
 * "A good manager knows when to step back and let the specialists do their work,
 * just like a good Texan rancher knows when to let the horses run!" - Texas Code Wrangler
 */

#include "braggi/codegen.h"
#include "braggi/codegen_arch.h"
#include "braggi/ecs.h"
#include "braggi/ecs/codegen_components.h"
#include "braggi/ecs/codegen_systems.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Debug print helper macro
#define DEBUG_PRINT(fmt, ...) do { fprintf(stderr, "CODEGEN MANAGER: " fmt "\n", ##__VA_ARGS__); fflush(stderr); } while (0)

// Maximum number of registered code generators
#define MAX_CODEGEN_BACKENDS 10

// Generator status flags to track lifecycle
typedef enum {
    GENERATOR_STATUS_INVALID = 0,
    GENERATOR_STATUS_INITIALIZED = 1,
    GENERATOR_STATUS_ACTIVE = 2,
    GENERATOR_STATUS_DESTROYING = 3,
    GENERATOR_STATUS_DESTROYED = 4
} GeneratorStatus;

// Extended generator structure with status tracking
typedef struct {
    CodeGenerator* generator;  // The actual generator
    GeneratorStatus status;    // Current lifecycle status
    bool owned_by_manager;     // Whether the manager should free this generator
} ManagedGenerator;

// Codegen manager structure
typedef struct {
    ManagedGenerator backends[MAX_CODEGEN_BACKENDS];
    int num_backends;
    ManagedGenerator* default_backend;
    ErrorHandler* error_handler;
    ECSWorld* ecs_world;  // ECS world for component-based code generation
} CodeGenManager;

// Singleton instance of the manager
static CodeGenManager* manager = NULL;

// Forward declarations
static void codegen_error(ErrorHandler* handler, const char* message);
static void codegen_warning(ErrorHandler* handler, const char* message);

// Initialize the code generation manager
bool braggi_codegen_manager_init(void) {
    if (manager) {
        DEBUG_PRINT("Manager already initialized");
        return true;  // Already initialized
    }
    
    DEBUG_PRINT("Initializing code generation manager");
    
    // Allocate the manager
    manager = (CodeGenManager*)calloc(1, sizeof(CodeGenManager));
    if (!manager) {
        DEBUG_PRINT("Failed to allocate memory for manager");
        return false;
    }
    
    // Create error handler
    manager->error_handler = (ErrorHandler*)malloc(sizeof(ErrorHandler));
    if (!manager->error_handler) {
        DEBUG_PRINT("Failed to allocate memory for error handler");
        free(manager);
        manager = NULL;
        return false;
    }
    
    manager->error_handler->error = codegen_error;
    manager->error_handler->warning = codegen_warning;
    manager->error_handler->context = manager;
    
    // Register available backends
    // x86_64 is considered our most mature backend
    braggi_register_x86_64_backend();
    
    // ARM is also fairly mature
    braggi_register_arm_backend();
    braggi_register_arm64_backend();
    
    // Create ECS world for component-based code generation
    manager->ecs_world = braggi_ecs_world_create(100, 32);  // Start with capacity for 100 entities and 32 component types
    if (!manager->ecs_world) {
        DEBUG_PRINT("Failed to create ECS world");
        free(manager->error_handler);
        free(manager);
        manager = NULL;
        return false;
    }
    
    // Register code generation components and systems
    braggi_ecs_register_codegen_components(manager->ecs_world);
    braggi_ecs_register_codegen_systems(manager->ecs_world);
    
    DEBUG_PRINT("Manager initialized successfully with %d backends", manager->num_backends);
    return true;
}

// Clean up the code generation manager
void braggi_codegen_manager_cleanup(void) {
    if (!manager) {
        return;
    }
    
    DEBUG_PRINT("Cleaning up code generation manager...");
    
    // First cleanup all active generators
    for (int i = 0; i < manager->num_backends; i++) {
        if (manager->backends[i].generator) {
            DEBUG_PRINT("Cleaning up generator %d: %s", i, 
                   manager->backends[i].generator->name ? manager->backends[i].generator->name : "(unnamed)");
            
            // Mark the generator as being destroyed
            manager->backends[i].status = GENERATOR_STATUS_DESTROYING;
            
            // Call the generator's destroy function if it exists
            if (manager->backends[i].generator->destroy) {
                manager->backends[i].generator->destroy(manager->backends[i].generator);
            }
            
            // Free the generator itself if it's owned by the manager
            if (manager->backends[i].owned_by_manager) {
                // Free strings first
                if (manager->backends[i].generator->name) {
                    free((void*)manager->backends[i].generator->name);
                }
                if (manager->backends[i].generator->description) {
                    free((void*)manager->backends[i].generator->description);
                }
                
                free(manager->backends[i].generator);
            }
            
            manager->backends[i].generator = NULL;
            manager->backends[i].status = GENERATOR_STATUS_DESTROYED;
        }
    }
    
    // Clear the default backend
    manager->default_backend = NULL;
    
    // Reset state
    manager->num_backends = 0;
    
    // CRITICAL FIX: Run one final validation check to ensure all references are cleaned up
    // This will help prevent segmentation faults from use-after-free issues
    if (manager->ecs_world) {
        DEBUG_PRINT("Running final validation check before manager cleanup completes");
        extern void braggi_codegen_manager_final_validation_check(ECSWorld* world);
        braggi_codegen_manager_final_validation_check(manager->ecs_world);
    }
    
    // Don't free the error handler, we don't own it
    manager->error_handler = NULL;
    
    // Free the manager itself
    free(manager);
    manager = NULL;
    
    DEBUG_PRINT("Code generation manager cleanup complete");
}

// Register a new code generator backend
bool braggi_codegen_manager_register_backend(CodeGenerator* backend) {
    if (!manager) {
        DEBUG_PRINT("ERROR: Manager not initialized");
        return false;
    }
    
    if (!backend) {
        DEBUG_PRINT("ERROR: NULL backend");
        return false;
    }
    
    // Check if we have room for another backend
    if (manager->num_backends >= MAX_CODEGEN_BACKENDS) {
        DEBUG_PRINT("ERROR: Too many code generator backends registered (max: %d)", MAX_CODEGEN_BACKENDS);
        codegen_error(manager->error_handler, "Too many code generator backends registered");
        return false;
    }
    
    DEBUG_PRINT("Registering backend: %s", backend->name ? backend->name : "(unnamed)");
    
    // Initialize the backend if it has an init function
    if (backend->init && !backend->init(backend, manager->error_handler)) {
        DEBUG_PRINT("ERROR: Failed to initialize code generator backend");
        codegen_error(manager->error_handler, "Failed to initialize code generator backend");
        return false;
    }
    
    // Add the backend to our list
    int backend_index = manager->num_backends++;
    manager->backends[backend_index].generator = backend;
    manager->backends[backend_index].status = GENERATOR_STATUS_ACTIVE;
    manager->backends[backend_index].owned_by_manager = true;
    
    // If this is the first backend, or if it's x86_64, make it the default
    if (manager->num_backends == 1 || (backend->name && strcmp(backend->name, "x86_64") == 0)) {
        DEBUG_PRINT("Setting as default backend");
        manager->default_backend = &manager->backends[backend_index];
    }
    
    DEBUG_PRINT("Backend registered successfully (index: %d)", backend_index);
    return true;
}

// Get a code generator for the specified architecture
CodeGenerator* braggi_codegen_manager_get_backend(TargetArch arch) {
    if (!manager) {
        DEBUG_PRINT("ERROR: Manager not initialized");
        return NULL;
    }
    
    // Look for a backend that supports the requested architecture
    const char* arch_name = braggi_codegen_arch_to_string(arch);
    DEBUG_PRINT("Looking for backend for architecture: %s", arch_name);
    
    for (int i = 0; i < manager->num_backends; i++) {
        if (manager->backends[i].generator && manager->backends[i].status == GENERATOR_STATUS_ACTIVE) {
            // ENHANCEMENT: Double check that the generator is actually valid
            if (manager->backends[i].generator->name == NULL) {
                DEBUG_PRINT("WARNING: Found backend with NULL name at index %d, skipping", i);
                continue;
            }
            
            if (strcmp(manager->backends[i].generator->name, arch_name) == 0) {
                DEBUG_PRINT("Found matching backend at index %d", i);
                // ENHANCEMENT: Double check one more time that the status is ACTIVE
                if (manager->backends[i].status != GENERATOR_STATUS_ACTIVE) {
                    DEBUG_PRINT("WARNING: Backend status changed to %d during lookup, skipping", 
                               manager->backends[i].status);
                    continue;
                }
                return manager->backends[i].generator;
            }
        }
    }
    
    // Fall back to the default backend
    if (manager->default_backend && manager->default_backend->status == GENERATOR_STATUS_ACTIVE) {
        // ENHANCEMENT: Double check the default backend is valid
        if (manager->default_backend->generator == NULL) {
            DEBUG_PRINT("ERROR: Default backend has NULL generator");
            return NULL;
        }
        
        DEBUG_PRINT("Using default backend for unsupported architecture");
        codegen_warning(manager->error_handler, "Using default backend for unsupported architecture");
        return manager->default_backend->generator;
    }
    
    DEBUG_PRINT("ERROR: No suitable backend found");
    return NULL;
}

// Mark a generator as destroyed - called after a generator's destroy function completes
void braggi_codegen_manager_mark_generator_destroyed(CodeGenerator* generator) {
    if (!manager || !generator) {
        return;
    }
    
    DEBUG_PRINT("Marking generator as destroyed: %p", (void*)generator);
    
    // ENHANCEMENT 1: Find and clean up any ECS entities that reference this generator
    if (manager->ecs_world) {
        DEBUG_PRINT("Checking ECS world for entities referencing generator %p", (void*)generator);
        
        // IMPROVEMENT: First, check for validation components
        ComponentTypeID validation_comp_type = 
            braggi_ecs_get_generator_validation_component_type(manager->ecs_world);
            
        if (validation_comp_type != INVALID_COMPONENT_TYPE) {
            // Create a component mask for our query
            ComponentMask mask = 0;
            braggi_ecs_mask_set(&mask, validation_comp_type);
            
            // Query for entities with validation components
            EntityQuery query = braggi_ecs_query_entities(manager->ecs_world, mask);
            EntityID entity;
            
            DEBUG_PRINT("Searching for ECS entities with validation components for destroyed generator");
            
            // Process each entity
            while (braggi_ecs_query_next(&query, &entity)) {
                // Get the validation component
                GeneratorValidationComponent* validation_comp = braggi_ecs_get_component(
                    manager->ecs_world, entity, validation_comp_type);
                
                if (validation_comp && validation_comp->generator == generator) {
                    DEBUG_PRINT("Found validation component for entity %u referencing destroyed generator", entity);
                    
                    // Update validation status to mark generator as invalid
                    validation_comp->is_valid = false;
                    validation_comp->validated = true;
                    validation_comp->validation_error = "Generator has been destroyed";
                    validation_comp->generator = NULL; // Clear the pointer
                }
            }
        }
        
        // Also check for codegen components as before
        // Get the codegen component type ID if it's registered
        ComponentTypeID codegen_component_type = 
            braggi_ecs_get_component_type_by_name(manager->ecs_world, "CodegenComponent");
        
        if (codegen_component_type != INVALID_COMPONENT_TYPE) {
            // Create a component mask for our query
            ComponentMask mask = 0;
            braggi_ecs_mask_set(&mask, codegen_component_type);
            
            // Query for entities with codegen components
            EntityQuery query = braggi_ecs_query_entities(manager->ecs_world, mask);
            EntityID entity;
            
            DEBUG_PRINT("Searching for ECS entities with references to destroyed generator");
            
            // Process each entity
            while (braggi_ecs_query_next(&query, &entity)) {
                // Get the component
                void* component = braggi_ecs_get_component(
                    manager->ecs_world, entity, codegen_component_type);
                
                if (component) {
                    // The actual component structure will depend on your ECS implementation
                    // This assumes a basic structure with a generator field
                    // Cast to appropriate type based on your actual component definition
                    CodegenComponent* codegen_comp = (CodegenComponent*)component;
                    
                    // Check if this component references our generator
                    if (codegen_comp->generator == generator) {
                        DEBUG_PRINT("Found entity %u with reference to destroyed generator", entity);
                        
                        // ENHANCEMENT 2: Nullify the reference to prevent use-after-free
                        DEBUG_PRINT("Nullifying generator reference in entity %u", entity);
                        codegen_comp->generator = NULL;
                        
                        // ADDITIONAL IMPROVEMENT: Add a validation component if it doesn't exist
                        if (validation_comp_type != INVALID_COMPONENT_TYPE && 
                            !braggi_ecs_has_component(manager->ecs_world, entity, validation_comp_type)) {
                            
                            GeneratorValidationComponent* validation_comp = braggi_ecs_add_component(
                                manager->ecs_world, entity, validation_comp_type);
                                
                            if (validation_comp) {
                                validation_comp->generator = NULL;
                                validation_comp->validated = true;
                                validation_comp->is_valid = false;
                                validation_comp->validation_magic = 0xB4C0DE47; // "BCODE G"
                                validation_comp->last_validated = time(NULL);
                                validation_comp->validation_error = "Generator was destroyed";
                            }
                        }
                    }
                }
            }
        }

        // BUGFIX: Also check and update CodeGenContextComponent references 
        ComponentTypeID context_comp_type = 
            braggi_ecs_get_codegen_context_component_type(manager->ecs_world);
        
        if (context_comp_type != INVALID_COMPONENT_TYPE) {
            // Create a component mask for our query
            ComponentMask mask = 0;
            braggi_ecs_mask_set(&mask, context_comp_type);
            
            // Query for entities with context components
            EntityQuery query = braggi_ecs_query_entities(manager->ecs_world, mask);
            EntityID entity;
            
            DEBUG_PRINT("Searching for CodeGenContextComponents referencing destroyed generator");
            
            // Process each entity
            while (braggi_ecs_query_next(&query, &entity)) {
                // Get the component
                CodeGenContextComponent* ctx_comp = braggi_ecs_get_component(
                    manager->ecs_world, entity, context_comp_type);
                
                if (ctx_comp && ctx_comp->ctx && ctx_comp->ctx->generator == generator) {
                    DEBUG_PRINT("Found CodeGenContextComponent for entity %u referencing destroyed generator", entity);
                    
                    // Clear the generator reference to prevent use-after-free
                    ctx_comp->ctx->generator = NULL;
                    
                    // ENHANCEMENT: Set a flag in the BraggiContext to indicate a generator is being destroyed
                    if (ctx_comp->ctx->braggi_ctx) {
                        DEBUG_PRINT("Setting generator cleanup flag in BraggiContext");
                        ctx_comp->ctx->braggi_ctx->flags |= BRAGGI_FLAG_CODEGEN_CLEANUP_IN_PROGRESS;
                    }
                }
            }
        }
    }
    
    // Find the generator in our list
    for (int i = 0; i < manager->num_backends; i++) {
        if (manager->backends[i].generator == generator) {
            DEBUG_PRINT("Found generator at index %d, marking as destroyed", i);
            
            // ENHANCEMENT 3: Add verification to catch any system attempting to access
            // Store the generator pointer in a verification list to check for illegal accesses
            // This would require adding a verification mechanism - for now we're just marking it
            manager->backends[i].status = GENERATOR_STATUS_DESTROYED;
            
            // If this was the default backend, clear that reference
            if (manager->default_backend && manager->default_backend->generator == generator) {
                DEBUG_PRINT("Clearing default backend reference");
                manager->default_backend = NULL;
                
                // Try to find a new default backend
                for (int j = 0; j < manager->num_backends; j++) {
                    if (manager->backends[j].status == GENERATOR_STATUS_ACTIVE) {
                        DEBUG_PRINT("Setting new default backend to index %d", j);
                        manager->default_backend = &manager->backends[j];
                        break;
                    }
                }
            }
            
            break;
        }
    }
    
    // BUGFIX: Instead of running a full ECS update, run only the validation system
    // This is safer as it avoids triggering code generation systems that might 
    // try to access the destroyed generator
    if (manager->ecs_world) {
        DEBUG_PRINT("Running validation system to synchronize state after generator destruction");
        
        // Get the validation system if it exists
        System* validation_system = braggi_ecs_get_system_by_name(manager->ecs_world, "GeneratorValidationSystem");
        
        if (validation_system) {
            // Run only the validation system, which should be safe
            validation_system->update_func(manager->ecs_world, validation_system, 0.0f);
            DEBUG_PRINT("Validation system update complete");
        } else {
            DEBUG_PRINT("WARNING: Validation system not found, skipping update");
        }
    }
}

// Generate code using the appropriate backend
bool braggi_codegen_manager_generate(CodeGenContext* ctx) {
    if (!manager || !ctx || !ctx->braggi_ctx) {
        DEBUG_PRINT("ERROR: Invalid parameters for code generation");
        return false;
    }
    
    // Get the appropriate backend
    CodeGenerator* backend = braggi_codegen_manager_get_backend(ctx->options.arch);
    if (!backend) {
        DEBUG_PRINT("ERROR: No suitable code generator backend available");
        codegen_error(manager->error_handler, "No suitable code generator backend available");
        return false;
    }
    
    // ENHANCEMENT: Verify the backend isn't already destroyed
    // Check if this generator exists in our backends list and its status
    bool generator_valid = false;
    for (int i = 0; i < manager->num_backends; i++) {
        if (manager->backends[i].generator == backend) {
            if (manager->backends[i].status != GENERATOR_STATUS_ACTIVE) {
                DEBUG_PRINT("ERROR: Attempting to use a destroyed or inactive generator: %p", (void*)backend);
                codegen_error(manager->error_handler, "Attempting to use a destroyed generator");
                return false;
            }
            generator_valid = true;
            break;
        }
    }
    
    if (!generator_valid) {
        DEBUG_PRINT("ERROR: Generator not found in manager's backend list: %p", (void*)backend);
        codegen_error(manager->error_handler, "Generator not found in backend list");
        return false;
    }
    
    // Extract the entropy field from the Braggi context
    EntropyField* field = ctx->options.entropy_field;
    if (!field && ctx->braggi_ctx->entropy_field) {
        field = ctx->braggi_ctx->entropy_field;
        ctx->options.entropy_field = field;
    }
    
    if (!field) {
        DEBUG_PRINT("ERROR: No entropy field available for code generation");
        codegen_error(manager->error_handler, "No entropy field available for code generation");
        return false;
    }
    
    // Store the backend in the context
    ctx->generator = backend;
    
    // Generate code
    DEBUG_PRINT("Generating code with backend: %s", backend->name ? backend->name : "(unnamed)");
    if (!backend->generate) {
        DEBUG_PRINT("ERROR: Backend has no generate function");
        codegen_error(manager->error_handler, "Backend has no generate function");
        return false;
    }
    
    if (!backend->generate(backend, field)) {
        DEBUG_PRINT("ERROR: Failed to generate code");
        codegen_error(manager->error_handler, "Failed to generate code");
        return false;
    }
    
    // Emit the generated code
    const char* output_file = ctx->options.output_file ? ctx->options.output_file : "output.s";
    DEBUG_PRINT("Emitting code to: %s", output_file);
    
    if (!backend->emit) {
        DEBUG_PRINT("ERROR: Backend has no emit function");
        codegen_error(manager->error_handler, "Backend has no emit function");
        return false;
    }
    
    if (!backend->emit(backend, output_file, ctx->options.format)) {
        DEBUG_PRINT("ERROR: Failed to emit generated code");
        codegen_error(manager->error_handler, "Failed to emit generated code");
        return false;
    }
    
    DEBUG_PRINT("Code generation completed successfully");
    return true;
}

// Generate code using ECS
bool braggi_codegen_manager_generate_ecs(BraggiContext* braggi_ctx, TargetArch arch, const char* output_file) {
    if (!manager || !braggi_ctx) {
        DEBUG_PRINT("ERROR: Invalid parameters for ECS code generation");
        return false;
    }
    
    // Initialize the manager if needed
    if (!manager->ecs_world) {
        DEBUG_PRINT("ERROR: ECS world not initialized");
        codegen_error(manager->error_handler, "ECS world not initialized");
        return false;
    }
    
    // Extract entropy field from Braggi context
    EntropyField* field = (EntropyField*)braggi_ctx->entropy_field;
    if (!field) {
        DEBUG_PRINT("ERROR: No entropy field available for code generation");
        codegen_error(manager->error_handler, "No entropy field available for code generation");
        return false;
    }
    
    // IMPROVEMENT: Get the generator for validation
    CodeGenerator* backend = braggi_codegen_manager_get_backend(arch);
    if (!backend) {
        DEBUG_PRINT("ERROR: No suitable backend found for architecture %s", 
                   braggi_codegen_arch_to_string(arch));
        codegen_error(manager->error_handler, "No suitable backend found");
        return false;
    }
    
    // Create a code generation entity
    DEBUG_PRINT("Creating code generation entity");
    EntityID entity = braggi_ecs_create_codegen_entity(manager->ecs_world, arch, field);
    if (entity == INVALID_ENTITY) {
        DEBUG_PRINT("ERROR: Failed to create code generation entity");
        codegen_error(manager->error_handler, "Failed to create code generation entity");
        return false;
    }
    
    // IMPROVEMENT: Add validation component and validate immediately
    ComponentTypeID validation_comp_type = 
        braggi_ecs_get_generator_validation_component_type(manager->ecs_world);
    
    if (validation_comp_type != INVALID_COMPONENT_TYPE) {
        DEBUG_PRINT("Validating backend before code generation");
        if (!braggi_ecs_validate_generator(manager->ecs_world, entity, backend)) {
            // Get the validation component to check error
            GeneratorValidationComponent* validation_comp = 
                braggi_ecs_get_component(manager->ecs_world, entity, validation_comp_type);
                
            if (validation_comp) {
                DEBUG_PRINT("ERROR: Generator validation failed: %s", 
                           validation_comp->validation_error ? validation_comp->validation_error : "Unknown error");
                codegen_error(manager->error_handler, validation_comp->validation_error ? 
                              validation_comp->validation_error : "Generator validation failed");
            } else {
                DEBUG_PRINT("ERROR: Generator validation failed with no error details");
                codegen_error(manager->error_handler, "Generator validation failed");
            }
            
            // Clean up the entity since we won't use it
            braggi_ecs_destroy_entity(manager->ecs_world, entity);
            return false;
        }
        
        DEBUG_PRINT("Backend validated successfully");
    }
    
    // If an output file was specified, set it in the context component
    if (output_file) {
        ComponentTypeID codegen_context_component_type = 
            braggi_ecs_get_codegen_context_component_type(manager->ecs_world);
        
        // Context component might not exist yet - it will be created by the system
        if (codegen_context_component_type != INVALID_COMPONENT_TYPE) {
            // Create the component if it doesn't exist
            if (!braggi_ecs_has_component(manager->ecs_world, entity, codegen_context_component_type)) {
                // The system will create the component, but we can create it here to set the output file
                CodeGenContextComponent* ctx_comp = braggi_ecs_add_component(
                    manager->ecs_world, entity, codegen_context_component_type);
                
                if (ctx_comp) {
                    ctx_comp->output_file = strdup(output_file);
                }
            } else {
                // Update existing component
                CodeGenContextComponent* ctx_comp = braggi_ecs_get_component(
                    manager->ecs_world, entity, codegen_context_component_type);
                
                if (ctx_comp) {
                    if (ctx_comp->output_file) {
                        free(ctx_comp->output_file);
                    }
                    ctx_comp->output_file = strdup(output_file);
                }
            }
        }
    }
    
    // Process the entity through all code generation systems
    DEBUG_PRINT("Processing code generation entity");
    bool success = braggi_ecs_process_codegen_entity(manager->ecs_world, entity);
    
    if (!success) {
        DEBUG_PRINT("ERROR: Failed to process code generation entity");
        codegen_error(manager->error_handler, "Failed to process code generation entity");
    }
    
    return success;
}

// Apply optimization
bool braggi_codegen_manager_optimize(CodeGenContext* ctx) {
    if (!manager || !ctx) {
        DEBUG_PRINT("ERROR: Invalid parameters for optimization");
        return false;
    }
    
    // Get the appropriate backend
    CodeGenerator* backend = braggi_codegen_manager_get_backend(ctx->options.arch);
    if (!backend) {
        DEBUG_PRINT("ERROR: No suitable code generator backend available");
        codegen_error(manager->error_handler, "No suitable code generator backend available");
        return false;
    }
    
    // Check if optimization is supported and enabled
    if (!backend->optimize) {
        DEBUG_PRINT("WARNING: Optimization not supported by backend");
        codegen_warning(manager->error_handler, "Optimization not supported by backend");
        return true;  // Not an error, just not supported
    }
    
    // Apply optimization if enabled
    if (ctx->options.optimize) {
        DEBUG_PRINT("Applying optimization at level %d", ctx->options.optimization_level);
        if (!backend->optimize(backend, ctx->options.optimization_level)) {
            DEBUG_PRINT("ERROR: Failed to apply optimization");
            codegen_error(manager->error_handler, "Failed to apply optimization");
            return false;
        }
    }
    
    return true;
}

// Set debug information generation
bool braggi_codegen_manager_set_debug_info(CodeGenContext* ctx, bool enable) {
    if (!manager || !ctx) {
        DEBUG_PRINT("ERROR: Invalid parameters for debug info");
        return false;
    }
    
    // Get the appropriate backend
    CodeGenerator* backend = braggi_codegen_manager_get_backend(ctx->options.arch);
    if (!backend) {
        DEBUG_PRINT("ERROR: No suitable code generator backend available");
        codegen_error(manager->error_handler, "No suitable code generator backend available");
        return false;
    }
    
    // Check if debug info generation is supported
    if (!backend->generate_debug_info) {
        DEBUG_PRINT("WARNING: Debug info generation not supported by backend");
        codegen_warning(manager->error_handler, "Debug info generation not supported by backend");
        return true;  // Not an error, just not supported
    }
    
    // Set debug info generation
    DEBUG_PRINT("Setting debug info generation: %s", enable ? "enabled" : "disabled");
    if (!backend->generate_debug_info(backend, enable)) {
        DEBUG_PRINT("ERROR: Failed to set debug info generation");
        codegen_error(manager->error_handler, "Failed to set debug info generation");
        return false;
    }
    
    return true;
}

// Error handler implementation
static void codegen_error(ErrorHandler* handler, const char* message) {
    fprintf(stderr, "ERROR: %s\n", message);
}

// Warning handler implementation
static void codegen_warning(ErrorHandler* handler, const char* message) {
    fprintf(stderr, "WARNING: %s\n", message);
}

// Update the braggi_codegen_generate function to use the manager
bool braggi_codegen_manager_generate_wrapper(CodeGenContext* ctx) {
    // Initialize the manager if needed
    if (!manager && !braggi_codegen_manager_init()) {
        fprintf(stderr, "ERROR: Failed to initialize code generation manager\n");
        return false;
    }
    
    // Delegate to the manager
    return braggi_codegen_manager_generate(ctx);
}

// Generate code using the ECS system
bool braggi_codegen_generate_ecs(BraggiContext* braggi_ctx, TargetArch arch, const char* output_file) {
    // Initialize the manager if needed
    if (!manager && !braggi_codegen_manager_init()) {
        fprintf(stderr, "ERROR: Failed to initialize code generation manager\n");
        return false;
    }
    
    // Delegate to the manager's ECS-based code generation
    return braggi_codegen_manager_generate_ecs(braggi_ctx, arch, output_file);
} 