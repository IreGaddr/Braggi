#include "braggi/mem/regime.h"
#include "braggi/mem/region.h"

/*
 * "Memory regimes are like dance partners - some pairs just work better together,
 * while others step on each other's toes."
 * - Irish-Texan Memory Choreographer
 */

/*
 * Check if two regimes are compatible for a given periscope direction
 */
bool braggi_mem_regime_compatible(RegimeType source, RegimeType target, PeriscopeDirection direction) {
    // Simple compatibility rules:
    // - Same regime types are always compatible
    // - REGIME_RAND can connect to anything
    // - REGIME_SEQ can only connect to itself and REGIME_RAND
    // - FIFO and FILO have specific direction constraints
    
    if (source == target) {
        return true; // Same regimes are always compatible
    }
    
    if (source == REGIME_RAND || target == REGIME_RAND) {
        return true; // RAND can connect to anything
    }
    
    if (source == REGIME_SEQ && target != REGIME_RAND) {
        // SEQ can only connect to itself (which we already checked) and RAND (which we also checked)
        return false;
    }
    
    // FIFO and FILO compatibility
    if (source == REGIME_FIFO && target == REGIME_FILO) {
        return direction == PERISCOPE_OUT; // FIFO can output to FILO
    }
    
    if (source == REGIME_FILO && target == REGIME_FIFO) {
        return direction == PERISCOPE_IN; // FILO can input from FIFO
    }
    
    // Default - incompatible
    return false;
} 