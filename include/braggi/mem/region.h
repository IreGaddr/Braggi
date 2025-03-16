#ifndef BRAGGI_MEM_REGION_H
#define BRAGGI_MEM_REGION_H

#include <stddef.h>
#include <stdbool.h>
#include "braggi/region_types.h"  // Include the existing region types

/*
 * "Memory regions: like Texas ranches for your bytes.
 * Big, efficient, and with clearly marked boundaries."
 * - A memory-conscious Texan-Irish programmer
 */

// Region structure
typedef struct Region {
    void* memory;          // Base pointer to the memory block
    size_t size;           // Total size of the region
    size_t used;           // Amount of memory currently in use
    RegimeType regime;     // Memory access regime
    bool owns_memory;      // Whether the region owns the memory (responsible for freeing)
} Region;

// Region creation and destruction functions
Region* braggi_mem_region_create(size_t size, RegimeType regime);
Region* braggi_mem_region_create_from_buffer(void* buffer, size_t size, RegimeType regime, bool take_ownership);
void braggi_mem_region_destroy(Region* region);

// Memory allocation functions
void* braggi_mem_region_alloc(Region* region, size_t size);
void* braggi_mem_region_calloc(Region* region, size_t count, size_t size);
void* braggi_mem_region_realloc(Region* region, void* ptr, size_t old_size, size_t new_size);

// Region status functions
size_t braggi_mem_region_get_used(const Region* region);
size_t braggi_mem_region_get_available(const Region* region);
float braggi_mem_region_get_usage_percent(const Region* region);
bool braggi_mem_region_can_allocate(const Region* region, size_t size);

// Reset the region (free all allocations)
void braggi_mem_region_reset(Region* region);

#endif // BRAGGI_MEM_REGION_H 