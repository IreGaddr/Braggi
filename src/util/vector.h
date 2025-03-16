/* 
 * Braggi - Dynamic Array Implementation
 * 
 * "A good vector's like a Texas ranch - plenty of room to grow
 * and all your critters in one place!" - Wise words from the prairie
 */

#ifndef BRAGGI_VECTOR_H
#define BRAGGI_VECTOR_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Vector struct - dynamically resizing array
typedef struct Vector {
    void** data;        // Array of void pointers
    size_t size;        // Current number of elements
    size_t capacity;    // Current capacity
    bool owns_elements; // Whether vector should free elements
} Vector;

// Create and destroy
Vector* braggi_vector_create(size_t initial_capacity);
Vector* braggi_vector_create_with_options(size_t initial_capacity, bool owns_elements);
void braggi_vector_destroy(Vector* vector);

// Element access
void* braggi_vector_get(const Vector* vector, size_t index);
bool braggi_vector_set(Vector* vector, size_t index, void* element);
void* braggi_vector_front(const Vector* vector);
void* braggi_vector_back(const Vector* vector);

// Modification
bool braggi_vector_push_back(Vector* vector, void* element);
void* braggi_vector_pop_back(Vector* vector);
bool braggi_vector_insert(Vector* vector, size_t index, void* element);
bool braggi_vector_remove(Vector* vector, size_t index);
bool braggi_vector_clear(Vector* vector);
bool braggi_vector_resize(Vector* vector, size_t new_capacity);

// Information
size_t braggi_vector_size(const Vector* vector);
size_t braggi_vector_capacity(const Vector* vector);
bool braggi_vector_is_empty(const Vector* vector);

// Iteration helpers
typedef bool (*VectorIteratorFn)(void* element, void* user_data);
bool braggi_vector_foreach(Vector* vector, VectorIteratorFn fn, void* user_data);

#endif /* BRAGGI_VECTOR_H */ 