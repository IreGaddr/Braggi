/*
 * Braggi - Hashmap Implementation
 * 
 * "Every key needs a home, and every value needs a way to be found.
 * A good hashmap provides both!" - Irish-Texan Data Structure Wisdom
 */

#ifndef BRAGGI_UTIL_HASHMAP_H
#define BRAGGI_UTIL_HASHMAP_H

#include <stdbool.h>
#include <stddef.h>

/* Forward declarations */
typedef struct HashMap HashMap;
typedef struct HashMapIterator HashMapIterator;

/* Function types */
typedef unsigned int (*HashFunction)(const void* key);
typedef bool (*EqualsFunction)(const void* a, const void* b);
typedef void (*FreeFunction)(void* ptr);

/* HashMap functions */
HashMap* braggi_hashmap_create(void);
void braggi_hashmap_destroy(HashMap* map);
size_t braggi_hashmap_size(const HashMap* map);
bool braggi_hashmap_put(HashMap* map, void* key, void* value);
void* braggi_hashmap_get(const HashMap* map, const void* key);
void* braggi_hashmap_remove(HashMap* map, const void* key);
void braggi_hashmap_set_functions(HashMap* map, HashFunction hash_fn, EqualsFunction equals_fn);
void braggi_hashmap_set_free_functions(HashMap* map, FreeFunction key_free_fn, FreeFunction value_free_fn);

/* Iterator functions */
void braggi_hashmap_iterator_init(const HashMap* map, HashMapIterator* iter);
bool braggi_hashmap_iterator_next(HashMapIterator* iter, void** key, void** value);

/* Utility functions */
unsigned int braggi_hashmap_hash_pointer(const void* key);
bool braggi_hashmap_equals_pointer(const void* a, const void* b);

#endif /* BRAGGI_UTIL_HASHMAP_H */ 