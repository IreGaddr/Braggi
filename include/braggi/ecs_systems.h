/*
 * Braggi - ECS Systems for the Compiler
 * 
 * "Good systems are like experienced ranch hands - 
 * they know exactly which critters they're responsible for!"
 * - Texas ECS wisdom
 */

#ifndef BRAGGI_ECS_SYSTEMS_H
#define BRAGGI_ECS_SYSTEMS_H

#include "braggi/ecs.h"
#include "braggi/ecs_components.h"

// Tokenization system - creates token entities from source text
void braggi_system_tokenize(ECSWorld* world, void* context);

// Entropy initialization system - creates entropy cells for tokens
void braggi_system_init_entropy(ECSWorld* world, void* context);

// Constraint creation system - creates constraints between cells
void braggi_system_create_constraints(ECSWorld* world, void* context);

// Constraint propagation system - propagates constraints through the entropy field
void braggi_system_propagate_constraints(ECSWorld* world, void* context);

// Entropy collapse system - collapses entropy cells to single states
void braggi_system_collapse_entropy(ECSWorld* world, void* context);

// AST building system - builds an AST from collapsed entropy cells
void braggi_system_build_ast(ECSWorld* world, void* context);

// Type checking system - performs type checking on the AST
void braggi_system_type_check(ECSWorld* world, void* context);

// Region analysis system - analyzes region usage in the code
void braggi_system_analyze_regions(ECSWorld* world, void* context);

// Code generation system - generates code from the typed AST
void braggi_system_generate_code(ECSWorld* world, void* context);

// Register allocation system - allocates registers for the code
void braggi_system_allocate_registers(ECSWorld* world, void* context);

// Output generation system - outputs the final code
void braggi_system_generate_output(ECSWorld* world, void* context);

// Initialize all systems for the Braggi compiler
void braggi_ecs_init_systems(ECSWorld* world);

#endif /* BRAGGI_ECS_SYSTEMS_H */ 