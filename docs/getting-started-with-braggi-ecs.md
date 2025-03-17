# Getting Started with Braggi ECS Development
## "From Tenderfoot to Trail Boss"

> "Ain't nothin' to it but to do it, pardner. Let's wrangle some code!" - Texas Programming Wisdom

## Introduction

This guide will help you get started with Braggi's Entity Component System (ECS) architecture. Whether you're extending the compiler, creating plugins, or just learning how the system works, this document will walk you through the basics and provide practical examples.

## Prerequisites

- Basic C programming knowledge
- Understanding of compiler concepts (helpful but not required)
- Braggi development environment set up

## Setting Up Your Development Environment

1. **Clone the Braggi repository**:
   ```bash
   git clone https://github.com/yourusername/Braggi.git
   cd Braggi
   ```

2. **Build the project**:
   ```bash
   ./build_and_test.sh
   ```

3. **Set up your editor**:
   - Configure your editor to recognize Braggi header files
   - Set include paths to include the `include` directory

## ECS Fundamentals

Before diving into code, let's review the core concepts of Entity Component System:

- **Entity**: Just an ID, representing a "thing" in your system
- **Component**: Pure data structures attached to entities
- **System**: Logic that processes entities with specific component combinations

In Braggi, entities might represent tokens, AST nodes, or code generation units. Components hold the data for these entities, and systems implement the compilation logic.

## Step 1: Creating a Simple ECS Application

Let's create a simple application that uses Braggi's ECS:

```c
#include "braggi/braggi.h"
#include "braggi/ecs.h"
#include <stdio.h>

int main() {
    // Initialize Braggi
    BraggiConfig config = braggi_config_create_default();
    BraggiContext* context = braggi_init(&config);
    
    if (!context) {
        fprintf(stderr, "Failed to initialize Braggi\n");
        return 1;
    }
    
    // Get the ECS world from the context
    ECSWorld* world = context->ecs_world;
    
    // Use the ECS world...
    
    // Clean up
    braggi_cleanup(context);
    return 0;
}
```

Compile this with:

```bash
gcc -o simple_ecs simple_ecs.c -lbraggi
```

## Step 2: Defining Components

Now let's define some custom components for our application:

```c
// In your_components.h
#ifndef YOUR_COMPONENTS_H
#define YOUR_COMPONENTS_H

#include "braggi/ecs.h"

// Position component
typedef struct {
    float x;
    float y;
} PositionComponent;

// Velocity component
typedef struct {
    float dx;
    float dy;
} VelocityComponent;

// Registering components
ComponentTypeID register_components(ECSWorld* world);

#endif
```

```c
// In your_components.c
#include "your_components.h"

ComponentTypeID position_type;
ComponentTypeID velocity_type;

ComponentTypeID register_components(ECSWorld* world) {
    position_type = braggi_ecs_register_component(world, sizeof(PositionComponent));
    velocity_type = braggi_ecs_register_component(world, sizeof(VelocityComponent));
    return position_type; // Return first component ID
}
```

## Step 3: Creating a System

Now let's create a system that processes entities with position and velocity components:

```c
// In movement_system.h
#ifndef MOVEMENT_SYSTEM_H
#define MOVEMENT_SYSTEM_H

#include "braggi/ecs.h"
#include "your_components.h"

// Create the movement system
System* create_movement_system(ECSWorld* world);

// Movement system update function
void movement_system_update(ECSWorld* world, System* system, float delta_time);

#endif
```

```c
// In movement_system.c
#include "movement_system.h"
#include <stdio.h>

extern ComponentTypeID position_type;
extern ComponentTypeID velocity_type;

void movement_system_update(ECSWorld* world, System* system, float delta_time) {
    // Create a query for entities with both position and velocity
    ComponentMask mask = 0;
    braggi_ecs_mask_set(&mask, position_type);
    braggi_ecs_mask_set(&mask, velocity_type);
    
    EntityQuery query = braggi_ecs_query_entities(world, mask);
    
    // Process matching entities
    EntityID entity;
    while (braggi_ecs_query_next(&query, &entity)) {
        // Get components
        PositionComponent* pos = braggi_ecs_get_component(world, entity, position_type);
        VelocityComponent* vel = braggi_ecs_get_component(world, entity, velocity_type);
        
        // Update position based on velocity and delta time
        pos->x += vel->dx * delta_time;
        pos->y += vel->dy * delta_time;
        
        printf("Entity %u moved to (%.2f, %.2f)\n", entity, pos->x, pos->y);
    }
}

System* create_movement_system(ECSWorld* world) {
    // Create mask requiring both position and velocity components
    ComponentMask mask = 0;
    braggi_ecs_mask_set(&mask, position_type);
    braggi_ecs_mask_set(&mask, velocity_type);
    
    // Create the system
    System* system = braggi_ecs_system_create(mask, movement_system_update, NULL);
    
    // Register the system with the world
    braggi_ecs_register_system(world, system);
    
    return system;
}
```

## Step 4: Putting It All Together

Now let's update our main application to use these components and systems:

```c
#include "braggi/braggi.h"
#include "braggi/ecs.h"
#include "your_components.h"
#include "movement_system.h"
#include <stdio.h>

int main() {
    // Initialize Braggi
    BraggiConfig config = braggi_config_create_default();
    BraggiContext* context = braggi_init(&config);
    
    if (!context) {
        fprintf(stderr, "Failed to initialize Braggi\n");
        return 1;
    }
    
    // Get the ECS world
    ECSWorld* world = context->ecs_world;
    
    // Register components
    register_components(world);
    
    // Create movement system
    System* movement_system = create_movement_system(world);
    
    // Create some entities
    for (int i = 0; i < 5; i++) {
        // Create entity
        EntityID entity = braggi_ecs_create_entity(world);
        
        // Add position component
        PositionComponent* pos = braggi_ecs_add_component(world, entity, position_type);
        pos->x = i * 10.0f;
        pos->y = i * 5.0f;
        
        // Add velocity component
        VelocityComponent* vel = braggi_ecs_add_component(world, entity, velocity_type);
        vel->dx = 1.0f + i * 0.5f;
        vel->dy = 0.5f + i * 0.25f;
        
        printf("Created entity %u at (%.2f, %.2f) with velocity (%.2f, %.2f)\n", 
               entity, pos->x, pos->y, vel->dx, vel->dy);
    }
    
    // Run the simulation for 10 steps
    for (int step = 0; step < 10; step++) {
        printf("\n--- Step %d ---\n", step);
        braggi_ecs_update_system(world, movement_system, 0.1f);
    }
    
    // Clean up
    braggi_cleanup(context);
    return 0;
}
```

## Step 5: Integrating with Braggi's Compiler

Now let's create a simple plugin that integrates with Braggi's compilation process:

```c
#include "braggi/braggi.h"
#include "braggi/ecs.h"
#include "braggi/plugin.h"

// Our custom component
typedef struct {
    int line_count;
    int char_count;
    char* filename;
} FileStatisticsComponent;

// Global component type ID
ComponentTypeID file_stats_type;

// System update function to analyze files
void file_stats_system_update(ECSWorld* world, System* system, float delta_time) {
    // Find entities with source code components
    ComponentMask mask = COMPONENT_MASK(source_component_type);
    EntityQuery query = braggi_ecs_query_entities(world, mask);
    
    EntityID entity;
    while (braggi_ecs_query_next(&query, &entity)) {
        // Get source component
        SourceComponent* source = braggi_ecs_get_component(
            world, entity, source_component_type);
        
        // Count lines and characters
        int lines = 0;
        int chars = 0;
        const char* p = source->code;
        while (*p) {
            if (*p == '\n') lines++;
            chars++;
            p++;
        }
        
        // Add or update statistics component
        FileStatisticsComponent* stats;
        if (braggi_ecs_has_component(world, entity, file_stats_type)) {
            stats = braggi_ecs_get_component(world, entity, file_stats_type);
        } else {
            stats = braggi_ecs_add_component(world, entity, file_stats_type);
            stats->filename = strdup(source->filename);
        }
        
        stats->line_count = lines;
        stats->char_count = chars;
        
        printf("File %s: %d lines, %d characters\n", 
               stats->filename, stats->line_count, stats->char_count);
    }
}

// Plugin initialization function
BRAGGI_PLUGIN_INIT {
    // Register our component
    ComponentTypeInfo info = {
        .name = "FileStatistics",
        .size = sizeof(FileStatisticsComponent),
        .constructor = NULL,
        .destructor = NULL
    };
    file_stats_type = braggi_ecs_register_component_type(context->ecs_world, &info);
    
    // Create and register our system
    SystemInfo system_info = {
        .name = "FileStatisticsSystem",
        .update_func = file_stats_system_update,
        .context = NULL,
        .priority = 100  // Run after tokenization
    };
    System* system = braggi_ecs_create_system(&system_info);
    braggi_ecs_register_system(context->ecs_world, system);
    
    return true;
}

// Plugin cleanup function
BRAGGI_PLUGIN_CLEANUP {
    // Nothing to clean up
}
```

Build this as a shared library and load it with Braggi:

```bash
gcc -shared -fPIC -o file_stats_plugin.so file_stats_plugin.c -lbraggi
```

Then use it with the Braggi compiler:

```bash
braggi --load-plugin=./file_stats_plugin.so your_code.bg
```

## Advanced ECS Usage

### Memory Regions and ECS

Braggi's region-based memory system integrates naturally with ECS:

```c
// Create memory regions for different component types
Region* token_region = braggi_region_create("token_region", REGION_MB(16));
Region* ast_region = braggi_region_create("ast_region", REGION_MB(32));

// Create ECS world with memory regions
ECSWorld* world = braggi_ecs_create_world_with_region(
    token_region,  // Region for ECS internals
    ast_region,    // Region for entity allocations
    REGIME_SEQ     // Regime type for memory access
);
```

### Multi-threading ECS Systems

For high-performance compilation, Braggi's ECS supports multi-threading:

```c
// Create a multi-threaded system
SystemInfo mt_system_info = {
    .name = "ParallelSystem",
    .update_func = parallel_system_update,
    .context = NULL,
    .priority = 10,
    .parallelizable = true,
    .chunk_size = 64  // Process entities in chunks of this size
};

System* parallel_system = braggi_ecs_create_system(&mt_system_info);
```

The system update function gets called on multiple threads with different entity chunks:

```c
void parallel_system_update(ECSWorld* world, System* system, float delta_time) {
    // This function may be called on multiple threads
    // Each thread gets a different slice of entities
    
    // thread_id and chunk_info available in system context
    ThreadContext* thread_ctx = (ThreadContext*)system->context;
    
    // Process entities in this thread's chunk
    // ...
}
```

### Custom Component Types

For complex components, you can provide custom constructor and destructor functions:

```c
void ast_node_constructor(void* component) {
    ASTNodeComponent* node = (ASTNodeComponent*)component;
    node->node_type = AST_INVALID;
    node->data = NULL;
}

void ast_node_destructor(void* component) {
    ASTNodeComponent* node = (ASTNodeComponent*)component;
    if (node->data) {
        // Free node data based on node type
        switch (node->node_type) {
            case AST_FUNCTION_DECL:
                free_function_decl_data(node->data);
                break;
            // Other node types...
        }
    }
}

// Register with custom handlers
ComponentTypeInfo ast_info = {
    .name = "ASTNode",
    .size = sizeof(ASTNodeComponent),
    .constructor = ast_node_constructor,
    .destructor = ast_node_destructor
};
ComponentTypeID ast_type = braggi_ecs_register_component_type(world, &ast_info);
```

## Extending Braggi's Compiler with ECS

### Adding a New Optimization Pass

Here's how to add a custom optimization pass to Braggi's compilation pipeline:

```c
// Optimization component to track optimization status
typedef struct {
    bool is_optimized;
    int optimizations_applied;
} OptimizationComponent;

// Optimization system
void constant_folding_system(ECSWorld* world, System* system, float delta_time) {
    // Find AST expression nodes
    ComponentMask mask = COMPONENT_MASK(ast_component_type);
    EntityQuery query = braggi_ecs_query_entities(world, mask);
    
    EntityID entity;
    while (braggi_ecs_query_next(&query, &entity)) {
        ASTNodeComponent* node = braggi_ecs_get_component(
            world, entity, ast_component_type);
        
        if (node->node_type == AST_BINARY_EXPR) {
            BinaryExprData* expr = (BinaryExprData*)node->data;
            
            // Check if both operands are literals
            if (is_literal_expression(expr->left) && is_literal_expression(expr->right)) {
                // Fold constant expression
                LiteralValue result = evaluate_constant_expression(expr);
                
                // Replace with literal
                node->node_type = AST_LITERAL;
                free(expr);
                node->data = create_literal_data(result);
                
                // Add or update optimization component
                OptimizationComponent* opt;
                if (braggi_ecs_has_component(world, entity, opt_component_type)) {
                    opt = braggi_ecs_get_component(world, entity, opt_component_type);
                } else {
                    opt = braggi_ecs_add_component(world, entity, opt_component_type);
                    opt->optimizations_applied = 0;
                }
                
                opt->is_optimized = true;
                opt->optimizations_applied++;
            }
        }
    }
}
```

### Creating a Custom Analysis Tool

You can use the ECS to create powerful static analysis tools:

```c
// Analysis result component
typedef struct {
    int complexity;
    int depth;
    char* warnings[10];
    int warning_count;
} ComplexityAnalysisComponent;

// Analysis system
void complexity_analysis_system(ECSWorld* world, System* system, float delta_time) {
    // Find function declaration nodes
    ComponentMask mask = COMPONENT_MASK(ast_component_type);
    EntityQuery query = braggi_ecs_query_entities(world, mask);
    
    EntityID entity;
    while (braggi_ecs_query_next(&query, &entity)) {
        ASTNodeComponent* node = braggi_ecs_get_component(
            world, entity, ast_component_type);
        
        if (node->node_type == AST_FUNCTION_DECL) {
            FunctionDeclData* func = (FunctionDeclData*)node->data;
            
            // Calculate complexity
            int complexity = calculate_cyclomatic_complexity(func);
            int depth = calculate_nesting_depth(func);
            
            // Add analysis component
            ComplexityAnalysisComponent* analysis = braggi_ecs_add_component(
                world, entity, complexity_analysis_type);
            
            analysis->complexity = complexity;
            analysis->depth = depth;
            analysis->warning_count = 0;
            
            // Add warnings if needed
            if (complexity > 10) {
                analysis->warnings[analysis->warning_count++] = 
                    "Function has high cyclomatic complexity";
            }
            
            if (depth > 5) {
                analysis->warnings[analysis->warning_count++] = 
                    "Function has excessive nesting depth";
            }
            
            // Print results
            printf("Function %s: complexity=%d, depth=%d, warnings=%d\n",
                   func->name, complexity, depth, analysis->warning_count);
        }
    }
}
```

## Conclusion

You've now learned the basics of working with Braggi's Entity Component System architecture. You can:

1. Define custom components to store data
2. Create systems to process entities with specific components
3. Integrate with Braggi's compilation pipeline
4. Create plugins to extend the compiler
5. Leverage advanced features like multi-threading and memory regions

As you become more familiar with the ECS approach, you'll find it provides a powerful, extensible foundation for creating compiler tools, language extensions, and optimizations.

Remember, in ECS, the focus is on:
- **Separation of data and logic**
- **Composition over inheritance**
- **Data-oriented design for performance**

These principles will guide you to creating maintainable, efficient code as you work with Braggi.

> "Now you're ridin' tall in the saddle, partner! You've got the tools, the knowledge, and the spirit to wrangle some serious code!" - Texas Programming Mentor 