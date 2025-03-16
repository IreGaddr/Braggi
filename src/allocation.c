/*
 * Braggi - Memory Allocation Tracking Implementation
 * 
 * "A good memory manager is like a good cattle rancher - 
 * they know where every head is, and they don't lose track of 'em!" - Irish-Texan Memory Wisdom
 */

#include "braggi/allocation.h"
#include "braggi/region.h"
#include "braggi/region_types.h"
#include "braggi/error.h"
// Temporarily comment out hashmap until it's available
// #include "braggi/util/hashmap.h"  /* Include hashmap for allocation tracking */
#include "braggi/util/region.h"   /* Include for low-level region allocator */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* 
 * Global allocation tracking hashmap 
 * Maps void* pointers to Allocation structs
 */
// static HashMap* global_allocations = NULL;

/* Allocation structure */
struct Allocation {
    void* ptr;                /* Pointer to allocated memory */
    size_t size;              /* Size of the allocation in bytes */
    AllocationType type;      /* Type of allocation */
    RegionId region_id;       /* ID of the owning region */
    uint32_t flags;           /* Flags for this allocation */
    SourcePosition source_pos; /* Source position for debugging */
    const char* label;        /* Optional label for debugging */
    time_t timestamp;         /* When allocation occurred */
};

/* Create a new allocation */
Allocation* braggi_mem_allocation_create(RegionId region_id, void* ptr, size_t size, uint32_t flags,
                                     SourcePosition source_pos, const char* label) {
    Allocation* alloc = (Allocation*)malloc(sizeof(Allocation));
    if (!alloc) return NULL;
    
    alloc->ptr = ptr;
    alloc->size = size;
    alloc->region_id = region_id;
    alloc->flags = flags;
    alloc->source_pos = source_pos;
    alloc->label = label ? strdup(label) : NULL;
    alloc->timestamp = time(NULL);
    alloc->type = ALLOCATION_TYPE_REGION; // Default to region type
    
    return alloc;
}

/* Destroy an allocation */
void braggi_mem_allocation_destroy(Allocation* alloc) {
    if (!alloc) return;
    
    /* Free the pointer if it exists */
    if (alloc->ptr) {
        free(alloc->ptr);
        alloc->ptr = NULL;
    }
    
    /* Free the label if it exists */
    if (alloc->label) {
        free((char*)alloc->label);
        alloc->label = NULL;
    }
    
    /* Free the allocation structure itself */
    free(alloc);
}

/* Get the size of an allocation */
size_t braggi_mem_allocation_size(const Allocation* alloc) {
    if (!alloc) return 0;
    return alloc->size;
}

/* Get the pointer to an allocation's memory */
void* braggi_mem_allocation_ptr(const Allocation* alloc) {
    if (!alloc) return NULL;
    return alloc->ptr;
}

/* Get the region ID of an allocation */
RegionId braggi_mem_allocation_region_id(const Allocation* alloc) {
    if (!alloc) return 0;
    return alloc->region_id;
}

/* Get the source position of an allocation */
SourcePosition braggi_mem_allocation_source_pos(const Allocation* alloc) {
    if (!alloc) {
        // Return an empty source position
        SourcePosition pos = {0};
        return pos;
    }
    return alloc->source_pos;
}

/* Get the label of an allocation */
const char* braggi_mem_allocation_label(const Allocation* alloc) {
    if (!alloc) return NULL;
    return alloc->label;
}

/* Initialize the allocation tracking system */
bool braggi_mem_allocation_init(void) {
    // Disabled until hashmap is available
    // if (global_allocations) {
    //     return true; /* Already initialized */
    // }
    // 
    // global_allocations = braggi_hashmap_create();
    // if (!global_allocations) {
    //     return false;
    // }
    // 
    // /* Set up proper functions for pointer keys */
    // braggi_hashmap_set_functions(global_allocations, 
    //                             braggi_hashmap_hash_pointer, 
    //                             braggi_hashmap_equals_pointer);
    // 
    // /* We'll manually manage the value freeing */
    // braggi_hashmap_set_free_functions(global_allocations, NULL, free);
    
    return true;
}

/* Shutdown the allocation tracking system */
void braggi_mem_allocation_shutdown(void) {
    // Disabled until hashmap is available
    // if (global_allocations) {
    //     braggi_hashmap_destroy(global_allocations);
    //     global_allocations = NULL;
    // }
}

/* Track a new allocation */
bool braggi_mem_allocation_track(void* ptr, size_t size, RegionId region_id, 
                               SourcePosition source_pos, const char* label) {
    // Disabled until hashmap is available
    // if (!ptr || !global_allocations) {
    //     return false;
    // }
    // 
    // /* Create the allocation record */
    // Allocation* alloc = malloc(sizeof(Allocation));
    // if (!alloc) {
    //     return false;
    // }
    // 
    // alloc->ptr = ptr;
    // alloc->size = size;
    // alloc->region_id = region_id;
    // alloc->flags = 0;
    // alloc->source_pos = source_pos;
    // alloc->label = label ? strdup(label) : NULL;
    // alloc->timestamp = time(NULL);
    // alloc->type = ALLOCATION_TYPE_REGION; // Default type
    // 
    // /* Add to the hashmap */
    // if (!braggi_hashmap_put(global_allocations, ptr, alloc)) {
    //     free(alloc);
    //     return false;
    // }
    
    return true;
}

/* Find an allocation record by pointer */
Allocation* braggi_mem_allocation_find(void* ptr) {
    // Disabled until hashmap is available
    // if (!ptr || !global_allocations) {
    //     return NULL;
    // }
    // 
    // return (Allocation*)braggi_hashmap_get(global_allocations, ptr);
    return NULL;
}

/* Mark an allocation as freed */
bool braggi_mem_allocation_mark_freed(void* ptr) {
    // Disabled until hashmap is available
    // if (!ptr || !global_allocations) {
    //     return false;
    // }
    // 
    // Allocation* alloc = braggi_hashmap_get(global_allocations, ptr);
    // if (!alloc) {
    //     return false;
    // }
    // 
    // alloc->flags |= ALLOCATION_FLAG_DEAD;
    return true;
}

/* Untrack an allocation */
bool braggi_mem_allocation_untrack(void* ptr) {
    // Disabled until hashmap is available
    // if (!ptr || !global_allocations) {
    //     return false;
    // }
    // 
    // Allocation* alloc = braggi_hashmap_remove(global_allocations, ptr);
    // if (!alloc) {
    //     return false;
    // }
    // 
    // /* Free the allocation record */
    // if (alloc->label) {
    //     free((char*)alloc->label);
    // }
    // free(alloc);
    
    return true;
}

/* Get statistics about tracked allocations */
void braggi_mem_allocation_get_stats(AllocationStats* stats) {
    if (!stats) {
        return;
    }
    
    // Disabled until hashmap is available
    // if (!global_allocations) {
    //     return;
    // }
    
    /* Initialize stats */
    stats->total_count = 0; // Was: braggi_hashmap_size(global_allocations);
    stats->total_bytes = 0;
    stats->active_count = 0;
    stats->active_bytes = 0;
    stats->freed_count = 0;
    stats->freed_bytes = 0;
    
    /* Iterate through all allocations */
    // HashMapIterator iter;
    // braggi_hashmap_iterator_init(global_allocations, &iter);
    // 
    // void* key;
    // Allocation* alloc;
    // 
    // while (braggi_hashmap_iterator_next(&iter, &key, (void**)&alloc)) {
    //     stats->total_bytes += alloc->size;
    //     
    //     if (alloc->flags & ALLOCATION_FLAG_DEAD) {
    //         stats->freed_count++;
    //         stats->freed_bytes += alloc->size;
    //     } else {
    //         stats->active_count++;
    //         stats->active_bytes += alloc->size;
    //     }
    // }
}

/* Print information about all tracked allocations */
void braggi_mem_allocation_print_all(FILE* stream) {
    if (!stream) {
        return;
    }
    
    // Disabled until hashmap is available
    // if (!global_allocations) {
    //     return;
    // }
    
    fprintf(stream, "=== ALLOCATION TRACKING REPORT ===\n");
    
    AllocationStats stats;
    braggi_mem_allocation_get_stats(&stats);
    
    fprintf(stream, "Total allocations: %zu (%zu bytes)\n", 
            stats.total_count, stats.total_bytes);
    fprintf(stream, "Active allocations: %zu (%zu bytes)\n", 
            stats.active_count, stats.active_bytes);
    fprintf(stream, "Freed allocations: %zu (%zu bytes)\n", 
            stats.freed_count, stats.freed_bytes);
    
    fprintf(stream, "\nDetailed allocation list:\n");
    fprintf(stream, "%-20s %-10s %-10s %-10s %-20s %s\n",
            "Address", "Size", "Region", "Status", "Source", "Label");
    fprintf(stream, "----------------------------------------------------------------------\n");
    
    /* Iterate through all allocations */
    // HashMapIterator iter;
    // braggi_hashmap_iterator_init(global_allocations, &iter);
    // 
    // void* key;
    // Allocation* alloc;
    // 
    // while (braggi_hashmap_iterator_next(&iter, &key, (void**)&alloc)) {
    //     fprintf(stream, "%-20p %-10zu %-10u %-10s %u:%-15u %s\n",
    //             alloc->ptr,
    //             alloc->size,
    //             alloc->region_id,
    //             (alloc->flags & ALLOCATION_FLAG_DEAD) ? "FREED" : "ACTIVE",
    //             alloc->source_pos.line,
    //             alloc->source_pos.column,
    //             alloc->label ? alloc->label : "(no label)");
    // }
    
    fprintf(stream, "==============================\n");
}

/* Find memory leaks (allocations that are still active but shouldn't be) */
size_t braggi_mem_allocation_find_leaks(FILE* stream) {
    size_t leak_count = 0;
    
    // Disabled until hashmap is available
    // if (!global_allocations) {
    //     return 0;
    // }
    
    if (stream) {
        fprintf(stream, "=== MEMORY LEAK REPORT ===\n");
    }
    
    /* Iterate through all allocations */
    // HashMapIterator iter;
    // braggi_hashmap_iterator_init(global_allocations, &iter);
    // 
    // void* key;
    // Allocation* alloc;
    // 
    // while (braggi_hashmap_iterator_next(&iter, &key, (void**)&alloc)) {
    //     /* Only count active allocations as leaks */
    //     if (!(alloc->flags & ALLOCATION_FLAG_DEAD)) {
    //         leak_count++;
    //         
    //         if (stream) {
    //             fprintf(stream, "LEAK: %p (%zu bytes) from region %u at %u:%u - %s\n",
    //                     alloc->ptr,
    //                     alloc->size,
    //                     alloc->region_id,
    //                     alloc->source_pos.line,
    //                     alloc->source_pos.column,
    //                     alloc->label ? alloc->label : "(no label)");
    //         }
    //     }
    // }
    
    if (stream) {
        fprintf(stream, "Total leaks: %zu\n", leak_count);
        fprintf(stream, "========================\n");
    }
    
    return leak_count;
}

/*
 * Braggi - Low-level memory allocation functions
 * 
 * "Memory's like Texas land - if ya don't stake yer claim right,
 * someone else will come along and take it!" - Frontier Memory Philosophy
 */

// Forward declaration for MemoryRegion structure
struct MemoryRegion {
    // Implementation details (not important for this allocation layer)
    void* blocks;       // Start of memory blocks
    size_t block_size;  // Size of each block
    size_t used;        // Used memory in current block
    size_t total;       // Total allocated memory
};

/*
 * Low-level memory allocation within a region
 * 
 * "This here function is like a land surveyor - it marks out
 * just the piece of memory territory you need!" - Irish Allocation Wisdom
 */
void* braggi_region_alloc(MemoryRegion* region, size_t size) {
    if (!region) return malloc(size);
    
    // This is a simplified implementation - in a real system, this would
    // manage blocks of memory more efficiently
    
    void* ptr = malloc(size);
    if (!ptr) return NULL;
    
    // In a real implementation, we'd track this allocation in the region
    
    return ptr;
}

/*
 * Duplicate a string in a memory region
 * 
 * "Like copying the deed to your ranch, this makes sure
 * you've got a perfect duplicate in your region!" - Texan String Management
 */
char* braggi_region_strdup(MemoryRegion* region, const char* str) {
    if (!region || !str) return NULL;
    
    size_t len = strlen(str) + 1;
    char* dup = (char*)braggi_region_alloc(region, len);
    
    if (dup) {
        memcpy(dup, str, len);
    }
    
    return dup;
}

/*
 * Allocate zeroed memory from a region
 * 
 * "Like buyin' fresh Texas farmland with nothin' on it yet -
 * all clean and ready for whatever you wanna build!" - Memory Rancher
 */
void* braggi_region_calloc(MemoryRegion* region, size_t count, size_t size) {
    size_t total = count * size;
    void* ptr = braggi_region_alloc(region, total);
    
    if (ptr) {
        memset(ptr, 0, total);
    }
    
    return ptr;
}

/*
 * Duplicate memory in a region
 * 
 * "When you need a twin for your memory, this function
 * delivers faster than an Irish midwife!" - Memory Duplication Proverb
 */
void* braggi_region_memdup(MemoryRegion* region, const void* ptr, size_t size) {
    if (!region || !ptr) return NULL;
    
    void* dup = braggi_region_alloc(region, size);
    if (dup) {
        memcpy(dup, ptr, size);
    }
    
    return dup;
} 