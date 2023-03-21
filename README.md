# My implementation of a virtual memory with a program managed heap!

This project uses a program managed heap implemented in the C program language to allocate and free memory from a contigous block of static memory, replacing the utility for malloc and free standard calls.

Additionally, the program implements a virtual memory simulation from scratch through the use of a single depth page table. 

For viewability of correctness, the program starts three concurrent threads (each its own process) and randomly has these processes select between allocate, deallocate, and request memory. The size of physical memory is five blocks and the virtual memory page table of each process is twice the size of total physical memory, meaning that the program heap must be used as a backing store for process requests which are not currently in physical memory.


To run the program, please execute...

```
make clean
make
./program
```

There is one bug in this program which I cannot find. For some reason, the executing process will freeze. While it will not segmentation fault, crash, or exit, it will simply stop running output. I believe the cause of this is that the custom free function in heap.c gets stuck in an infinite while loop, however, I am ultimately unsure and still debugging.

