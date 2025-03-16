/*
 * Braggi - Region-based Memory Allocator
 * 
 * "Memory regions are like Texas ranches - buy a big plot of land
 * and you can put whatever you want on it. When you're done, sell the
 * whole darn thing instead of sellin' each cow separately!" - Irish-Texan Code Philosophy
 */

#include "braggi/util/region.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

/* Default block size for memory regions (64KB) */
#define REGION_DEFAULT_BLOCK_SIZE (64 * 1024)

/* Minimum block size (1KB) */
#define REGION_MIN_BLOCK_SIZE (1 * 1024)

/* Memory block structure */
typedef struct MemoryBlock {
    void* data;                    /* Pointer to allocated block */
    size_t size;                   /* Size of the block */
    size_t used;                   /* Amount of memory used in the block */
    struct MemoryBlock* next;      /* Pointer to next block */
} MemoryBlock;

/* Memory region structure */
struct MemoryRegion {
    MemoryBlock* first_block;      /* First memory block */
    MemoryBlock* current_block;    /* Current block for allocations */
    size_t block_size;             /* Default size of new blocks */
    size_t total_allocated;        /* Total memory allocated */
    size_t total_used;             /* Total memory used */
    size_t allocation_count;       /* Number of allocations made */
};

/* Create a new memory block for a region */
static MemoryBlock* create_memory_block(size_t size) {
    MemoryBlock* block = (MemoryBlock*)malloc(sizeof(MemoryBlock));
    if (!block) return NULL;

    block->data = malloc(size);
    if (!block->data) {
        free(block);
        return NULL;
    }

    block->size = size;
    block->used = 0;
    block->next = NULL;

    return block;
}

/* Free a memory block */
static void free_memory_block(MemoryBlock* block) {
    if (!block) return;
    if (block->data) free(block->data);
    free(block);
}

/* Create a new memory region */
MemoryRegion* braggi_region_create(void) {
    return braggi_region_create_with_block_size(REGION_DEFAULT_BLOCK_SIZE);
}

/* Create a new memory region with specified block size */
MemoryRegion* braggi_region_create_with_block_size(size_t block_size) {
    /* Ensure minimum block size */
    if (block_size < REGION_MIN_BLOCK_SIZE) {
        block_size = REGION_MIN_BLOCK_SIZE;
    }

    MemoryRegion* region = (MemoryRegion*)malloc(sizeof(MemoryRegion));
    if (!region) return NULL;

    region->block_size = block_size;
    region->total_allocated = 0;
    region->total_used = 0;
    region->allocation_count = 0;

    /* Create initial block */
    region->first_block = create_memory_block(block_size);
    if (!region->first_block) {
        free(region);
        return NULL;
    }

    region->current_block = region->first_block;
    region->total_allocated = block_size;

    return region;
}

/* Destroy a memory region and all its allocations */
void braggi_region_destroy(MemoryRegion* region) {
    if (!region) return;

    /* Free all blocks */
    MemoryBlock* block = region->first_block;
    while (block) {
        MemoryBlock* next = block->next;
        free_memory_block(block);
        block = next;
    }

    free(region);
}

/* Align a size to the specified alignment */
static size_t align_size(size_t size, size_t alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}

/* Allocate memory from a region */
void* braggi_region_alloc(MemoryRegion* region, size_t size) {
    if (!region || size == 0) return NULL;

    /* Align size to 8 bytes for better memory access */
    size = align_size(size, 8);

    /* Find a block with enough space */
    MemoryBlock* block = region->current_block;

    /* If current block doesn't have enough space, try to find another or create a new one */
    if (block->used + size > block->size) {
        /* Check if any existing block has enough space */
        MemoryBlock* iter = region->first_block;
        while (iter) {
            if (iter->used + size <= iter->size) {
                block = iter;
                region->current_block = block;
                break;
            }
            iter = iter->next;
        }

        /* If no block has enough space, create a new one */
        if (block->used + size > block->size) {
            size_t new_block_size = region->block_size;
            
            /* If the allocation is larger than our default block size, make a custom-sized block */
            if (size > new_block_size) {
                new_block_size = size;
            }

            MemoryBlock* new_block = create_memory_block(new_block_size);
            if (!new_block) return NULL;

            /* Add to the chain */
            new_block->next = region->first_block;
            region->first_block = new_block;
            region->current_block = new_block;
            block = new_block;

            /* Update totals */
            region->total_allocated += new_block_size;
        }
    }

    /* Allocate from the block */
    void* ptr = (char*)block->data + block->used;
    block->used += size;
    region->total_used += size;
    region->allocation_count++;

    return ptr;
}

/* Allocate zeroed memory from a region */
void* braggi_region_calloc(MemoryRegion* region, size_t count, size_t size) {
    size_t total_size = count * size;
    
    /* Check for overflow */
    if (count > 0 && total_size / count != size) return NULL;
    
    void* ptr = braggi_region_alloc(region, total_size);
    if (ptr) {
        memset(ptr, 0, total_size);
    }
    
    return ptr;
}

/* Duplicate a string into memory from a region */
char* braggi_region_strdup(MemoryRegion* region, const char* str) {
    if (!region || !str) return NULL;
    
    size_t len = strlen(str) + 1;
    char* dup = (char*)braggi_region_alloc(region, len);
    if (dup) {
        memcpy(dup, str, len);
    }
    
    return dup;
}

/* Duplicate a memory block into a region */
void* braggi_region_memdup(MemoryRegion* region, const void* ptr, size_t size) {
    if (!region || !ptr || size == 0) return NULL;
    
    void* dup = braggi_region_alloc(region, size);
    if (dup) {
        memcpy(dup, ptr, size);
    }
    
    return dup;
}

/* Reset a memory region (free all allocations but keep the region) */
void braggi_region_reset(MemoryRegion* region) {
    if (!region) return;
    
    /* Reset all blocks to empty */
    MemoryBlock* block = region->first_block;
    while (block) {
        block->used = 0;
        block = block->next;
    }
    
    /* Reset region stats */
    region->total_used = 0;
    region->allocation_count = 0;
    region->current_block = region->first_block;
}

/* Get the total memory currently allocated by a region */
size_t braggi_region_total_allocated(const MemoryRegion* region) {
    if (!region) return 0;
    return region->total_allocated;
}

/* Get the current memory usage of a region */
size_t braggi_region_current_usage(const MemoryRegion* region) {
    if (!region) return 0;
    return region->total_used;
}

/* Get the memory wasted by a region (allocated but not used) */
size_t braggi_region_wasted_memory(const MemoryRegion* region) {
    if (!region) return 0;
    return region->total_allocated - region->total_used;
}

/* Debugging function to print region stats */
void braggi_region_print_stats(const MemoryRegion* region, FILE* stream) {
    if (!region) return;
    if (!stream) stream = stdout;
    
    fprintf(stream, "Region Stats:\n");
    fprintf(stream, "  Block size: %zu bytes\n", region->block_size);
    fprintf(stream, "  Total allocated: %zu bytes\n", region->total_allocated);
    fprintf(stream, "  Total used: %zu bytes\n", region->total_used);
    fprintf(stream, "  Wasted: %zu bytes (%.2f%%)\n", 
            region->total_allocated - region->total_used,
            (region->total_allocated > 0) 
                ? ((region->total_allocated - region->total_used) * 100.0 / region->total_allocated) 
                : 0);
    fprintf(stream, "  Allocations: %zu\n", region->allocation_count);
    
    /* Count blocks */
    size_t block_count = 0;
    MemoryBlock* block = region->first_block;
    while (block) {
        block_count++;
        block = block->next;
    }
    
    fprintf(stream, "  Blocks: %zu\n", block_count);
} 