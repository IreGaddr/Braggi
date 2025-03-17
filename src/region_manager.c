/*
 * Braggi - Region Manager Implementation
 * 
 * "A good ranch manager keeps track of all the land - and a good region manager
 * keeps track of all that precious memory!" - Irish-Texan Programming Wisdom
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "braggi/region.h"
#include "braggi/util/vector.h"

// Structure for the region manager
typedef struct RegionManager {
    Vector* regions;          // Vector of Region* (managed regions)
    Region* global_region;    // Global region for application-lifetime allocations
    size_t total_allocation;  // Track total memory allocated
    size_t peak_allocation;   // Track peak memory usage
    bool initialized;         // Whether the manager is initialized
} RegionManager;

/*
 * Create a new region manager
 * 
 * "Starting a new region manager is like marking your territory -
 * you gotta let folks know this memory is being managed properly!"
 */
RegionManager* braggi_region_manager_create(void) {
    RegionManager* manager = (RegionManager*)malloc(sizeof(RegionManager));
    if (!manager) {
        return NULL;
    }
    
    // Initialize manager
    manager->regions = braggi_vector_create(sizeof(Region*));
    if (!manager->regions) {
        free(manager);
        return NULL;
    }
    
    // Create global region
    manager->global_region = braggi_named_region_create("global", 0, 0);
    if (!manager->global_region) {
        braggi_vector_destroy(manager->regions);
        free(manager);
        return NULL;
    }
    
    manager->total_allocation = 0;
    manager->peak_allocation = 0;
    manager->initialized = true;
    
    return manager;
}

/*
 * Destroy a region manager and all its managed regions
 * 
 * "When it's time to clean up, a good Texan leaves no trash behind -
 * and a good memory manager leaves no leaks!" 
 */
void braggi_region_manager_destroy(RegionManager* manager) {
    if (!manager) {
        return;
    }
    
    // Clean up all managed regions
    if (manager->regions) {
        for (size_t i = 0; i < braggi_vector_size(manager->regions); i++) {
            Region* region = *(Region**)braggi_vector_get(manager->regions, i);
            if (region && region != manager->global_region) {  // Avoid double free of global region
                braggi_named_region_destroy(region);
            }
        }
        braggi_vector_destroy(manager->regions);
    }
    
    // Clean up global region
    if (manager->global_region) {
        braggi_named_region_destroy(manager->global_region);
    }
    
    // Free the manager itself
    free(manager);
}

/*
 * Create a new region and add it to the manager
 * 
 * "Every new field needs a fence, and every new memory region
 * needs proper management. That's just common sense, y'all!"
 */
Region* braggi_region_manager_create_region(RegionManager* manager, const char* name, int regime) {
    if (!manager || !name) {
        return NULL;
    }
    
    // Create a new region
    Region* region = braggi_named_region_create(name, regime, 0);
    if (!region) {
        return NULL;
    }
    
    // Add to managed regions
    if (!braggi_vector_push(manager->regions, &region)) {
        braggi_named_region_destroy(region);
        return NULL;
    }
    
    return region;
}

/*
 * Get the global region from the manager
 * 
 * "The global region is like the town square - everyone knows where it is
 * and everyone can use it, but don't leave your trash there too long!"
 */
Region* braggi_region_manager_get_global(RegionManager* manager) {
    if (!manager) {
        return NULL;
    }
    
    return manager->global_region;
}

/*
 * Get statistics about memory usage
 * 
 * "Keeping track of your herd is just as important as keeping track
 * of your memory - know what you've got at all times!"
 */
void braggi_region_manager_get_stats(RegionManager* manager, size_t* total, size_t* peak) {
    if (!manager) {
        if (total) *total = 0;
        if (peak) *peak = 0;
        return;
    }
    
    if (total) *total = manager->total_allocation;
    if (peak) *peak = manager->peak_allocation;
} 