/*
 * Braggi - Region Manager Header
 * 
 * "A good memory manager is like a wise ranch foreman - knows exactly
 * where everythin' is, and makes sure nothin' gets left behind!"
 */

#ifndef BRAGGI_REGION_MANAGER_H
#define BRAGGI_REGION_MANAGER_H

#include <stddef.h>
#include "braggi/region.h"

// Forward declaration
typedef struct RegionManager RegionManager;

/**
 * Create a new region manager
 * 
 * @return A newly allocated region manager, or NULL on failure
 */
RegionManager* braggi_region_manager_create(void);

/**
 * Destroy a region manager and all its managed regions
 * 
 * @param manager The manager to destroy
 */
void braggi_region_manager_destroy(RegionManager* manager);

/**
 * Create a new region and add it to the manager
 * 
 * @param manager The region manager
 * @param name The name of the region
 * @param regime The regime type for the region
 * @return A newly allocated region, or NULL on failure
 */
Region* braggi_region_manager_create_region(RegionManager* manager, const char* name, int regime);

/**
 * Get the global region from the manager
 * 
 * @param manager The region manager
 * @return The global region, or NULL if the manager is invalid
 */
Region* braggi_region_manager_get_global(RegionManager* manager);

/**
 * Get statistics about memory usage
 * 
 * @param manager The region manager
 * @param total Pointer to store the total allocation, can be NULL
 * @param peak Pointer to store the peak allocation, can be NULL
 */
void braggi_region_manager_get_stats(RegionManager* manager, size_t* total, size_t* peak);

#endif /* BRAGGI_REGION_MANAGER_H */ 