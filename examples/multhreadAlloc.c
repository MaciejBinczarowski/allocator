#include <allocator.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

// simple example of multithreaded allocation
void* allocate_memory_thread(void* count) 
{
    for (int i = 1; i < *((int*) count) * 50; i++)
    {
        int size = i * 8 + 8;
        void* ptr = alloc(size);
        printf("Allocated %d bytes at address %p, by thread %d\n", size, ptr, *((int*) count));
        dealloc(ptr);
    }

    return NULL;
}

int main() {

    // in order to use allocator, it must be initialized,
    // when using gcc, it is initialized automatically
    #ifndef __GNUC__
        initAllocator();
    #endif

    pthread_t thread1, thread2;
    int number1 = 1;
    int number2 = 2;

    pthread_create(&thread1, NULL, allocate_memory_thread, &number1);
    pthread_create(&thread2, NULL, allocate_memory_thread, &number2);

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    // print memory dump at the end of the program (block were merged into one block)
    printBlockList();

    return 0;
}
