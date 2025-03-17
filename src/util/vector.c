/*
 * Braggi - Vector Utility Implementation
 * 
 * "If y'all need to store a whole mess of items that keeps growin',
 * ya need yerself a good vector!" - Texan Programming Wisdom
 */

#include "braggi/util/vector.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>  // For ssize_t

// Include the necessary headers for region support
#include "braggi/mem/region.h"
#include "braggi/mem/regime.h"

// Forward declaration if needed for any remaining MemoryRegion references
typedef struct MemoryRegion MemoryRegion;

/*
 * "Good code is like a good pair of boots - well-constructed, 
 * dependable, and doesn't leak when you step in something unexpected."
 * - Texan-Irish Programmer's Handbook
 */

// Default initial capacity for vectors
#define VECTOR_DEFAULT_CAPACITY 8

// Growth factor when resizing
#define VECTOR_GROWTH_FACTOR 2

/* 
 * Create a new vector with the specified element size
 */
Vector* braggi_vector_create(size_t elem_size) {
    return braggi_vector_create_with_capacity(elem_size, VECTOR_DEFAULT_CAPACITY);
}

/*
 * Create a new vector with the specified element size and initial capacity
 */
Vector* braggi_vector_create_with_capacity(size_t elem_size, size_t initial_capacity) {
    Vector* vector = (Vector*)malloc(sizeof(Vector));
    if (!vector) {
        return NULL;
    }
    
    vector->elem_size = elem_size;
    vector->size = 0;
    vector->capacity = initial_capacity;
    
    vector->data = malloc(vector->capacity * vector->elem_size);
    if (!vector->data) {
        free(vector);
        return NULL;
    }
    
    return vector;
}

/*
 * Create a new vector in the specified memory region
 */
Vector* braggi_vector_create_in_region(Region* region, size_t elem_size) {
    Vector* vector;
    
    if (region) {
        vector = (Vector*)braggi_mem_region_alloc(region, sizeof(Vector));
    } else {
        vector = (Vector*)malloc(sizeof(Vector));
    }
    
    if (!vector) {
        return NULL;
    }
    
    // Set default element size
    vector->elem_size = elem_size > 0 ? elem_size : sizeof(void*);
    vector->size = 0;
    vector->capacity = VECTOR_DEFAULT_CAPACITY;
    
    // Allocate data array from the region or standard memory
    if (region) {
        vector->data = braggi_mem_region_alloc(region, vector->capacity * vector->elem_size);
    } else {
        vector->data = malloc(vector->capacity * vector->elem_size);
    }
    
    if (!vector->data) {
        if (region) {
            // No need to free the vector as it's part of the region
        } else {
            free(vector);
        }
        return NULL;
    }
    
    return vector;
}

/*
 * Destroy a vector and free its memory
 */
void braggi_vector_destroy(Vector* vector) {
    if (vector) {
        if (vector->data) {
            free(vector->data);
        }
        free(vector);
    }
}

/*
 * Resize a vector to a new capacity
 */
bool braggi_vector_resize(Vector* vector, size_t new_capacity) {
    if (!vector || new_capacity < vector->size) {
        return false;
    }
    
    void* new_data = realloc(vector->data, new_capacity * vector->elem_size);
    if (!new_data) {
        return false;
    }
    
    vector->data = new_data;
    vector->capacity = new_capacity;
    
    return true;
}

/*
 * Add an element to the end of the vector
 */
bool braggi_vector_push(Vector* vector, const void* element) {
    if (!vector || !element) {
        return false;
    }
    
    // Resize if necessary
    if (vector->size >= vector->capacity) {
        size_t new_capacity = vector->capacity * 2;
        if (!braggi_vector_resize(vector, new_capacity)) {
            return false;
        }
    }
    
    // Copy the element to the end of the vector
    char* dst = (char*)vector->data + (vector->size * vector->elem_size);
    memcpy(dst, element, vector->elem_size);
    
    // Increment size
    vector->size++;
    
    return true;
}

/*
 * Remove the last element from the vector
 */
bool braggi_vector_pop(Vector* vector) {
    if (!vector || vector->size == 0) {
        return false;
    }
    
    vector->size--;
    return true;
}

/*
 * Get a pointer to the element at the specified index
 */
void* braggi_vector_get(Vector* vector, size_t index) {
    if (!vector || index >= vector->size) {
        return NULL;
    }
    
    return (char*)vector->data + (index * vector->elem_size);
}

/*
 * Set the element at the specified index
 */
bool braggi_vector_set(Vector* vector, size_t index, const void* element) {
    if (!vector || !element || index >= vector->size) {
        return false;
    }
    
    void* dst = (char*)vector->data + (index * vector->elem_size);
    memcpy(dst, element, vector->elem_size);
    
    return true;
}

/*
 * Insert an element at the specified index
 */
bool braggi_vector_insert(Vector* vector, size_t index, const void* element) {
    if (!vector || !element || index > vector->size) {
        return false;
    }
    
    // If inserting at the end, use push
    if (index == vector->size) {
        return braggi_vector_push(vector, element);
    }
    
    // Resize if necessary
    if (vector->size >= vector->capacity) {
        size_t new_capacity = vector->capacity * 2;
        if (!braggi_vector_resize(vector, new_capacity)) {
            return false;
        }
    }
    
    // Shift elements after the insertion point
    char* src = (char*)vector->data + (index * vector->elem_size);
    char* dst = src + vector->elem_size;
    size_t bytes_to_move = (vector->size - index) * vector->elem_size;
    memmove(dst, src, bytes_to_move);
    
    // Insert the new element
    memcpy(src, element, vector->elem_size);
    
    // Increment size
    vector->size++;
    
    return true;
}

/*
 * Remove an element at the specified index
 */
bool braggi_vector_remove_at(Vector* vector, size_t index) {
    if (!vector || index >= vector->size) {
        return false;
    }
    
    // If removing the last element, use pop
    if (index == vector->size - 1) {
        return braggi_vector_pop(vector);
    }
    
    // Shift elements after the removal point
    char* dst = (char*)vector->data + (index * vector->elem_size);
    char* src = dst + vector->elem_size;
    size_t bytes_to_move = (vector->size - index - 1) * vector->elem_size;
    memmove(dst, src, bytes_to_move);
    
    // Decrement size
    vector->size--;
    
    return true;
}

/*
 * Remove an element with the specified value
 */
bool braggi_vector_remove(Vector* vector, const void* elem) {
    if (!vector || !elem) {
        return false;
    }
    
    // Find the element
    for (size_t i = 0; i < vector->size; i++) {
        void* current = (char*)vector->data + (i * vector->elem_size);
        if (memcmp(current, elem, vector->elem_size) == 0) {
            // Remove the element at this index
            return braggi_vector_remove_at(vector, i);
        }
    }
    
    return false;  // Element not found
}

/*
 * Clear all elements from the vector
 */
void braggi_vector_clear(Vector* vector) {
    if (vector) {
        vector->size = 0;
    }
}

/*
 * Get the number of elements in the vector
 */
size_t braggi_vector_size(const Vector* vector) {
    return vector ? vector->size : 0;
}

/*
 * Get the capacity of the vector
 */
size_t braggi_vector_capacity(const Vector* vector) {
    return vector ? vector->capacity : 0;
}

/*
 * Check if the vector is empty
 */
bool braggi_vector_empty(const Vector* vector) {
    return vector ? vector->size == 0 : true;
}

/*
 * Ensure the vector has enough capacity
 */
static bool ensure_capacity(Vector* vector, size_t capacity) {
    if (vector->capacity >= capacity) {
        return true;
    }
    
    size_t new_capacity = vector->capacity;
    while (new_capacity < capacity) {
        new_capacity *= VECTOR_GROWTH_FACTOR;
    }
    
    void* new_data = realloc(vector->data, new_capacity * vector->elem_size);
    if (!new_data) {
        return false;
    }
    
    vector->data = new_data;
    vector->capacity = new_capacity;
    
    return true;
}

/*
 * Add an element to the end of a vector
 */
bool braggi_vector_push_back(Vector* vector, const void* elem) {
    if (!vector) {
        return false;
    }
    
    if (vector->size >= vector->capacity) {
        if (!ensure_capacity(vector, vector->size + 1)) {
            return false;
        }
    }
    
    memcpy((char*)vector->data + (vector->size * vector->elem_size), elem, vector->elem_size);
    vector->size++;
    return true;
}

/*
 * Remove and return the last element of a vector
 */
bool braggi_vector_pop_back(Vector* vector, void* out_elem) {
    if (vector->size == 0) {
        return false;
    }
    
    if (out_elem) {
        memcpy(out_elem, (char*)vector->data + ((vector->size - 1) * vector->elem_size), vector->elem_size);
    }
    
    vector->size--;
    return true;
}

/*
 * Remove an element at a specific position and free it using the provided destructor
 */
bool braggi_vector_remove_and_free(Vector* vector, size_t index, void (*destructor)(void*)) {
    if (!vector || index >= vector->size) {
        return false;
    }
    
    // Get pointer to element
    void* element = (char*)vector->data + (index * vector->elem_size);
    
    // Destroy element if a destructor was provided
    if (destructor) {
        destructor(element);
    }
    
    return braggi_vector_remove_at(vector, index);
}

/*
 * Find an element in a vector
 */
ssize_t braggi_vector_find(const Vector* vector, const void* element, int (*compare)(const void*, const void*)) {
    if (!vector || !element || !compare) {
        return -1;
    }
    
    for (size_t i = 0; i < vector->size; i++) {
        void* current = (char*)vector->data + (i * vector->elem_size);
        if (compare(current, element) == 0) {
            return i;
        }
    }
    
    return -1;
}

/*
 * Sort a vector
 */
void braggi_vector_sort(Vector* vector, VectorElementSort compare) {
    if (!vector || !compare || vector->size <= 1) {
        return;
    }
    
    qsort(vector->data, vector->size, vector->elem_size, compare);
}

/*
 * Copy a vector
 */
Vector* braggi_vector_copy(const Vector* vector, void* (*copy_func)(const void*)) {
    if (!vector) {
        return NULL;
    }
    
    Vector* new_vector = braggi_vector_create_with_capacity(vector->elem_size, vector->capacity);
    if (!new_vector) {
        return NULL;
    }
    
    if (copy_func) {
        // Deep copy with custom copy function
        for (size_t i = 0; i < vector->size; i++) {
            void* src_elem = (char*)vector->data + (i * vector->elem_size);
            void* new_elem = copy_func(src_elem);
            memcpy((char*)new_vector->data + (i * vector->elem_size), new_elem, vector->elem_size);
            // If copy_func allocates memory, caller is responsible for freeing it
            free(new_elem);
        }
    } else {
        // Shallow copy
        memcpy(new_vector->data, vector->data, vector->size * vector->elem_size);
    }
    
    new_vector->size = vector->size;
    return new_vector;
}

/*
 * Reserve capacity for a vector
 */
bool braggi_vector_reserve(Vector* vector, size_t capacity) {
    if (!vector || capacity <= vector->capacity) {
        return true; // Nothing to do
    }
    
    void* new_data = realloc(vector->data, capacity * vector->elem_size);
    if (!new_data) {
        return false;
    }
    
    vector->data = new_data;
    vector->capacity = capacity;
    return true;
}

/*
 * Shrink a vector's capacity to fit its size
 */
void braggi_vector_shrink_to_fit(Vector* vector) {
    if (!vector || vector->size == 0) {
        return;
    }
    
    if (vector->size == vector->capacity) {
        return;
    }
    
    void* new_data = realloc(vector->data, vector->size * vector->elem_size);
    if (new_data) {
        vector->data = new_data;
        vector->capacity = vector->size;
    }
}

/*
 * For each element in the vector, call the given function
 */
void braggi_vector_foreach(const Vector* vec, void (*func)(void* elem, void* user_data), void* user_data) {
    if (!vec || !func) return;
    
    for (size_t i = 0; i < vec->size; i++) {
        func((char*)vec->data + (i * vec->elem_size), user_data);
    }
}

/*
 * Set the destroy function for elements - removed since Vector doesn't have this member
 */
void braggi_vector_set_destroy_fn(Vector* vector, void (*destroy_fn)(void*)) {
    // This function is kept for API compatibility, but doesn't do anything
    // since Vector doesn't have a destroy_fn member
    (void)vector;  // Prevent unused parameter warning
    (void)destroy_fn;
}

/*
 * Add space for a new element at the end of the vector and return a pointer to it.
 * 
 * "Like makin' room for one more cowboy at the campfire, this 
 * function gives ya a spot to put yer data without havin' to
 * pass it in right away!" - Texas Data Wrangler
 */
void* braggi_vector_emplace(Vector* vector) {
    if (!vector) {
        return NULL;
    }
    
    if (vector->size >= vector->capacity) {
        size_t new_capacity = vector->capacity * 2;
        void* new_data = realloc(vector->data, new_capacity * vector->elem_size);
        if (!new_data) {
            return NULL;
        }
        
        vector->data = new_data;
        vector->capacity = new_capacity;
    }
    
    // Return pointer to the new element position
    void* element_ptr = (char*)vector->data + (vector->size * vector->elem_size);
    
    // Clear the memory at the new position
    memset(element_ptr, 0, vector->elem_size);
    
    // Increment size after providing the pointer
    vector->size++;
    
    return element_ptr;
}

/**
 * Erase an element at the specified index
 * Similar to remove_at but doesn't set the error message
 */
bool braggi_vector_erase(Vector* vector, size_t index) {
    // Check for invalid parameters
    if (!vector || index >= vector->size) {
        return false;
    }
    
    // Calculate pointer to the element to remove
    char* data = (char*)vector->data;
    char* elem_ptr = data + (index * vector->elem_size);
    
    // Calculate pointer to the element after the one to remove
    char* next_elem_ptr = elem_ptr + vector->elem_size;
    
    // Number of bytes to move
    size_t bytes_to_move = (vector->size - index - 1) * vector->elem_size;
    
    // Move all elements after index one position back
    if (bytes_to_move > 0) {
        memmove(elem_ptr, next_elem_ptr, bytes_to_move);
    }
    
    // Update size
    vector->size--;
    
    return true;
} 