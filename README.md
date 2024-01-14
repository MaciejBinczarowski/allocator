# Own custom allocator
## Description
This custom allocator employs a simple "first fit" allocation algorithm, and during deallocation, it merges adjacent free blocks. Each memory header contains a CRC for verifying whether the header has been overwritten improperly. CRC is calculated and updated with each operation on a given memory block. During allocation, the requested memory size is aligned to the header alignment. We use sbrk for process memory allocation. The allocator requires initialization through the init() function; if using gcc or clang, it will be initialized automatically. Upon program completion, memory utilization statistics will be displayed on the standard output. Additionally, during runtime, it's possible to create a full memory dump.

## Simple use case
    #include <stdio.h>
    #include "allocator.h"
    
    int main() {
    // Initialize the allocator (if necessary)
    initAllocator();
  
    // Allocate memory
    int* ptr1 = alloc(sizeof(int);
    *ptr1 = 8;

    char* ptr2 = alloc(10 * sizeof(char));
    strcpy(ptr2, "Hello!");
    
    // Print memory dump
    printBlockList();
    
    // Deallocate memory
    dealloc(ptr1);
    dealloc(ptr2);
  
    return 0;
  }

## Installation
To install this library, you need to first use `git clone`. Then, within the cloned repository directory, use `make install DESTINATION=<target_directory>` (if you don't specify a directory, the library installs in the default system directory). You'll need to enter your user password since root permissions are required to install the necessary environment.

Everything should now be set up, and you can start using my custom allocator in your project :)
