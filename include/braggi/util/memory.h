/*
 * Braggi - Memory Management Utilities
 * 
 * "Memory managin' is like herdin' cattle - keep track of every last one
 * or you'll be chasin' strays till the cows come home!" - Irish-Texan Wisdom
 */

#ifndef BRAGGI_MEMORY_H
#define BRAGGI_MEMORY_H

#include <stddef.h>
#include <stdbool.h>

/**
 * Allocates memory with error checking and tracking
 * 
 * @param size The size in bytes to allocate
 * @param file Source file (use __FILE__)
 * @param line Line number (use __LINE__)
 * @return Pointer to allocated memory or NULL on failure
 */
void* braggi_malloc(size_t size, const char* file, int line);

/**
 * Allocates zeroed memory with error checking and tracking
 *
 * @param count Number of elements
 * @param size Size of each element
 * @param file Source file (use __FILE__)
 * @param line Line number (use __LINE__)
 * @return Pointer to allocated memory or NULL on failure
 */
void* braggi_calloc(size_t count, size_t size, const char* file, int line);

/**
 * Reallocates memory with error checking and tracking
 *
 * @param ptr Pointer to existing memory
 * @param size New size in bytes
 * @param file Source file (use __FILE__)
 * @param line Line number (use __LINE__)
 * @return Pointer to reallocated memory or NULL on failure
 */
void* braggi_realloc(void* ptr, size_t size, const char* file, int line);

/**
 * Frees memory with tracking
 *
 * @param ptr Pointer to memory to free
 * @param file Source file (use __FILE__)
 * @param line Line number (use __LINE__)
 */
void braggi_free(void* ptr, const char* file, int line);

/**
 * Creates a duplicate of a string
 *
 * @param str String to duplicate
 * @param file Source file (use __FILE__)
 * @param line Line number (use __LINE__)
 * @return Pointer to duplicated string or NULL on failure
 */
char* braggi_strdup(const char* str, const char* file, int line);

/**
 * Creates a duplicate of a memory block
 *
 * @param ptr Pointer to memory to duplicate
 * @param size Size of memory block
 * @param file Source file (use __FILE__)
 * @param line Line number (use __LINE__)
 * @return Pointer to duplicated memory or NULL on failure
 */
void* braggi_memdup(const void* ptr, size_t size, const char* file, int line);

/**
 * Gets memory usage statistics
 *
 * @param current_bytes Pointer to store current allocated bytes
 * @param peak_bytes Pointer to store peak allocated bytes
 * @param total_allocations Pointer to store total number of allocations
 * @param total_frees Pointer to store total number of frees
 */
void braggi_memory_stats(size_t* current_bytes, size_t* peak_bytes, 
                        size_t* total_allocations, size_t* total_frees);

/**
 * Reports memory leaks to stderr
 *
 * @return Number of leaks detected
 */
size_t braggi_memory_report_leaks(void);

/**
 * Convenience macros for tracking file and line
 */
#define BRAGGI_MALLOC(size) braggi_malloc(size, __FILE__, __LINE__)
#define BRAGGI_CALLOC(count, size) braggi_calloc(count, size, __FILE__, __LINE__)
#define BRAGGI_REALLOC(ptr, size) braggi_realloc(ptr, size, __FILE__, __LINE__)
#define BRAGGI_FREE(ptr) braggi_free(ptr, __FILE__, __LINE__)
#define BRAGGI_STRDUP(str) braggi_strdup(str, __FILE__, __LINE__)
#define BRAGGI_MEMDUP(ptr, size) braggi_memdup(ptr, size, __FILE__, __LINE__)

#endif /* BRAGGI_MEMORY_H */ 