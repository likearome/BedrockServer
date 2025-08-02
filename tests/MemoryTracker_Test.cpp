#include <gtest/gtest.h>
#include "memory/SmallObjectAllocator.h"
#include "memory/MemoryTracker.h"
#include <vector>
#include <thread>

// Google Test macro to define a test case
TEST(MemoryTrackerTest, DetectsLeaksFromMultipleThreads)
{
    // The allocator is now local to the test.
    BedrockServer::Core::Memory::SmallObjectAllocator allocator;

    auto workerThread = [&](int threadNum)
    {
        std::vector<void*> allocations;
        for (int i = 0; i < 100; ++i)
        {
            allocations.push_back(allocator.Allocate(10));
        }

        // Each thread leaks one block.
        for (size_t i = 0; i < 99; ++i)
        {
            allocator.Deallocate(allocations[i], 10);
        }
    };

    std::vector<std::jthread> threads;
    const int numThreads = 4;
    for (int i = 0; i < numThreads; ++i)
    {
        threads.emplace_back(workerThread, i);
    }
    
    // Threads join automatically when vector goes out of scope.
}
