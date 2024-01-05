#include <stdio.h>
#include <unistd.h>

#define HEADER_SIZE sizeof(struct Header)

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
    char* string = (char*) alloc(10 * sizeof(char));

    for (int i = 0; i < 10; i++)
    {
        *(string + i) = 'k'; 
    }

    printf("%s", string);
    return 0;
}

void* alloc(size_t size_to_alloc)
{
    struct Header* currentHeader = initialHeader;
    struct Header* priorHeader = initialHeader->priorHeader;
    while (currentHeader != NULL)
    {
        if (currentHeader->status == 0 && currentHeader->blockSize > size_to_alloc)
        {
            // use this block
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

struct Header* initialHeader = NULL;
void initAllocator(void)
{
    void* startBrk = sbrk(0);
    if(startBrk == (void*) -1)
    {
        // memory fault
    }

    void* newStartBrk = sbrk(HEADER_SIZE);
    if (newStartBrk == (void*) -1)
    {
        //memory fault
    }
    
    initialHeader = (struct Header*) startBrk;
    initialHeader->blockSize = 0;
    initialHeader->status = 1;
    initialHeader->priorHeader = NULL;
}