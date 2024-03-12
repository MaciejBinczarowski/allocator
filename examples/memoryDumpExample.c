#include <allocator.h>
#include <stdio.h>
#include <string.h>

// simple example of memorydump
// leave memory allocated (visible in statistics at the end of the program)
int main() {
       
    // in order to use allocator, it must be initialized,
    // when using gcc, it is initialized automatically
    #ifndef __GNUC__
        initAllocator();
    #endif

    // malloc 10 bytes
    char* str = alloc(15 * sizeof(char));

    // pass the string to the allocated memory
    strcpy(str, "Hello Word!");

    // print the string
    printf("%s\n", str);
    printf("Memory address: %p\n", str);

    // allocate another memory block
    int* arr = alloc(80 * sizeof(int));
    printf("Memory address: %p\n", arr);

    // memory dump
    printBlockList();

    // here we should deallocate the memory, but we will not do it

    return 0;
}