#include <stdio.h>
#include <unistd.h>
#include <cstdlib>

#define HEADER_SIZE sizeof(struct Header)
#define MIN_BLOCK_SIZE 32

void* alloc(size_t);
void dealloc(void*);
void printBlockList(void);
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
    printf("zalokowano pamięć dla charów\n");
    printBlockList();

    int* tab = (int*) alloc(100* sizeof(int));
    printf("zalokowano pamięć dla intów\n");
    printBlockList();

    for(int i = 0; i < 100; i++)
    {
        tab[i] = i;
    }

    // for (int i = 0; i < 10; i++)
    // {
    //     *(string + i) = 'k'; 
    //     printf("%p\n", &string[i + 1]);
    // }
    // string[9] = '\0';
    // printf("%s", string);

    // for (int i = 0; i < 100; i++)
    // {
    //     printf("%d %p\n", tab[i], &tab[i]);
    // }

    long* tab1 = (long*) alloc(37 * sizeof(long));
    printf("zalokowano pamięć dla longów\n");
    printBlockList();

    dealloc(tab);
    printBlockList();

    short* shorts = (short*) alloc(49 * sizeof(short));
    printBlockList();

    dealloc(string);
    printBlockList();

    dealloc(shorts);
    printBlockList();

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

void dealloc(void* block_to_dealloc)
{
    struct Header* header_to_dealloc = (struct Header*) ((char*)(block_to_dealloc) - HEADER_SIZE);
    header_to_dealloc->status = 0;

    if (header_to_dealloc->nextHeader != NULL)
    {
        if (header_to_dealloc->nextHeader->status == 0)
        {
            header_to_dealloc->blockSize += header_to_dealloc->nextHeader->blockSize + HEADER_SIZE;
            
            if(header_to_dealloc->nextHeader->nextHeader != NULL)
            {
                header_to_dealloc->nextHeader->nextHeader->priorHeader = header_to_dealloc;
                header_to_dealloc->nextHeader = header_to_dealloc->nextHeader->nextHeader;
            }
            else
            {
                header_to_dealloc->nextHeader = NULL;
            }
        }
    }
    
    if (header_to_dealloc->priorHeader != NULL)
    {
        if (header_to_dealloc->priorHeader->status == 0)
        {
            header_to_dealloc->priorHeader->blockSize += header_to_dealloc->blockSize + HEADER_SIZE;

            if (header_to_dealloc->nextHeader != NULL)
            {
                header_to_dealloc->priorHeader->nextHeader = header_to_dealloc->nextHeader;
                header_to_dealloc->nextHeader->priorHeader = header_to_dealloc->priorHeader;
            }
            else
            {
                header_to_dealloc->priorHeader->nextHeader = NULL;
            }
        }
    }
    //sprawdź czy blok przed nim jest wolny
    //jeśli tak to scal z tym kolejnym blokiem
    // to znaczy weź powiększ swój rozmiar o wielkość headera i rozmiar kolejnego bloku
    // weź następnego sąsiada tego usuwanego bloku i zapisz go jako swojego następce
    // temu samemu sąsiadowi ustaw samego siebie jako poprzednika
    //sprawdź czy blok po nim jest wolny
    //jeśli tak to scal go z poprzednim blokiem
    // to znaczy weź powiększ rozmiar poprzednika o swój rozmiar + rozmiar headera
    // weź ustaw poprzednikowi samego siebie jako następnika, a swojemu następnikowi ustaw mojego poprzednika jako porzednika
    // gotowe 

}

void printBlockList(void)
{
    struct Header* currentHeader = initialHeader;
    while (currentHeader != NULL)
    {
        printf("size: %zu, status: %d\n", currentHeader->blockSize, currentHeader->status);
        currentHeader = currentHeader->nextHeader;
    } 
}