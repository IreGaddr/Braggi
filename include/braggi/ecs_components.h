/*
 * Braggi - ECS Components for the Compiler
 * 
 * "Just like a good Texas ranch has different types of cattle,
 * a good ECS has specialized components for every need!"
 * - Irish-Texan software design wisdom
 */

#ifndef BRAGGI_ECS_COMPONENTS_H
#define BRAGGI_ECS_COMPONENTS_H

#include "braggi/ecs.h"
#include "braggi/entropy.h"
#include "braggi/token.h"
#include "braggi/source.h"
#include "braggi/region.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Component types for the Braggi compiler
typedef enum {
    // Source code related components
    COMPONENT_TOKEN,              // Token data
    COMPONENT_SOURCE_POSITION,    // Position in source code
    COMPONENT_SOURCE_REFERENCE,   // Reference to source file
    
    // Entropy and state components
    COMPONENT_ENTROPY_CELL,       // Entropy cell data
    COMPONENT_STATE,              // Current state
    COMPONENT_POSSIBLE_STATES,    // Possible states
    
    // Constraint related components
    COMPONENT_CONSTRAINT,         // Constraint data
    COMPONENT_CONSTRAINT_TARGET,  // Target of a constraint
    
    // AST related components
    COMPONENT_AST_NODE,           // AST node data
    COMPONENT_AST_REFERENCE,      // Reference to another AST node
    
    // Type system components
    COMPONENT_TYPE,               // Type information
    COMPONENT_TYPE_CONSTRAINT,    // Type constraint
    
    // Region system components
    COMPONENT_REGION,             // Region data
    COMPONENT_ALLOCATION,         // Allocation in a region
    COMPONENT_PERISCOPE,          // Periscope between regions
    
    // Code generation components
    COMPONENT_CODEGEN_INFO,       // Code generation metadata
    COMPONENT_REGISTER,           // Register allocation
    COMPONENT_MEMORY_LOCATION,    // Memory location

    // Add more component types as needed
} BraggiComponentType;

// Token component
typedef struct {
    Token token;                  // Token data
} TokenComponent;

// Source position component
typedef struct {
    SourcePosition position;      // Position in source code
} SourcePositionComponent;

// Source reference component
typedef struct {
    SourceFile* file;             // Reference to source file
    uint32_t line;                // Line number (cached for quick access)
    uint32_t column;              // Column number (cached for quick access)
} SourceReferenceComponent;

// Entropy cell component
typedef struct {
    Cell* cell;                   // Reference to entropy cell
    float entropy;                // Current entropy value (cached)
} EntropyCellComponent;

// State component
typedef struct {
    State* state;                 // Current state
    uint32_t state_id;            // State ID
} StateComponent;

// Possible states component
typedef struct {
    State** states;               // Array of possible states
    size_t count;                 // Number of states
    size_t capacity;              // Capacity of states array
} PossibleStatesComponent;

// Constraint component
typedef struct {
    Constraint* constraint;       // Reference to constraint
    ConstraintType type;          // Type of constraint
    bool is_satisfied;            // Whether the constraint is satisfied
} ConstraintComponent;

// Constraint target component
typedef struct {
    EntityID constraint_entity;   // Entity representing the constraint
    EntityID* target_entities;    // Entities affected by the constraint
    size_t target_count;          // Number of target entities
} ConstraintTargetComponent;

// AST node component
typedef struct {
    enum {
        AST_PROGRAM,
        AST_FUNCTION_DECL,
        AST_VARIABLE_DECL,
        AST_REGION_DECL,
        AST_REGIME_DECL,
        AST_BLOCK,
        AST_EXPRESSION,
        AST_BINARY_EXPR,
        AST_UNARY_EXPR,
        AST_LITERAL,
        AST_IDENTIFIER,
        AST_CALL,
        AST_IF_STMT,
        AST_WHILE_STMT,
        AST_FOR_STMT,
        AST_RETURN_STMT,
        AST_COLLAPSE_STMT,
        AST_SUPERPOSE_STMT,
        AST_PERISCOPE_STMT
    } node_type;
    
    void* data;                   // Node-specific data
} ASTNodeComponent;

// AST reference component
typedef struct {
    EntityID target_entity;       // Target AST node entity
    enum {
        AST_REF_PARENT,
        AST_REF_CHILD,
        AST_REF_SIBLING,
        AST_REF_DECLARATION
    } reference_type;
} ASTReferenceComponent;

// Type component
typedef struct {
    enum {
        TYPE_VOID,
        TYPE_BOOL,
        TYPE_INT,
        TYPE_UINT,
        TYPE_FLOAT,
        TYPE_STRING,
        TYPE_ARRAY,
        TYPE_STRUCT,
        TYPE_ENUM,
        TYPE_FUNCTION,
        TYPE_REGION,
        TYPE_REFERENCE
    } base_type;
    
    size_t size;                  // Size in bytes
    void* type_data;              // Type-specific data
} TypeComponent;

// Region component
typedef struct {
    char* name;                   // Region name
    size_t size;                  // Region size
    RegimeType regime;            // Region regime
    EntityID* allocations;        // Entities representing allocations
    size_t allocation_count;      // Number of allocations
} RegionComponent;

// Allocation component
typedef struct {
    EntityID region_entity;       // Entity representing the region
    size_t size;                  // Size of allocation
    uint32_t source_pos;          // Source position of allocation
    char* label;                  // Label for debugging
} AllocationComponent;

// Periscope component
typedef struct {
    EntityID source_entity;       // Entity representing the source region
    EntityID target_entity;       // Entity representing the target region
    PeriscopeDirection direction; // Direction of periscope
} PeriscopeComponent;

// Code generation info component
typedef struct {
    enum {
        CODEGEN_GLOBAL,
        CODEGEN_FUNCTION,
        CODEGEN_VARIABLE,
        CODEGEN_EXPRESSION,
        CODEGEN_STATEMENT,
        CODEGEN_CONTROL_FLOW
    } code_type;
    
    char* name;                   // Symbol name
    void* codegen_data;           // Code generation specific data
} CodeGenInfoComponent;

// Register component
typedef struct {
    int register_id;              // Register ID
    bool is_allocated;            // Whether the register is allocated
    EntityID owner_entity;        // Entity that owns the register
} RegisterComponent;

// Memory location component
typedef struct {
    enum {
        MEMORY_GLOBAL,
        MEMORY_STACK,
        MEMORY_REGION,
        MEMORY_REGISTER
    } location_type;
    
    size_t offset;                // Offset from base
    size_t size;                  // Size in bytes
    EntityID base_entity;         // Entity representing the base location
} MemoryLocationComponent;

// Initialize component types for the Braggi compiler
void braggi_ecs_init_component_types(ECSWorld* world);

// Helper functions for common component operations
// (These would be implemented in ecs_components.c)

#endif /* BRAGGI_ECS_COMPONENTS_H */ 