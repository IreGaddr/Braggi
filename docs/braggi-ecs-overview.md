# Braggi ECS Integration Overview
## "Wrangling Code Like Cattle"

> "A good ECS is like a cattle drive - entities are the cattle, components are the brands, and systems are the cowboys doin' all the work!" - Texas ECS Wisdom

## Introduction

The Braggi compiler is built on a complete Entity Component System (ECS) architecture that spans from the frontend parsing to the backend code generation. This document provides a high-level overview of the ECS integration and practical examples of how to use it.

## What is an Entity Component System?

An Entity Component System is an architectural pattern that separates:

- **Entities**: Simple identifiers that represent objects in your system
- **Components**: Pure data structures that can be attached to entities
- **Systems**: Logic that processes entities based on their component combinations

This separation provides many benefits:
- Clear boundaries between data and behavior
- Highly parallelizable processing
- Compositional design that avoids deep inheritance hierarchies
- Cache-friendly memory layout for performance

## Braggi's ECS Architecture

Braggi uses the ECS pattern for the entire compilation pipeline:

```
Source Code → [Tokenization System] → Token Entities → [WFCCC Systems] → 
State Entities → [AST Systems] → AST Entities → [Code Generation Systems] → Output
```

Each phase consists of systems that process specific entity types and transform them into the next representation.

## Core ECS Components in Braggi

### Source Code Components

```c
// A token in the source code
typedef struct {
    Token token;                  // Token data
} TokenComponent;

// Source code position
typedef struct {
    SourcePosition position;      // Position in source code
} SourcePositionComponent;
```

### WFCCC Components

```c
// Entropy cell component
typedef struct {
    Cell* cell;                   // Reference to entropy cell
    float entropy;                // Current entropy value
} EntropyCellComponent;

// Constraint component
typedef struct {
    Constraint* constraint;       // Reference to constraint
    ConstraintType type;          // Type of constraint
    bool is_satisfied;            // Whether the constraint is satisfied
} ConstraintComponent;
```

### AST Components

```c
// AST node component
typedef struct {
    enum { /* AST node types */ } node_type;
    void* data;                   // Node-specific data
} ASTNodeComponent;

// Type component
typedef struct {
    enum { /* type kinds */ } base_type;
    size_t size;                  // Size in bytes
    void* type_data;              // Type-specific data
} TypeComponent;
```

### Region Components

```c
// Region component
typedef struct {
    char* name;                   // Region name
    RegimeType regime;            // Region regime
    EntityID* allocations;        // Entities representing allocations
    size_t allocation_count;      // Number of allocations
} RegionComponent;
```

### Code Generation Components

```c
// Target architecture component
typedef struct {
    TargetArch arch;             // Target architecture
    OutputFormat format;         // Output format
    bool optimize;               // Enable optimizations
    int optimization_level;      // Optimization level
} TargetArchComponent;

// Generated code component
typedef struct {
    void* data;                 // Generated code
    size_t size;                // Size of the data
    bool is_binary;             // Flag for binary or text
} CodeBlobComponent;
```

## Core ECS Systems in Braggi

### Tokenization Systems

```c
// Tokenize source code into entities
void braggi_system_tokenize(ECSWorld* world, void* context);
```

### WFCCC Systems

```c
// Initialize entropy cells
void braggi_system_init_entropy(ECSWorld* world, void* context);

// Create constraints between cells
void braggi_system_create_constraints(ECSWorld* world, void* context);

// Propagate constraints through the entropy field
void braggi_system_propagate_constraints(ECSWorld* world, void* context);

// Collapse entropy cells to single states
void braggi_system_collapse_entropy(ECSWorld* world, void* context);
```

### AST Systems

```c
// Build AST from collapsed cells
void braggi_system_build_ast(ECSWorld* world, void* context);

// Perform type checking
void braggi_system_type_check(ECSWorld* world, void* context);

// Analyze region usage
void braggi_system_analyze_regions(ECSWorld* world, void* context);
```

### Code Generation Systems

```c
// Generate code from the AST
void braggi_system_generate_code(ECSWorld* world, void* context);

// Allocate registers for code
void braggi_system_allocate_registers(ECSWorld* world, void* context);

// Output the final code
void braggi_system_generate_output(ECSWorld* world, void* context);
```

## Using the ECS API

### Creating an ECS World

```c
// Create a new ECS world
ECSWorld* world = braggi_ecs_world_create(1000, 50);

// Or with a memory region for improved performance
Region* region = braggi_region_create("compiler", REGION_MB(64));
ECSWorld* world = braggi_ecs_create_world_with_region(region, region, REGIME_SEQ);
```

### Registering Components

```c
// Register a basic component type
ComponentTypeID token_type = braggi_ecs_register_component(world, sizeof(TokenComponent));

// Register with constructor/destructor
ComponentTypeInfo info = {
    .name = "ASTNode",
    .size = sizeof(ASTNodeComponent),
    .constructor = ast_node_constructor,
    .destructor = ast_node_destructor
};
ComponentTypeID ast_type = braggi_ecs_register_component_type(world, &info);
```

### Working with Entities

```c
// Create a new entity
EntityID entity = braggi_ecs_create_entity(world);

// Add a component
TokenComponent* token = braggi_ecs_add_component(world, entity, token_type);
token->token = /* initialize token data */;

// Get a component
SourcePositionComponent* pos = braggi_ecs_get_component(world, entity, pos_type);

// Check if an entity has a component
if (braggi_ecs_has_component(world, entity, type_type)) {
    // Entity has a type component
}

// Remove a component
braggi_ecs_remove_component(world, entity, token_type);

// Destroy an entity
braggi_ecs_destroy_entity(world, entity);
```

### Creating and Using Systems

```c
// Create a simple system
System* tokenize_system = braggi_ecs_system_create(
    COMPONENT_MASK(source_type), // Required components
    tokenize_system_func,        // Update function
    context                      // User data
);

// Create a system with detailed info
SystemInfo info = {
    .name = "CodeGeneration",
    .update_func = codegen_system_func,
    .context = context,
    .priority = 10  // Higher priority runs earlier
};
System* codegen_system = braggi_ecs_create_system(&info);

// Register a system with the world
braggi_ecs_register_system(world, tokenize_system);

// Update a specific system
braggi_ecs_update_system(world, codegen_system, 0.0f);

// Update all systems
braggi_ecs_update(world, 0.0f);
```

### Querying Entities

```c
// Create a query for entities with specific components
ComponentMask mask = COMPONENT_MASK(ast_type) | COMPONENT_MASK(type_type);
EntityQuery query = braggi_ecs_query_entities(world, mask);

// Process matching entities
EntityID entity;
while (braggi_ecs_query_next(&query, &entity)) {
    ASTNodeComponent* node = braggi_ecs_get_component(world, entity, ast_type);
    TypeComponent* type = braggi_ecs_get_component(world, entity, type_type);
    
    // Process node and type
}

// Get all entities with specific components
Vector* entities = braggi_ecs_get_entities_with_components(world, mask);
```

## Examples

### Example 1: Custom Tokenization System

```c
void custom_tokenizer_system(ECSWorld* world, System* system, float delta_time) {
    // Get the source code entity
    ComponentMask source_mask = COMPONENT_MASK(source_component_type);
    EntityQuery query = braggi_ecs_query_entities(world, source_mask);
    
    EntityID source_entity;
    if (braggi_ecs_query_next(&query, &source_entity)) {
        SourceComponent* source = braggi_ecs_get_component(
            world, source_entity, source_component_type);
        
        // Tokenize the source code
        TokenizeResult result = tokenize_source(source->code);
        
        // Create entities for each token
        for (size_t i = 0; i < result.token_count; i++) {
            EntityID token_entity = braggi_ecs_create_entity(world);
            
            // Add token component
            TokenComponent* token = braggi_ecs_add_component(
                world, token_entity, token_component_type);
            token->token = result.tokens[i];
            
            // Add position component
            SourcePositionComponent* pos = braggi_ecs_add_component(
                world, token_entity, position_component_type);
            pos->position = result.positions[i];
        }
    }
}
```

### Example 2: Custom AST Building System

```c
void custom_ast_builder_system(ECSWorld* world, System* system, float delta_time) {
    // Get all collapsed state entities
    ComponentMask state_mask = COMPONENT_MASK(state_component_type) | 
                              COMPONENT_MASK(cell_component_type);
    Vector* state_entities = braggi_ecs_get_entities_with_components(world, state_mask);
    
    // Build the AST from collapsed states
    EntityID program_entity = braggi_ecs_create_entity(world);
    ASTNodeComponent* program = braggi_ecs_add_component(
        world, program_entity, ast_component_type);
    program->node_type = AST_PROGRAM;
    program->data = create_program_data();
    
    // Process tokens and build AST nodes
    for (size_t i = 0; i < state_entities->size; i++) {
        EntityID state_entity = *(EntityID*)braggi_vector_get(state_entities, i);
        StateComponent* state = braggi_ecs_get_component(
            world, state_entity, state_component_type);
        CellComponent* cell = braggi_ecs_get_component(
            world, state_entity, cell_component_type);
        
        // Based on state type, create appropriate AST nodes
        create_ast_node_for_state(world, program_entity, state, cell);
    }
    
    braggi_vector_destroy(state_entities);
}
```

### Example 3: Custom Code Generation System

```c
void custom_codegen_system(ECSWorld* world, System* system, float delta_time) {
    // Find the root AST entity (program)
    ComponentMask program_mask = COMPONENT_MASK(ast_component_type);
    EntityQuery query = braggi_ecs_query_entities(world, program_mask);
    
    EntityID program_entity;
    if (braggi_ecs_query_next(&query, &program_entity)) {
        ASTNodeComponent* program = braggi_ecs_get_component(
            world, program_entity, ast_component_type);
        
        if (program->node_type == AST_PROGRAM) {
            // Create a code generation entity
            EntityID codegen_entity = braggi_ecs_create_entity(world);
            
            // Add target architecture component
            TargetArchComponent* target = braggi_ecs_add_component(
                world, codegen_entity, target_arch_component_type);
            target->arch = TARGET_ARCH_X86_64;
            target->format = OUTPUT_FORMAT_EXECUTABLE;
            target->optimize = true;
            target->optimization_level = 2;
            
            // Add reference to AST
            CodeGenRefComponent* ref = braggi_ecs_add_component(
                world, codegen_entity, codegen_ref_component_type);
            ref->ast_entity = program_entity;
            
            // Generate code
            braggi_ecs_process_codegen_entity(world, codegen_entity);
        }
    }
}
```

## ECS and WFCCC Integration

The integration of ECS with the Wave Function Constraint Collapse Compilation is one of Braggi's key innovations:

```c
// Initialize entropy-ECS integration
bool braggi_entropy_ecs_initialize(BraggiContext* context) {
    ECSWorld* world = context->ecs_world;
    
    // Register entropy components
    ComponentTypeID component_ids[5];
    if (!braggi_entropy_register_components(world, component_ids)) {
        return false;
    }
    
    // Create and register entropy systems
    System* sync_system = braggi_entropy_create_sync_system(context);
    System* constraint_system = braggi_entropy_create_constraint_system(context);
    System* wfc_system = braggi_entropy_create_wfc_system(context);
    
    braggi_ecs_register_system(world, sync_system);
    braggi_ecs_register_system(world, constraint_system);
    braggi_ecs_register_system(world, wfc_system);
    
    return true;
}

// Apply the Wave Function Collapse algorithm using the ECS
bool braggi_entropy_apply_wfc_with_ecs(BraggiContext* context) {
    ECSWorld* world = context->ecs_world;
    
    // Synchronize entropy field with ECS entities
    braggi_entropy_sync_to_ecs(context);
    
    // Run the constraint system
    braggi_ecs_update_system(world, context->constraint_system, 0.0f);
    
    // Run the WFC system
    braggi_ecs_update_system(world, context->wfc_system, 0.0f);
    
    // Check if all cells are collapsed
    return braggi_entropy_field_is_fully_collapsed(context->entropy_field);
}
```

## Benefits of the ECS Approach

1. **Parallelization**: ECS naturally enables parallel processing of entities
2. **Composability**: Easy to add new features by adding components and systems
3. **Testability**: Systems can be tested in isolation with mock entities
4. **Performance**: Data-oriented design improves cache locality
5. **Extensibility**: Plugin systems can add new components and systems
6. **Visualization**: ECS structure makes it easy to visualize the compilation process

## Advanced ECS Techniques

### System Dependencies and Ordering

```c
// Define system dependencies
SystemDependency dependencies[] = {
    { tokenize_system_id, init_entropy_system_id },  // tokenize must run before init_entropy
    { init_entropy_system_id, constraint_system_id }, // init_entropy must run before constraints
    // ...additional dependencies
};

// Register dependencies
braggi_ecs_register_system_dependencies(world, dependencies, 
                                      sizeof(dependencies) / sizeof(dependencies[0]));
```

### Multi-threading with ECS

```c
// Create a multi-threaded ECS world
ThreadConfig thread_config = {
    .thread_count = 4,
    .use_thread_pool = true
};
ECSWorld* mt_world = braggi_ecs_create_mt_world(1000, 50, &thread_config);

// Register system as parallelizable
SystemInfo system_info = {
    // ... other fields
    .parallelizable = true,
    .chunk_size = 64  // Process entities in chunks of 64
};
System* parallel_system = braggi_ecs_create_system(&system_info);
```

### Custom Memory Management

```c
// Create regions for different entity types
Region* token_region = braggi_region_create("tokens", REGION_MB(16));
Region* ast_region = braggi_region_create("ast", REGION_MB(32));

// Register component types with specific regions
ComponentTypeInfo token_info = {
    .name = "Token",
    .size = sizeof(TokenComponent),
    .region = token_region,
    // ...other fields
};

ComponentTypeInfo ast_info = {
    .name = "ASTNode",
    .size = sizeof(ASTNodeComponent),
    .region = ast_region,
    // ...other fields
};
```

## Conclusion

The Entity Component System architecture in Braggi provides a powerful, flexible framework for compiler implementation. By separating data (components) from logic (systems), and using lightweight entities to tie them together, Braggi achieves a highly modular and parallelizable compilation pipeline.

This ECS integration with the Wave Function Constraint Collapse algorithm creates a unique approach to compiler construction that enables precise error detection, parallel processing, and exceptional extensibility.

> "When your compiler's built like a well-organized ranch, the code wranglin' happens mighty smooth!" - Texas Programming Proverb 