#ifndef BRAGGI_MEM_REGIME_H
#define BRAGGI_MEM_REGIME_H

#include <stdbool.h>
#include "braggi/region_types.h"  // Include existing region types instead of redefining

/*
 * "Memory regimes: because sometimes your bits need a little structure
 * and discipline to keep from causing a ruckus."
 * - A Texan-Irish Memory Manager
 */

// Forward declaration for Region
typedef struct Region Region;

// Function declarations
bool braggi_mem_regime_compatible(RegimeType source, RegimeType target, PeriscopeDirection direction);
void braggi_mem_region_destroy(Region* region);

#endif // BRAGGI_MEM_REGIME_H 