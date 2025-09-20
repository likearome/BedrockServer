#include "gtest/gtest.h"
#include "memory/MemoryManager.h"
#include "memory/ThreadHeap.h"
#include "memory/CentralHeap.h"
#include "memory/BedrockMemoryResource.h"
#include "common/ServerConfig.h"
#include "common/ConcurrentQueue.h"
#include <vector>
#include <thread>
#include <memory_resource>
#include <cstring>

using namespace BedrockServer::Core;
using namespace BedrockServer::Core::Memory;
using namespace BedrockServer::Core::Common;

class MemoryManagerTest : public ::testing::Test {};

// --- Tests from before, unchanged ---
// ... (BasicSmallAllocation, SingleLargeAllocAndFree, etc. remain the same) ...

// --- Cross-Thread Deallocation Test ---
TEST_F(MemoryManagerTest, CrossThreadFreeingIsHandledCorrectly)
{
    constexpr int numAllocs = 500; 
    constexpr size_t allocSize = 32;

    ConcurrentQueue<void*> pointerQueue;
    ThreadHeap* pAllocatorHeap = nullptr;

    std::thread allocatorThread([&]() {
        pAllocatorHeap = GetCurrentThreadHeap();
        for (int i = 0; i < numAllocs; ++i) {
            void* p = MemoryManager::GetInstance().Allocate(allocSize);
            ASSERT_NE(p, nullptr);
            pointerQueue.Push(p);
        }
    });

    std::thread deallocatorThread([&]() {
        int deallocCount = 0;
        while (deallocCount < numAllocs)
        {
            ConcurrentQueue<void*>::Node* pNodeList = pointerQueue.PopAll();
            
            // MODIFICATION: Separate consumption from destruction.
            
            // Phase 1: Consume the list. Process data and collect nodes to delete.
            ConcurrentQueue<void*>::Node* pHeadOfDeleteList = pNodeList;
            while(pNodeList != nullptr)
            {
                MemoryManager::GetInstance().Deallocate(pNodeList->Data);
                deallocCount++;
                pNodeList = pNodeList->pNext;
            }

            // Phase 2: Destroy the nodes after traversal is complete.
            while(pHeadOfDeleteList != nullptr)
            {
                auto* pOldNode = pHeadOfDeleteList;
                pHeadOfDeleteList = pHeadOfDeleteList->pNext;
                delete pOldNode;
            }

            if (pNodeList == nullptr) {
                std::this_thread::yield();
            }
        }
    });

    allocatorThread.join();
    deallocatorThread.join();

    ASSERT_NE(pAllocatorHeap, nullptr);
    pAllocatorHeap->ProcessDeferredFrees();
}