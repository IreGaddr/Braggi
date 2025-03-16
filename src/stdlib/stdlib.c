/*
 * Braggi - Standard Library Management
 * 
 * "A good standard library is like the tool belt of a master craftsman -
 * everything you need within arm's reach!"
 * - Irish-Texan programming wisdom
 */

// Include the complete context definition first
#include "braggi/braggi_context.h"  
#include "braggi/braggi.h"
#include "braggi/builtins/builtins.h"
#include "braggi/stdlib.h"
#include "braggi/error.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>

// Forward declarations for registry functions
static void register_math_builtins(BraggiBuiltinRegistry* registry);
static void register_string_builtins(BraggiBuiltinRegistry* registry);
static void register_io_builtins(BraggiBuiltinRegistry* registry);
static void register_system_builtins(BraggiBuiltinRegistry* registry);

// Forward declare the stub implementations of the example functions
static BraggiValue* math_add(BraggiValue** args, size_t arg_count, void* context);
static BraggiValue* math_subtract(BraggiValue** args, size_t arg_count, void* context);
static BraggiValue* string_length(BraggiValue** args, size_t arg_count, void* context);
static BraggiValue* io_print(BraggiValue** args, size_t arg_count, void* context);
static BraggiValue* system_exit(BraggiValue** args, size_t arg_count, void* context);

// Default library paths to search
static const char* default_library_paths[] = {
    "./lib",           // Current directory lib
    "../lib",          // Parent directory lib
    "/usr/local/lib/braggi",  // System lib
    NULL
};

// Environment variable for library path
#define BRAGGI_LIB_PATH_ENV "BRAGGI_LIB_PATH"

// Check if a file exists
static bool file_exists(const char* path) {
    struct stat buffer;
    return (stat(path, &buffer) == 0);
}

// Find a standard library file
char* braggi_stdlib_find_file(const char* name) {
    // Buffer for constructing paths
    char path_buffer[1024];
    
    // Check for environment variable first
    const char* env_path = getenv(BRAGGI_LIB_PATH_ENV);
    if (env_path) {
        snprintf(path_buffer, sizeof(path_buffer), "%s/%s", env_path, name);
        if (file_exists(path_buffer)) {
            return strdup(path_buffer);
        }
    }
    
    // Check default paths
    for (int i = 0; default_library_paths[i] != NULL; i++) {
        snprintf(path_buffer, sizeof(path_buffer), "%s/%s", default_library_paths[i], name);
        if (file_exists(path_buffer)) {
            return strdup(path_buffer);
        }
    }
    
    // Not found
    return NULL;
}

// Debug function to show library paths
void braggi_stdlib_debug_paths(void) {
    printf("Braggi standard library paths:\n");
    printf("  Environment: %s\n", getenv(BRAGGI_LIB_PATH_ENV) ? getenv(BRAGGI_LIB_PATH_ENV) : "(not set)");
    for (int i = 0; default_library_paths[i] != NULL; i++) {
        printf("  Default: %s (%s)\n", default_library_paths[i], 
               file_exists(default_library_paths[i]) ? "exists" : "not found");
    }
}

// Use the existing type from builtins.h:
// typedef BraggiValue* (*BraggiBuiltinFunc)(BraggiValue** args, size_t arg_count, void* context);

// Builtin function entry
typedef struct BraggiBuiltinEntry {
    const char* name;            // Function name
    BraggiBuiltinFunc function;  // Function pointer
    const char* description;     // Function description
    const char* signature;       // Function type signature
    void* context;               // Context for the function
    struct BraggiBuiltinEntry* next;  // Next in linked list
} BraggiBuiltinEntry;

// Actual implementation of BraggiBuiltinRegistry structure
typedef struct BraggiBuiltinRegistry {
    BraggiBuiltinEntry* functions;  // Linked list of functions
    size_t count;                   // Number of registered functions
} BraggiBuiltinRegistry;

// Proper implementation for builtin registry creation
BraggiBuiltinRegistry* braggi_builtin_registry_create(void) {
    // Allocate and initialize a registry
    BraggiBuiltinRegistry* registry = (BraggiBuiltinRegistry*)malloc(sizeof(BraggiBuiltinRegistry));
    if (registry) {
        registry->functions = NULL;
        registry->count = 0;
    }
    return registry;
}

// Helper to register a single builtin function
static bool register_builtin(BraggiBuiltinRegistry* registry, 
                             const char* name,
                             BraggiBuiltinFunc func, 
                             const char* description,
                             const char* signature,
                             void* context) {
    if (!registry || !name || !func) {
        return false;
    }
    
    // Create a new entry
    BraggiBuiltinEntry* entry = (BraggiBuiltinEntry*)malloc(sizeof(BraggiBuiltinEntry));
    if (!entry) {
        return false;
    }
    
    // Initialize the entry
    entry->name = strdup(name);
    entry->function = func;
    entry->description = description ? strdup(description) : NULL;
    entry->signature = signature ? strdup(signature) : NULL;
    entry->context = context;
    
    // Add to the list (at the beginning for simplicity)
    entry->next = registry->functions;
    registry->functions = entry;
    registry->count++;
    
    return true;
}

// Proper implementation for builtin registry destruction
void braggi_builtin_registry_destroy(BraggiBuiltinRegistry* registry) {
    if (!registry) {
        return;
    }
    
    // Free all entries
    BraggiBuiltinEntry* entry = registry->functions;
    while (entry) {
        BraggiBuiltinEntry* next = entry->next;
        
        // Free strings
        if (entry->name) free((void*)entry->name);
        if (entry->description) free((void*)entry->description);
        if (entry->signature) free((void*)entry->signature);
        
        // Free the entry
        free(entry);
        
        entry = next;
    }
    
    // Free the registry
    free(registry);
}

// Example builtin functions matching the real signature
static BraggiValue* math_add(BraggiValue** args, size_t arg_count, void* context) {
    // In a real implementation, this would add two numbers
    printf("math.add called with %zu args\n", arg_count);
    (void)args;
    (void)context;
    return NULL; // Return a BraggiValue* in a real implementation
}

static BraggiValue* math_subtract(BraggiValue** args, size_t arg_count, void* context) {
    // In a real implementation, this would subtract two numbers
    printf("math.subtract called with %zu args\n", arg_count);
    (void)args;
    (void)context;
    return NULL;
}

static BraggiValue* string_length(BraggiValue** args, size_t arg_count, void* context) {
    // In a real implementation, this would return the length of a string
    printf("string.length called with %zu args\n", arg_count);
    (void)args;
    (void)context;
    return NULL;
}

static BraggiValue* io_print(BraggiValue** args, size_t arg_count, void* context) {
    // In a real implementation, this would print output
    printf("io.print called with %zu args\n", arg_count);
    (void)args;
    (void)context;
    return NULL;
}

static BraggiValue* system_exit(BraggiValue** args, size_t arg_count, void* context) {
    // In a real implementation, this would exit the program
    printf("system.exit called with %zu args\n", arg_count);
    (void)args;
    (void)context;
    return NULL;
}

// Update registration functions to match correct function signatures
static void register_math_builtins(BraggiBuiltinRegistry* registry) {
    if (!registry) return;
    
    // Register math functions
    register_builtin(registry, "math.add", math_add, 
                    "Add two numbers", 
                    "func(a: number, b: number) -> number",
                    NULL);
                    
    register_builtin(registry, "math.subtract", math_subtract,
                    "Subtract two numbers",
                    "func(a: number, b: number) -> number",
                    NULL);
                    
    // More math functions would be added here
}

static void register_string_builtins(BraggiBuiltinRegistry* registry) {
    if (!registry) return;
    
    // Register string functions
    register_builtin(registry, "string.length", string_length,
                    "Get the length of a string",
                    "func(s: string) -> number",
                    NULL);
                    
    // More string functions would be added here
}

static void register_io_builtins(BraggiBuiltinRegistry* registry) {
    if (!registry) return;
    
    // Register I/O functions
    register_builtin(registry, "io.print", io_print,
                    "Print to standard output",
                    "func(value: any) -> void",
                    NULL);
                    
    // More I/O functions would be added here
}

static void register_system_builtins(BraggiBuiltinRegistry* registry) {
    if (!registry) return;
    
    // Register system functions
    register_builtin(registry, "system.exit", system_exit,
                    "Exit the program with a status code",
                    "func(code: number) -> void",
                    NULL);
                    
    // More system functions would be added here
}

// Load a standard library module
bool braggi_stdlib_load_module(BraggiContext* context, const char* module_name) {
    if (!context || !module_name) {
        return false;
    }
    
    // Find the module path - this is a simplified approach
    char file_path[1024];
    snprintf(file_path, sizeof(file_path), "modules/%s.bg", module_name);
    
    // Check if the file exists
    FILE* file = fopen(file_path, "r");
    if (!file) {
        const char* error_message = "Module not found";
        
        // Set error - using error handling API instead of direct field access
        braggi_error_report_ctx(
            ERROR_CATEGORY_SYSTEM, 
            ERROR_SEVERITY_ERROR, 
            0, 0, // No line/column for system errors
            NULL, 
            error_message,
            NULL);
        
        return false;
    }
    fclose(file);
    
    // Process the file - we'll need to implement this function
    bool result = false;
    // Commenting out for now - this function isn't defined yet
    // bool result = braggi_process_file(context, file_path);
    
    return result;
}

// Initialize the standard library for a context
bool braggi_stdlib_initialize(BraggiContext* context) {
    if (!context) {
        return false;
    }
    
    // Create a builtin registry
    BraggiBuiltinRegistry* registry = braggi_builtin_registry_create();
    if (!registry) {
        return false;
    }
    
    // Register all standard library functions
    register_math_builtins(registry);
    register_string_builtins(registry);
    register_io_builtins(registry);
    register_system_builtins(registry);
    
    // Since BraggiContext doesn't have a builtin_registry field,
    // we'll store it in the user_data field if available, or free it
    // In a real implementation, BraggiContext would have a dedicated field
    
    // Store registry in context somehow - for now, let's just hold onto it
    // and free it in cleanup
    static BraggiBuiltinRegistry* global_registry = NULL;
    
    // Clean up old registry if it exists
    if (global_registry) {
        braggi_builtin_registry_destroy(global_registry);
    }
    
    // Store the new registry
    global_registry = registry;
    
    return true;
}

// Clean up standard library resources
void braggi_stdlib_cleanup(BraggiContext* context) {
    if (!context) {
        return;
    }
    
    // In a real implementation, we would retrieve the registry from context
    // For now we use our static variable
    static BraggiBuiltinRegistry* global_registry = NULL;
    
    // Clean up the registry if it exists
    if (global_registry) {
        braggi_builtin_registry_destroy(global_registry);
        global_registry = NULL;
    }
}

// Update our utility lookup function to properly call the registry lookup
BraggiBuiltinFunc braggi_stdlib_lookup_builtin(BraggiContext* context, const char* name) {
    if (!context || !name) {
        return NULL;
    }
    
    // In a real implementation, we would retrieve the registry from context
    static BraggiBuiltinRegistry* global_registry = NULL;
    
    // Create the registry if it doesn't exist
    if (!global_registry) {
        global_registry = braggi_builtin_registry_create();
        if (!global_registry) return NULL;
    }
    
    // Call the registry lookup function with the correct parameters
    void* builtin_context = NULL;
    return braggi_builtin_registry_lookup(global_registry, name, &builtin_context);
} 