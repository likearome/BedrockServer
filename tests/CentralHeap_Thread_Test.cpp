#include "gtest/gtest.h"
#include "memory/CentralHeap.h"
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>
#include <numeric>

using namespace BedrockServer::Core::Memory;

TEST(CentralHeapTest, SingleThreadSanityCheck)
{
    CentralHeap& centralHeap = CentralHeap::GetInstance();
    const size_t sizeClassIndex = 0;

    size_t initialPageCount = centralHeap.GetStats(sizeClassIndex).FreePageCount;
    for(size_t i=0; i < initialPageCount; ++i) centralHeap.GetPage(sizeClassIndex);
    ASSERT_EQ(centralHeap.GetStats(sizeClassIndex).FreePageCount, 0);

    MemoryPage* pPage = centralHeap.GetPage(sizeClassIndex);
    ASSERT_NE(pPage, nullptr);

    size_t pagesAfterAlloc = centralHeap.GetStats(sizeClassIndex).FreePageCount;

    centralHeap.ReturnPage(pPage);
    
    EXPECT_EQ(centralHeap.GetStats(sizeClassIndex).FreePageCount, pagesAfterAlloc + 1);
}

TEST(CentralHeapTest, SingleThreadPerformanceCheck)
{
    CentralHeap& centralHeap = CentralHeap::GetInstance();
    const size_t sizeClassIndex = 1;
    constexpr int operations = 100000;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < operations; ++i)
    {
        MemoryPage* pPage = centralHeap.GetPage(sizeClassIndex);
        ASSERT_NE(pPage, nullptr);
        centralHeap.ReturnPage(pPage);
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = end - start;
    
    std::cout << "[ PERF TEST ] " << operations << " Get/Return operations took " 
              << elapsed.count() << " ms." << std::endl;
}

TEST(CentralHeapTest, MaxContentionStressTest)
{
    CentralHeap& centralHeap = CentralHeap::GetInstance();
    const size_t sizeClassIndex = 5;
    const unsigned int numThreads = std::thread::hardware_concurrency() * 2;
    constexpr int opsPerThread = 10000;

#ifndef NDEBUG
    centralHeap.ResetDebugCounters();
#endif
    
    // 1. Measure the initial state.
    size_t initialPageCount = centralHeap.GetStats(sizeClassIndex).FreePageCount;
    size_t initialCreatedCount = 0;
#ifndef NDEBUG
    initialCreatedCount = centralHeap.GetDebugPagesCreated();
#endif

    std::vector<std::thread> threads;
    for (unsigned int i = 0; i < numThreads; ++i)
    {
        threads.emplace_back([&]() {
            for (int j = 0; j < opsPerThread; ++j)
            {
                MemoryPage* pPage = centralHeap.GetPage(sizeClassIndex);
                ASSERT_NE(pPage, nullptr);
                centralHeap.ReturnPage(pPage);
            }
        });
    }

    for (auto& t : threads)
    {
        t.join();
    }

    // 2. Final Verification using the balance sheet method.
    size_t finalPageCount = centralHeap.GetStats(sizeClassIndex).FreePageCount;
    size_t finalCreatedCount = 0;
#ifndef NDEBUG
    finalCreatedCount = centralHeap.GetDebugPagesCreated();
#endif

    // The number of pages created during this specific test run.
    size_t pagesCreatedDuringTest = finalCreatedCount - initialCreatedCount;

    // Final count = Initial count + Pages created during the test.
    // The get/return operations within the threads should have a net effect of zero.
    EXPECT_EQ(finalPageCount, initialPageCount + pagesCreatedDuringTest);
}

TEST(CentralHeapTest, ExtremeStressAndSoakTest)
{
    CentralHeap& centralHeap = CentralHeap::GetInstance();
    const size_t sizeClassIndex = 6; // A fresh size class for the final test
    
    // Increase thread count for maximum context switching and contention.
    const unsigned int numThreads = std::thread::hardware_concurrency() * 4;
    
    // Set a duration for the test to run.
    const std::chrono::seconds testDuration = std::chrono::seconds(5);

#ifndef NDEBUG
    centralHeap.ResetDebugCounters();
#endif
    
    size_t initialPageCount = centralHeap.GetStats(sizeClassIndex).FreePageCount;
    size_t initialCreatedCount = 0;
#ifndef NDEBUG
    initialCreatedCount = centralHeap.GetDebugPagesCreated();
#endif

    std::atomic<bool> shouldStop = false;
    std::atomic<uint64_t> totalOpsCompleted = 0;
    std::vector<std::thread> threads;

    for (unsigned int i = 0; i < numThreads; ++i)
    {
        threads.emplace_back([&]() {
            uint64_t ops = 0;
            while (!shouldStop.load(std::memory_order_acquire))
            {
                MemoryPage* pPage = centralHeap.GetPage(sizeClassIndex);
                if (pPage) {
                    centralHeap.ReturnPage(pPage);
                    ops++;
                }
            }
            totalOpsCompleted.fetch_add(ops);
        });
    }

    // Let the threads run for the specified duration.
    std::this_thread::sleep_for(testDuration);
    shouldStop.store(true, std::memory_order_release);

    for (auto& t : threads)
    {
        t.join();
    }

    // Final Verification using the balance sheet method.
    size_t finalPageCount = centralHeap.GetStats(sizeClassIndex).FreePageCount;
    size_t finalCreatedCount = 0;
#ifndef NDEBUG
    finalCreatedCount = centralHeap.GetDebugPagesCreated();
#endif
    
    size_t pagesCreatedDuringTest = finalCreatedCount - initialCreatedCount;

    std::cout << "[ SOAK TEST ] Completed " << totalOpsCompleted.load() << " operations in " 
              << testDuration.count() << " seconds." << std::endl;
    std::cout << "[ SOAK TEST ] Throughput: " 
              << (totalOpsCompleted.load() / testDuration.count()) / 1000000.0 << " M-ops/sec" << std::endl;

    EXPECT_EQ(finalPageCount, initialPageCount + pagesCreatedDuringTest);
}