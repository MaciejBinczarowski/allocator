#include <stdio.h>
#include <unistd.h>
#include <cstdlib>
#include <zlib.h>
#include <string.h>
#include <pthread.h>

#define HEADER_SIZE sizeof(struct Header)
#define MIN_BLOCK_SIZE 32
#define MAX_FILENAME_LENGTH 64
#define alloc(size_to_alloc) allocate(size_to_alloc, __FILE__, __LINE__)

void* allocate(size_t, const char*, int);
void dealloc(void*);
static uLong calculateControlSumForHeader(struct Header*);
void printBlockList(void);
static void printStatistics(void);
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
    char file[MAX_FILENAME_LENGTH];
    int line;
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

void* allocate(size_t size_to_alloc, const char* file, int line)
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

            // save the file and the line where the allocation was called
            const char* fileName = strrchr(file, '/');
            if (strlen(fileName) > MAX_FILENAME_LENGTH)
            {
                strcpy(currentHeader->file, "Unknown file");
            }
            else
            {
                strcpy(currentHeader->file, fileName);
            }

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

    const char* fileName = strrchr(file, '/');
    if (strlen(fileName) > MAX_FILENAME_LENGTH)
    {
        strcpy(newHeader->file, "Unknown file");
    }
    else
    {
        strcpy(newHeader->file, fileName);
    }

    newHeader->line = line;

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

    // mark block as free, restart header, and clear its memory
    header_to_dealloc->status = 0;
    memset(header_to_dealloc->file, '\0', MAX_FILENAME_LENGTH);
    header_to_dealloc->line = 0;
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
void printBlockList(void)
{
    struct Header* currentHeader = initialHeader;
    printf("memory dump:\n");
    while (currentHeader != NULL)
    {
        printf("size: %zu, controlSum: %lu, status: %d, ", currentHeader->blockSize, currentHeader->controlSum, currentHeader->status);
        printf("start: %p, end: %p, ", currentHeader + 1, (char*)(currentHeader + 1) + currentHeader->blockSize);
        printf("file: %s, line: %d\n", currentHeader->file, currentHeader->line);
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

    struct Header* currentHeader = initialHeader->nextHeader;
    printf("unfreed blocks: %d\n", allocatorStatistics.allocatedBlocksCount);
    while (currentHeader != NULL)
    {
        if (currentHeader->status == 1)
        {
            printf("filename: %s, line: %d\n", currentHeader->file, currentHeader->line);
        }
        currentHeader = currentHeader->nextHeader;
    } 
}