/*
 * Braggi - Code Generation
 * 
 * "Generating code is like bakin' biscuits - follow the recipe, 
 * but add your own special touch!" - Texas Code Baker
 */

#ifndef BRAGGI_CODEGEN_H
#define BRAGGI_CODEGEN_H

#include "braggi/braggi_context.h"
#include <stdint.h>
#include <stdbool.h>

/*
 * Target architectures
 */
typedef enum {
    ARCH_X86,
    ARCH_X86_64,
    ARCH_ARM,
    ARCH_ARM64,
    ARCH_WASM,
    ARCH_BYTECODE,
    ARCH_COUNT
} TargetArch;

/*
 * Output formats
 */
typedef enum {
    FORMAT_OBJECT,
    FORMAT_EXECUTABLE,
    FORMAT_LIBRARY,
    FORMAT_ASM,
    FORMAT_COUNT
} OutputFormat;

/*
 * Code generator options
 */
typedef struct {
    TargetArch arch;             /* Target architecture */
    OutputFormat format;         /* Output format */
    bool optimize;               /* Enable optimizations */
    int optimization_level;      /* Optimization level (0-3) */
    bool emit_debug_info;        /* Include debug information */
    char* output_file;           /* Output file name */
    EntropyField* entropy_field; /* Entropy field for code generation */
} CodeGenOptions;

/*
 * Code generator context
 */
typedef struct {
    BraggiContext* braggi_ctx;   /* Compiler context */
    CodeGenOptions options;      /* Code generator options */
    void* arch_data;             /* Architecture-specific data */
    struct CodeGenerator* generator; /* The code generator backend */
} CodeGenContext;

/* Function prototypes */

/* Initialize the code generator */
bool braggi_codegen_init(CodeGenContext* ctx, BraggiContext* braggi_ctx, CodeGenOptions options);

/* Clean up the code generator */
void braggi_codegen_cleanup(CodeGenContext* ctx);

/* Generate code from the AST */
bool braggi_codegen_generate(CodeGenContext* ctx);

/* Generate code using the Entity Component System */
bool braggi_codegen_generate_ecs(BraggiContext* braggi_ctx, TargetArch arch, const char* output_file);

/* Write output to a file */
bool braggi_codegen_write_output(CodeGenContext* ctx, const char* filename);

/* Architecture-specific initialization functions */
void braggi_codegen_x86_init(void);
void braggi_codegen_x86_64_init(void);
void braggi_codegen_arm_init(void);
void braggi_codegen_arm64_init(void);
void braggi_codegen_wasm_init(void);
void braggi_codegen_bytecode_init(void);

/* Get default code generator options for an architecture */
CodeGenOptions braggi_codegen_get_default_options(TargetArch arch);

/* Get string representation of an architecture */
const char* braggi_codegen_arch_to_string(TargetArch arch);

/* Get string representation of an output format */
const char* braggi_codegen_format_to_string(OutputFormat format);

/* Initialize the code generation manager */
bool braggi_codegen_manager_init(void);

/* Clean up the code generation manager */
void braggi_codegen_manager_cleanup(void);

/* Apply optimizations to the generated code */
bool braggi_codegen_optimize(CodeGenContext* ctx);

/* Enable or disable debug information generation */
bool braggi_codegen_set_debug_info(CodeGenContext* ctx, bool enable);

#endif /* BRAGGI_CODEGEN_H */ 