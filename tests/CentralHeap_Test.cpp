#include "gtest/gtest.h"
#include "memory/CentralHeap.h"
#include <thread>
#include <vector>
#include <numeric>

using namespace BedrockServer::Core::Memory;

TEST(CentralHeapTest, SingleThreadSanityCheck)
{
    // GetInstance now returns a pointer.
    CentralHeap* centralHeap = CentralHeap::GetInstance();
    // Ensure the singleton has been initialized before running tests.
    ASSERT_NE(centralHeap, nullptr);

    const size_t sizeClassIndex = 0;

    // 1. Setup: Ensure there is at least one page in the free list.
    MemoryPage* pInitialPage = centralHeap->GetPage(sizeClassIndex);
    ASSERT_NE(pInitialPage, nullptr);
    centralHeap->ReturnPage(pInitialPage);

    // 2. Save the now-stable initial state.
    size_t initialPageCount = centralHeap->GetStats(sizeClassIndex).FreePageCount;
    ASSERT_GT(initialPageCount, 0); 

    // 3. Get a page.
    MemoryPage* pPage = centralHeap->GetPage(sizeClassIndex);
    ASSERT_NE(pPage, nullptr);

    // 4. Verify the page count decreased by one.
    EXPECT_EQ(centralHeap->GetStats(sizeClassIndex).FreePageCount, initialPageCount - 1);

    // 5. Return the same page.
    centralHeap->ReturnPage(pPage);
    
    // 6. Verify that the final state is identical to the initial stable state.
    EXPECT_EQ(centralHeap->GetStats(sizeClassIndex).FreePageCount, initialPageCount);
}

TEST(CentralHeapTest, BankAccountStressTest)
{
    // MODIFICATION: GetInstance now returns a pointer.
    CentralHeap* centralHeap = CentralHeap::GetInstance();
    // Ensure the singleton has been initialized before running tests.
    ASSERT_NE(centralHeap, nullptr);

    const size_t sizeClassIndex = 5; 
    const unsigned int numThreads = std::thread::hardware_concurrency() * 4;
    constexpr int opsPerThread = 20000;
    
    // 1. Pre-populate the heap to a known state to control the test environment.
    const size_t initialBalance = numThreads * 2;
    std::vector<MemoryPage*> tempPages;
    tempPages.reserve(initialBalance);

    // First, empty the free list for this size class to ensure a clean slate.
    while(centralHeap->GetStats(sizeClassIndex).FreePageCount > 0) {
        tempPages.push_back(centralHeap->GetPage(sizeClassIndex));
    }
    tempPages.clear();

    // Now, populate it with exactly initialBalance pages.
    for (size_t i = 0; i < initialBalance; ++i)
    {
        MemoryPage* p = centralHeap->GetPage(sizeClassIndex);
        ASSERT_NE(p, nullptr);
        tempPages.push_back(p);
    }
    for (MemoryPage* pPage : tempPages)
    {
        centralHeap->ReturnPage(pPage);
    }
    
    size_t initialPageCount = centralHeap->GetStats(sizeClassIndex).FreePageCount;
    ASSERT_EQ(initialPageCount, initialBalance);

    // 2. All threads concurrently get and return pages.
    std::vector<std::thread> threads;
    for (unsigned int i = 0; i < numThreads; ++i)
    {
        threads.emplace_back([&]() {
            for (int j = 0; j < opsPerThread; ++j)
            {
                MemoryPage* pPage = centralHeap->GetPage(sizeClassIndex);
                ASSERT_NE(pPage, nullptr);
                centralHeap->ReturnPage(pPage);
            }
        });
    }

    for (auto& t : threads)
    {
        t.join();
    }

    // 3. After all transactions, the final balance must equal the initial balance.
    size_t finalPageCount = centralHeap->GetStats(sizeClassIndex).FreePageCount;
    EXPECT_EQ(finalPageCount, initialPageCount);
}