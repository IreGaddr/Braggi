/*
 * Braggi - ARM64 (AArch64) Code Generation
 *
 * "ARM64 is like the longneck of processor architectures - 
 * efficient, elegant, and found in all the best places!" - Texas tech wisdom
 */

#include "braggi/codegen.h"
#include "braggi/region.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ARM64-specific register information
typedef enum {
    ARM64_REG_X0,
    ARM64_REG_X1,
    ARM64_REG_X2,
    ARM64_REG_X3,
    ARM64_REG_X4,
    ARM64_REG_X5,
    ARM64_REG_X6,
    ARM64_REG_X7,
    // ... more registers
    ARM64_REG_SP,
    ARM64_REG_LR,
    ARM64_REG_XZR  // Zero register
} ARM64Register;

// ARM64-specific code generation context
typedef struct ARM64Context {
    // ARM64-specific state
    uint8_t* code_buffer;
    size_t code_size;
    size_t code_capacity;
    
    // Register allocation state
    bool register_used[32];  // Track register usage
    
    // Procedure linkage information
    size_t stack_size;
} ARM64Context;

// ARM64-specific data
typedef struct {
    // Registers in use
    uint32_t used_registers;
    
    // Current stack size
    int32_t stack_size;
    
    // Current function depth
    int32_t func_depth;
    
    // Region tracking
    Vector* active_regions;  // Currently active regions in scope
    
    // Assembly output buffer
    char* asm_buffer;
    size_t asm_size;
    size_t asm_capacity;
} ARM64Data;

// Helper: Append text to the assembly buffer
static bool append_asm(ARM64Data* data, const char* text) {
    if (!data || !text) return false;
    
    size_t text_len = strlen(text);
    
    // Check if we need to resize the buffer
    if (data->asm_size + text_len + 1 > data->asm_capacity) {
        size_t new_capacity = data->asm_capacity * 2;
        if (new_capacity < data->asm_size + text_len + 1) {
            new_capacity = data->asm_size + text_len + 1;
        }
        
        char* new_buffer = (char*)realloc(data->asm_buffer, new_capacity);
        if (!new_buffer) return false;
        
        data->asm_buffer = new_buffer;
        data->asm_capacity = new_capacity;
    }
    
    // Append the text
    memcpy(data->asm_buffer + data->asm_size, text, text_len);
    data->asm_size += text_len;
    data->asm_buffer[data->asm_size] = '\0';
    
    return true;
}

// ARM64 initialization
static bool arm64_init(CodeGenerator* generator, ErrorHandler* error_handler) {
    if (!generator || !error_handler) return false;
    
    // Set generator fields
    generator->arch = TARGET_ARM64;
    generator->error_handler = error_handler;
    
    // Create ARM64-specific data
    ARM64Data* arm64_data = (ARM64Data*)malloc(sizeof(ARM64Data));
    if (!arm64_data) return false;
    
    // Initialize ARM64 data
    arm64_data->used_registers = 0;
    arm64_data->stack_size = 0;
    arm64_data->func_depth = 0;
    arm64_data->active_regions = braggi_vector_create(8);
    
    // Initialize assembly buffer
    arm64_data->asm_capacity = 4096;  // Initial 4KB buffer
    arm64_data->asm_buffer = (char*)malloc(arm64_data->asm_capacity);
    if (!arm64_data->asm_buffer || !arm64_data->active_regions) {
        if (arm64_data->asm_buffer) free(arm64_data->asm_buffer);
        if (arm64_data->active_regions) braggi_vector_destroy(arm64_data->active_regions);
        free(arm64_data);
        return false;
    }
    
    arm64_data->asm_size = 0;
    arm64_data->asm_buffer[0] = '\0';
    
    // Store ARM64-specific data
    generator->target_data = arm64_data;
    
    return true;
}

// ARM64 cleanup
static void arm64_destroy(CodeGenerator* generator) {
    if (!generator) return;
    
    // Free ARM64-specific data
    ARM64Data* arm64_data = (ARM64Data*)generator->target_data;
    if (arm64_data) {
        if (arm64_data->asm_buffer) {
            free(arm64_data->asm_buffer);
        }
        if (arm64_data->active_regions) {
            braggi_vector_destroy(arm64_data->active_regions);
        }
        free(arm64_data);
    }
    
    // Free code buffer
    if (generator->code_buffer) {
        free(generator->code_buffer);
    }
    
    // Free symbols and relocations
    // (In a real implementation, this would be more complex)
    if (generator->symbols) {
        free(generator->symbols);
    }
    if (generator->relocations) {
        free(generator->relocations);
    }
}

// Generate code for region creation
static bool generate_region_create(CodeGenerator* generator, Region* region) {
    if (!generator || !region) return false;
    
    ARM64Data* arm64_data = (ARM64Data*)generator->target_data;
    
    // Generate assembly for region creation
    char asm_buffer[256];
    snprintf(asm_buffer, sizeof(asm_buffer),
             "    // Create region %u (%s)\n"
             "    mov x0, #%u             // Region size\n"
             "    mov x1, #%u             // Region regime (%s)\n"
             "    bl _braggi_region_create\n"
             "    str x0, [x29, #%d]      // Store region handle\n\n",
             region->id,
             region->name ? region->name : "unnamed",
             (unsigned int)region->size,
             (unsigned int)region->regime,
             regime_type_to_string(region->regime),
             arm64_data->stack_size);
    
    // Update stack size for region handle
    arm64_data->stack_size += 8;  // 64-bit region handle
    
    // Add to active regions
    braggi_vector_push_back(arm64_data->active_regions, region);
    
    return append_asm(arm64_data, asm_buffer);
}

// Generate code for region destruction
static bool generate_region_destroy(CodeGenerator* generator, Region* region) {
    if (!generator || !region) return false;
    
    ARM64Data* arm64_data = (ARM64Data*)generator->target_data;
    
    // Find region offset in stack
    int region_offset = -1;
    for (size_t i = 0; i < braggi_vector_size(arm64_data->active_regions); i++) {
        if (braggi_vector_get(arm64_data->active_regions, i) == region) {
            region_offset = (int)(i * 8);
            break;
        }
    }
    
    if (region_offset == -1) return false;  // Region not found
    
    // Generate assembly for region destruction
    char asm_buffer[256];
    snprintf(asm_buffer, sizeof(asm_buffer),
             "    // Destroy region %u (%s)\n"
             "    ldr x0, [x29, #%d]      // Load region handle\n"
             "    bl _braggi_region_destroy\n\n",
             region->id,
             region->name ? region->name : "unnamed",
             region_offset);
    
    // Remove from active regions
    for (size_t i = 0; i < braggi_vector_size(arm64_data->active_regions); i++) {
        if (braggi_vector_get(arm64_data->active_regions, i) == region) {
            braggi_vector_remove(arm64_data->active_regions, i);
            break;
        }
    }
    
    return append_asm(arm64_data, asm_buffer);
}

// Generate code for allocation within a region
static bool generate_region_alloc(CodeGenerator* generator, Region* region, size_t size, 
                               uint32_t source_pos, const char* label) {
    if (!generator || !region) return false;
    
    ARM64Data* arm64_data = (ARM64Data*)generator->target_data;
    
    // Find region offset in stack
    int region_offset = -1;
    for (size_t i = 0; i < braggi_vector_size(arm64_data->active_regions); i++) {
        if (braggi_vector_get(arm64_data->active_regions, i) == region) {
            region_offset = (int)(i * 8);
            break;
        }
    }
    
    if (region_offset == -1) return false;  // Region not found
    
    // Generate assembly for allocation
    char asm_buffer[256];
    snprintf(asm_buffer, sizeof(asm_buffer),
             "    // Allocate %zu bytes in region %u (%s)\n"
             "    ldr x0, [x29, #%d]      // Load region handle\n"
             "    mov x1, #%zu            // Size to allocate\n"
             "    mov x2, #%u             // Source position\n"
             "    adrp x3, L_%s@PAGE     // Label\n"
             "    add x3, x3, L_%s@PAGEOFF\n"
             "    bl _braggi_region_alloc\n"
             "    str x0, [x29, #%d]      // Store allocation pointer\n\n",
             size,
             region->id,
             region->name ? region->name : "unnamed",
             region_offset,
             size,
             source_pos,
             label ? label : "unnamed",
             label ? label : "unnamed",
             arm64_data->stack_size);
    
    // Update stack size for allocation pointer
    arm64_data->stack_size += 8;  // 64-bit pointer
    
    // Also add the label if provided
    if (label) {
        char label_buffer[128];
        snprintf(label_buffer, sizeof(label_buffer),
                 "L_%s: .asciz \"%s\"\n",
                 label, label);
        append_asm(arm64_data, label_buffer);
    }
    
    return append_asm(arm64_data, asm_buffer);
}

// Generate code for periscope creation
static bool generate_periscope_create(CodeGenerator* generator, Region* source, Region* target,
                                  PeriscopeDirection direction) {
    if (!generator || !source || !target) return false;
    
    ARM64Data* arm64_data = (ARM64Data*)generator->target_data;
    
    // Find region offsets in stack
    int source_offset = -1;
    int target_offset = -1;
    
    for (size_t i = 0; i < braggi_vector_size(arm64_data->active_regions); i++) {
        Region* region = braggi_vector_get(arm64_data->active_regions, i);
        if (region == source) {
            source_offset = (int)(i * 8);
        } else if (region == target) {
            target_offset = (int)(i * 8);
        }
    }
    
    if (source_offset == -1 || target_offset == -1) return false;  // Regions not found
    
    // Generate assembly for periscope creation
    char asm_buffer[256];
    snprintf(asm_buffer, sizeof(asm_buffer),
             "    // Create periscope from region %u to region %u (direction: %s)\n"
             "    ldr x0, [x29, #%d]      // Load source region handle\n"
             "    ldr x1, [x29, #%d]      // Load target region handle\n"
             "    mov x2, #%u             // Direction\n"
             "    bl _braggi_region_create_periscope\n"
             "    str x0, [x29, #%d]      // Store result status\n\n",
             source->id,
             target->id,
             (direction == PERISCOPE_IN) ? "IN" : "OUT",
             source_offset,
             target_offset,
             (unsigned int)direction,
             arm64_data->stack_size);
    
    // Update stack size for result status
    arm64_data->stack_size += 8;  // 64-bit status
    
    return append_asm(arm64_data, asm_buffer);
}

// Generate function prologue
static bool generate_function_prologue(CodeGenerator* generator, const char* func_name) {
    if (!generator || !func_name) return false;
    
    ARM64Data* arm64_data = (ARM64Data*)generator->target_data;
    
    // Increase function depth
    arm64_data->func_depth++;
    
    // Generate assembly for function prologue
    char asm_buffer[256];
    snprintf(asm_buffer, sizeof(asm_buffer),
             ".globl _%s\n"
             "_%s:\n"
             "    stp x29, x30, [sp, #-16]!  // Save frame pointer and link register\n"
             "    mov x29, sp                // Set up frame pointer\n\n",
             func_name, func_name);
    
    return append_asm(arm64_data, asm_buffer);
}

// Generate function epilogue
static bool generate_function_epilogue(CodeGenerator* generator) {
    if (!generator) return false;
    
    ARM64Data* arm64_data = (ARM64Data*)generator->target_data;
    
    // Decrease function depth
    arm64_data->func_depth--;
    
    // Generate assembly for function epilogue
    const char* epilogue = 
        "    mov sp, x29                // Restore stack pointer\n"
        "    ldp x29, x30, [sp], #16    // Restore frame pointer and link register\n"
        "    ret                        // Return\n\n";
    
    return append_asm(arm64_data, epilogue);
}

// Main code generation entry point
static bool arm64_generate(CodeGenerator* generator, EntropyField* field) {
    if (!generator || !field) return false;
    
    ARM64Data* arm64_data = (ARM64Data*)generator->target_data;
    
    // Reset assembly buffer
    arm64_data->asm_size = 0;
    arm64_data->asm_buffer[0] = '\0';
    
    // Start with assembly directives
    append_asm(arm64_data, "// ARM64 assembly generated by Braggi Compiler\n\n");
    append_asm(arm64_data, ".section __TEXT,__text,regular,pure_instructions\n");
    append_asm(arm64_data, ".align 4\n\n");
    
    // In a real implementation, we would traverse the collapsed entropy field
    // and generate code based on the cell states.
    
    // For demonstration purposes, let's generate a simple program that:
    // 1. Creates two regions
    // 2. Allocates memory in the regions
    // 3. Creates a periscope
    // 4. Uses the memory
    // 5. Destroys the regions
    
    // Create a main function
    generate_function_prologue(generator, "main");
    
    // Create sample regions
    Region region1 = {
        .id = 1,
        .name = "stack_region",
        .size = 1024,
        .regime = REGIME_FILO,
        .allocations = NULL,
        .num_allocations = 0
    };
    
    Region region2 = {
        .id = 2,
        .name = "heap_region",
        .size = 4096,
        .regime = REGIME_RAND,
        .allocations = NULL,
        .num_allocations = 0
    };
    
    // Generate code to create regions
    generate_region_create(generator, &region1);
    generate_region_create(generator, &region2);
    
    // Allocate memory in regions
    generate_region_alloc(generator, &region1, 64, 100, "stack_buffer");
    generate_region_alloc(generator, &region2, 256, 200, "heap_buffer");
    
    // Create a periscope from stack to heap (FILO->RAND is allowed)
    generate_periscope_create(generator, &region1, &region2, PERISCOPE_OUT);
    
    // Use the memory (simplified for demo)
    append_asm(arm64_data,
               "    // Use the allocated memory\n"
               "    ldr x0, [x29, #16]      // Load stack buffer pointer\n"
               "    mov w1, #42             // Sample value\n"
               "    str w1, [x0]            // Store value to stack buffer\n\n"
               
               "    ldr x0, [x29, #24]      // Load heap buffer pointer\n"
               "    ldr x1, [x29, #16]      // Load stack buffer pointer\n"
               "    ldr w1, [x1]            // Load value from stack buffer\n"
               "    str w1, [x0]            // Store value to heap buffer (via periscope)\n\n");
    
    // Destroy regions (reverse order of creation)
    generate_region_destroy(generator, &region2);
    generate_region_destroy(generator, &region1);
    
    // Return 0
    append_asm(arm64_data,
               "    // Return 0\n"
               "    mov x0, #0\n\n");
    
    // End main function
    generate_function_epilogue(generator);
    
    // Store the generated assembly as the code buffer
    if (generator->code_buffer) {
        free(generator->code_buffer);
    }
    generator->code_buffer = strdup(arm64_data->asm_buffer);
    generator->code_size = arm64_data->asm_size;
    
    return true;
}

// Emit the generated code to a file
static bool arm64_emit(CodeGenerator* generator, const char* filename, OutputFormat format) {
    if (!generator || !filename) return false;
    
    // For simplicity, we'll just write the assembly to a file
    // In a real implementation, we would handle different output formats
    
    // Open the output file
    FILE* output = fopen(filename, "w");
    if (!output) return false;
    
    // Write the assembly
    if (generator->code_buffer) {
        fputs((const char*)generator->code_buffer, output);
    }
    
    // Close the file
    fclose(output);
    
    return true;
}

// Create an ARM64 code generator
CodeGenerator* braggi_codegen_create_arm64(ErrorHandler* error_handler) {
    CodeGenerator* generator = (CodeGenerator*)malloc(sizeof(CodeGenerator));
    if (!generator) return NULL;
    
    // Initialize fields
    generator->arch = TARGET_ARM64;
    generator->os = OS_UNKNOWN;  // Will be set later
    generator->target_data = NULL;
    generator->error_handler = error_handler;
    generator->init = arm64_init;
    generator->destroy = arm64_destroy;
    generator->generate = arm64_generate;
    generator->emit = arm64_emit;
    generator->code_buffer = NULL;
    generator->code_size = 0;
    generator->symbols = NULL;
    generator->relocations = NULL;
    
    // Initialize the generator
    if (!generator->init(generator, error_handler)) {
        free(generator);
        return NULL;
    }
    
    return generator;
}

// Register ARM64 backend with the code generation system
void braggi_register_arm64_backend(void) {
    // Register ARM64-specific functions with the code generator
    // This would be called during compiler initialization
} 