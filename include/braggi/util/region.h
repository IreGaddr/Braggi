/*
 * Braggi - Region-based Memory Allocator
 * 
 * "Memory regions are like Texas ranches - buy a big plot of land
 * and you can put whatever you want on it. When you're done, sell the
 * whole darn thing instead of sellin' each cow separately!" - Irish-Texan Code Philosophy
 */

#ifndef BRAGGI_UTIL_REGION_H
#define BRAGGI_UTIL_REGION_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>  /* For FILE* */

// Include common type definitions
#include "braggi/region_types.h"

/**
 * MemoryRegion - A fast region-based memory allocator
 * 
 * Allocates memory in large chunks and allows for fast allocation
 * from these chunks. Freeing individual allocations is not supported,
 * instead entire regions are freed at once.
 */
typedef struct MemoryRegion MemoryRegion;

/**
 * Create a new memory region with default block size
 * 
 * @return A new memory region or NULL on failure
 */
MemoryRegion* braggi_region_create(void);

/**
 * Create a new memory region with specified block size
 * 
 * @param block_size The size of each memory block
 * @return A new memory region or NULL on failure
 */
MemoryRegion* braggi_region_create_with_block_size(size_t block_size);

/**
 * Destroy a memory region and all its allocations
 * 
 * @param region The region to destroy
 */
void braggi_region_destroy(MemoryRegion* region);

/**
 * Allocate memory from a region
 * 
 * @param region The region to allocate from
 * @param size The size in bytes to allocate
 * @return Pointer to allocated memory or NULL on failure
 */
void* braggi_region_alloc(MemoryRegion* region, size_t size);

/**
 * Allocate zeroed memory from a region
 * 
 * @param region The region to allocate from
 * @param count Number of elements
 * @param size Size of each element
 * @return Pointer to allocated memory or NULL on failure
 */
void* braggi_region_calloc(MemoryRegion* region, size_t count, size_t size);

/**
 * Duplicate a string into memory from a region
 * 
 * @param region The region to allocate from
 * @param str The string to duplicate
 * @return Pointer to duplicated string or NULL on failure
 */
char* braggi_region_strdup(MemoryRegion* region, const char* str);

/**
 * Duplicate a memory block into a region
 * 
 * @param region The region to allocate from
 * @param ptr Pointer to memory to duplicate
 * @param size Size of memory block
 * @return Pointer to duplicated memory or NULL on failure
 */
void* braggi_region_memdup(MemoryRegion* region, const void* ptr, size_t size);

/**
 * Reset a memory region (free all allocations but keep the region)
 * 
 * @param region The region to reset
 */
void braggi_region_reset(MemoryRegion* region);

/**
 * Get the total memory currently allocated by a region
 * 
 * @param region The region to query
 * @return The total memory in bytes
 */
size_t braggi_region_total_allocated(const MemoryRegion* region);

/**
 * Get the current memory usage of a region
 * 
 * @param region The region to query
 * @return The current used memory in bytes
 */
size_t braggi_region_current_usage(const MemoryRegion* region);

/**
 * Get the memory wasted by a region (allocated but not used)
 * 
 * @param region The region to query
 * @return The wasted memory in bytes
 */
size_t braggi_region_wasted_memory(const MemoryRegion* region);

/**
 * Print statistics about a memory region
 * 
 * @param region The region to print statistics for
 * @param stream Output stream (stdout if NULL)
 */
void braggi_region_print_stats(const MemoryRegion* region, FILE* stream);

#endif /* BRAGGI_UTIL_REGION_H */ 