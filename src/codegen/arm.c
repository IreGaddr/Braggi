/*
 * Braggi - ARM (32-bit) Code Generation
 *
 * "32-bit ARM is like Texas BBQ - it's been around a long time,
 * and it's still mighty good for what it does!" - Irish-Texan proverb
 */

#include "braggi/codegen.h"
#include "braggi/codegen_arch.h"
#include "braggi/region.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ARM register information
typedef enum {
    ARM_REG_R0,
    ARM_REG_R1,
    ARM_REG_R2,
    ARM_REG_R3,
    ARM_REG_R4,
    ARM_REG_R5,
    ARM_REG_R6,
    ARM_REG_R7,
    ARM_REG_R8,
    ARM_REG_R9,
    ARM_REG_R10,
    ARM_REG_R11,  // Frame pointer
    ARM_REG_R12,  // IP (Intra-Procedure call scratch register)
    ARM_REG_SP,   // Stack pointer (R13)
    ARM_REG_LR,   // Link register (R14)
    ARM_REG_PC    // Program counter (R15)
} ARMRegister;

// ARM instruction set
typedef enum {
    ARM_MODE_ARM,    // Standard 32-bit ARM mode
    ARM_MODE_THUMB,  // 16-bit Thumb mode
    ARM_MODE_THUMB2  // Enhanced Thumb mode
} ARMMode;

// ARM-specific code generation data
typedef struct {
    // Current ARM mode
    ARMMode mode;
    
    // Register allocation tracking
    uint32_t used_registers;
    
    // Current stack frame size
    int32_t stack_size;
    
    // Current function depth
    int32_t func_depth;
    
    // Region tracking
    Vector* active_regions;
    
    // Assembly output buffer
    char* asm_buffer;
    size_t asm_size;
    size_t asm_capacity;
} ARMData;

// Forward declarations of internal functions
static bool arm_init(CodeGenerator* generator, ErrorHandler* error_handler);
static void arm_destroy(CodeGenerator* generator);
static bool arm_generate(CodeGenerator* generator, EntropyField* field);
static bool arm_emit(CodeGenerator* generator, const char* filename, OutputFormat format);
static bool arm_register_function(CodeGenerator* generator, const char* name, void* func_ptr);
static bool arm_optimize(CodeGenerator* generator, int level);
static bool arm_generate_debug_info(CodeGenerator* generator, bool enable);

// Append assembly text to the buffer
static bool append_asm(ARMData* data, const char* text) {
    if (!data || !text) {
        return false;
    }
    
    size_t len = strlen(text);
    
    // Check if we need to resize the buffer
    if (data->asm_size + len + 1 > data->asm_capacity) {
        size_t new_capacity = data->asm_capacity * 2;
        if (new_capacity < data->asm_size + len + 1) {
            new_capacity = data->asm_size + len + 1 + 1024;  // Add some extra space
        }
        
        char* new_buffer = realloc(data->asm_buffer, new_capacity);
        if (!new_buffer) {
            return false;  // Allocation failed
        }
        
        data->asm_buffer = new_buffer;
        data->asm_capacity = new_capacity;
    }
    
    // Append the text
    strcpy(data->asm_buffer + data->asm_size, text);
    data->asm_size += len;
    
    return true;
}

// Generate function prologue
static bool generate_function_prologue(CodeGenerator* generator, const char* func_name) {
    if (!generator || !generator->arch_data || !func_name) {
        return false;
    }
    
    ARMData* data = (ARMData*)generator->arch_data;
    
    // Standard ARM function prologue
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), 
             "\n.global %s\n"
             "%s:\n"
             "    push {fp, lr}\n"  // Save frame pointer and link register
             "    add fp, sp, #4\n" // Set up frame pointer
             "    sub sp, sp, #%d\n", // Reserve stack space
             func_name, func_name, 
             data->stack_size);
    
    if (!append_asm(data, buffer)) {
        return false;
    }
    
    // Update function depth tracker
    data->func_depth++;
    
    return true;
}

// Generate function epilogue
static bool generate_function_epilogue(CodeGenerator* generator) {
    if (!generator || !generator->arch_data) {
        return false;
    }
    
    ARMData* data = (ARMData*)generator->arch_data;
    
    // Standard ARM function epilogue
    if (!append_asm(data, "    sub sp, fp, #4\n"    // Restore stack pointer
                          "    pop {fp, pc}\n")) {  // Restore frame pointer and return
        return false;
    }
    
    // Update function depth tracker
    data->func_depth--;
    
    return true;
}

// Initialize the ARM code generator
static bool arm_init(CodeGenerator* generator, ErrorHandler* error_handler) {
    if (!generator) {
        return false;
    }
    
    // Create and initialize ARM-specific data
    ARMData* data = malloc(sizeof(ARMData));
    if (!data) {
        if (error_handler) {
            error_handler->error(error_handler, "Failed to allocate memory for ARM code generator");
        }
        return false;
    }
    
    // Initialize the data
    memset(data, 0, sizeof(ARMData));
    data->mode = ARM_MODE_ARM;  // Default to standard ARM mode
    
    // Initialize assembly buffer
    data->asm_capacity = 4096;  // Initial buffer size
    data->asm_buffer = malloc(data->asm_capacity);
    if (!data->asm_buffer) {
        free(data);
        if (error_handler) {
            error_handler->error(error_handler, "Failed to allocate memory for ARM assembly buffer");
        }
        return false;
    }
    data->asm_buffer[0] = '\0';
    data->asm_size = 0;
    
    // Initialize region tracking
    data->active_regions = braggi_vector_create(sizeof(Region*));
    if (!data->active_regions) {
        free(data->asm_buffer);
        free(data);
        if (error_handler) {
            error_handler->error(error_handler, "Failed to allocate memory for ARM region tracking");
        }
        return false;
    }
    
    // Assign the data to the generator
    generator->arch_data = data;
    
    return true;
}

// Clean up the ARM code generator
static void arm_destroy(CodeGenerator* generator) {
    if (!generator || !generator->arch_data) {
        return;
    }
    
    ARMData* data = (ARMData*)generator->arch_data;
    
    // Free assembly buffer
    if (data->asm_buffer) {
        free(data->asm_buffer);
        data->asm_buffer = NULL;
    }
    
    // Free active regions vector
    if (data->active_regions) {
        braggi_vector_destroy(data->active_regions);
        data->active_regions = NULL;
    }
    
    // Free the data itself
    free(data);
    generator->arch_data = NULL;
}

// Generate code from the entropy field
static bool arm_generate(CodeGenerator* generator, EntropyField* field) {
    if (!generator || !generator->arch_data || !field) {
        return false;
    }
    
    ARMData* data = (ARMData*)generator->arch_data;
    
    // Header comments and directives
    if (!append_asm(data, "@ Generated by Braggi Compiler - ARM Backend\n")) {
        return false;
    }
    
    if (!append_asm(data, ".syntax unified\n")) {
        return false;
    }
    
    if (data->mode == ARM_MODE_THUMB || data->mode == ARM_MODE_THUMB2) {
        if (!append_asm(data, ".thumb\n")) {
            return false;
        }
    } else {
        if (!append_asm(data, ".arm\n")) {
            return false;
        }
    }
    
    // Start main function
    data->stack_size = 16;  // Default stack size for main
    if (!generate_function_prologue(generator, "main")) {
        return false;
    }
    
    // Simple return 0 for now
    if (!append_asm(data, "    mov r0, #0\n")) {
        return false;
    }
    
    // End main function
    if (!generate_function_epilogue(generator)) {
        return false;
    }
    
    return true;
}

// Write output to a file
static bool arm_emit(CodeGenerator* generator, const char* filename, OutputFormat format) {
    if (!generator || !generator->arch_data || !filename) {
        return false;
    }
    
    ARMData* data = (ARMData*)generator->arch_data;
    
    // Open output file
    FILE* file = fopen(filename, "w");
    if (!file) {
        return false;
    }
    
    // Write assembly
    if (data->asm_buffer && data->asm_size > 0) {
        fprintf(file, "%s", data->asm_buffer);
    } else {
        fprintf(file, "@ Empty ARM assembly generated by Braggi\n");
    }
    
    fclose(file);
    return true;
}

// Register an external function
static bool arm_register_function(CodeGenerator* generator, const char* name, void* func_ptr) {
    if (!generator || !generator->arch_data || !name || !func_ptr) {
        return false;
    }
    
    // This is a placeholder for future implementation
    // In a real implementation, we would register the function in a way
    // that allows it to be called from generated code
    
    return true;
}

// Enable optimization
static bool arm_optimize(CodeGenerator* generator, int level) {
    if (!generator || !generator->arch_data || level < 0 || level > 3) {
        return false;
    }
    
    // This is a placeholder for future implementation
    // In a real implementation, we would adjust various optimization settings
    // based on the requested level
    
    return true;
}

// Enable/disable debug information generation
static bool arm_generate_debug_info(CodeGenerator* generator, bool enable) {
    if (!generator || !generator->arch_data) {
        return false;
    }
    
    // This is a placeholder for future implementation
    // In a real implementation, we would enable/disable debug info generation
    
    return true;
}

// ARM initialization function for braggi_codegen.h
void braggi_codegen_arm_init(void) {
    // This would register our ARM backend with the central code generator system
}

// ARM backend registration function for codegen_arch.h
void braggi_register_arm_backend(void) {
    // Register with code generator system
    // This would register the ARM functions with a central registry
    CodeGenerator generator;
    
    generator.name = "arm";
    generator.description = "ARM (32-bit) code generator";
    generator.arch_data = NULL;
    
    generator.init = arm_init;
    generator.destroy = arm_destroy;
    generator.generate = arm_generate;
    generator.emit = arm_emit;
    generator.register_function = arm_register_function;
    generator.optimize = arm_optimize;
    generator.generate_debug_info = arm_generate_debug_info;
    
    // In a real implementation, we would register this with a central system
    // For now, just print that we've initialized
    printf("Braggi ARM backend initialized\n");
} 