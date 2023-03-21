#ifndef HEAP_H
#define HEAP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Define a memory block header
typedef struct mem_block {
    size_t size;               // Size of the free memory block
    struct mem_block *next;    // Pointer to the next free block
} mem_block_t;

// Initialize the memory pool and free list
void init_memory_pool(void);

// Allocate memory from the memory pool
void *my_malloc(size_t size);

// Free a pointer and update the free list
void my_free(void *ptr);

// Display the current state of the free list
void print_free_list(void);

#endif /* HEAP_H */
