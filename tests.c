#include "allocator.h"
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

void testAllocation(void)
{
    CU_ASSERT_TRUE(alloc(sizeof(int)) != NULL);
    CU_ASSERT_TRUE(getAllocatorStatistics().allocatedBlocksCount == 1);
}

void testDealloc(void)
{
    int* i = (int*) alloc(sizeof(int) * 20);
    dealloc(i);
    CU_ASSERT_TRUE(getAllocatorStatistics().allocatedBlocksCount == 1);
}

void testUseExistingBlocksAndMergBlocks(void)
{
    int* i = (int*) alloc(sizeof(int) * 20); // reuse block from prior test
    CU_ASSERT_TRUE(getAllocatorStatistics().totalSbrkRequests == 2); // if only sbrk calls from previous tests
    char* j = (char*) alloc(sizeof(char) * 100); // new block
    CU_ASSERT_TRUE(getAllocatorStatistics().totalSbrkRequests == 3);
    char* k = (char*) alloc(sizeof(char) * 150); // another new block
    CU_ASSERT_TRUE(getAllocatorStatistics().totalSbrkRequests == 4);
    
    // dealloc in order "left" block, "right" block, "middle" block
    // and checks if the blocks have merged together 
    // by allocating the size equal to the sum of these blocks (excluding headers)
    dealloc(i);
    dealloc(k);
    dealloc(j);

    char* newMergedBlocksUse = (char*) alloc(sizeof(char) * (20 * sizeof(int)) + 250);
    CU_ASSERT_TRUE(getAllocatorStatistics().totalSbrkRequests == 4);  // reuse merged blocks, so sbrk is not needed
    CU_ASSERT_TRUE(getAllocatorStatistics().allocatedBlocksCount == 2);
    dealloc(newMergedBlocksUse);
}

// the test verifies the division of a block when its size is greater than the required size
void testDivideBlockBeforAllocation(void)
{   
    // two allocation at that one big block from prior test
    char* firstPartOfBlock = (char*) alloc(sizeof(char) * 100);
    char* secondPartOfBlock = (char*) alloc(sizeof(char) * 150 + sizeof(char) * (20 * sizeof(int)));
    CU_ASSERT_TRUE(getAllocatorStatistics().totalSbrkRequests == 4); 
    CU_ASSERT_TRUE(getAllocatorStatistics().allocatedBlocksCount == 3);

    dealloc(firstPartOfBlock);
    dealloc(secondPartOfBlock);
}


int main(int argc, char const *argv[])
{
    CU_initialize_registry();

    CU_pSuite suite = CU_add_suite("allocation", NULL, NULL);

    CU_add_test(suite, "TestAllocation", testAllocation);
    CU_add_test(suite, "TestDealloc", testDealloc);
    CU_add_test(suite, "testUseExistingBlocksAndMergBlocks", testUseExistingBlocksAndMergBlocks);
    CU_add_test(suite, "testDivideBlockBeforAllocation", testDivideBlockBeforAllocation);

    CU_basic_run_tests();

    CU_cleanup_registry();
    
    return CU_get_error();
}
