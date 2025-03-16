/* 
 * Braggi - Memory Allocation Tracking
 * 
 * "Every byte of memory deserves a good home with a white picket fence,
 * and that's what we provide with regions!" - Texas memory folklore
 */

#ifndef BRAGGI_ALLOCATION_H
#define BRAGGI_ALLOCATION_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

#include "braggi/region.h"
#include "braggi/source.h"

/* Allocation flags */
#define ALLOCATION_FLAG_DEAD      0x0001  /* Allocation has been freed */
#define ALLOCATION_FLAG_INTERNAL  0x0002  /* Internal allocation used by the system */

/* Allocation type enumeration */
typedef enum {
    ALLOCATION_TYPE_MALLOC,   /* Standard malloc allocation */
    ALLOCATION_TYPE_REGION,   /* Region-based allocation */
    ALLOCATION_TYPE_CUSTOM    /* Custom allocation from user-defined allocator */
} AllocationType;

/* Statistics about tracked allocations */
typedef struct AllocationStats {
    size_t total_count;    /* Total number of allocations */
    size_t total_bytes;    /* Total bytes allocated */
    size_t active_count;   /* Number of active allocations */
    size_t active_bytes;   /* Bytes in active allocations */
    size_t freed_count;    /* Number of freed allocations */
    size_t freed_bytes;    /* Bytes in freed allocations */
} AllocationStats;

// Forward declaration of the allocation struct (defined in allocation.c)
typedef struct Allocation Allocation;

// Create a new allocation 
Allocation* braggi_mem_allocation_create(RegionId region_id, void* ptr, size_t size, uint32_t flags, 
                                    SourcePosition source_pos, const char* label);

// Destroy an allocation
void braggi_mem_allocation_destroy(Allocation* alloc);

// Get allocation properties
void* braggi_mem_allocation_ptr(const Allocation* alloc);
size_t braggi_mem_allocation_size(const Allocation* alloc);
RegionId braggi_mem_allocation_region_id(const Allocation* alloc);
SourcePosition braggi_mem_allocation_source_pos(const Allocation* alloc);
const char* braggi_mem_allocation_label(const Allocation* alloc);

// Memory operations with region awareness
// IMPORTANT: These are implemented in region.c, not allocation.c
// void* braggi_mem_region_alloc(Region* region, size_t size, SourcePosition source_pos, const char* label);
// void* braggi_mem_region_calloc(Region* region, size_t nmemb, size_t size, SourcePosition source_pos, const char* label);
// void* braggi_mem_region_realloc(Region* region, void* ptr, size_t size, SourcePosition source_pos, const char* label);
// char* braggi_mem_region_strdup(Region* region, const char* str, SourcePosition source_pos, const char* label);
// bool braggi_mem_region_free(Region* region, void* ptr);

/* Allocation tracking functions */
bool braggi_mem_allocation_init(void);
void braggi_mem_allocation_shutdown(void);
bool braggi_mem_allocation_track(void* ptr, size_t size, RegionId region_id, SourcePosition source_pos, const char* label);
bool braggi_mem_allocation_untrack(void* ptr);
Allocation* braggi_mem_allocation_find(void* ptr);
bool braggi_mem_allocation_mark_freed(void* ptr);
void braggi_mem_allocation_get_stats(AllocationStats* stats);
void braggi_mem_allocation_print_all(FILE* stream);
size_t braggi_mem_allocation_find_leaks(FILE* stream);

// Forward declarations
struct MemoryRegion;
typedef struct MemoryRegion MemoryRegion;

// Low-level memory allocation functions that work with MemoryRegion
void* braggi_region_alloc(MemoryRegion* region, size_t size);
char* braggi_region_strdup(MemoryRegion* region, const char* str);
void* braggi_region_calloc(MemoryRegion* region, size_t count, size_t size);
void* braggi_region_memdup(MemoryRegion* region, const void* ptr, size_t size);

// Additional memory management utilities can be added here

#endif /* BRAGGI_ALLOCATION_H */ 