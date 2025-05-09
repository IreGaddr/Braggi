/*
 * Braggi - ARM64 (AArch64) Code Generation
 *
 * "ARM64 is like the longneck of processor architectures - 
 * efficient, elegant, and found in all the best places!" - Texas tech wisdom
 */

#include "braggi/codegen.h"
#include "braggi/codegen_arch.h"
#include "braggi/region.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ARM64-specific data structure
typedef struct {
    // Assembly output buffer
    char* asm_buffer;
    size_t asm_size;
    size_t asm_capacity;
} ARM64Data;

// Basic initialization function
static bool arm64_init(CodeGenerator* generator, ErrorHandler* error_handler) {
    if (!generator) return false;
    
    // Allocate backend data
    ARM64Data* data = (ARM64Data*)malloc(sizeof(ARM64Data));
    if (!data) return false;
    
    // Initialize data
    data->asm_buffer = NULL;
    data->asm_size = 0;
    data->asm_capacity = 0;
    
    // Store it in the generator
    generator->arch_data = data;
    
    return true;
}

// Cleanup function
static void arm64_destroy(CodeGenerator* generator) {
    if (!generator) return;
    
    ARM64Data* data = (ARM64Data*)generator->arch_data;
    if (data) {
        if (data->asm_buffer) {
            free(data->asm_buffer);
        }
        free(data);
    }
    
    generator->arch_data = NULL;
}

// Code generation function
static bool arm64_generate(CodeGenerator* generator, EntropyField* field) {
    if (!generator || !field) return false;
    
    ARM64Data* data = (ARM64Data*)generator->arch_data;
    if (!data) return false;
    
    // Create or clear assembly buffer
    if (data->asm_buffer) {
        free(data->asm_buffer);
    }
    
    // Just create a simple assembly stub
    const char* stub_code = "// Generated by Braggi ARM64 Backend\n"
                           ".text\n"
                           ".global _start\n"
                           "_start:\n"
                           "    mov x0, #0\n"
                           "    mov x8, #93\n"
                           "    svc #0\n";
    
    size_t len = strlen(stub_code);
    data->asm_buffer = strdup(stub_code);
    if (!data->asm_buffer) return false;
    
    data->asm_size = len;
    data->asm_capacity = len + 1;
    
    return true;
}

// Emit function
static bool arm64_emit(CodeGenerator* generator, const char* filename, OutputFormat format) {
    if (!generator || !filename) return false;
    
    ARM64Data* data = (ARM64Data*)generator->arch_data;
    if (!data || !data->asm_buffer) return false;
    
    FILE* file = fopen(filename, "w");
    if (!file) return false;
    
    fputs(data->asm_buffer, file);
    fclose(file);
    
    return true;
}

// Backend registration
void braggi_register_arm64_backend(void) {
    // Create the generator
    CodeGenerator* generator = (CodeGenerator*)malloc(sizeof(CodeGenerator));
    if (!generator) return;
    
    // Initialize fields
    generator->name = "ARM64";
    generator->description = "ARM64 (AArch64) backend";
    generator->arch_data = NULL;
    
    // Set function pointers
    generator->init = arm64_init;
    generator->destroy = arm64_destroy;
    generator->generate = arm64_generate;
    generator->emit = arm64_emit;
    generator->register_function = NULL;
    generator->optimize = NULL;
    generator->generate_debug_info = NULL;
    
    // Register with manager
    extern bool braggi_codegen_manager_register_backend(CodeGenerator* generator);
    if (!braggi_codegen_manager_register_backend(generator)) {
        free(generator);
    }
} 