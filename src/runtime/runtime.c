/*
 * Braggi - Runtime Support Library Implementation
 * 
 * "Good memory management is like managing livestock on a ranch -
 * keep 'em in the right regions, mind who can visit who, and
 * clean up promptly when they're done!"
 * - Texas rancher/programmer wisdom
 */

#include "braggi/runtime.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Allocation record structure
typedef struct BraggiAllocation {
    void* memory;                // Pointer to allocated memory
    size_t size;                 // Size of allocation
    uint32_t source_pos;         // Source position of allocation
    const char* label;           // Label for debugging
    struct BraggiAllocation* next; // Next allocation in list
} BraggiAllocation;

// Region structure
struct BraggiRegion {
    void* memory_pool;           // Memory block for the region
    size_t size;                 // Total size of region
    size_t used;                 // Amount of memory used
    BraggiRegimeType regime;     // Access regime
    BraggiAllocation* allocations; // List of allocations
    size_t alloc_count;          // Number of allocations
    
    // Regime-specific pointers
    void* next_alloc;            // Next allocation point (FIFO, FILO, SEQ)
    
    // Periscopes pointing to this region
    struct BraggiPeriscope** incoming_periscopes;
    size_t incoming_periscope_count;
    
    // Periscopes from this region
    struct BraggiPeriscope** outgoing_periscopes;
    size_t outgoing_periscope_count;
};

// Periscope structure
struct BraggiPeriscope {
    BraggiRegionHandle source;
    BraggiRegionHandle target;
    BraggiPeriscopeDirection direction;
};

// Last error code
static BraggiRuntimeError last_error = BRAGGI_RT_SUCCESS;

// Set the last error
static void set_error(BraggiRuntimeError error) {
    last_error = error;
}

// Create a new region
BraggiRegionHandle braggi_rt_region_create(size_t size, BraggiRegimeType regime) {
    // Validate parameters
    if (size == 0) {
        set_error(BRAGGI_RT_ERROR_INVALID_SIZE);
        return NULL;
    }
    
    if (regime != BRAGGI_REGIME_FIFO && 
        regime != BRAGGI_REGIME_FILO && 
        regime != BRAGGI_REGIME_SEQ && 
        regime != BRAGGI_REGIME_RAND) {
        set_error(BRAGGI_RT_ERROR_INVALID_REGIME);
        return NULL;
    }
    
    // Allocate region structure
    BraggiRegionHandle region = (BraggiRegionHandle)malloc(sizeof(struct BraggiRegion));
    if (!region) {
        set_error(BRAGGI_RT_ERROR_OUT_OF_MEMORY);
        return NULL;
    }
    
    // Allocate memory pool
    region->memory_pool = malloc(size);
    if (!region->memory_pool) {
        free(region);
        set_error(BRAGGI_RT_ERROR_OUT_OF_MEMORY);
        return NULL;
    }
    
    // Initialize region
    region->size = size;
    region->used = 0;
    region->regime = regime;
    region->allocations = NULL;
    region->alloc_count = 0;
    region->next_alloc = region->memory_pool;  // Start at beginning of pool
    region->incoming_periscopes = NULL;
    region->incoming_periscope_count = 0;
    region->outgoing_periscopes = NULL;
    region->outgoing_periscope_count = 0;
    
    set_error(BRAGGI_RT_SUCCESS);
    return region;
}

// Destroy a region
void braggi_rt_region_destroy(BraggiRegionHandle region) {
    if (!region) {
        set_error(BRAGGI_RT_ERROR_INVALID_HANDLE);
        return;
    }
    
    // Free all allocations
    BraggiAllocation* alloc = region->allocations;
    while (alloc) {
        BraggiAllocation* next = alloc->next;
        free(alloc);
        alloc = next;
    }
    
    // Destroy all periscopes pointing to this region
    for (size_t i = 0; i < region->incoming_periscope_count; i++) {
        free(region->incoming_periscopes[i]);
    }
    free(region->incoming_periscopes);
    
    // Destroy all periscopes from this region
    for (size_t i = 0; i < region->outgoing_periscope_count; i++) {
        free(region->outgoing_periscopes[i]);
    }
    free(region->outgoing_periscopes);
    
    // Free the memory pool
    free(region->memory_pool);
    
    // Free the region itself
    free(region);
    
    set_error(BRAGGI_RT_SUCCESS);
}

// Find an allocation in a region by pointer
static BraggiAllocation* find_allocation(BraggiRegionHandle region, void* ptr) {
    BraggiAllocation* alloc = region->allocations;
    while (alloc) {
        if (alloc->memory == ptr) {
            return alloc;
        }
        alloc = alloc->next;
    }
    return NULL;
}

// Allocate memory from a region
void* braggi_rt_region_alloc(BraggiRegionHandle region, size_t size, 
                        uint32_t source_pos, const char* label) {
    if (!region) {
        set_error(BRAGGI_RT_ERROR_INVALID_HANDLE);
        return NULL;
    }
    
    if (size == 0) {
        set_error(BRAGGI_RT_ERROR_INVALID_SIZE);
        return NULL;
    }
    
    // Check if there's enough space
    if (region->used + size > region->size) {
        set_error(BRAGGI_RT_ERROR_REGION_FULL);
        return NULL;
    }
    
    // Allocate memory based on regime
    void* memory = NULL;
    
    switch (region->regime) {
        case BRAGGI_REGIME_FIFO:
        case BRAGGI_REGIME_FILO:
        case BRAGGI_REGIME_SEQ:
            // For sequential allocation regimes, allocate from next_alloc
            memory = region->next_alloc;
            region->next_alloc = (char*)region->next_alloc + size;
            break;
            
        case BRAGGI_REGIME_RAND:
            // For random access, find a free block of sufficient size
            // For simplicity, we'll just use the next available space
            // In a real implementation, this would be more sophisticated
            memory = (char*)region->memory_pool + region->used;
            break;
    }
    
    // Create allocation record
    BraggiAllocation* alloc = (BraggiAllocation*)malloc(sizeof(BraggiAllocation));
    if (!alloc) {
        set_error(BRAGGI_RT_ERROR_OUT_OF_MEMORY);
        return NULL;
    }
    
    alloc->memory = memory;
    alloc->size = size;
    alloc->source_pos = source_pos;
    alloc->label = label ? strdup(label) : NULL;
    
    // Add to allocation list based on regime
    if (region->regime == BRAGGI_REGIME_FILO) {
        // For FILO, add at the beginning of the list
        alloc->next = region->allocations;
        region->allocations = alloc;
    } else {
        // For other regimes, add at the end
        alloc->next = NULL;
        if (!region->allocations) {
            region->allocations = alloc;
        } else {
            BraggiAllocation* last = region->allocations;
            while (last->next) {
                last = last->next;
            }
            last->next = alloc;
        }
    }
    
    // Update region state
    region->used += size;
    region->alloc_count++;
    
    set_error(BRAGGI_RT_SUCCESS);
    return memory;
}

// Free memory from a region
void braggi_rt_region_free(BraggiRegionHandle region, void* ptr) {
    if (!region) {
        set_error(BRAGGI_RT_ERROR_INVALID_HANDLE);
        return;
    }
    
    if (!ptr) {
        set_error(BRAGGI_RT_ERROR_INVALID_ALLOCATION);
        return;
    }
    
    // Find the allocation
    BraggiAllocation* prev = NULL;
    BraggiAllocation* alloc = region->allocations;
    
    while (alloc && alloc->memory != ptr) {
        prev = alloc;
        alloc = alloc->next;
    }
    
    if (!alloc) {
        set_error(BRAGGI_RT_ERROR_INVALID_ALLOCATION);
        return;
    }
    
    // Remove from allocation list
    if (prev) {
        prev->next = alloc->next;
    } else {
        region->allocations = alloc->next;
    }
    
    // Free label if present
    if (alloc->label) {
        free((void*)alloc->label);
    }
    
    // Update region state
    region->used -= alloc->size;
    region->alloc_count--;
    
    // Free allocation record
    free(alloc);
    
    // Note: We don't actually free the memory, as it's part of the region's pool
    
    set_error(BRAGGI_RT_SUCCESS);
}

// Check if a pointer is in a region
bool braggi_rt_region_contains(BraggiRegionHandle region, void* ptr) {
    if (!region || !ptr) {
        set_error(BRAGGI_RT_ERROR_INVALID_HANDLE);
        return false;
    }
    
    // Check if the pointer is within the region's memory pool
    char* start = (char*)region->memory_pool;
    char* end = start + region->size;
    char* check = (char*)ptr;
    
    bool result = (check >= start && check < end);
    
    set_error(BRAGGI_RT_SUCCESS);
    return result;
}

// Get total used memory in a region
size_t braggi_rt_region_get_used_memory(BraggiRegionHandle region) {
    if (!region) {
        set_error(BRAGGI_RT_ERROR_INVALID_HANDLE);
        return 0;
    }
    
    set_error(BRAGGI_RT_SUCCESS);
    return region->used;
}

// Get free memory in a region
size_t braggi_rt_region_get_free_memory(BraggiRegionHandle region) {
    if (!region) {
        set_error(BRAGGI_RT_ERROR_INVALID_HANDLE);
        return 0;
    }
    
    set_error(BRAGGI_RT_SUCCESS);
    return region->size - region->used;
}

// Get allocation count in a region
size_t braggi_rt_region_get_allocation_count(BraggiRegionHandle region) {
    if (!region) {
        set_error(BRAGGI_RT_ERROR_INVALID_HANDLE);
        return 0;
    }
    
    set_error(BRAGGI_RT_SUCCESS);
    return region->alloc_count;
}

// Create a periscope between regions
BraggiRuntimeError braggi_rt_region_create_periscope(BraggiRegionHandle source, 
                                            BraggiRegionHandle target,
                                            BraggiPeriscopeDirection direction) {
    if (!source || !target) {
        set_error(BRAGGI_RT_ERROR_INVALID_HANDLE);
        return BRAGGI_RT_ERROR_INVALID_HANDLE;
    }
    
    // Check regime compatibility (this would be more complex in a real implementation)
    bool compatible = true;
    
    // FILO->RAND is valid, RAND->FIFO is valid, but FILO->FIFO is not
    if (source->regime == BRAGGI_REGIME_FILO && target->regime == BRAGGI_REGIME_FIFO) {
        compatible = false;
    }
    
    if (!compatible) {
        set_error(BRAGGI_RT_ERROR_INCOMPATIBLE_REGIMES);
        return BRAGGI_RT_ERROR_INCOMPATIBLE_REGIMES;
    }
    
    // Create the periscope
    struct BraggiPeriscope* periscope = (struct BraggiPeriscope*)malloc(sizeof(struct BraggiPeriscope));
    if (!periscope) {
        set_error(BRAGGI_RT_ERROR_OUT_OF_MEMORY);
        return BRAGGI_RT_ERROR_OUT_OF_MEMORY;
    }
    
    periscope->source = source;
    periscope->target = target;
    periscope->direction = direction;
    
    // Add to source's outgoing periscopes
    void* new_outgoing = realloc(source->outgoing_periscopes, 
                               (source->outgoing_periscope_count + 1) * 
                               sizeof(struct BraggiPeriscope*));
    if (!new_outgoing) {
        free(periscope);
        set_error(BRAGGI_RT_ERROR_OUT_OF_MEMORY);
        return BRAGGI_RT_ERROR_OUT_OF_MEMORY;
    }
    
    source->outgoing_periscopes = (struct BraggiPeriscope**)new_outgoing;
    source->outgoing_periscopes[source->outgoing_periscope_count++] = periscope;
    
    // Add to target's incoming periscopes
    void* new_incoming = realloc(target->incoming_periscopes, 
                               (target->incoming_periscope_count + 1) * 
                               sizeof(struct BraggiPeriscope*));
    if (!new_incoming) {
        // Rollback
        source->outgoing_periscope_count--;
        free(periscope);
        set_error(BRAGGI_RT_ERROR_OUT_OF_MEMORY);
        return BRAGGI_RT_ERROR_OUT_OF_MEMORY;
    }
    
    target->incoming_periscopes = (struct BraggiPeriscope**)new_incoming;
    target->incoming_periscopes[target->incoming_periscope_count++] = periscope;
    
    set_error(BRAGGI_RT_SUCCESS);
    return BRAGGI_RT_SUCCESS;
}

// Destroy a periscope
void braggi_rt_region_destroy_periscope(BraggiPeriscopeHandle periscope) {
    if (!periscope) {
        set_error(BRAGGI_RT_ERROR_INVALID_HANDLE);
        return;
    }
    
    BraggiRegionHandle source = periscope->source;
    BraggiRegionHandle target = periscope->target;
    
    // Remove from source's outgoing periscopes
    for (size_t i = 0; i < source->outgoing_periscope_count; i++) {
        if (source->outgoing_periscopes[i] == periscope) {
            // Move the last element to this position
            source->outgoing_periscopes[i] = 
                source->outgoing_periscopes[--source->outgoing_periscope_count];
            break;
        }
    }
    
    // Remove from target's incoming periscopes
    for (size_t i = 0; i < target->incoming_periscope_count; i++) {
        if (target->incoming_periscopes[i] == periscope) {
            // Move the last element to this position
            target->incoming_periscopes[i] = 
                target->incoming_periscopes[--target->incoming_periscope_count];
            break;
        }
    }
    
    // Free the periscope
    free(periscope);
    
    set_error(BRAGGI_RT_SUCCESS);
}

// Get error message for a runtime error code
const char* braggi_rt_error_string(BraggiRuntimeError error) {
    switch (error) {
        case BRAGGI_RT_SUCCESS:
            return "Success";
        case BRAGGI_RT_ERROR_INVALID_HANDLE:
            return "Invalid handle";
        case BRAGGI_RT_ERROR_OUT_OF_MEMORY:
            return "Out of memory";
        case BRAGGI_RT_ERROR_INVALID_SIZE:
            return "Invalid size";
        case BRAGGI_RT_ERROR_INVALID_REGIME:
            return "Invalid regime type";
        case BRAGGI_RT_ERROR_INVALID_PERISCOPE:
            return "Invalid periscope";
        case BRAGGI_RT_ERROR_INCOMPATIBLE_REGIMES:
            return "Incompatible regimes";
        case BRAGGI_RT_ERROR_INVALID_ACCESS:
            return "Invalid access";
        case BRAGGI_RT_ERROR_REGION_FULL:
            return "Region full";
        case BRAGGI_RT_ERROR_INVALID_ALLOCATION:
            return "Invalid allocation";
        case BRAGGI_RT_ERROR_DANGLING_REFERENCE:
            return "Dangling reference";
        default:
            return "Unknown error";
    }
} 