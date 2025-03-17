/*
 * Braggi - Code Generation ECS Components
 * 
 * "Components are like cattle brands - they mark what your entities are capable of,
 * and help the systems know which critters to wrangle!" - Texas ECS Rancher
 */

#ifndef BRAGGI_ECS_CODEGEN_COMPONENTS_H
#define BRAGGI_ECS_CODEGEN_COMPONENTS_H

#include "braggi/codegen.h"
#include "braggi/codegen_arch.h"
#include "braggi/ecs.h"
#include <stdbool.h>

// Forward declarations
typedef struct EntropyField EntropyField;

/*
 * TargetArchComponent - Defines which architecture this entity should target
 */
typedef struct {
    TargetArch arch;             // Target architecture (x86_64, ARM, etc.)
    OutputFormat format;         // Output format (object, executable, etc.)
    bool optimize;               // Enable optimizations
    int optimization_level;      // Optimization level (0-3)
    bool emit_debug_info;        // Include debug information
} TargetArchComponent;

/*
 * CodeGenContextComponent - Contains the CodeGenContext for an entity
 */
typedef struct {
    CodeGenContext* ctx;        // Code generation context
    char* output_file;          // Output file name
} CodeGenContextComponent;

/*
 * EntropyFieldComponent - References an entropy field for code generation
 */
typedef struct {
    EntropyField* field;        // Entropy field representing the code structure
    bool ready_for_codegen;     // Flag indicating if the field is ready for code generation
} EntropyFieldComponent;

/*
 * BackendComponent - References a specific architecture backend
 */
typedef struct {
    const char* backend_name;   // Name of the backend (e.g., "x86_64", "arm")
    void* backend_data;         // Backend-specific data
    bool initialized;           // Flag indicating if the backend is initialized
} BackendComponent;

/*
 * CodegenComponent - Contains the code generator instance
 */
typedef struct {
    CodeGenerator* generator;   // The code generator instance
    bool initialized;           // Flag indicating if the generator is initialized
    uint32_t generator_id;      // Unique ID for this generator instance
} CodegenComponent;

/*
 * CodeBlobComponent - Holds generated code
 */
typedef struct {
    void* data;                 // Generated code or assembly text
    size_t size;                // Size of the data
    bool is_binary;             // Flag indicating if the data is binary or text
} CodeBlobComponent;

// NEW COMPONENT: Generator validation component
typedef struct {
    CodeGenerator* generator;    // The generator being validated
    bool validated;              // Whether validation has been performed
    bool is_valid;               // Result of validation
    uint32_t validation_magic;   // Magic number to verify validation
    uint64_t last_validated;     // Timestamp of last validation
    const char* validation_error; // Error message if validation failed
} GeneratorValidationComponent;

/*
 * Component type registration
 */

// Register all code generation component types
void braggi_ecs_register_codegen_components(ECSWorld* world);

// Register individual component types
ComponentTypeID braggi_ecs_register_target_arch_component(ECSWorld* world);
ComponentTypeID braggi_ecs_register_codegen_context_component(ECSWorld* world);
ComponentTypeID braggi_ecs_register_entropy_field_component(ECSWorld* world);
ComponentTypeID braggi_ecs_register_backend_component(ECSWorld* world);
ComponentTypeID braggi_ecs_register_code_blob_component(ECSWorld* world);
ComponentTypeID braggi_ecs_register_generator_validation_component(ECSWorld* world);
ComponentTypeID braggi_ecs_register_codegen_component(ECSWorld* world);

// Get component type IDs
ComponentTypeID braggi_ecs_get_target_arch_component_type(ECSWorld* world);
ComponentTypeID braggi_ecs_get_codegen_context_component_type(ECSWorld* world);
ComponentTypeID braggi_ecs_get_entropy_field_component_type(ECSWorld* world);
ComponentTypeID braggi_ecs_get_backend_component_type(ECSWorld* world);
ComponentTypeID braggi_ecs_get_code_blob_component_type(ECSWorld* world);
ComponentTypeID braggi_ecs_get_generator_validation_component_type(ECSWorld* world);
ComponentTypeID braggi_ecs_get_codegen_component_type(ECSWorld* world);

// Create a code generation entity
EntityID braggi_ecs_create_codegen_entity(ECSWorld* world, TargetArch arch, EntropyField* field);

// NEW HELPERS: For generator validation
bool braggi_ecs_validate_generator(ECSWorld* world, EntityID entity, CodeGenerator* generator);
bool braggi_ecs_is_generator_valid(ECSWorld* world, EntityID entity);

#endif /* BRAGGI_ECS_CODEGEN_COMPONENTS_H */ 