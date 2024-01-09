#include <stdio.h>
#include <unistd.h>
#include <cstdlib>
#include <zlib.h>
#include <string.h>
#include <pthread.h>

#define HEADER_SIZE sizeof(struct Header)
#define MIN_BLOCK_SIZE 32
#define alloc(size_to_alloc) alloc(size_to_alloc, __FILE__, __LINE__)

void* alloc(size_t);
void dealloc(void*);
static uLong calculateControlSumForHeader(struct Header*);
static void printBlockList(void);
void initAllocator(void);

/*
 *  if compilator is GNUC then allocator gets inititialized at start of the program
 * otherwise user needs to initialize allocator manually
 * when static variable isInitialized is 0 that means it is not initialized and 1 points it is
 */
static int isInitialized;
#ifdef __GNUC__
void __attribute__((constructor)) init(void)
{
    initAllocator();
}
#endif

pthread_mutex_t mutex;

struct Header
{
    size_t blockSize;
    int status;
    struct Header* priorHeader;
    struct Header* nextHeader;
    uLong controlSum;
};


static struct 
{
    size_t totalAllocatedBytes;
    size_t currentAllocatedBytes;
    size_t maxMemoryUsage;
    size_t averageAllocatedBytes;
    int allocationCount;
    int totalSbrkRequests;
    int corruptedBlocksCount;
    int allocatedBlocksCount;
} allocatorStatistics;

int main(void)
{
    // initAllocator();
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

/*
 * initialization puts header of blocksize = 0 wich is the start of headers' list structure
 */
struct Header* initialHeader = NULL;
void initAllocator(void)
{   
    if(isInitialized == 1)
    {
        fprintf(stderr, "allocator is already initialized");
        exit(EXIT_FAILURE);
    }

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

    atexit(printStatistics);

    isInitialized = 1;

    if (pthread_mutex_init(&mutex, NULL) != 0) {
        fprintf(stderr, "Mutex initialization failed\n");
        exit(EXIT_FAILURE);
    }
}

void* alloc(size_t size_to_alloc, const char* file, int line)
{
    if(isInitialized == 0)
    {
        fprintf(stderr, "allocator is not initialized");
        exit(EXIT_FAILURE);
    }

    // Synchronization using a mutex to prevent concurrent access by multiple threads.
    pthread_mutex_lock(&mutex);

    allocatorStatistics.allocationCount += 1;

    struct Header* currentHeader = initialHeader;
    struct Header* priorHeader = initialHeader->priorHeader;
    while (currentHeader != NULL)
    {
        if (currentHeader->status == 0 && currentHeader->blockSize > size_to_alloc)
        {
            // Check CRC to verify if the header has been corrupted before alloc
            if (currentHeader->controlSum != calculateControlSumForHeader(currentHeader))
            {
                abort();
            }

            if (currentHeader->blockSize > size_to_alloc + HEADER_SIZE + MIN_BLOCK_SIZE)
            {   
                size_t secondBlockSize = currentHeader->blockSize - size_to_alloc - HEADER_SIZE;
                struct Header* secondHeader = (struct Header*)(((char*)currentHeader) + HEADER_SIZE + size_to_alloc);
                secondHeader->blockSize = secondBlockSize;
                secondHeader->status = 0;
                secondHeader->priorHeader = currentHeader;
                secondHeader->nextHeader = currentHeader->nextHeader;
                secondHeader->controlSum = calculateControlSumForHeader(secondHeader);
                
                currentHeader->blockSize = size_to_alloc;
                currentHeader->status = 1;
                currentHeader->nextHeader = secondHeader;
                currentHeader->controlSum = calculateControlSumForHeader(currentHeader);
            }
            else
            {
                currentHeader->status = 1;
                currentHeader->controlSum = calculateControlSumForHeader(currentHeader);
            }

            // collecting statistic
            allocatorStatistics.totalAllocatedBytes += size_to_alloc;
            allocatorStatistics.currentAllocatedBytes += size_to_alloc;
            allocatorStatistics.averageAllocatedBytes = allocatorStatistics.totalAllocatedBytes / allocatorStatistics.allocationCount;
            allocatorStatistics.allocatedBlocksCount += 1;

            if (allocatorStatistics.currentAllocatedBytes > allocatorStatistics.maxMemoryUsage)
            {
                allocatorStatistics.maxMemoryUsage = allocatorStatistics.currentAllocatedBytes;
            }

            pthread_mutex_unlock(&mutex);
            return (void*) (currentHeader + 1);
        }
        priorHeader = currentHeader;
        currentHeader = currentHeader->nextHeader;
    }

    void* currentBrk = sbrk(0);
    void* newBrk = sbrk(HEADER_SIZE + size_to_alloc);

    allocatorStatistics.totalSbrkRequests += 1;

    if (newBrk == (void*) -1)
    {
        pthread_mutex_unlock(&mutex);
        return NULL;
    }

    struct Header* newHeader = (struct Header*) currentBrk;
    newHeader->blockSize = size_to_alloc;
    newHeader->status = 1;
    newHeader->priorHeader = priorHeader;
    newHeader->nextHeader = NULL;
    newHeader->controlSum = calculateControlSumForHeader(newHeader);

    // update priorHeader and calculate new controlSum
    priorHeader->nextHeader = newHeader;
    priorHeader->controlSum = calculateControlSumForHeader(priorHeader);

    // collecting statistic of max and total allocated memory
    allocatorStatistics.totalAllocatedBytes += size_to_alloc;
    allocatorStatistics.currentAllocatedBytes += size_to_alloc;
    allocatorStatistics.averageAllocatedBytes = allocatorStatistics.totalAllocatedBytes / allocatorStatistics.allocationCount;
    allocatorStatistics.allocatedBlocksCount += 1;
    
    if (allocatorStatistics.currentAllocatedBytes > allocatorStatistics.maxMemoryUsage)
    {
        allocatorStatistics.maxMemoryUsage = allocatorStatistics.currentAllocatedBytes;
    }

    pthread_mutex_unlock(&mutex);

    return (void*) (newHeader + 1);
}

void dealloc(void* block_to_dealloc)
{
    if(isInitialized == 0)
    {
        fprintf(stderr, "allocator is not initialized");
        exit(EXIT_FAILURE);
    }

    // synchronization using a mutex to prevent concurrent access by multiple threads.
    pthread_mutex_lock(&mutex);

    struct Header* header_to_dealloc = (struct Header*) ((char*)(block_to_dealloc) - HEADER_SIZE);

    // Check CRC to verify if the header has been corrupted
    uLong currentCRC = header_to_dealloc->controlSum;
    uLong newCRC = calculateControlSumForHeader(header_to_dealloc);

    if (currentCRC != newCRC)
    {
        abort();
    }

    // mark block as free and clear its memory
    header_to_dealloc->status = 0;
    memset(header_to_dealloc + 1, 0, header_to_dealloc->blockSize);
    header_to_dealloc->controlSum = calculateControlSumForHeader(header_to_dealloc);

    // update statistics
    allocatorStatistics.currentAllocatedBytes -= header_to_dealloc->blockSize;
    allocatorStatistics.allocatedBlocksCount -= 1;

    // if next block is then free merge them together
    if (header_to_dealloc->nextHeader != NULL)
    {
        if (header_to_dealloc->nextHeader->status == 0)
        {
            header_to_dealloc->blockSize += header_to_dealloc->nextHeader->blockSize + HEADER_SIZE;
            
            if(header_to_dealloc->nextHeader->nextHeader != NULL)
            {
                header_to_dealloc->nextHeader->nextHeader->priorHeader = header_to_dealloc;
                header_to_dealloc->nextHeader = header_to_dealloc->nextHeader->nextHeader;

                 // calculate new controlSum for nexHeader and for headerToDeallloc
                header_to_dealloc->nextHeader->controlSum = calculateControlSumForHeader(header_to_dealloc->nextHeader);
                header_to_dealloc->controlSum = calculateControlSumForHeader(header_to_dealloc);
            }
            else
            {
                header_to_dealloc->nextHeader = NULL;

                //update controlSum
                header_to_dealloc->controlSum = calculateControlSumForHeader(header_to_dealloc);
            }
        }
    }
    
    // if prior block is free then merge it together
    if (header_to_dealloc->priorHeader != NULL)
    {
        if (header_to_dealloc->priorHeader->status == 0)
        {
            header_to_dealloc->priorHeader->blockSize += header_to_dealloc->blockSize + HEADER_SIZE;

            if (header_to_dealloc->nextHeader != NULL)
            {
                header_to_dealloc->priorHeader->nextHeader = header_to_dealloc->nextHeader;
                header_to_dealloc->nextHeader->priorHeader = header_to_dealloc->priorHeader;

                //calculate controlSum for priorHeader and nextHeader
                header_to_dealloc->priorHeader->controlSum = calculateControlSumForHeader(header_to_dealloc->priorHeader);
                header_to_dealloc->nextHeader->controlSum = calculateControlSumForHeader(header_to_dealloc->nextHeader);
            }
            else
            {
                header_to_dealloc->priorHeader->nextHeader = NULL;

                //update controlSum
                header_to_dealloc->priorHeader->controlSum = calculateControlSumForHeader(header_to_dealloc->priorHeader);
            }
        }
    }
    pthread_mutex_unlock(&mutex);
}

// function wich calculates CRC for passed header
static uLong calculateControlSumForHeader(struct Header* header)
{
    uLong controlSum = 0;
    controlSum = crc32(controlSum, (Bytef*) &(header->blockSize), sizeof(header->blockSize));
    controlSum = crc32(controlSum, (Bytef*) &(header->status), sizeof(header->status));
    controlSum = crc32(controlSum, (Bytef*) &(header->priorHeader), sizeof(header->priorHeader));
    controlSum = crc32(controlSum, (Bytef*) &(header->nextHeader), sizeof(header->nextHeader));
    return controlSum;
}

// function prints all existing blocks of memory
static void printBlockList(void)
{
    struct Header* currentHeader = initialHeader;
    printf("Bloki w pamięci:\n");
    while (currentHeader != NULL)
    {
        printf("size: %zu, controlSum: %lu, status: %d\n", currentHeader->blockSize, currentHeader->controlSum, currentHeader->status);
        currentHeader = currentHeader->nextHeader;
    } 
}

void printStatistics()
{
    printf("allocation count: %d\ntotal bytes allocated: %zu\n", allocatorStatistics.allocationCount, allocatorStatistics.totalAllocatedBytes);
    printf("current alocated bytes: %zu\nmax memory usage: %zu", allocatorStatistics.currentAllocatedBytes, allocatorStatistics.maxMemoryUsage);
    printf("total sbrk requests: %d\naverage allocated bytes: %zu", allocatorStatistics.totalSbrkRequests, allocatorStatistics.averageAllocatedBytes);
}