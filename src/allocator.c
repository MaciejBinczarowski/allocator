#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <zlib.h>
#include <zconf.h>
#include <string.h>
#include <pthread.h>

#include "allocator.h"
#define MAX_FILENAME_LENGTH 64
#define HEADER_ALIGMENT sizeof(struct Header*)

struct Header
{
    struct Header* priorHeader;
    struct Header* nextHeader;
    uLong controlSum;
    size_t blockSize;
    int status;
    int line;
    const char* fileName;
};

#define HEADER_SIZE sizeof(struct Header)
#define MIN_BLOCK_SIZE 32

static uLong calculateControlSumForHeader(struct Header* header);
static void printStatistics(void);

/*
 *  if compilator is GNUC then allocator gets inititialized at start of the program
 * otherwise user needs to initialize allocator manually
 * when static variable allocatorContext.isInitialized is 0 that means it is not initialized and 1 points it is
 */
#ifdef __GNUC__
void __attribute__((constructor)) init(void)
{
    initAllocator();
}
#endif

#define allocatorStatistics allocatorContext.allocatorStatisticsContext
static struct
{
    struct AllocatorStatistics allocatorStatisticsContext;
    pthread_mutex_t mutex;
    struct Header* initialHeader;
    int isInitialized;

} allocatorContext = {.isInitialized = 0};

/*
 * initialization puts header of blocksize = 0 wich is the start of headers' list structure
 */
void initAllocator(void)
{   
    if(allocatorContext.isInitialized == 1)
    {
        (void)fprintf(stderr, "allocator is already initialized");
        pthread_exit((void*)EXIT_FAILURE);
    }

    void* startBrk = sbrk(0);
    if(startBrk == (void*) -1) //NOLINT
    {
        (void)fprintf(stderr, "Heap error");
        pthread_exit((void*)EXIT_FAILURE);
    }

    void* newStartBrk = sbrk(HEADER_SIZE);
    if (newStartBrk == (void*) -1)  //NOLINT
    {
        (void)fprintf(stderr, "Out of memory");
        pthread_exit((void*)EXIT_FAILURE);
    }
    
    allocatorContext.initialHeader = (struct Header*) startBrk;
    allocatorContext.initialHeader->blockSize = 0;
    allocatorContext.initialHeader->status = 1;
    allocatorContext.initialHeader->priorHeader = NULL;
    allocatorContext.initialHeader->nextHeader = NULL;

    // print statistics at the end of the program
    (void)atexit(printStatistics);

    // mark initialization control flag
    allocatorContext.isInitialized = 1;

    if (pthread_mutex_init(&(allocatorContext.mutex), NULL) != 0)
    {
        (void)fprintf(stderr, "Mutex initialization failed\n");
        pthread_exit((void*)EXIT_FAILURE);
    }
}

void* allocate(size_t size_to_alloc, const char* file, int line)
{
    if(allocatorContext.isInitialized == 0)
    {
        (void)fprintf(stderr, "allocator is not initialized");
        pthread_exit((void*)EXIT_FAILURE);
    }

    // Synchronization using a mutex to prevent concurrent access by multiple threads.
    pthread_mutex_lock(&(allocatorContext.mutex));

    allocatorStatistics.allocationCount += 1;

    if (size_to_alloc <= 0)
    {
        return NULL;
    }

    // Align the allocated memory to the alignment of the header
    while (size_to_alloc % HEADER_ALIGMENT != 0)
    {
        size_to_alloc++;
    }

    struct Header* currentHeader = allocatorContext.initialHeader;
    struct Header* priorHeader = allocatorContext.initialHeader->priorHeader;
    while (currentHeader != NULL)
    {
        if (currentHeader->status == 0 && currentHeader->blockSize >= size_to_alloc)
        {
            // Check CRC to verify if the header has been corrupted before alloc
            if (currentHeader->controlSum != calculateControlSumForHeader(currentHeader))
            {
                abort();
                
            }

            // save the file and the line where the allocation was called
            (void)strrchr(file, '/');
            currentHeader->fileName = file;
            currentHeader->line = line;

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
            allocatorStatistics.totalAllocatedBytes += currentHeader->blockSize;
            allocatorStatistics.currentAllocatedBytes += currentHeader->blockSize;
            allocatorStatistics.averageAllocatedBytes = allocatorStatistics.totalAllocatedBytes / allocatorStatistics.allocationCount;
            allocatorStatistics.allocatedBlocksCount += 1;

            if (allocatorStatistics.currentAllocatedBytes > allocatorStatistics.maxMemoryUsage)
            {
                allocatorStatistics.maxMemoryUsage = allocatorStatistics.currentAllocatedBytes;
            }

            pthread_mutex_unlock(&(allocatorContext.mutex));
            return (void*) (currentHeader + 1);
        }
        priorHeader = currentHeader;
        currentHeader = currentHeader->nextHeader;
    }

    void* currentBrk = sbrk(0);
    void* newBrk = sbrk(HEADER_SIZE + size_to_alloc);

    allocatorStatistics.totalSbrkRequests += 1;

    if (newBrk == (void*) -1)  //NOLINT
    {
        pthread_mutex_unlock(&(allocatorContext.mutex));
        return NULL;
    }

    struct Header* newHeader = (struct Header*) currentBrk;
    newHeader->blockSize = size_to_alloc;
    newHeader->status = 1;
    newHeader->priorHeader = priorHeader;
    newHeader->nextHeader = NULL;
    newHeader->controlSum = calculateControlSumForHeader(newHeader);

    (void)strrchr(file, '/');
    newHeader->fileName = file;
    newHeader->line = line;

    // update priorHeader and calculate new controlSum
    priorHeader->nextHeader = newHeader;
    priorHeader->controlSum = calculateControlSumForHeader(priorHeader);

    // collecting statistic of max and total allocated memory
    allocatorStatistics.totalAllocatedBytes += newHeader->blockSize;
    allocatorStatistics.currentAllocatedBytes += newHeader->blockSize;
    allocatorStatistics.averageAllocatedBytes = allocatorStatistics.totalAllocatedBytes / allocatorStatistics.allocationCount;
    allocatorStatistics.allocatedBlocksCount += 1;
    
    if (allocatorStatistics.currentAllocatedBytes > allocatorStatistics.maxMemoryUsage)
    {
        allocatorStatistics.maxMemoryUsage = allocatorStatistics.currentAllocatedBytes;
    }

    pthread_mutex_unlock(&(allocatorContext.mutex));

    return (void*) (newHeader + 1);
}

void dealloc(void* block_to_dealloc)
{
    if(allocatorContext.isInitialized == 0)
    {
        (void)fprintf(stderr, "allocator is not initialized");
        pthread_exit((void*)EXIT_FAILURE);
    }

    // synchronization using a mutex to prevent concurrent access by multiple threads.
    pthread_mutex_lock(&(allocatorContext.mutex));

    struct Header* header_to_dealloc = (struct Header*) ((char*)(block_to_dealloc) - HEADER_SIZE);

    // if block is already free then abort
    if(header_to_dealloc->status == 0)
    {
        abort();
    }

    // Check CRC to verify if the header has been corrupted
    uLong currentCRC = header_to_dealloc->controlSum;
    uLong newCRC = calculateControlSumForHeader(header_to_dealloc);

    if (currentCRC != newCRC)
    {
        abort();
    }

    // mark block as free, restart header, and clear its memory
    header_to_dealloc->status = 0;
    header_to_dealloc->fileName = NULL;
    header_to_dealloc->line = 0;
    memset(header_to_dealloc + 1, 0, header_to_dealloc->blockSize); //NOLINT
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
    pthread_mutex_unlock(&(allocatorContext.mutex));
}

// function wich calculates CRC for passed header
static uLong calculateControlSumForHeader(struct Header* header)
{
    uLong controlSum = 0;
    controlSum = crc32(controlSum, (Bytef*) &(header->blockSize), sizeof(header->blockSize));
    controlSum = crc32(controlSum, (Bytef*) &(header->status), sizeof(header->status));
    controlSum = crc32(controlSum, (Bytef*) &(header->priorHeader), sizeof(struct Header*));
    controlSum = crc32(controlSum, (Bytef*) &(header->nextHeader), sizeof(struct Header*));
    return controlSum;
}

// function prints all existing blocks of memory
void printBlockList(void)
{
    struct Header* currentHeader = allocatorContext.initialHeader;
    printf("memory dump:\n");
    while (currentHeader != NULL)
    {
        printf("size: %zu, controlSum: %lu, status: %d, ", currentHeader->blockSize, currentHeader->controlSum, currentHeader->status);
        printf("start: %p, end: %p, ", currentHeader + 1, (char*)(currentHeader + 1) + currentHeader->blockSize);
        printf("file: %s, line: %d\n", currentHeader->fileName, currentHeader->line);
        currentHeader = currentHeader->nextHeader;
    } 
}

void printStatistics(void)
{
    printf("total allocated bytes: %zu\n", allocatorStatistics.totalAllocatedBytes);
    printf("max memory usage: %zu\n", allocatorStatistics.maxMemoryUsage);
    printf("average bytes allocated: %zu\n", allocatorStatistics.averageAllocatedBytes);
    printf("alloction count: %d\n", allocatorStatistics.allocationCount);
    printf("total sbrk requests: %d\n", allocatorStatistics.totalSbrkRequests);
    printf("current allocated bytes: %zu\n", allocatorStatistics.currentAllocatedBytes);
    printf("corrupted blocks: at least %d\n", allocatorStatistics.corruptedBlocksCount);

    struct Header* currentHeader = allocatorContext.initialHeader->nextHeader;
    printf("unfreed blocks: %d\n", allocatorStatistics.allocatedBlocksCount);
    while (currentHeader != NULL)
    {
        if (currentHeader->status == 1)
        {
            printf("aloctaing filename: %s, line: %d\n", currentHeader->fileName, currentHeader->line);
        }
        currentHeader = currentHeader->nextHeader;
    } 
}

struct AllocatorStatistics getAllocatorStatistics(void) 
{
    return allocatorStatistics;
}