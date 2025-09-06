#include "CentralHeap.h"
#include "common/Assert.h"
#include <sys/mman.h>
#include <new> // for placement new

namespace BedrockServer::Core::Memory
{
    CentralHeap::CentralHeap()
    {
        // Initialize all list heads to null.
        FreePages.fill(nullptr);
    }

    CentralHeap::~CentralHeap() {}

    CentralHeap& CentralHeap::GetInstance()
    {
        // Thread-safe Meyers' Singleton.
        static CentralHeap instance;
        return instance;
    }

    // CentralHeap.cpp의 GetPage 함수를 아래 코드로 교체
    MemoryPage* CentralHeap::GetPage(size_t sizeClassIndex)
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        if (FreePages[sizeClassIndex] != nullptr)
        {
            MemoryPage* pPage = FreePages[sizeClassIndex];
            FreePages[sizeClassIndex] = pPage->pNextPage;
            pPage->pNextPage = nullptr;
            return pPage;
        }

        const size_t blockSize = (sizeClassIndex + 1) * ServerConfig::POOL_ALIGNMENT;
        const size_t numPagesToAlloc = 16;
        const size_t totalSize = ServerConfig::PAGE_SIZE * numPagesToAlloc;

        void* pMem = mmap(nullptr, totalSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (pMem == MAP_FAILED) return nullptr;

        #ifndef NDEBUG
            // ❗️생성된 페이지 수를 카운트합니다.
            m_DebugPagesCreated += numPagesToAlloc;
        #endif
        
        // Construct all 16 page headers first.
        for(size_t i = 0; i < numPagesToAlloc; ++i)
        {
            void* pPageMem = static_cast<std::byte*>(pMem) + (i * ServerConfig::PAGE_SIZE);
            new (pPageMem) MemoryPage(blockSize, 0, sizeClassIndex);
        }

        // The first page is reserved for the caller.
        MemoryPage* pPageToReturn = reinterpret_cast<MemoryPage*>(pMem);

        // The remaining pages (from index 1 to 15) form the new free list.
        if (numPagesToAlloc > 1)
        {
            // The new head of the free list is the second page (index 1).
            MemoryPage* pNewHead = reinterpret_cast<MemoryPage*>(static_cast<std::byte*>(pMem) + ServerConfig::PAGE_SIZE);

            // Link pages from index 1 to 14.
            for(size_t i = 1; i < numPagesToAlloc - 1; ++i) 
            {
                reinterpret_cast<MemoryPage*>(static_cast<std::byte*>(pMem) + (i * ServerConfig::PAGE_SIZE))->pNextPage = 
                    reinterpret_cast<MemoryPage*>(static_cast<std::byte*>(pMem) + ((i + 1) * ServerConfig::PAGE_SIZE));
            }
            
            // The last page (index 15) must point to the existing free list.
            reinterpret_cast<MemoryPage*>(static_cast<std::byte*>(pMem) + ((numPagesToAlloc-1) * ServerConfig::PAGE_SIZE))->pNextPage = FreePages[sizeClassIndex];

            // Now, update the main free list head to point to the new sub-list.
            FreePages[sizeClassIndex] = pNewHead;
        }

        return pPageToReturn;
    }

    void CentralHeap::ReturnPage(MemoryPage* pPage)
    {
        if (pPage == nullptr) return;

        // Lock the entire function to ensure exclusive access.
        std::lock_guard<std::mutex> lock(m_Mutex);
        
        const size_t sizeClassIndex = pPage->SizeClassIndex;

        // Push the page to the front of the singly-linked list.
        pPage->pNextPage = FreePages[sizeClassIndex];
        FreePages[sizeClassIndex] = pPage;
    }

    CentralHeap::PageStats CentralHeap::GetStats(size_t sizeClassIndex)
    {
        // Lock to ensure we can safely traverse the list for an accurate count.
        std::lock_guard<std::mutex> lock(m_Mutex);
        
        PageStats stats;
        size_t count = 0;
        MemoryPage* pCurrent = FreePages[sizeClassIndex];
        while (pCurrent != nullptr)
        {
            count++;
            pCurrent = pCurrent->pNextPage;
        }
        stats.FreePageCount = count;
        return stats;
    }
}