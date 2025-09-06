#include "memory/ThreadHeap.h"
#include "memory/CentralHeap.h"
#include "common/ServerConfig.h"
#include "common/Assert.h"
#include <new>

namespace
{
    // Helper function to find the start of the page from any pointer within it.
    BedrockServer::Core::Memory::MemoryPage* GetPageFromPointer(void* p)
    {
        static_assert(std::has_single_bit(BedrockServer::Core::ServerConfig::PAGE_SIZE), "PAGE_SIZE must be a power of two.");
        return reinterpret_cast<BedrockServer::Core::Memory::MemoryPage*>(
            reinterpret_cast<uintptr_t>(p) & ~(BedrockServer::Core::ServerConfig::PAGE_SIZE - 1)
        );
    }
}

namespace BedrockServer::Core::Memory
{
    // A global, thread-local pointer for fast access to the current thread's heap.
    thread_local ThreadHeap* pCurrentThreadHeap = nullptr;

    ThreadHeap::ThreadHeap()
    {
        pCurrentThreadHeap = this;
        Pages.fill(nullptr);
    }

    ThreadHeap::~ThreadHeap()
    {
        // When a thread exits, its ThreadHeap is destroyed.
        // We must return all cached pages back to the CentralHeap.
        for (size_t i = 0; i < Pages.size(); ++i)
        {
            MemoryPage* pPage = Pages[i];
            while (pPage != nullptr)
            {
                MemoryPage* pNext = pPage->pLocalNext;
                CentralHeap::GetInstance().ReturnPage(pPage);
                pPage = pNext;
            }
        }
    }

    void* ThreadHeap::Allocate(std::size_t size)
    {
        // A counter for the adaptive spin-wait.
        constexpr int SPIN_COUNT_BEFORE_YIELD = 5000;
        int spinCount = 0;

        const std::size_t sizeClassIndex = (size + ServerConfig::POOL_ALIGNMENT - 1) / ServerConfig::POOL_ALIGNMENT -1;
        CHECK(sizeClassIndex < NumSizeClasses);

        while (true) // Top-level loop to retry allocation
        {
            MemoryPage* pPage = Pages[sizeClassIndex];
            
            // 1. Check if we have a usable page.
            if (pPage != nullptr && pPage->pFreeList.load() != nullptr)
            {
                // 2. Try to allocate from the page's free list.
                PageFreeBlock* pBlock = pPage->pFreeList.load();

                // Since only this thread can pop from this list, we don't need a CAS loop.
                // A simple atomic exchange is sufficient and faster.
                PageFreeBlock* pNext = pBlock->pNext;
                pPage->pFreeList.store(pNext); // A simple store might also work if ordering is guaranteed.

                pPage->UsedBlocks.fetch_add(1);
                return static_cast<void*>(pBlock);
            }

            // 3. If we are here, it means we have no page or the page is full.
            //    Request a new page from the CentralHeap.
            MemoryPage* pNewPage = CentralHeap::GetInstance().GetPage(sizeClassIndex);
            if (pNewPage != nullptr)
            {
                pNewPage->pOwnerHeap = this;
                pNewPage->pLocalNext = Pages[sizeClassIndex];
                Pages[sizeClassIndex] = pNewPage;
                // Loop again to retry the allocation with the new page.
            }
            else
            {
                return nullptr; // Truly out of memory.
            }
                    
            // Adaptive spin-wait.
            if (++spinCount > SPIN_COUNT_BEFORE_YIELD)
            {
                spinCount = 0;
                std::this_thread::yield(); // Yield the CPU slice to other threads.
            }
        }
    }

    void ThreadHeap::Deallocate(void* pPayload)
    {
        // A counter for the adaptive spin-wait.
        constexpr int SPIN_COUNT_BEFORE_YIELD = 5000;
        int spinCount = 0;

        if (pPayload == nullptr) return;

        // From the payload pointer, find the header of the page it belongs to.
        MemoryPage* pPage = GetPageFromPointer(pPayload);

        // Get the current thread's heap.
        ThreadHeap* pCurrentHeap = pCurrentThreadHeap; // Use the thread_local variable
        CHECK(pCurrentHeap != nullptr);

        // Check if the page is owned by the current thread's heap.
        if (pPage->pOwnerHeap == pCurrentHeap) 
        {
            // --- Case 1: Same-thread deallocation (Fast Path) ---
            // Just add the block back to the page's local free list.
            PageFreeBlock* pBlock = static_cast<PageFreeBlock*>(pPayload);
            
            // Atomically push to the page's free list.
            PageFreeBlock* pCurrentHead = pPage->pFreeList.load();
            do {
                // Adaptive spin-wait.
                if (++spinCount > SPIN_COUNT_BEFORE_YIELD)
                {
                    spinCount = 0;
                    std::this_thread::yield(); // Yield the CPU slice to other threads.
                }

                pBlock->pNext = pCurrentHead;
            } while (!pPage->pFreeList.compare_exchange_weak(pCurrentHead, pBlock));

            uint32_t prevUsed = pPage->UsedBlocks.fetch_sub(1);
            if (prevUsed == 1) // This was the last block
            {
                ReturnPageIfEmpty(pPage);
            }
        }
        else
        {
            // --- Case 2: Cross-thread deallocation (Slow Path) ---
            // Add the block to the original owner's deferred free queue.
            // This requires a thread-safe queue implementation.
            pPage->pOwnerHeap->DeferredFreeQueue.Push(pPayload);
        }
    }
    void ThreadHeap::ProcessDeferredFrees()
    {
        // A counter for the adaptive spin-wait.
        constexpr int SPIN_COUNT_BEFORE_YIELD = 5000;
        int spinCount = 0;

        // Atomically grab the entire list of pending frees from other threads.
        Common::ConcurrentQueue<void*>::Node* pFrees = DeferredFreeQueue.PopAll();

        // Now, process the list lock-free.
        while (pFrees != nullptr)
        {
            void* pPayload = pFrees->Data;
            
            // This is a "local" deallocation now.
            MemoryPage* pPage = GetPageFromPointer(pPayload);
            PageFreeBlock* pBlock = static_cast<PageFreeBlock*>(pPayload);
            
            // Atomically push to the page's free list.
            PageFreeBlock* pCurrentHead = pPage->pFreeList.load();
            do {
                // Adaptive spin-wait.
                if (++spinCount > SPIN_COUNT_BEFORE_YIELD)
                {
                    spinCount = 0;
                    std::this_thread::yield(); // Yield the CPU slice to other threads.
                }
                pBlock->pNext = pCurrentHead;
            } while (!pPage->pFreeList.compare_exchange_weak(pCurrentHead, pBlock));
            
            uint32_t prevUsed = pPage->UsedBlocks.fetch_sub(1);
            if (prevUsed == 1)
            {
                ReturnPageIfEmpty(pPage);
            }
            
            // Move to the next item and delete the queue node.
            auto* pOldNode = pFrees;
            pFrees = pFrees->pNext;
            delete pOldNode;
        }
    }
    void ThreadHeap::ReturnPageIfEmpty(MemoryPage* pPage)
    {
        const size_t sizeClassIndex = pPage->SizeClassIndex;
        MemoryPage*& pHead = Pages[sizeClassIndex];

        // --- Unlink the page from this thread's local list ---
        if (pHead == pPage)
        {
            pHead = pPage->pLocalNext;
        }
        else
        {
            MemoryPage* pCurrent = pHead;
            while (pCurrent != nullptr && pCurrent->pLocalNext != pPage)
            {
                pCurrent = pCurrent->pLocalNext;
            }

            if (pCurrent != nullptr)
            {
                pCurrent->pLocalNext = pPage->pLocalNext;
            }
        }
        
        pPage->pLocalNext = nullptr;
        CentralHeap::GetInstance().ReturnPage(pPage);
    }
}