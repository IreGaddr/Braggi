/*
 * Braggi - Dynamic Vector Implementation
 * 
 * "A good vector is like a Texas cattle ranch - it starts small,
 * but has plenty of room to grow!" - Data Structure Philosophy
 */

#ifndef BRAGGI_UTIL_VECTOR_H
#define BRAGGI_UTIL_VECTOR_H

#include <stddef.h>
#include <stdbool.h>
#include <sys/types.h>  // For ssize_t
#include "braggi/mem/region.h"

// Forward declaration - will be included by implementation
struct MemoryRegion;
typedef struct MemoryRegion MemoryRegion;

/**
 * Callback type definitions for vector operations
 */
typedef void (*VectorElementDestructor)(void* element);
typedef int (*VectorElementCompare)(const void* a, const void* b);
typedef int (*VectorElementSort)(const void* a, const void* b);
typedef void* (*VectorElementCopy)(const void* element);

/**
 * Vector - A dynamically resizable array
 */
typedef struct Vector {
    void* data;          /* Pointer to the array data */
    size_t elem_size;    /* Size of each element in bytes */
    size_t capacity;     /* Current capacity in elements */
    size_t size;         /* Current number of elements */
} Vector;

/* Function prototypes */

/**
 * Create a new vector with a specific element size
 * 
 * @param elem_size Size of each element in bytes
 * @return A new vector, or NULL if allocation fails
 */
Vector* braggi_vector_create(size_t elem_size);

/**
 * Create a new vector with a specific initial capacity
 * 
 * @param elem_size Size of each element in bytes
 * @param initial_capacity Initial capacity in elements
 * @return A new vector, or NULL if allocation fails
 */
Vector* braggi_vector_create_with_capacity(size_t elem_size, size_t initial_capacity);

/**
 * Create a new vector using a memory region
 * 
 * @param region The memory region to allocate from
 * @param elem_size Size of each element in bytes
 * @return A new vector allocated from the region, or NULL if allocation fails
 */
Vector* braggi_vector_create_in_region(Region* region, size_t elem_size);

/**
 * Destroy a vector and free its resources
 * 
 * @param vector The vector to destroy
 */
void braggi_vector_destroy(Vector* vector);

// Set the destroy function for elements
void braggi_vector_set_destroy_fn(Vector* vector, void (*destroy_fn)(void*));

/**
 * Get the size of the vector
 * 
 * @param vector The vector
 * @return The size in elements
 */
size_t braggi_vector_size(const Vector* vector);

/**
 * Get the capacity of the vector
 * 
 * @param vector The vector
 * @return The capacity in elements
 */
size_t braggi_vector_capacity(const Vector* vector);

/**
 * Check if the vector is empty
 * 
 * @param vector The vector
 * @return true if empty, false otherwise
 */
bool braggi_vector_empty(const Vector* vector);

/**
 * Reserve capacity in a vector
 * 
 * @param vector The vector
 * @param capacity The capacity to reserve
 * @return true if successful, false if allocation fails
 */
bool braggi_vector_reserve(Vector* vector, size_t capacity);

/**
 * Push an element to the back of the vector
 */
bool braggi_vector_push(Vector* vector, const void* elem);

/**
 * Push an element to the back of the vector (alias of push)
 */
bool braggi_vector_push_back(Vector* vector, const void* elem);

/**
 * Pop an element from the back of the vector
 */
bool braggi_vector_pop(Vector* vector);

/**
 * Pop an element from the back of the vector and store it
 */
bool braggi_vector_pop_back(Vector* vector, void* elem_out);

/**
 * Get a pointer to an element in the vector
 * 
 * @param vector The vector
 * @param index The index of the element
 * @return Pointer to the element, or NULL if out of bounds
 */
void* braggi_vector_get(Vector* vector, size_t index);

/**
 * Set an element in the vector
 * 
 * @param vector The vector
 * @param index The index of the element
 * @param elem Pointer to the element data to copy
 * @return true if successful, false if out of bounds
 */
bool braggi_vector_set(Vector* vector, size_t index, const void* elem);

/**
 * Insert an element at a specific position
 * 
 * @param vector The vector
 * @param index The index to insert at
 * @param elem Pointer to the element data to copy
 * @return true if successful, false if allocation fails or index is out of bounds
 */
bool braggi_vector_insert(Vector* vector, size_t index, const void* elem);

/**
 * Clear all elements from the vector
 */
void braggi_vector_clear(Vector* vector);

/**
 * Find the index of an element using a comparison function
 */
ssize_t braggi_vector_find(const Vector* vector, const void* element, VectorElementCompare compare);

/**
 * Sort the vector using the provided comparison function
 */
void braggi_vector_sort(Vector* vector, VectorElementSort compare);

/**
 * Create a deep copy of the vector
 */
Vector* braggi_vector_copy(const Vector* vector, VectorElementCopy copy_func);

/**
 * Resize a vector to a specific size
 */
bool braggi_vector_resize(Vector* vector, size_t new_size);

/**
 * Shrink a vector's capacity to fit its size
 */
void braggi_vector_shrink_to_fit(Vector* vector);

/**
 * Iterate over each element in the vector
 */
void braggi_vector_foreach(const Vector* vector, void (*func)(void* element, void* user_data), void* user_data);

/**
 * Add space for a new element at the end of the vector and return a pointer to it
 */
void* braggi_vector_emplace(Vector* vector);

/**
 * Remove an element at the specified index
 */
bool braggi_vector_remove_at(Vector* vector, size_t index);

/**
 * Remove an element with the specified value
 */
bool braggi_vector_remove(Vector* vector, const void* elem);

/**
 * Erase an element at the specified index (similar to remove_at but without error message)
 */
bool braggi_vector_erase(Vector* vector, size_t index);

// Convenience function to get the last element
static inline void* braggi_vector_back(const Vector* vector) {
    if (!vector || vector->size == 0) return NULL;
    return vector->data;
}

#endif /* BRAGGI_UTIL_VECTOR_H */ 