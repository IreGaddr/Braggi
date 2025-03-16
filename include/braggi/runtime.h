/*
 * Braggi - Runtime Support Library
 * 
 * "Even the fanciest quantum-inspired compiler needs a sturdy runtime,
 * just like a rodeo cowboy needs a good pair of boots!"
 * - Irish-Texan runtime wisdom
 */

#ifndef BRAGGI_RUNTIME_H
#define BRAGGI_RUNTIME_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Runtime region handle
typedef struct BraggiRegion* BraggiRegionHandle;

// Runtime periscope handle
typedef struct BraggiPeriscope* BraggiPeriscopeHandle;

// Region regime types (same as in region.h, duplicated for runtime use)
typedef enum {
    BRAGGI_REGIME_FIFO,    // First-In-First-Out
    BRAGGI_REGIME_FILO,    // First-In-Last-Out (stack-like)
    BRAGGI_REGIME_SEQ,     // Sequential access
    BRAGGI_REGIME_RAND     // Random access
} BraggiRegimeType;

// Periscope direction (same as in region.h)
typedef enum {
    BRAGGI_PERISCOPE_IN,
    BRAGGI_PERISCOPE_OUT,
    BRAGGI_PERISCOPE_BIDIRECTIONAL
} BraggiPeriscopeDirection;

// Runtime error codes
typedef enum BraggiRuntimeError {
    BRAGGI_RT_SUCCESS = 0,  // Changed from BRAGGI_SUCCESS to avoid conflict
    BRAGGI_RT_ERROR_INVALID_HANDLE,
    BRAGGI_RT_ERROR_OUT_OF_MEMORY,
    BRAGGI_RT_ERROR_INVALID_SIZE,
    BRAGGI_RT_ERROR_INVALID_REGIME,
    BRAGGI_RT_ERROR_INVALID_PERISCOPE,
    BRAGGI_RT_ERROR_INCOMPATIBLE_REGIMES,
    BRAGGI_RT_ERROR_INVALID_ACCESS,
    BRAGGI_RT_ERROR_REGION_FULL,
    BRAGGI_RT_ERROR_INVALID_ALLOCATION,
    BRAGGI_RT_ERROR_DANGLING_REFERENCE
} BraggiRuntimeError;

// Region management functions
BraggiRegionHandle braggi_rt_region_create(size_t size, BraggiRegimeType regime);
void braggi_rt_region_destroy(BraggiRegionHandle region);

// Memory allocation functions
void* braggi_rt_region_alloc(BraggiRegionHandle region, size_t size, uint32_t source_pos, const char* label);
void braggi_rt_region_free(BraggiRegionHandle region, void* ptr);
bool braggi_rt_region_contains(BraggiRegionHandle region, void* ptr);

// Periscope functions
BraggiRuntimeError braggi_rt_region_create_periscope(BraggiRegionHandle source, 
                                               BraggiRegionHandle target,
                                               BraggiPeriscopeDirection direction);
void braggi_rt_region_destroy_periscope(BraggiPeriscopeHandle periscope);

// Runtime statistics
size_t braggi_rt_region_get_used_memory(BraggiRegionHandle region);
size_t braggi_rt_region_get_free_memory(BraggiRegionHandle region);
size_t braggi_rt_region_get_allocation_count(BraggiRegionHandle region);
const char* braggi_rt_error_string(BraggiRuntimeError error);

#endif /* BRAGGI_RUNTIME_H */ 