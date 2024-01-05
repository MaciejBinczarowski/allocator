#include <stdio.h>
#include <unistd.h>

#define HEADER_SIZE sizeof(struct Header)

void* alloc(size_t);

struct Header
{
    size_t blockSize;
    int status;
};

int main(void)
{
    char* string = (char*) alloc(10 * sizeof(char));

    for (int i = 0; i < 10; i++)
    {
        *(string + i + 1) = 'k'; 
    }

    printf("%s", string);
    return 0;
}

void* alloc(size_t size_to_alloc)
{
    void* currentBrk = sbrk(0);
    void* newBrk = sbrk(HEADER_SIZE + size_to_alloc);

    if (newBrk == (void*) -1)
    {
        return NULL;
    }

    struct Header* header = (struct Header*) currentBrk;
    header->blockSize = size_to_alloc;
    header->status = 1;

    return (void*) (header + 1);
}