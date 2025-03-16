/*
 * Braggi - Region-based Memory Management Implementation
 * 
 * "Good fences make good neighbors, and good regions make good memory!"
 * - Irish-Texan Programming Wisdom
 */

#include "braggi/region.h"
#include "braggi/allocation.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Define aliases for the utility memory region functions to distinguish from our high-level functions
#define util_region_alloc(region, size) braggi_region_alloc((region), (size))
#define util_region_calloc(region, count, size) braggi_region_calloc((region), (count), (size))
#define util_region_strdup(region, str) braggi_region_strdup((region), (str))

/* Region type enumeration */
typedef enum {
    REGION_TYPE_NORMAL,        /* Standard region */
    REGION_TYPE_MEMORY_REGION  /* Region backed by a memory region allocator */
} RegionType;

/* Region flags */
#define REGION_FLAG_USE_MALLOC    0x0001  /* Region uses malloc for allocations */
#define REGION_FLAG_OWNS_MEMORY   0x0002  /* Region owns its memory resources */

/* Allocation flags */
#define ALLOCATION_FLAG_DEAD      0x0001  /* Allocation has been freed */
#define ALLOCATION_FLAG_INTERNAL  0x0002  /* Internal allocation used by the system */

/*
 * Periscope - A connection between two regions
 */
typedef struct Periscope {
    RegionId source_id;     /* Source region ID */
    RegionId target_id;     /* Target region ID */
    PeriscopeDirection direction; /* Direction of periscope */
} Periscope;

/*
 * Allocation - Structure to track a single allocation in a region
 */
typedef struct Allocation {
    void* ptr;               /* Pointer to the allocated memory */
    size_t size;             /* Size in bytes */
    RegionId region_id;      /* ID of the owning region */
    uint32_t flags;          /* Flags for this allocation */
    SourcePosition source_pos; /* Source position for debugging */
    const char* label;       /* Optional label for debugging */
} Allocation;

// Initial allocation capacity for regions
#define REGION_INITIAL_ALLOCATION_CAPACITY 16
#define REGION_INITIAL_PERISCOPE_CAPACITY 4

// Full definition of the Region struct
struct Region {
    RegionId id;                // Unique identifier
    const char* name;           // Optional name for debugging
    RegimeType regime;          // Memory access regime
    RegionId parent;            // Parent region ID (0 for no parent)
    RegionType type;            // Type of region
    uint32_t flags;             // Region flags
    Allocation** allocations;   // Array of allocations
    size_t num_allocations;     // Current number of allocations
    size_t capacity;            // Capacity of allocations array
    Periscope** periscopes;     // Array of periscopes
    size_t num_periscopes;      // Current number of periscopes
    size_t periscope_capacity;  // Capacity of periscopes array
    MemoryRegion* memory_region; // Backing memory region allocator (can be NULL)
};

/* 
 * Forward declarations for functions used before they're defined
 */
static Allocation* allocation_create(RegionId region_id, void* ptr, size_t size, uint32_t flags,
                                    SourcePosition source_pos, const char* label);
static void allocation_destroy(Allocation* alloc);
static Allocation* find_allocation(Region* region, void* ptr);

/* Helper functions for array manipulation - moved to the top */
static bool array_append(void** array, size_t* count, size_t* capacity, void* item) {
    if (!array || !count || !capacity || !item) return false;
    
    // Check if we need to grow the array
    if (*count >= *capacity) {
        size_t new_capacity = *capacity > 0 ? *capacity * 2 : 4;
        void** new_array = (void**)realloc(*array, new_capacity * sizeof(void*));
        if (!new_array) return false;
        
        *array = new_array;
        *capacity = new_capacity;
    }
    
    // Add the item - properly cast void pointer to void**
    void** typed_array = (void**)*array;
    typed_array[*count] = item;
    (*count)++;
    return true;
}

// Simplified versions for our specific array types
static bool array_append_allocation(Region* region, Allocation* alloc) {
    if (!region || !alloc) return false;
    
    return array_append((void**)&region->allocations, 
                        &region->num_allocations, 
                        &region->capacity, 
                        alloc);
}

static bool array_append_periscope(Region* region, Periscope* periscope) {
    if (!region || !periscope) return false;
    
    // Initialize periscopes array if it doesn't exist
    if (region->periscopes == NULL) {
        region->periscopes = (Periscope**)malloc(REGION_INITIAL_PERISCOPE_CAPACITY * sizeof(Periscope*));
        if (!region->periscopes) return false;
        region->periscope_capacity = REGION_INITIAL_PERISCOPE_CAPACITY;
        region->num_periscopes = 0;
    }
    
    return array_append((void**)&region->periscopes, 
                        &region->num_periscopes, 
                        &region->periscope_capacity, 
                        periscope);
}

static void array_remove_last(void** array, size_t* count) {
    if (!array || !count || *count == 0) return;
    
    (*count)--;
}

// Find a specific allocation in a region by pointer
static Allocation* find_allocation(Region* region, void* ptr) {
    if (!region || !ptr) return NULL;
    
    for (size_t i = 0; i < region->num_allocations; i++) {
        if (region->allocations[i]->ptr == ptr) {
            return region->allocations[i];
        }
    }
    
    return NULL;
}

// Regime compatibility matrix:
// [source_regime][target_regime][direction]
// Where direction is 0 for PERISCOPE_IN, 1 for PERISCOPE_OUT
// True means allowed, false means prohibited
const bool regime_compatibility_matrix[4][4][2] = {
    // REGIME_FIFO source
    {
        // FIFO target
        { true, true },    // FIFO->FIFO is always compatible
        // FILO target
        { true, false },   // FIFO->FILO is compatible for PERISCOPE_IN only
        // SEQ target
        { true, true },    // FIFO->SEQ is always compatible
        // RAND target
        { false, false }   // FIFO->RAND is never compatible
    },
    // REGIME_FILO source
    {
        // FIFO target
        { false, true },   // FILO->FIFO is compatible for PERISCOPE_OUT only
        // FILO target
        { true, true },    // FILO->FILO is always compatible
        // SEQ target
        { false, true },   // FILO->SEQ is compatible for PERISCOPE_OUT only
        // RAND target
        { false, false }   // FILO->RAND is never compatible
    },
    // REGIME_SEQ source
    {
        // FIFO target 
        { true, false },   // SEQ->FIFO is compatible for PERISCOPE_IN only
        // FILO target
        { true, false },   // SEQ->FILO is compatible for PERISCOPE_IN only
        // SEQ target
        { true, true },    // SEQ->SEQ is always compatible
        // RAND target
        { false, false }   // SEQ->RAND is never compatible
    },
    // REGIME_RAND source
    {
        // FIFO target
        { false, false },  // RAND->FIFO is never compatible
        // FILO target
        { false, false },  // RAND->FILO is never compatible
        // SEQ target
        { false, false },  // RAND->SEQ is never compatible
        // RAND target
        { true, true }     // RAND->RAND is always compatible
    }
};

// Create a new allocation - local implementation
static Allocation* allocation_create(RegionId region_id, void* ptr, size_t size, uint32_t flags,
                                     SourcePosition source_pos, const char* label) {
    Allocation* alloc = (Allocation*)malloc(sizeof(Allocation));
    if (!alloc) return NULL;
    
    alloc->ptr = ptr;
    alloc->size = size;
    alloc->region_id = region_id;
    alloc->flags = flags;
    alloc->source_pos = source_pos;
    alloc->label = label ? strdup(label) : NULL;
    
    return alloc;
}

// Destroy an allocation - local implementation
static void allocation_destroy(Allocation* alloc) {
    if (!alloc) return;
    
    // Free the actual memory if owned by the allocation
    if (alloc->ptr) {
        free(alloc->ptr);
    }
    
    // Free the label if present
    if (alloc->label) {
        free((char*)alloc->label);
    }
    
    free(alloc);
}

// Create a new region
Region* braggi_mem_region_create(const char* name, RegimeType regime, RegionId parent) {
    return braggi_mem_region_create_with_allocator(name, regime, parent, NULL);
}

// Create a region with a memory region allocator
Region* braggi_mem_region_create_with_allocator(const char* name, RegimeType regime, 
                                          RegionId parent, MemoryRegion* memory_region) {
    Region* region;
    
    // Allocate the region struct
    if (memory_region) {
        region = (Region*)braggi_region_alloc(memory_region, sizeof(Region));
    } else {
        region = (Region*)malloc(sizeof(Region));
    }
    
    if (!region) return NULL;
    
    // Initialize region structure
    if (memory_region) {
        region->name = name ? braggi_region_strdup(memory_region, name) : NULL;
    } else {
        region->name = name ? strdup(name) : NULL;
    }
    
    region->regime = regime;
    region->parent = parent;
    region->num_allocations = 0;
    region->capacity = REGION_INITIAL_ALLOCATION_CAPACITY;
    region->memory_region = memory_region;
    
    // Initialize new fields
    region->type = memory_region ? REGION_TYPE_MEMORY_REGION : REGION_TYPE_NORMAL;
    region->flags = memory_region ? 0 : REGION_FLAG_USE_MALLOC;
    region->periscopes = NULL;
    region->num_periscopes = 0;
    region->periscope_capacity = 0;
    
    // Allocate initial allocations array
    if (memory_region) {
        region->allocations = (Allocation**)braggi_region_alloc(memory_region, 
                                                            region->capacity * sizeof(Allocation*));
    } else {
        region->allocations = (Allocation**)malloc(region->capacity * sizeof(Allocation*));
    }
    
    if (!region->allocations) {
        if (region->name) {
            if (memory_region) {
                // Memory is owned by region, no need to free
            } else {
                free((char*)region->name);
            }
        }
        
        if (memory_region) {
            // Memory is owned by region, no need to free
        } else {
            free(region);
        }
        
        return NULL;
    }
    
    // Generate a new region ID (in a real implementation, we'd have a more
    // sophisticated ID generator to ensure uniqueness)
    static RegionId next_id = 1;  // Start at 1, 0 is reserved for invalid/none
    region->id = next_id++;
    
    return region;
}

// Destroy a region and its allocations
void braggi_mem_region_destroy(Region* region) {
    if (!region) return;
    
    // If using a memory region allocator, we don't need to free individual allocations
    if (region->memory_region) {
        // For non-memory region objects inside allocations, we still need to free them
        for (size_t i = 0; i < region->num_allocations; i++) {
            Allocation* alloc = region->allocations[i];
            if (alloc && alloc->label) {
                // If label was not allocated with the memory region, free it
                // This is tricky because we don't know if it was or not
                // A more robust solution would track this information
            }
        }
        
        // Memory will be freed when the memory region is destroyed
        return;
    }
    
    // Free all allocations in the region
    for (size_t i = 0; i < region->num_allocations; i++) {
        Allocation* alloc = region->allocations[i];
        if (alloc) {
            allocation_destroy(alloc);
        }
    }
    
    // Free periscopes if they exist
    if (region->periscopes) {
        free(region->periscopes);
    }
    
    // Free region resources
    if (region->name) free((char*)region->name);
    free(region->allocations);
    free(region);
}

// Check if a regime is compatible with another
bool braggi_mem_regime_compatible(RegimeType source, RegimeType target, PeriscopeDirection direction) {
    // Validate enum values
    if (source > REGIME_RAND || target > REGIME_RAND || direction > PERISCOPE_OUT) {
        return false;
    }
    
    return regime_compatibility_matrix[source][target][direction];
}

// Get the ID of a region
RegionId braggi_mem_region_get_id(const Region* region) {
    if (!region) return 0;
    return region->id;
}

// Get the name of a region
const char* braggi_mem_region_get_name(const Region* region) {
    if (!region) return NULL;
    return region->name;
}

// Get the regime of a region
RegimeType braggi_mem_region_get_regime(const Region* region) {
    if (!region) return REGIME_RAND; // Default to RAND
    return region->regime;
}

// Get the memory region allocator of a region
MemoryRegion* braggi_mem_region_get_allocator(const Region* region) {
    if (!region) return NULL;
    return region->memory_region;
}

// Allocate memory in a region
void* braggi_mem_region_alloc(Region* region, size_t size, SourcePosition source_pos, const char* label) {
    if (!region) return NULL;
    
    void* ptr = NULL;
    Allocation* alloc = NULL;
    
    switch (region->type) {
        case REGION_TYPE_NORMAL:
            // Normal region without explicit memory region
            if (region->flags & REGION_FLAG_USE_MALLOC) {
                ptr = malloc(size);
                if (!ptr) return NULL;
                
                // Create an allocation record
                alloc = allocation_create(region->id, ptr, size, 0, source_pos, label);
                if (!alloc) {
                    free(ptr);
                    return NULL;
                }
                
                // Add to active allocations
                if (!array_append_allocation(region, alloc)) {
                    allocation_destroy(alloc);
                    return NULL;
                }
            } else {
                // Use a memory regime to manage memory
                // Logic depends on the regime
                ptr = NULL; // TBD based on regime
            }
            break;
            
        case REGION_TYPE_MEMORY_REGION:
            // Region backed by a memory region allocator
            if (!region->memory_region) return NULL;
            
            ptr = braggi_region_alloc(region->memory_region, size);
            if (!ptr) return NULL;
            
            // Create an allocation record
            alloc = allocation_create(region->id, ptr, size, 0, source_pos, label);
            if (!alloc) {
                // Can't free from memory region, but at least we won't track it
                return NULL;
            }
            
            // Add to active allocations
            if (!array_append_allocation(region, alloc)) {
                allocation_destroy(alloc);
                return NULL;
            }
            break;
            
        default:
            // Unknown region type
            return NULL;
    }
    
    return ptr;
}

// Free memory in a region
bool braggi_mem_region_free(Region* region, void* ptr) {
    if (!region || !ptr) return false;
    
    // Find the allocation
    Allocation* alloc = find_allocation(region, ptr);
    if (!alloc) {
        return false;
    }
    
    // Mark it as dead
    alloc->flags |= ALLOCATION_FLAG_DEAD;
    
    // If this is a malloc-based region, actually free the memory
    if (region->flags & REGION_FLAG_USE_MALLOC) {
        free(ptr);
    }
    
    // Success
    return true;
}

// Create a periscope from one region to another
bool braggi_mem_region_create_periscope(Region* source, Region* target, PeriscopeDirection direction) {
    if (!source || !target) return false;
    
    // Check if regimes are compatible
    if (!braggi_mem_regime_compatible(source->regime, target->regime, direction)) {
        return false;
    }
    
    // Create an empty source position for internal allocations
    SourcePosition empty_pos = {0};
    
    // Create a periscope record for the source region
    Periscope* periscope = (Periscope*)malloc(sizeof(Periscope));
    if (!periscope) return false;
    
    periscope->source_id = source->id;
    periscope->target_id = target->id;
    periscope->direction = direction;
    
    // Add to source region's periscopes
    if (!array_append_periscope(source, periscope)) {
        free(periscope);
        return false;
    }
    
    // Create an allocation record for the periscope in the source region
    Allocation* alloc = allocation_create(source->id, periscope, sizeof(Periscope), 
                                        ALLOCATION_FLAG_INTERNAL, empty_pos, "periscope");
    if (!alloc) {
        // Remove the periscope
        array_remove_last((void**)&source->periscopes, &source->num_periscopes);
        free(periscope);
        return false;
    }
    
    // Add to source region's allocations
    if (!array_append_allocation(source, alloc)) {
        // Remove the periscope
        array_remove_last((void**)&source->periscopes, &source->num_periscopes);
        allocation_destroy(alloc);
        return false;
    }
    
    return true;
}

// Get a human-readable name for a regime type
const char* braggi_mem_regime_name(RegimeType regime) {
    switch (regime) {
        case REGIME_FIFO: return "FIFO";
        case REGIME_FILO: return "FILO";
        case REGIME_SEQ:  return "SEQ";
        case REGIME_RAND: return "RAND";
        default:          return "UNKNOWN";
    }
}

/* Calloc memory in a region */
void* braggi_mem_region_calloc(Region* region, size_t nmemb, size_t size, SourcePosition source_pos, const char* label) {
    if (!region) return NULL;
    
    size_t total_size = nmemb * size;
    void* ptr = braggi_mem_region_alloc(region, total_size, source_pos, label);
    if (ptr) {
        memset(ptr, 0, total_size);
    }
    
    return ptr;
}

/* Reallocate memory in a region */
void* braggi_mem_region_realloc(Region* region, void* ptr, size_t size, SourcePosition source_pos, const char* label) {
    if (!region) return NULL;
    
    // Region allocator doesn't generally support realloc, so we use a workaround
    
    // If ptr is NULL, this is just an alloc
    if (!ptr) {
        return braggi_mem_region_alloc(region, size, source_pos, label);
    }
    
    // If size is 0, this is a free
    if (size == 0) {
        braggi_mem_region_free(region, ptr);
        return NULL;
    }
    
    // Find the allocation
    Allocation* alloc = find_allocation(region, ptr);
    if (!alloc) {
        // Ptr not owned by this region!
        return NULL;
    }
    
    // Allocate new memory
    void* new_ptr = braggi_mem_region_alloc(region, size, source_pos, label);
    if (!new_ptr) {
        return NULL;
    }
    
    // Copy data
    size_t copy_size = (alloc->size < size) ? alloc->size : size;
    memcpy(new_ptr, ptr, copy_size);
    
    // We don't actually free the old memory because region allocator
    // doesn't generally support individual frees, but we can mark it
    // as dead in our tracking
    alloc->flags |= ALLOCATION_FLAG_DEAD;
    
    return new_ptr;
}

/* Duplicate a string in a region */
char* braggi_mem_region_strdup(Region* region, const char* str, SourcePosition source_pos, const char* label) {
    if (!region || !str) return NULL;
    
    size_t len = strlen(str) + 1;
    char* new_str = (char*)braggi_mem_region_alloc(region, len, source_pos, label);
    if (new_str) {
        memcpy(new_str, str, len);
    }
    
    return new_str;
}

/*
 * "A good memory manager doesn't just allocate bytes -
 * it creates a home where your data can live its best life."
 * - Texas Memory Ranch Wisdom
 */

// Deprecated implementation (keeping for reference)
typedef struct LegacyRegion {
    char* name;
    void* memory;
    size_t size;
    size_t used;
    RegimeType regime;
    RegionId id;
    RegionId parent_id;
    // Additional fields as needed
} LegacyRegion;

// Forward declaration for compatibility functions
static void convert_legacy_region_functions(void);

Region* braggi_named_region_create(const char* name, RegimeType regime, RegionId parent) {
    // Create a modern region using the existing well-tested function
    return braggi_mem_region_create(name, regime, parent);
}

void braggi_named_region_destroy(Region* region) {
    if (!region) {
        return;
    }
    
    // Clean up using the modern implementation
    braggi_mem_region_destroy(region);
}

void* braggi_named_region_alloc(Region* region, size_t size, SourcePosition source_pos, const char* label) {
    if (!region) {
        return NULL;
    }
    
    // Allocate using the modern implementation
    return braggi_mem_region_alloc(region, size, source_pos, label);
}

void* braggi_named_region_calloc(Region* region, size_t nmemb, size_t size, SourcePosition source_pos, const char* label) {
    if (!region) {
        return NULL;
    }
    
    // Calloc using the modern implementation
    return braggi_mem_region_calloc(region, nmemb, size, source_pos, label);
}

void* braggi_named_region_realloc(Region* region, void* ptr, size_t size, SourcePosition source_pos, const char* label) {
    if (!region) {
        return NULL;
    }
    
    // Realloc using the modern implementation
    return braggi_mem_region_realloc(region, ptr, size, source_pos, label);
}

void braggi_named_region_free(Region* region, void* ptr) {
    if (!region) {
        return;
    }
    
    // Free using the modern implementation
    // Note: this could be a no-op in a real region system that does batch cleanup
}

size_t braggi_named_region_available(Region* region) {
    if (!region) {
        return 0;
    }
    
    // Calculate available space from allocations
    // This is approximate since modern regions track individual allocations differently
    size_t total_allocated = 0;
    for (size_t i = 0; i < region->num_allocations; i++) {
        if (region->allocations[i]) {
            total_allocated += region->allocations[i]->size;
        }
    }
    
    // Just return a large number - memory regions can grow as needed
    return (1024 * 1024) - total_allocated;  // 1MB arbitrary limit
}

size_t braggi_named_region_used(Region* region) {
    if (!region) {
        return 0;
    }
    
    // Calculate used space from allocations
    size_t total_used = 0;
    for (size_t i = 0; i < region->num_allocations; i++) {
        if (region->allocations[i]) {
            total_used += region->allocations[i]->size;
        }
    }
    
    return total_used;
}

RegimeType braggi_named_region_regime(Region* region) {
    if (!region) {
        return REGIME_RAND;  // Default regime
    }
    
    return region->regime;
}

RegionId braggi_named_region_id(Region* region) {
    if (!region) {
        return 0;
    }
    
    return region->id;
}

RegionId braggi_named_region_parent(Region* region) {
    if (!region) {
        return 0;
    }
    
    return region->parent;
}

void braggi_named_region_add_child(Region* parent, Region* child) {
    if (!parent || !child) {
        return;
    }
    
    // Set the parent ID
    child->parent = parent->id;
}

void braggi_named_region_remove_child(Region* parent, Region* child) {
    if (!parent || !child || child->parent != parent->id) {
        return;
    }
    
    // Clear the parent ID
    child->parent = 0;
}

bool braggi_named_region_is_ancestor(Region* ancestor, Region* descendant) {
    if (!ancestor || !descendant) {
        return false;
    }
    
    // Direct parent-child relationship
    if (descendant->parent == ancestor->id) {
        return true;
    }
    
    // Check ancestor chain
    RegionId parent_id = descendant->parent;
    while (parent_id != 0) {
        if (parent_id == ancestor->id) {
            return true;
        }
        
        // Find the parent region in our regions table and get its parent
        // This is just a stub - real implementation would search all regions
        parent_id = 0;  // No more parents to check
    }
    
    return false;
}

/* 
 * Get the global region singleton
 * "Every good program needs a solid foundation - the global region
 * is like the ranch house where all your memory critters start from!"
 */
Region* braggi_named_region_get_global(void) {
    // Our global region singleton
    static Region* global_region = NULL;
    
    // Create it if it doesn't exist yet
    if (!global_region) {
        global_region = braggi_named_region_create("global", REGIME_RAND, 0); // 0 means no parent
        if (!global_region) {
            // If we can't create it, we're in big trouble, partner!
            fprintf(stderr, "CRITICAL ERROR: Failed to create global region!\n");
            exit(EXIT_FAILURE);
        }
    }
    
    return global_region;
}

// ... remaining code ... 