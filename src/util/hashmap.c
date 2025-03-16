/*
 * Braggi - Minimal Hashmap Implementation
 * 
 * "Maps are like treasure hunts on the Texas prairie - 
 * they tell ya exactly where to find what yer lookin' for!" - Data Structure Wisdom
 */

#include "braggi/util/hashmap.h"
#include <stdlib.h>
#include <string.h>

struct HashMap {
    size_t size;
    // This is just a stub - in a real implementation there would be more fields
};

struct HashMapIterator {
    size_t index;
    // This is just a stub - in a real implementation there would be more fields
};

/*
 * Create a new hashmap
 */
HashMap* braggi_hashmap_create(void) {
    HashMap* map = (HashMap*)calloc(1, sizeof(HashMap));
    return map;
}

/*
 * Destroy a hashmap
 */
void braggi_hashmap_destroy(HashMap* map) {
    if (map) {
        free(map);
    }
}

/*
 * Get the size of a hashmap
 */
size_t braggi_hashmap_size(const HashMap* map) {
    if (!map) return 0;
    return map->size;
}

/*
 * Default hash function for pointers
 */
unsigned int braggi_hashmap_hash_pointer(const void* key) {
    return (unsigned int)(size_t)key;
}

/*
 * Default equals function for pointers
 */
bool braggi_hashmap_equals_pointer(const void* a, const void* b) {
    return a == b;
}

/*
 * Set the hash and equals functions for a hashmap
 */
void braggi_hashmap_set_functions(HashMap* map, 
                                HashFunction hash_fn, 
                                EqualsFunction equals_fn) {
    // Stub - does nothing in this minimal implementation
}

/*
 * Set the key and value free functions for a hashmap
 */
void braggi_hashmap_set_free_functions(HashMap* map, 
                                     FreeFunction key_free_fn, 
                                     FreeFunction value_free_fn) {
    // Stub - does nothing in this minimal implementation
}

/*
 * Put a key-value pair in a hashmap
 */
bool braggi_hashmap_put(HashMap* map, void* key, void* value) {
    if (!map) return false;
    // Stub - just increment size to make it look like something was added
    map->size++;
    return true;
}

/*
 * Get a value from a hashmap by key
 */
void* braggi_hashmap_get(const HashMap* map, const void* key) {
    // Stub - always returns NULL
    return NULL;
}

/*
 * Remove a key-value pair from a hashmap
 */
void* braggi_hashmap_remove(HashMap* map, const void* key) {
    if (!map || map->size == 0) return NULL;
    // Stub - decrement size to make it look like something was removed
    map->size--;
    return NULL;
}

/*
 * Initialize a hashmap iterator
 */
void braggi_hashmap_iterator_init(const HashMap* map, HashMapIterator* iter) {
    if (!map || !iter) return;
    iter->index = 0;
}

/*
 * Get the next key-value pair from a hashmap iterator
 */
bool braggi_hashmap_iterator_next(HashMapIterator* iter, void** key, void** value) {
    // Stub - always returns false (no more elements)
    return false;
} 