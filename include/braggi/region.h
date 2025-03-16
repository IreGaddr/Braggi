/*
 * Braggi - Region-based Memory Management
 * 
 * "Good fences make good neighbors, and good regions make good memory!"
 * - Irish-Texan Programming Wisdom
 */

#ifndef BRAGGI_REGION_H
#define BRAGGI_REGION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "braggi/region_types.h"
#include "braggi/source_position.h"
#include "braggi/util/region.h"  /* Include for low-level memory region functions */

/*
 * "Memory regions in Braggi are like well-planned ranch plots -
 * each with its own purpose, boundaries, and governing laws."
 * - Irish-Texan Memory Management Manifesto
 */

// Forward declarations
typedef uint32_t RegionId;
typedef struct Region Region;

// Core region operations - renamed to avoid conflicts with mem/region.h
Region* braggi_named_region_create(const char* name, RegimeType regime, RegionId parent);
void braggi_named_region_destroy(Region* region);

// Memory allocation with source tracking
void* braggi_named_region_alloc(Region* region, size_t size, SourcePosition source_pos, const char* label);
void* braggi_named_region_calloc(Region* region, size_t nmemb, size_t size, SourcePosition source_pos, const char* label);
void* braggi_named_region_realloc(Region* region, void* ptr, size_t size, SourcePosition source_pos, const char* label);
void braggi_named_region_free(Region* region, void* ptr);

// Region query functions
size_t braggi_named_region_available(Region* region);
size_t braggi_named_region_used(Region* region);
RegimeType braggi_named_region_regime(Region* region);
const char* braggi_named_region_name(Region* region);
RegionId braggi_named_region_id(Region* region);
RegionId braggi_named_region_parent(Region* region);

// Region tree operations
void braggi_named_region_add_child(Region* parent, Region* child);
void braggi_named_region_remove_child(Region* parent, Region* child);
bool braggi_named_region_is_ancestor(Region* ancestor, Region* descendant);

// Region iterator
typedef void (*RegionVisitor)(Region* region, void* user_data);
void braggi_named_region_traverse(Region* root, RegionVisitor visitor, void* user_data);

// Default regions
Region* braggi_named_region_get_global(void);
Region* braggi_named_region_get_temporary(void);

/* Create a region with a memory region allocator */
Region* braggi_mem_region_create_with_allocator(const char* name, RegimeType regime, 
                                          RegionId parent, MemoryRegion* memory_region);

/* Create a periscope from one region to another */
bool braggi_mem_region_create_periscope(Region* source, Region* target, PeriscopeDirection direction);

/* Check if a regime is compatible with another */
bool braggi_mem_regime_compatible(RegimeType source, RegimeType target, PeriscopeDirection direction);

/* Get the memory region allocator of a region */
MemoryRegion* braggi_mem_region_get_allocator(const Region* region);

/* Get a human-readable name for a regime type */
const char* braggi_mem_regime_name(RegimeType regime);

#endif /* BRAGGI_REGION_H */ 