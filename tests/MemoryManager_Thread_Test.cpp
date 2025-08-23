#include "gtest/gtest.h"
#include "memory/MemoryManager.h"
#include "memory/Memory.h"
#include <thread>
#include <vector>
#include <list> // Using list for easy pop_front

// This is the test for your actual, thread-safe MemoryManager.
// It will pass with your per-thread design.
// It WILL FAIL if you force all threads to use allocator[0].
TEST(MemoryManagerThreadTest, StressedConcurrentAllocAndDealloc)
{
    const unsigned int num_threads = std::thread::hardware_concurrency();
    ASSERT_GE(num_threads, 2);

    constexpr int ITERATIONS = 50000;
    // Each thread will maintain a list of 100 active pointers at all times.
    constexpr int POINTER_LIST_SIZE = 100;

    std::vector<std::thread> threads;
    
    for (unsigned int i = 0; i < num_threads; ++i) {
        threads.emplace_back([]() {
            // Use std::list for efficient add/remove from the front.
            std::list<int*> pointers;

            // Pre-fill the list.
            for (int k = 0; k < POINTER_LIST_SIZE; ++k) {
                pointers.push_back(new int);
            }

            // In each iteration, deallocate the oldest and allocate a new one.
            // This creates constant churn on the free list.
            for (int k = 0; k < ITERATIONS; ++k) {
                delete pointers.front();
                pointers.pop_front();
                pointers.push_back(new int);
            }

            // Clean up the remaining pointers.
            for (int* p : pointers) {
                delete p;
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    // If this test completes without crashing, it's a success.
    SUCCEED();
}