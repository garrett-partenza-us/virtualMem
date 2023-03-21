#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "heap.h"
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

#define BLOCK_SIZE 4096 // Page size
#define NUM_BLOCKS 5 // Number of pages in physical memory
#define PHYS_MEM_SIZE BLOCK_SIZE * NUM_BLOCKS // Total size of physical memory
#define NUM_ENTRIES NUM_BLOCKS * 2 // Number of entries in a process page table
#define NUM_THREADS 3 // Number of concurrent processes

// Page table entry struct
typedef struct entry
{
    bool isPresent;
    void *address;
    int block;
} entry_t;

// Page table struct
typedef struct pagetable
{
    entry_t *entries[NUM_ENTRIES];
} pagetable_t;

// Process struct
typedef struct process
{
    int pid;
    pagetable_t *pagetable;
} process_t;

// Block of physical memory struct
typedef struct block
{
    time_t timestamp;
    process_t *process;
    void *address;
    bool used;
    int vm_idx;
} block_t;

// Physical memory struct
typedef struct phys_mem
{
    void *memory;
    bool isFull;
    block_t *blocks;
} phys_mem_t;

// Global variables
phys_mem_t *phys_mem; // Physical memory
pthread_mutex_t mem_lock = PTHREAD_MUTEX_INITIALIZER; // Mutex lock for thread safety


// Update physical memory isFull attribute
void update()
{
    for (int i = 0; i < NUM_BLOCKS; i++)
    {
        if (phys_mem->blocks[i].used == false) // set isFull to false if block available
        {
            phys_mem->isFull = false;
            return;
        }
    }
    phys_mem->isFull = true;
    return;
}


// Run the LRU algorithm and swap process memory with oldest page to backing store
void LRU(process_t *process, int vm_idx)
{
    int oldest = 0; // default to first block
    unsigned long max = phys_mem->blocks[oldest].timestamp;
    for (int i = 1; i < NUM_BLOCKS; i++)
    {
        if (phys_mem->blocks[i].timestamp < max) // if older page exists set new oldest
        {
            oldest = i;
            max = phys_mem->blocks[i].timestamp;
        }
    }
    printf("Oldest page is %d\n\n", oldest); // log page being swapped
    void *ptr = my_malloc(BLOCK_SIZE); // allocate backing store memory using program managed heap

    // Update process to reflect new backing store address
    phys_mem->blocks[oldest].process->pagetable->entries[phys_mem->blocks[oldest].vm_idx]->address = ptr;
    phys_mem->blocks[oldest].process->pagetable->entries[phys_mem->blocks[oldest].vm_idx]->isPresent = false;
    phys_mem->blocks[oldest].process->pagetable->entries[phys_mem->blocks[oldest].vm_idx]->block = -1;

    // Update new process to be included in the physical memory
    process->pagetable->entries[vm_idx]->address = phys_mem->blocks[oldest].address;
    process->pagetable->entries[vm_idx]->isPresent = true;
    process->pagetable->entries[vm_idx]->block = oldest;
    phys_mem->blocks[oldest].timestamp = time(NULL);
    phys_mem->blocks[oldest].used = true;
    phys_mem->blocks[oldest].process = process;
    phys_mem->blocks[oldest].vm_idx = vm_idx;

    update(); // update physical memory isFull attribute
}


// Allocate physical memory to a process page table virtual memory location
void allocate(process_t *process, int vm_idx)
{
    printf("Allocating memory for PROCESS_%d at ADDRESS_%d\n\n", process->pid, vm_idx);
    // If physical memory is not full allocate open block
    if (!phys_mem->isFull)
    {
        // Find open block
        for (int i = 0; i < NUM_BLOCKS; i++)
        {
            if (phys_mem->blocks[i].used == false)
            {
                // Update process page table to reflect being in physical memory
                process->pagetable->entries[vm_idx]->address = phys_mem->blocks[i].address;
                process->pagetable->entries[vm_idx]->isPresent = true;
                process->pagetable->entries[vm_idx]->block = i;
                // Update physical memory to reflect the new process
                phys_mem->blocks[i].timestamp = time(NULL);
                phys_mem->blocks[i].used = true;
                phys_mem->blocks[i].process = process;
                phys_mem->blocks[i].vm_idx = vm_idx;
                update();// update physical memory isFull attribute
                break;
            }
        }
    }
    // Otherwise run LRU algorithm for page swap
    else
    {
        LRU(process, vm_idx);
    }
}


// Deallocate memory of a process from the page table virtual memory location
void deallocate(process_t *process, int vm_idx)
{
    printf("Deallocating memory for PROCESS_%d at ADDRESS_%d\n\n", process->pid, vm_idx);
    // Accoutn for if the memory is in physical memory 
    if (process->pagetable->entries[vm_idx]->isPresent == true){
        phys_mem->blocks[process->pagetable->entries[vm_idx]->block].timestamp = time(NULL);
        phys_mem->blocks[process->pagetable->entries[vm_idx]->block].used = false;
        phys_mem->blocks[process->pagetable->entries[vm_idx]->block].process = NULL;
        phys_mem->blocks[process->pagetable->entries[vm_idx]->block].vm_idx = -1;
    }
    // Update process to reflect deallocated memory attributes
    process->pagetable->entries[vm_idx]->address = NULL;
    process->pagetable->entries[vm_idx]->isPresent = false;
    process->pagetable->entries[vm_idx]->block = -1;
    update();
}


// Simulate a process requesting memory for page faults
void request(process_t *process, int vm_idx)
{
    printf("Requesting access for PROCESS_%d at ADDRESS_%d\n\n", process->pid, vm_idx);
    // If process is in physical memory just update timestamp
    if (process->pagetable->entries[vm_idx]->isPresent)
    {
        phys_mem->blocks[process->pagetable->entries[vm_idx]->block].timestamp = time(NULL);
    }
    // Otherwise run LRU algorithm
    else
    {
        // Only run LRU algorithm if physical memory is full
        if (phys_mem->isFull){
            LRU(process, vm_idx);
        }
        // Otherwise allocate the memory directly
        else{
            allocate(process, vm_idx);
        }
    }
}


// Get a random valid virtual memory address for a process
int randomAddress(process_t *process, bool exists)
{
    // Keep track of valid indexes for selecting randomly later
    int *arr = my_malloc(NUM_ENTRIES * sizeof(int));
    int length = 0;
    // Iterate over all virtual memory addresses and collect valid indexes
    for (int i = 0; i < NUM_ENTRIES; i++)
    {
        // Switch depending on if requester wants empty or allocated virtual memory addresses
        if (exists) {
            if (process->pagetable->entries[i]->address != NULL)
            {
                arr[length++] = i;
            }
        } else {
            if (process->pagetable->entries[i]->address == NULL)
            {
                arr[length++] = i;
            }
        }
    }
    // If found valid indexes select a random one
    if (length > 0)
    {
        int random = rand() % length;
        my_free(arr); // free allocated array from program managed heap
        return arr[random];
    }
    my_free(arr); // free allocated array from program managed heap
    return -1; // return negative one if no valid indexes
}


// Incremental function for allocating process IDs to new processes
int getNextPID()
{
    static int nextPID = 0;
    return nextPID++; // post-increment
}


// Threaded function which runs multiple concurrent processes
void *runProcess()
{
    // Create new process
    process_t *process = my_malloc(sizeof(process_t));
    process->pid = getNextPID();
    process->pagetable = my_malloc(sizeof(pagetable_t));

    // Initialize virtual memory entries as empty
    for (int i = 0; i < NUM_ENTRIES; i++)
    {
        process->pagetable->entries[i] = my_malloc(sizeof(entry_t));
        process->pagetable->entries[i]->isPresent = false;
        process->pagetable->entries[i]->address = NULL;
    }

    int choice;
    // Infinite loop
    while (true)
    {
        choice = rand() % 3; // randomly select three outcomes (allocate, request, deallocate)
        if (choice == 0)
        {
            int randIdx = randomAddress(process, false);
            // Only allocate if page table is not full
            if (randIdx != -1){
                pthread_mutex_lock(&(mem_lock));
                allocate(process, randIdx);
                pthread_mutex_unlock(&(mem_lock));
            }
        }
        else if (choice == 1)
        {
            int randIdx = randomAddress(process, true);
            // Only request if page table has a single entry or more
            if (randIdx != -1)
            {
                pthread_mutex_lock(&(mem_lock));
                request(process, randIdx);
                pthread_mutex_unlock(&(mem_lock));
            }
        }
        else if (choice == 2)
        {
            int randIdx = randomAddress(process, true);
            // Only deallocate if page table has a single entry or more
            if (randIdx != -1)
            {
                pthread_mutex_lock(&(mem_lock));
                deallocate(process, randIdx);
                pthread_mutex_unlock(&(mem_lock));
            }
        }
        sleep(1);
    }
}


// Main function
int main()
{

    init_memory_pool(); // initialize program managed heap

    phys_mem = my_malloc(sizeof(phys_mem)); // allocate physical memory struct using program managed heap
    phys_mem->memory = my_malloc(PHYS_MEM_SIZE); // allocate contigous block of physical memory
    phys_mem->isFull = false; // physical memory is empty initially
    phys_mem->blocks = my_malloc(sizeof(block_t) * NUM_BLOCKS); // allocate blocks array for segmenting physical memory pages

    printf("Physical memory start address: %p \n\n", phys_mem->memory); // show the starting address for physical memory

    // Initialize the block structs of the contigous physical memory
    for (int i = 0; i < NUM_BLOCKS; i++)
    {

        void *ptr = (char *)phys_mem->memory + i * BLOCK_SIZE;
        printf("BLOCK_%d Address: %p \n\n", i, ptr); // log the addresses of each block to show 4096 gaps
        phys_mem->blocks[i].address = ptr;
        phys_mem->blocks[i].used = false;
        phys_mem->blocks[i].process = NULL;
        phys_mem->blocks[i].timestamp = time(NULL);
        phys_mem->blocks[i].vm_idx = -1;
    }

    srand(time(NULL)); // seed random number generator
    pthread_t myThreads[NUM_THREADS]; // initialze threads 
    // Begin multithreaded processes executing memory operations
    for (int i = 0; i < NUM_THREADS; i++)
    {
        pthread_create(&myThreads[i], NULL, runProcess, NULL);
    }
    pthread_exit(NULL);
}