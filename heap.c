#include "heap.h"
#include <stdlib.h>
#include <math.h>
#include <time.h>



// Define the memory pool and the free list
#define POOL_SIZE 1048576 * 100  // Size of the memory pool (1MB * NUM)
static char memory_pool[POOL_SIZE]; // Allocate contigious memory to stack
static mem_block_t *free_list = NULL; // Declare free list globally

// Initialize the memory pool and free list
void init_memory_pool(void) {
    mem_block_t *block = (mem_block_t *)memory_pool; // Create the initial memory block
    block->size = POOL_SIZE - sizeof(mem_block_t);
    block->next = NULL;

    // Add the block to the free list
    free_list = block;
}

// Allocate memory from the memory pool
void *my_malloc(size_t size) {
    
    mem_block_t *block = free_list;
    mem_block_t *prev = NULL;

    // Search the free list for a block that is large enough
    while (block != NULL) {
        if (block->size >= size) {
            // Found a suitable block, remove it from the free list
            if (prev == NULL) {
                free_list = block->next; // Drop first block in free list and update head
            } else {
                prev->next = block->next; // Exclude memory block from free list
            }

            // Split the block if necessary
            if (block->size >= size + sizeof(mem_block_t)) {
                mem_block_t *new_block = (mem_block_t *)((char *)block + sizeof(mem_block_t) + size); // Calculate the starting address of the unusued space
                new_block->size = block->size - size - sizeof(mem_block_t); // Update the new blocks size
                new_block->next = block->next; // Update the new blocks next node
                block->size = size; // Update the old blocks size
                block->next = NULL; 
                // Set head or notify preceeding memory block
                if (prev == NULL) {
                    free_list = new_block;
                } else {
                    prev->next = new_block;
                }
            }

            // Return the allocated memory
            return ((char *)block + sizeof(mem_block_t));
        }

        prev = block;
        block = block->next;
    }

    // No suitable block was found
    printf("ERROR: No suitable block found in program heap\n\n");
    print_free_list();
    return NULL;
}


// Free a pointer and update the free list
void my_free(void *ptr) {

    // Do nothing for NULL pointers
    if (ptr == NULL) {
        return;
    }

    mem_block_t *block = (mem_block_t *)((char *)ptr - sizeof(mem_block_t)); // Calculate the header address of the block
    mem_block_t *current = free_list;
    mem_block_t *prev = NULL;

    // Find the correct position in the free list to insert the block
    while (current != NULL && current < block) {
        prev = current;
        current = current->next;
    }

    // Insert the block into the free list
    if (prev == NULL) {
        free_list = block;
    } else {
        prev->next = block;
    }

    block->next = current;

    // Merge adjacent free blocks
    while (current != NULL && (char *)block + block->size + sizeof(mem_block_t) == (char *)current) {
        block->size += current->size + sizeof(mem_block_t);
        block->next = current->next;
        current = current->next;
    }

    while (prev != NULL && (char *)prev + prev->size + sizeof(mem_block_t) == (char *)block) {
        prev->size += block->size + sizeof(mem_block_t);
        prev->next = block->next;
        block = prev;
        prev = prev->next;
    }
}


// Display the current state of the free list
void print_free_list(void) {

    // Initialize tracking variables
    int num_blocks = 0;
    size_t total_free = 0;
    mem_block_t *block = free_list;

    // Iterate over free list and update tracking varaibles
    while (block != NULL) {
        num_blocks++;
        total_free += block->size;
        block = block->next;
    }

    // Display free list information
    printf("Free List:\n");
    printf("  Number of blocks: %d\n", num_blocks);
    printf("  Total free memory: %zu bytes\n", total_free);
    printf("  Block details:\n");

    // Display individual block information
    block = free_list;
    while (block != NULL) {
        printf("    Size: %zu bytes\n", block->size);
        printf("    Address: %p\n", block);
        block = block->next;
    }
}


