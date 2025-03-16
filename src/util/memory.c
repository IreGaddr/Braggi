/*
 * Braggi - Memory Management Implementation
 * 
 * "Memory leaks are like lost cattle - round 'em up before they
 * stampede your whole darn program!" - Ranch Code Principles
 */

#include "braggi/util/memory.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

/* Memory tracking structure */
typedef struct MemoryBlock {
    void* ptr;
    size_t size;
    const char* file;
    int line;
    struct MemoryBlock* next;
} MemoryBlock;

/* Global memory tracking state */
static MemoryBlock* memory_blocks = NULL;
static size_t current_allocated_bytes = 0;
static size_t peak_allocated_bytes = 0;
static size_t total_allocations = 0;
static size_t total_frees = 0;
static pthread_mutex_t memory_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Helper to add memory block to tracking */
static void track_allocation(void* ptr, size_t size, const char* file, int line) {
    if (!ptr) return;

    pthread_mutex_lock(&memory_mutex);
    
    MemoryBlock* block = malloc(sizeof(MemoryBlock));
    if (block) {
        block->ptr = ptr;
        block->size = size;
        block->file = file;
        block->line = line;
        block->next = memory_blocks;
        memory_blocks = block;
        
        current_allocated_bytes += size;
        if (current_allocated_bytes > peak_allocated_bytes) {
            peak_allocated_bytes = current_allocated_bytes;
        }
        total_allocations++;
    }
    
    pthread_mutex_unlock(&memory_mutex);
}

/* Helper to remove memory block from tracking */
static void untrack_allocation(void* ptr, const char* file, int line) {
    if (!ptr) return;

    pthread_mutex_lock(&memory_mutex);
    
    MemoryBlock** pp = &memory_blocks;
    while (*pp) {
        MemoryBlock* block = *pp;
        if (block->ptr == ptr) {
            *pp = block->next;
            current_allocated_bytes -= block->size;
            total_frees++;
            free(block);
            pthread_mutex_unlock(&memory_mutex);
            return;
        }
        pp = &block->next;
    }
    
    /* If we get here, we're trying to free something we didn't allocate */
    fprintf(stderr, "Warning: Attempt to free untracked memory at %p from %s:%d\n", 
            ptr, file, line);
    
    pthread_mutex_unlock(&memory_mutex);
}

void* braggi_malloc(size_t size, const char* file, int line) {
    void* ptr = malloc(size);
    if (ptr) {
        track_allocation(ptr, size, file, line);
    } else if (size > 0) {
        fprintf(stderr, "Error: Failed to allocate %zu bytes at %s:%d\n", 
                size, file, line);
    }
    return ptr;
}

void* braggi_calloc(size_t count, size_t size, const char* file, int line) {
    void* ptr = calloc(count, size);
    if (ptr) {
        track_allocation(ptr, count * size, file, line);
    } else if (count > 0 && size > 0) {
        fprintf(stderr, "Error: Failed to allocate %zu bytes at %s:%d\n", 
                count * size, file, line);
    }
    return ptr;
}

void* braggi_realloc(void* ptr, size_t size, const char* file, int line) {
    if (ptr == NULL) {
        return braggi_malloc(size, file, line);
    }
    
    /* Find existing block to update size */
    size_t old_size = 0;
    
    pthread_mutex_lock(&memory_mutex);
    MemoryBlock* block = memory_blocks;
    while (block) {
        if (block->ptr == ptr) {
            old_size = block->size;
            break;
        }
        block = block->next;
    }
    pthread_mutex_unlock(&memory_mutex);
    
    void* new_ptr = realloc(ptr, size);
    if (new_ptr) {
        if (old_size > 0) {
            untrack_allocation(ptr, file, line);
        }
        track_allocation(new_ptr, size, file, line);
    } else if (size > 0) {
        fprintf(stderr, "Error: Failed to reallocate %p to %zu bytes at %s:%d\n", 
                ptr, size, file, line);
    }
    
    return new_ptr;
}

void braggi_free(void* ptr, const char* file, int line) {
    if (ptr) {
        untrack_allocation(ptr, file, line);
        free(ptr);
    }
}

char* braggi_strdup(const char* str, const char* file, int line) {
    if (!str) return NULL;
    
    size_t len = strlen(str) + 1;
    char* dup = braggi_malloc(len, file, line);
    if (dup) {
        memcpy(dup, str, len);
    }
    return dup;
}

void* braggi_memdup(const void* ptr, size_t size, const char* file, int line) {
    if (!ptr) return NULL;
    
    void* dup = braggi_malloc(size, file, line);
    if (dup) {
        memcpy(dup, ptr, size);
    }
    return dup;
}

void braggi_memory_stats(size_t* current_bytes, size_t* peak_bytes, 
                       size_t* total_allocs, size_t* total_free) {
    pthread_mutex_lock(&memory_mutex);
    
    if (current_bytes) *current_bytes = current_allocated_bytes;
    if (peak_bytes) *peak_bytes = peak_allocated_bytes;
    if (total_allocs) *total_allocs = total_allocations;
    if (total_free) *total_free = total_frees;
    
    pthread_mutex_unlock(&memory_mutex);
}

size_t braggi_memory_report_leaks(void) {
    size_t leak_count = 0;
    size_t total_leaked_bytes = 0;
    
    pthread_mutex_lock(&memory_mutex);
    
    MemoryBlock* block = memory_blocks;
    while (block) {
        fprintf(stderr, "Memory leak: %zu bytes at %p allocated at %s:%d\n",
                block->size, block->ptr, block->file, block->line);
        leak_count++;
        total_leaked_bytes += block->size;
        block = block->next;
    }
    
    if (leak_count > 0) {
        fprintf(stderr, "Total memory leaks: %zu blocks, %zu bytes\n", 
                leak_count, total_leaked_bytes);
    }
    
    pthread_mutex_unlock(&memory_mutex);
    
    return leak_count;
} 