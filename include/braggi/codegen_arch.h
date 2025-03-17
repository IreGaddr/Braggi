/*
 * Braggi - Architecture-specific Code Generation Interface
 * 
 * "Every architecture's got its own personality, but they all gotta follow
 * the same dance steps when workin' with Braggi!" - Texas Code Ranch wisdom
 */

#ifndef BRAGGI_CODEGEN_ARCH_H
#define BRAGGI_CODEGEN_ARCH_H

#include "braggi/codegen.h"
#include "braggi/error.h"
#include "braggi/entropy.h"
#include <stdbool.h>

/*
 * Code Generator structure - holds function pointers for architecture-specific implementations
 */
typedef struct CodeGenerator {
    const char* name;             /* Name of the code generator */
    const char* description;      /* Description of the code generator */
    void* arch_data;              /* Architecture-specific data */
    
    /* Initialize the code generator */
    bool (*init)(struct CodeGenerator* generator, ErrorHandler* error_handler);
    
    /* Clean up the code generator */
    void (*destroy)(struct CodeGenerator* generator);
    
    /* Generate code from the entropy field */
    bool (*generate)(struct CodeGenerator* generator, EntropyField* field);
    
    /* Emit generated code to a file */
    bool (*emit)(struct CodeGenerator* generator, const char* filename, OutputFormat format);
    
    /* Optional: Platform-specific function callback registration */
    bool (*register_function)(struct CodeGenerator* generator, const char* name, void* func_ptr);
    
    /* Optional: Platform-specific optimization */
    bool (*optimize)(struct CodeGenerator* generator, int level);
    
    /* Optional: Generate debug information */
    bool (*generate_debug_info)(struct CodeGenerator* generator, bool enable);
} CodeGenerator;

/*
 * Architecture registration functions - implemented by each architecture backend
 */
void braggi_register_x86_backend(void);
void braggi_register_x86_64_backend(void);
void braggi_register_arm_backend(void);
void braggi_register_arm64_backend(void);
void braggi_register_wasm_backend(void);
void braggi_register_bytecode_backend(void);

/*
 * Common utility functions that may be useful for multiple architectures
 */

/* Convert a register index to its name (architecture-specific implementation) */
const char* braggi_codegen_register_name(int reg_index, TargetArch arch);

/* Check if a register is caller-saved (architecture-specific implementation) */
bool braggi_codegen_is_caller_saved(int reg_index, TargetArch arch);

/* Check if a register is callee-saved (architecture-specific implementation) */
bool braggi_codegen_is_callee_saved(int reg_index, TargetArch arch);

/* Get the number of available registers for a given architecture */
int braggi_codegen_register_count(TargetArch arch);

/* Get the stack alignment for a given architecture */
int braggi_codegen_stack_alignment(TargetArch arch);

#endif /* BRAGGI_CODEGEN_ARCH_H */ 