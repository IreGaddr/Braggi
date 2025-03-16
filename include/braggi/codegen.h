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
} CodeGenOptions;

/*
 * Code generator context
 */
typedef struct {
    BraggiContext* braggi_ctx;   /* Compiler context */
    CodeGenOptions options;      /* Code generator options */
    void* arch_data;             /* Architecture-specific data */
} CodeGenContext;

/* Function prototypes */

/* Initialize the code generator */
bool braggi_codegen_init(CodeGenContext* ctx, BraggiContext* braggi_ctx, CodeGenOptions options);

/* Clean up the code generator */
void braggi_codegen_cleanup(CodeGenContext* ctx);

/* Generate code from the AST */
bool braggi_codegen_generate(CodeGenContext* ctx);

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

#endif /* BRAGGI_CODEGEN_H */ 