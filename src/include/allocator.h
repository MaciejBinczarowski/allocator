#include <stddef.h>

#ifndef ALLOCATOR_H
#define ALLOCATOR_H

// struct for collecting statistics
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

// returns struct with stats
struct AllocatorStatistics getAllocatorStatistics(void);

// it works with macro alloc(size_to_alloc)
// alligns block size to header size
void* allocate(size_t, const char*, int);

// simply dealocs blocks and merge with free neighboors
void dealloc(void*);

// memory dump
void printBlockList(void);

// function to initialize allocator
void initAllocator(void);

#define alloc(size_to_alloc) allocate(size_to_alloc, __FILE__, __LINE__)

#endif