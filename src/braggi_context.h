/*
 * Braggi - Compiler Context with ECS Integration
 * 
 * "A good context is like a trusty saddle - it holds everything
 * together for the long ride ahead!" - Texas compiler wisdom
 */

#ifndef BRAGGI_CONTEXT_H
#define BRAGGI_CONTEXT_H

#include "braggi/braggi.h"
#include "braggi/ecs.h"
#include "braggi/entropy.h"
#include "braggi/error.h"
#include "braggi/source.h"
#include "braggi/codegen.h"
#include "braggi/util/vector.h"
#include "braggi/builtins.h"

// The main compiler context structure
struct BraggiContext {
    // ECS world - the core of our quantum-inspired compiler
    ECSWorld* world;
    
    // Legacy systems for backward compatibility
    Vector* source_files;         // Vector of SourceFile*
    EntropyField* entropy_field;  // Now primarily for visualization
    
    // Error handling
    ErrorHandler* error_handler;
    
    // Target configuration
    TargetArchitecture target_arch;
    TargetOS target_os;
    
    // Code generation
    CodeGenContext* codegen;
    
    // Last error message (for convenient retrieval)
    char* last_error;
    
    // Builtin registry for standard library functions
    BraggiBuiltinRegistry* builtin_registry;
};

#endif /* BRAGGI_CONTEXT_H */ 