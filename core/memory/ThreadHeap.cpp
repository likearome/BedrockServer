#include "memory/ThreadHeap.h"
#include "memory/CentralHeap.h"
#include "memory/MemoryUtil.h"
#include "common/ConcurrentQueue.h"
#include "common/Assert.h"
#include <new>
#include <cstdlib> // For std::malloc

namespace BedrockServer::Core::Memory
{
    // A thread-local pointer to each thread's own heap instance.
    thread_local ThreadHeap* g_pCurrentThreadHeap = nullptr;

    // Flag to ensure the heap is created only once per thread.
    thread_local bool g_isThreadHeapInitialized = false;

    ThreadHeap* GetCurrentThreadHeap()
    {
        // This is the modern C++ way to create a thread-local singleton.
        // The 'instance' object is constructed automatically on first use for each thread.
        // When the thread exits, its destructor is called and its memory is
        // reclaimed automatically by the C++ runtime.
        thread_local ThreadHeap instance;
        return &instance;
    }

    // --- ThreadHeap Member Function Implementations ---
    
    ThreadHeap::ThreadHeap() { Pages.fill(nullptr); }
    ThreadHeap::~ThreadHeap()
    {
        // When a thread exits, its ThreadHeap instance is destroyed.
        // We must return all the memory pages it was holding back to the CentralHeap.
        // If we don't, these pages are orphaned and all allocations within them
        // will be reported as memory leaks.
        ProcessDeferredFrees();

        for (size_t i = 0; i < NumSizeClasses; ++i)
        {
            MemoryPage* pPage = Pages[i];
            while (pPage != nullptr)
            {
                MemoryPage* pNext = pPage->pLocalNext;
                CentralHeap::GetInstance()->ReturnPage(pPage);
                pPage = pNext;
            }
        }
    }

    void* ThreadHeap::Allocate(std::size_t size)
    {
        const std::size_t sizeClassIndex = (size + ServerConfig::POOL_ALIGNMENT - 1) / ServerConfig::POOL_ALIGNMENT - 1;
        CHECK(sizeClassIndex < NumSizeClasses);

        while (true)
        {
            MemoryPage* pPage = Pages[sizeClassIndex];
            if (pPage != nullptr)
            {
                PageFreeBlock* pCurrentHead = pPage->pFreeList.load(std::memory_order_acquire);
                while (pCurrentHead != nullptr)
                {
                    PageFreeBlock* pNext = pCurrentHead->pNext;
                    if (pPage->pFreeList.compare_exchange_weak(pCurrentHead, pNext, std::memory_order_acq_rel))
                    {
                        pPage->UsedBlocks.fetch_add(1, std::memory_order_relaxed);
                        return static_cast<void*>(pCurrentHead);
                    }
                }
            }

            MemoryPage* pNewPage = CentralHeap::GetInstance()->GetPage(sizeClassIndex);
            if (pNewPage != nullptr)
            {
                pNewPage->pOwnerHeap = this;
                const uint32_t blockSize = (sizeClassIndex + 1) * ServerConfig::POOL_ALIGNMENT;
                const uint32_t numBlocks = (ServerConfig::PAGE_SIZE - sizeof(MemoryPage)) / blockSize;
                
                // Call the newly added InitFreeList function to prepare the page.
                pNewPage->InitFreeList(numBlocks, blockSize);
                
                pNewPage->pLocalNext = Pages[sizeClassIndex];
                Pages[sizeClassIndex] = pNewPage;
            }
            else
            {
                return nullptr;
            }
        }
    }

    void ThreadHeap::Deallocate(void* pPayload)
    {
        if (pPayload == nullptr) return;
        MemoryPage* pPage = GetPageFromPointer(pPayload);
        if (pPage->pOwnerHeap == this) 
        {
            FreeBlockInternal(pPayload);
        }
        else
        {
            pPage->pOwnerHeap->DeferredFreeQueue.Push(pPayload);
        }
    }

    void ThreadHeap::FreeBlockInternal(void* pPayload)
    {
        MemoryPage* pPage = GetPageFromPointer(pPayload);
        PageFreeBlock* pBlock = static_cast<PageFreeBlock*>(pPayload);

        PageFreeBlock* pCurrentHead = pPage->pFreeList.load(std::memory_order_acquire);
        do {
            pBlock->pNext = pCurrentHead;
        } while (!pPage->pFreeList.compare_exchange_weak(pCurrentHead, pBlock, std::memory_order_release));

        if (pPage->UsedBlocks.fetch_sub(1, std::memory_order_acq_rel) == 1)
        {
            ReturnPageIfEmpty(pPage);
        }
    }

    void ThreadHeap::ProcessDeferredFrees()
    {
        using Node = Common::ConcurrentQueue<void*>::Node;
        Node* pFrees = DeferredFreeQueue.PopAll();

        // MODIFICATION: Separate consumption from destruction.

        // Phase 1: Consume the list. Process data and collect nodes to delete.
        Node* pHeadOfDeleteList = pFrees;
        while (pFrees != nullptr)
        {
            FreeBlockInternal(pFrees->Data);
            pFrees = pFrees->pNext;
        }

        // Phase 2: Destroy the nodes after traversal is complete.
        while (pHeadOfDeleteList != nullptr)
        {
            auto* pOldNode = pHeadOfDeleteList;
            pHeadOfDeleteList = pHeadOfDeleteList->pNext;
            delete pOldNode;
        }
    }

    void ThreadHeap::ReturnPageIfEmpty(MemoryPage* pPage)
    {
        const size_t sizeClassIndex = pPage->SizeClassIndex;
        MemoryPage*& pHead = Pages[sizeClassIndex];

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
        CentralHeap::GetInstance()->ReturnPage(pPage);
    }
}