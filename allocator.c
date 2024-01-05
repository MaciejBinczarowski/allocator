#include <stdio.h>
#include <unistd.h>
#include <cstdlib>

#define HEADER_SIZE sizeof(struct Header)
#define MIN_BLOCK_SIZE 32

void* alloc(size_t);
void initAllocator(void);

struct Header
{
    size_t blockSize;
    int status;
    struct Header* priorHeader;
    struct Header* nextHeader;
};

int main(void)
{
    initAllocator();
    char* string = (char*) alloc(10 * sizeof(char));

    for (int i = 0; i < 10; i++)
    {
        *(string + i) = 'k'; 
    }

    printf("%s", string);
    return 0;
}

struct Header* initialHeader = NULL;
void initAllocator(void)
{
    void* startBrk = sbrk(0);
    if(startBrk == (void*) -1)
    {
        fprintf(stderr, "Heap error");
        exit(EXIT_FAILURE);
    }

    void* newStartBrk = sbrk(HEADER_SIZE);
    if (newStartBrk == (void*) -1)
    {
        fprintf(stderr, "Out of memory");
        exit(EXIT_FAILURE);
    }
    
    initialHeader = (struct Header*) startBrk;
    initialHeader->blockSize = 0;
    initialHeader->status = 1;
    initialHeader->priorHeader = NULL;
    initialHeader->nextHeader = NULL;
}

void* alloc(size_t size_to_alloc)
{
    struct Header* currentHeader = initialHeader;
    struct Header* priorHeader = initialHeader->priorHeader;
    while (currentHeader != NULL)
    {
        if (currentHeader->status == 0 && currentHeader->blockSize > size_to_alloc)
        {
            if (currentHeader->blockSize > size_to_alloc + HEADER_SIZE + MIN_BLOCK_SIZE)
            {   
                size_t secondBlockSize = currentHeader->blockSize - size_to_alloc - HEADER_SIZE;
                struct Header* secondHeader = (struct Header*)(((char*)currentHeader) + HEADER_SIZE + size_to_alloc);
                secondHeader->blockSize = secondBlockSize;
                secondHeader->status = 0;
                secondHeader->priorHeader = currentHeader;
                secondHeader->nextHeader = currentHeader->nextHeader;
                
                currentHeader->blockSize = size_to_alloc;
                currentHeader->status = 1;
                currentHeader->nextHeader = secondHeader;
            }
            return (void*) (currentHeader + 1);
        }
        priorHeader = currentHeader;
        currentHeader = currentHeader->nextHeader;
    }

    void* currentBrk = sbrk(0);
    void* newBrk = sbrk(HEADER_SIZE + size_to_alloc);

    if (newBrk == (void*) -1)
    {
        return NULL;
    }

    struct Header* newHeader = (struct Header*) currentBrk;
    newHeader->blockSize = size_to_alloc;
    newHeader->status = 1;
    newHeader->priorHeader = priorHeader;
    newHeader->nextHeader = NULL;

    priorHeader->nextHeader = newHeader;

    return (void*) (newHeader + 1);
}