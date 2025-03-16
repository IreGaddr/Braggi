/*
 * Braggi - Region & Regime Type Definitions
 * 
 * "Ya gotta know what type o' cattle yer herdin' before ya start buildin' fences!"
 * - Irish-Texan Type System Wisdom
 */

#ifndef BRAGGI_REGION_TYPES_H
#define BRAGGI_REGION_TYPES_H

#include <stdint.h>

/*
 * RegionId - A unique identifier for regions
 */
typedef uint32_t RegionId;

/*
 * RegimeType - Memory access patterns for regions
 * 
 * FIFO: First-In-First-Out - Elements must be processed in order they were created
 * FILO: First-In-Last-Out - Stack-like behavior
 * SEQ:  Sequential access - Elements accessed in order, but with random accessibility
 * RAND: Random access - Elements can be accessed in any order
 */
typedef enum {
    REGIME_FIFO = 0,
    REGIME_FILO = 1,
    REGIME_SEQ  = 2,
    REGIME_RAND = 3
} RegimeType;

/*
 * PeriscopeDirection - Direction of a periscope operation
 * 
 * PERISCOPE_IN:  Value enters a region
 * PERISCOPE_OUT: Value exits a region
 */
typedef enum {
    PERISCOPE_IN  = 0,
    PERISCOPE_OUT = 1
} PeriscopeDirection;

/*
 * Forward declarations for opaque struct types
 */
typedef struct Region Region;
typedef struct MemoryRegion MemoryRegion;

#endif /* BRAGGI_REGION_TYPES_H */ 