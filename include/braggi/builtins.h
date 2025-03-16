/*
 * Braggi - Builtin Functions
 * 
 * "In the realm of programming languages, builtins are like the hearty Irish stew 
 * of a Texas ranch - filling, satisfying, and gets the job done!"
 */

#ifndef BRAGGI_BUILTINS_H
#define BRAGGI_BUILTINS_H

#include "braggi/runtime.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Opaque type for Braggi values
typedef struct BraggiValue BraggiValue;

// Builtin function type
typedef BraggiValue* (*BraggiBuiltinFunc)(BraggiValue** args, size_t arg_count, void* context);

// Value types
typedef enum {
    BRAGGI_NULL,
    BRAGGI_BOOL,
    BRAGGI_INT,
    BRAGGI_FLOAT,
    BRAGGI_STRING,
    BRAGGI_ARRAY,
    BRAGGI_MAP,
    BRAGGI_OBJECT,
    BRAGGI_FUNCTION,
    BRAGGI_REGION_REF,
    BRAGGI_SUPERPOSITION
} BraggiValueType;

// Value structure
struct BraggiValue {
    BraggiValueType type;
    
    // Region this value belongs to
    BraggiRegionHandle region;
    
    union {
        bool bool_val;
        int64_t int_val;
        double float_val;
        struct {
            char* data;
            size_t length;
        } string_val;
        struct {
            BraggiValue** elements;
            size_t length;
            size_t capacity;
        } array_val;
        struct {
            // Implementation depends on map structure
            void* map_data;
        } map_val;
        struct {
            // Implementation depends on object system
            void* object_data;
            char* type_name;
        } object_val;
        struct {
            BraggiBuiltinFunc builtin;
            void* context;
        } builtin_func_val;
        struct {
            // For user-defined functions
            void* func_data;
        } func_val;
        BraggiRegionHandle region_ref_val;
        struct {
            // Implementation depends on superposition mechanism
            void* superposition_data;
        } superposition_val;
    };
};

// Value creation functions
BraggiValue* braggi_value_create_null(BraggiRegionHandle region);
BraggiValue* braggi_value_create_bool(BraggiRegionHandle region, bool value);
BraggiValue* braggi_value_create_int(BraggiRegionHandle region, int64_t value);
BraggiValue* braggi_value_create_float(BraggiRegionHandle region, double value);
BraggiValue* braggi_value_create_string(BraggiRegionHandle region, const char* value);
BraggiValue* braggi_value_create_array(BraggiRegionHandle region, size_t initial_capacity);
BraggiValue* braggi_value_create_map(BraggiRegionHandle region);
BraggiValue* braggi_value_create_builtin(BraggiRegionHandle region, BraggiBuiltinFunc func, void* context);
BraggiValue* braggi_value_create_superposition(BraggiRegionHandle region);

// Value access functions
bool braggi_value_as_bool(BraggiValue* value);
int64_t braggi_value_as_int(BraggiValue* value);
double braggi_value_as_float(BraggiValue* value);
const char* braggi_value_as_string(BraggiValue* value);
BraggiValue* braggi_value_array_get(BraggiValue* array, size_t index);
void braggi_value_array_set(BraggiValue* array, size_t index, BraggiValue* value);
BraggiValue* braggi_value_map_get(BraggiValue* map, BraggiValue* key);
void braggi_value_map_set(BraggiValue* map, BraggiValue* key, BraggiValue* value);

// Value operations
bool braggi_value_equals(BraggiValue* a, BraggiValue* b);
BraggiValue* braggi_value_add(BraggiValue* a, BraggiValue* b, BraggiRegionHandle region);
BraggiValue* braggi_value_call(BraggiValue* func, BraggiValue** args, size_t arg_count);

// Superposition operations
void braggi_value_superposition_add_possibility(BraggiValue* superposition, BraggiValue* value, double probability);
double braggi_value_superposition_get_probability(BraggiValue* superposition, BraggiValue* value);
BraggiValue* braggi_value_superposition_collapse(BraggiValue* superposition);

// Builtin function registration
typedef struct BraggiBuiltinRegistry BraggiBuiltinRegistry;
BraggiBuiltinRegistry* braggi_builtin_registry_create();
void braggi_builtin_registry_destroy(BraggiBuiltinRegistry* registry);
void braggi_builtin_registry_register(BraggiBuiltinRegistry* registry, const char* name, BraggiBuiltinFunc func, void* context);
BraggiBuiltinFunc braggi_builtin_registry_lookup(BraggiBuiltinRegistry* registry, const char* name, void** out_context);

// Standard library initialization
bool braggi_initialize_stdlib(BraggiBuiltinRegistry* registry);

/*
 * Standard Library Usage:
 * 
 * The Braggi standard library consists of several modules:
 * 
 * - core: Basic utilities and type operations
 * - io: Input/output functions
 * - string: String manipulation
 * - math: Mathematical functions
 * - collections: Array and map operations
 * - region: Region management
 * - quantum: Quantum-inspired operations
 * - error: Error handling
 * 
 * These modules can be imported in Braggi code using:
 * 
 * import string from "lib/string.bg";
 * 
 * The compiler automatically imports core and io modules.
 * 
 * Builtin functions (prefixed with __builtin_) are implemented in C
 * and provide the foundation for the standard library. These functions
 * should not be called directly from user code - use the standard
 * library wrappers instead.
 */

#endif /* BRAGGI_BUILTINS_H */ 