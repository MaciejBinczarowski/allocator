#include <allocator.h>
#include <stdio.h>

// simple example of using allocator (main purpose of this library)
int main() 
{

    // in order to use allocator, it must be initialized,
    // when using gcc, it is initialized automatically
    #ifndef __GNUC__
        initAllocator();
    #endif

    // allocate 5 integers array
    int* arr = alloc(5 * sizeof(int));

    // check if allocation was successful
    if (arr != NULL) 
    {
        // fill array with values
        for (int i = 0; i < 5; i++) 
        {
            arr[i] = i * 2;
        }

        // print array
        for (int k = 0; k < 5; k++)
        {
            printf("%d\n", arr[k]);
        }

        // deallocate array
        dealloc(arr);
    } 
    else 
    {
        printf("Allocation failed\n");
    }

    return 0;
}
