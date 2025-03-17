/*
 * Braggi - Code Generation ECS Components Implementation
 * 
 * "Good components are like good cattle - they do their job without much fuss,
 * and they're easy to brand and track!" - Texas Component Rustler
 */

#include "braggi/ecs/codegen_components.h"
#include "braggi/codegen.h"
#include "braggi/entropy.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>  // Add this for time() function

// Component type IDs - initialized when registered
static ComponentTypeID TARGET_ARCH_COMPONENT_TYPE = INVALID_COMPONENT_TYPE;
static ComponentTypeID CODEGEN_CONTEXT_COMPONENT_TYPE = INVALID_COMPONENT_TYPE;
static ComponentTypeID ENTROPY_FIELD_COMPONENT_TYPE = INVALID_COMPONENT_TYPE;
static ComponentTypeID BACKEND_COMPONENT_TYPE = INVALID_COMPONENT_TYPE;
static ComponentTypeID CODE_BLOB_COMPONENT_TYPE = INVALID_COMPONENT_TYPE;
static ComponentTypeID CODEGEN_COMPONENT_TYPE = INVALID_COMPONENT_TYPE;

// Add the validation component type
static ComponentTypeID generator_validation_component_type = INVALID_COMPONENT_TYPE;

// Magic number for validation
#define GENERATOR_VALIDATION_MAGIC 0xB4C0DE47 // "BCODE G"

// Component constructors and destructors
static void target_arch_component_init(void* component) {
    TargetArchComponent* comp = (TargetArchComponent*)component;
    
    // Set default values
    comp->arch = ARCH_X86_64;  // Default to x86_64
    comp->format = FORMAT_EXECUTABLE;
    comp->optimize = false;
    comp->optimization_level = 0;
    comp->emit_debug_info = true;
}

static void codegen_context_component_init(void* component) {
    CodeGenContextComponent* comp = (CodeGenContextComponent*)component;
    
    comp->ctx = NULL;
    comp->output_file = NULL;
}

static void codegen_context_component_destroy(void* component) {
    CodeGenContextComponent* comp = (CodeGenContextComponent*)component;
    
    // Don't free ctx here - it's owned by another system
    
    // Free the output file string if we own it
    if (comp->output_file) {
        free(comp->output_file);
        comp->output_file = NULL;
    }
}

static void entropy_field_component_init(void* component) {
    EntropyFieldComponent* comp = (EntropyFieldComponent*)component;
    
    comp->field = NULL;
    comp->ready_for_codegen = false;
}

static void backend_component_init(void* component) {
    BackendComponent* comp = (BackendComponent*)component;
    
    comp->backend_name = NULL;
    comp->backend_data = NULL;
    comp->initialized = false;
}

static void code_blob_component_init(void* component) {
    CodeBlobComponent* comp = (CodeBlobComponent*)component;
    
    comp->data = NULL;
    comp->size = 0;
    comp->is_binary = false;
}

static void code_blob_component_destroy(void* component) {
    CodeBlobComponent* comp = (CodeBlobComponent*)component;
    
    if (comp->data) {
        free(comp->data);
        comp->data = NULL;
        comp->size = 0;
    }
}

// Add constructor for the CodegenComponent
static void codegen_component_init(void* component) {
    CodegenComponent* comp = (CodegenComponent*)component;
    
    comp->generator = NULL;
    comp->initialized = false;
    comp->generator_id = 0;
}

// Register all code generation component types
void braggi_ecs_register_codegen_components(ECSWorld* world) {
    if (!world) return;
    
    braggi_ecs_register_target_arch_component(world);
    braggi_ecs_register_codegen_context_component(world);
    braggi_ecs_register_entropy_field_component(world);
    braggi_ecs_register_backend_component(world);
    braggi_ecs_register_code_blob_component(world);
    
    // Register our new component types
    braggi_ecs_register_generator_validation_component(world);
    braggi_ecs_register_codegen_component(world);
}

// Register individual component types
ComponentTypeID braggi_ecs_register_target_arch_component(ECSWorld* world) {
    if (!world) return INVALID_COMPONENT_TYPE;
    
    // Only register if not already registered
    if (TARGET_ARCH_COMPONENT_TYPE == INVALID_COMPONENT_TYPE) {
        ComponentTypeInfo info = {
            .name = "TargetArchComponent",
            .size = sizeof(TargetArchComponent),
            .constructor = target_arch_component_init,
            .destructor = NULL  // No destructor needed for simple data
        };
        
        TARGET_ARCH_COMPONENT_TYPE = braggi_ecs_register_component_type(world, &info);
    }
    
    return TARGET_ARCH_COMPONENT_TYPE;
}

ComponentTypeID braggi_ecs_register_codegen_context_component(ECSWorld* world) {
    if (!world) return INVALID_COMPONENT_TYPE;
    
    // Only register if not already registered
    if (CODEGEN_CONTEXT_COMPONENT_TYPE == INVALID_COMPONENT_TYPE) {
        ComponentTypeInfo info = {
            .name = "CodeGenContextComponent",
            .size = sizeof(CodeGenContextComponent),
            .constructor = codegen_context_component_init,
            .destructor = codegen_context_component_destroy
        };
        
        CODEGEN_CONTEXT_COMPONENT_TYPE = braggi_ecs_register_component_type(world, &info);
    }
    
    return CODEGEN_CONTEXT_COMPONENT_TYPE;
}

ComponentTypeID braggi_ecs_register_entropy_field_component(ECSWorld* world) {
    if (!world) return INVALID_COMPONENT_TYPE;
    
    // Only register if not already registered
    if (ENTROPY_FIELD_COMPONENT_TYPE == INVALID_COMPONENT_TYPE) {
        ComponentTypeInfo info = {
            .name = "EntropyFieldComponent",
            .size = sizeof(EntropyFieldComponent),
            .constructor = entropy_field_component_init,
            .destructor = NULL  // We don't own the field, so no destructor
        };
        
        ENTROPY_FIELD_COMPONENT_TYPE = braggi_ecs_register_component_type(world, &info);
    }
    
    return ENTROPY_FIELD_COMPONENT_TYPE;
}

ComponentTypeID braggi_ecs_register_backend_component(ECSWorld* world) {
    if (!world) return INVALID_COMPONENT_TYPE;
    
    // Only register if not already registered
    if (BACKEND_COMPONENT_TYPE == INVALID_COMPONENT_TYPE) {
        ComponentTypeInfo info = {
            .name = "BackendComponent",
            .size = sizeof(BackendComponent),
            .constructor = backend_component_init,
            .destructor = NULL  // We don't own the backend, so no destructor
        };
        
        BACKEND_COMPONENT_TYPE = braggi_ecs_register_component_type(world, &info);
    }
    
    return BACKEND_COMPONENT_TYPE;
}

ComponentTypeID braggi_ecs_register_code_blob_component(ECSWorld* world) {
    if (!world) return INVALID_COMPONENT_TYPE;
    
    // Only register if not already registered
    if (CODE_BLOB_COMPONENT_TYPE == INVALID_COMPONENT_TYPE) {
        ComponentTypeInfo info = {
            .name = "CodeBlobComponent",
            .size = sizeof(CodeBlobComponent),
            .constructor = code_blob_component_init,
            .destructor = code_blob_component_destroy
        };
        
        CODE_BLOB_COMPONENT_TYPE = braggi_ecs_register_component_type(world, &info);
    }
    
    return CODE_BLOB_COMPONENT_TYPE;
}

// Register the generator validation component
ComponentTypeID braggi_ecs_register_generator_validation_component(ECSWorld* world) {
    if (!world) return INVALID_COMPONENT_TYPE;
    
    // Only register once
    if (generator_validation_component_type != INVALID_COMPONENT_TYPE) {
        return generator_validation_component_type;
    }
    
    ComponentTypeInfo info = {
        .name = "GeneratorValidationComponent",
        .size = sizeof(GeneratorValidationComponent),
        .constructor = NULL,
        .destructor = NULL  // We'll handle cleanup manually
    };
    
    generator_validation_component_type = braggi_ecs_register_component_type(world, &info);
    return generator_validation_component_type;
}

// Register the CodegenComponent
ComponentTypeID braggi_ecs_register_codegen_component(ECSWorld* world) {
    if (!world) return INVALID_COMPONENT_TYPE;
    
    if (CODEGEN_COMPONENT_TYPE != INVALID_COMPONENT_TYPE) {
        return CODEGEN_COMPONENT_TYPE;
    }
    
    ComponentTypeInfo info = {
        .name = "CodegenComponent",
        .size = sizeof(CodegenComponent),
        .constructor = codegen_component_init,
        .destructor = NULL
    };
    
    CODEGEN_COMPONENT_TYPE = braggi_ecs_register_component_type(world, &info);
    return CODEGEN_COMPONENT_TYPE;
}

// Get component type IDs
ComponentTypeID braggi_ecs_get_target_arch_component_type(ECSWorld* world) {
    if (TARGET_ARCH_COMPONENT_TYPE == INVALID_COMPONENT_TYPE) {
        TARGET_ARCH_COMPONENT_TYPE = braggi_ecs_get_component_type_by_name(world, "TargetArchComponent");
    }
    return TARGET_ARCH_COMPONENT_TYPE;
}

ComponentTypeID braggi_ecs_get_codegen_context_component_type(ECSWorld* world) {
    if (CODEGEN_CONTEXT_COMPONENT_TYPE == INVALID_COMPONENT_TYPE) {
        CODEGEN_CONTEXT_COMPONENT_TYPE = braggi_ecs_get_component_type_by_name(world, "CodeGenContextComponent");
    }
    return CODEGEN_CONTEXT_COMPONENT_TYPE;
}

ComponentTypeID braggi_ecs_get_entropy_field_component_type(ECSWorld* world) {
    if (ENTROPY_FIELD_COMPONENT_TYPE == INVALID_COMPONENT_TYPE) {
        ENTROPY_FIELD_COMPONENT_TYPE = braggi_ecs_get_component_type_by_name(world, "EntropyFieldComponent");
    }
    return ENTROPY_FIELD_COMPONENT_TYPE;
}

ComponentTypeID braggi_ecs_get_backend_component_type(ECSWorld* world) {
    if (BACKEND_COMPONENT_TYPE == INVALID_COMPONENT_TYPE) {
        BACKEND_COMPONENT_TYPE = braggi_ecs_get_component_type_by_name(world, "BackendComponent");
    }
    return BACKEND_COMPONENT_TYPE;
}

ComponentTypeID braggi_ecs_get_code_blob_component_type(ECSWorld* world) {
    if (CODE_BLOB_COMPONENT_TYPE == INVALID_COMPONENT_TYPE) {
        CODE_BLOB_COMPONENT_TYPE = braggi_ecs_get_component_type_by_name(world, "CodeBlobComponent");
    }
    return CODE_BLOB_COMPONENT_TYPE;
}

// Get the component type ID
ComponentTypeID braggi_ecs_get_generator_validation_component_type(ECSWorld* world) {
    return generator_validation_component_type;
}

// Get the CodegenComponent type ID
ComponentTypeID braggi_ecs_get_codegen_component_type(ECSWorld* world) {
    return CODEGEN_COMPONENT_TYPE;
}

// Create a code generation entity with basic components
EntityID braggi_ecs_create_codegen_entity(ECSWorld* world, TargetArch arch, EntropyField* field) {
    if (!world || !field) {
        return INVALID_ENTITY;
    }
    
    // Ensure all component types are registered
    braggi_ecs_register_codegen_components(world);
    
    // Create a new entity
    EntityID entity = braggi_ecs_create_entity(world);
    if (entity == INVALID_ENTITY) {
        return INVALID_ENTITY;
    }
    
    // Add target architecture component
    TargetArchComponent* target_comp = braggi_ecs_add_component(world, entity, TARGET_ARCH_COMPONENT_TYPE);
    if (target_comp) {
        target_comp->arch = arch;
        target_comp->format = FORMAT_EXECUTABLE;  // Default to executable
        target_comp->optimize = false;            // Default to no optimization
        target_comp->optimization_level = 0;
        target_comp->emit_debug_info = true;      // Default to including debug info
    }
    
    // Add entropy field component
    EntropyFieldComponent* field_comp = braggi_ecs_add_component(world, entity, ENTROPY_FIELD_COMPONENT_TYPE);
    if (field_comp) {
        field_comp->field = field;
        field_comp->ready_for_codegen = true;     // Assume it's ready
    }
    
    // Add code blob component (initially empty)
    braggi_ecs_add_component(world, entity, CODE_BLOB_COMPONENT_TYPE);
    
    return entity;
}

// Validate a generator and store results in the component
bool braggi_ecs_validate_generator(ECSWorld* world, EntityID entity, CodeGenerator* generator) {
    if (!world || entity == INVALID_ENTITY || !generator) {
        return false;
    }
    
    // Get or create the validation component
    ComponentTypeID comp_type = braggi_ecs_get_generator_validation_component_type(world);
    if (comp_type == INVALID_COMPONENT_TYPE) {
        return false;
    }
    
    GeneratorValidationComponent* comp;
    if (braggi_ecs_has_component(world, entity, comp_type)) {
        comp = braggi_ecs_get_component(world, entity, comp_type);
    } else {
        comp = braggi_ecs_add_component(world, entity, comp_type);
    }
    
    if (!comp) {
        return false;
    }
    
    // Initialize the component
    comp->generator = generator;
    comp->validated = false;
    comp->is_valid = false;
    comp->validation_magic = 0;
    comp->last_validated = 0;
    comp->validation_error = NULL;
    
    // Perform validation checks
    
    // Check 1: Generator is not NULL
    if (!generator) {
        comp->validation_error = "Generator is NULL";
        comp->validated = true;
        return false;
    }
    
    // Check 2: Generator has required function pointers
    if (!generator->generate) {
        comp->validation_error = "Generator missing generate function";
        comp->validated = true;
        return false;
    }
    
    if (!generator->emit) {
        comp->validation_error = "Generator missing emit function";
        comp->validated = true;
        return false;
    }
    
    // Check 3: Generator has a valid name
    if (!generator->name) {
        comp->validation_error = "Generator has no name";
        comp->validated = true;
        return false;
    }
    
    // Check 4: Generator's arch_data is not NULL (unless it's a special case)
    if (!generator->arch_data) {
        comp->validation_error = "Generator has no architecture data";
        comp->validated = true;
        return false;
    }
    
    // All validation checks passed
    comp->validated = true;
    comp->is_valid = true;
    comp->validation_magic = GENERATOR_VALIDATION_MAGIC;
    comp->last_validated = time(NULL); // Current timestamp
    
    return true;
}

// Check if a generator has been validated and is valid
bool braggi_ecs_is_generator_valid(ECSWorld* world, EntityID entity) {
    if (!world || entity == INVALID_ENTITY) {
        return false;
    }
    
    ComponentTypeID comp_type = braggi_ecs_get_generator_validation_component_type(world);
    if (comp_type == INVALID_COMPONENT_TYPE) {
        return false;
    }
    
    // If no validation component, not valid
    if (!braggi_ecs_has_component(world, entity, comp_type)) {
        return false;
    }
    
    GeneratorValidationComponent* comp = braggi_ecs_get_component(world, entity, comp_type);
    if (!comp) {
        return false;
    }
    
    // Check if the validation has been performed and magic matches
    if (!comp->validated || comp->validation_magic != GENERATOR_VALIDATION_MAGIC) {
        return false;
    }
    
    return comp->is_valid;
} 