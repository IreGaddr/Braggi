#include "braggi/mem/region.h"
#include "braggi/mem/regime.h"
#include <stdlib.h>
#include <string.h>

/*
 * "Managing memory is like herding cattle - if you don't keep track of 'em,
 * they'll wander off and you'll have a heap of trouble."
 * - Texan Memory Rancher
 */

/*
 * Create a new memory region with the specified size and regime
 */
Region* braggi_mem_region_create(size_t size, RegimeType regime) {
    Region* region = (Region*)malloc(sizeof(Region));
    if (!region) {
        return NULL;
    }
    
    region->memory = malloc(size);
    if (!region->memory) {
        free(region);
        return NULL;
    }
    
    region->size = size;
    region->used = 0;
    region->regime = regime;
    region->owns_memory = true;
    
    return region;
}

/*
 * Create a new memory region from an existing buffer
 */
Region* braggi_mem_region_create_from_buffer(void* buffer, size_t size, RegimeType regime, bool take_ownership) {
    if (!buffer) {
        return NULL;
    }
    
    Region* region = (Region*)malloc(sizeof(Region));
    if (!region) {
        return NULL;
    }
    
    region->memory = buffer;
    region->size = size;
    region->used = 0;
    region->regime = regime;
    region->owns_memory = take_ownership;
    
    return region;
}

/*
 * Destroy a memory region and free its resources
 */
void braggi_mem_region_destroy(Region* region) {
    if (!region) {
        return;
    }
    
    if (region->owns_memory && region->memory) {
        free(region->memory);
    }
    
    free(region);
}

/*
 * Allocate memory from a region
 */
void* braggi_mem_region_alloc(Region* region, size_t size) {
    if (!region || size == 0) {
        return NULL;
    }
    
    // Check if there's enough space
    if (region->used + size > region->size) {
        return NULL;
    }
    
    // Allocate from the region
    void* ptr = (char*)region->memory + region->used;
    region->used += size;
    
    return ptr;
}

/*
 * Allocate zeroed memory from a region
 */
void* braggi_mem_region_calloc(Region* region, size_t count, size_t size) {
    size_t total_size = count * size;
    void* ptr = braggi_mem_region_alloc(region, total_size);
    
    if (ptr) {
        memset(ptr, 0, total_size);
    }
    
    return ptr;
}

/*
 * Reallocate memory from a region
 * Note: This only works for the last allocation in a region
 */
void* braggi_mem_region_realloc(Region* region, void* ptr, size_t old_size, size_t new_size) {
    if (!region || !ptr) {
        return NULL;
    }
    
    // Check if this is the last allocation
    if ((char*)ptr + old_size != (char*)region->memory + region->used) {
        // Not the last allocation, can't resize in-place
        return NULL;
    }
    
    // Calculate the difference in size
    ptrdiff_t size_diff = (ptrdiff_t)new_size - (ptrdiff_t)old_size;
    
    // Check if we're shrinking or growing
    if (size_diff <= 0) {
        // Shrinking, just update the used count
        region->used += size_diff;
        return ptr;
    } else {
        // Growing, check if there's enough space
        if (region->used + size_diff > region->size) {
            return NULL;
        }
        
        // Update the used count
        region->used += size_diff;
        return ptr;
    }
}

/*
 * Get the amount of memory used in a region
 */
size_t braggi_mem_region_get_used(const Region* region) {
    return region ? region->used : 0;
}

/*
 * Get the amount of memory available in a region
 */
size_t braggi_mem_region_get_available(const Region* region) {
    return region ? (region->size - region->used) : 0;
}

/*
 * Get the percentage of memory used in a region
 */
float braggi_mem_region_get_usage_percent(const Region* region) {
    if (!region || region->size == 0) {
        return 0.0f;
    }
    
    return ((float)region->used / (float)region->size) * 100.0f;
}

/*
 * Check if a region can allocate the specified amount of memory
 */
bool braggi_mem_region_can_allocate(const Region* region, size_t size) {
    return region && (region->used + size <= region->size);
}

/*
 * Reset a region (free all allocations)
 */
void braggi_mem_region_reset(Region* region) {
    if (region) {
        region->used = 0;
    }
} 