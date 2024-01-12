#include <zconf.h>

#ifndef ALLOCATOR_H
#define ALLOCATOR_H

struct AllocatorStatistics {
    size_t totalAllocatedBytes;
    size_t currentAllocatedBytes;
    size_t maxMemoryUsage;
    size_t averageAllocatedBytes;
    int allocationCount;
    int totalSbrkRequests;
    int corruptedBlocksCount;
    int allocatedBlocksCount;
};

struct AllocatorStatistics getAllocatorStatistics(void);

void* allocate(size_t, const char*, int);
void dealloc(void*);
void printBlockList(void);
void initAllocator(void);

#define alloc(size_to_alloc) allocate(size_to_alloc, __FILE__, __LINE__)

#endif