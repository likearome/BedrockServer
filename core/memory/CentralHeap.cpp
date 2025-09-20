#include "memory/CentralHeap.h"
#include "memory/MemoryPage.h"
#include "common/ServerConfig.h"
#include <sys/mman.h>
#include <new>

namespace BedrockServer::Core::Memory
{
    CentralHeap* CentralHeap::s_pInstance = nullptr;

    CentralHeap* CentralHeap::GetInstance() { return s_pInstance; }
    void CentralHeap::SetInstance(CentralHeap* instance) { s_pInstance = instance; }
    
    CentralHeap::CentralHeap()
    {
        FreePages.fill(nullptr);
    }

    CentralHeap::~CentralHeap() {}

    MemoryPage* CentralHeap::GetPage(size_t sizeClassIndex)
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        if (FreePages[sizeClassIndex] != nullptr)
        {
            MemoryPage* pPage = FreePages[sizeClassIndex];
            FreePages[sizeClassIndex] = pPage->pNextPageInCentralHeap;
            pPage->pNextPageInCentralHeap = nullptr;
            return pPage;
        }

        const size_t blockSize = (sizeClassIndex + 1) * ServerConfig::POOL_ALIGNMENT;
        constexpr size_t numPagesToAlloc = 16;
        const size_t totalSize = ServerConfig::PAGE_SIZE * numPagesToAlloc;

#ifdef __SANITIZE_THREAD__
        // When using TSan, use malloc so the sanitizer can track the allocation.
        void* pMem = std::malloc(totalSize);
#else
        // In normal builds, use mmap for performance.
        void* pMem = mmap(nullptr, totalSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
#endif
        if (pMem == MAP_FAILED) return nullptr;
        
        m_DebugPagesCreated.fetch_add(numPagesToAlloc, std::memory_order_relaxed);
        
        MemoryPage* pPageToReturn = new (pMem) MemoryPage(blockSize, sizeClassIndex);

        for(size_t i = 1; i < numPagesToAlloc; ++i)
        {
            void* pPageMem = static_cast<std::byte*>(pMem) + (i * ServerConfig::PAGE_SIZE);
            MemoryPage* pNewPage = new (pPageMem) MemoryPage(blockSize, sizeClassIndex);
            pNewPage->pNextPageInCentralHeap = FreePages[sizeClassIndex];
            FreePages[sizeClassIndex] = pNewPage;
        }

        return pPageToReturn;
    }

    void CentralHeap::ReturnPage(MemoryPage* pPage)
    {
        if (pPage == nullptr) return;
        
        std::lock_guard<std::mutex> lock(m_Mutex);
        const size_t sizeClassIndex = pPage->SizeClassIndex;

        pPage->pNextPageInCentralHeap = FreePages[sizeClassIndex];
        FreePages[sizeClassIndex] = pPage;
    }

    CentralHeap::PageStats CentralHeap::GetStats(size_t sizeClassIndex)
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        PageStats stats;

        MemoryPage* pCurrent = FreePages[sizeClassIndex];
        size_t count = 0;
        while (pCurrent != nullptr)
        {
            count++;
            pCurrent = pCurrent->pNextPageInCentralHeap;
        }
        stats.FreePageCount = count;
        return stats;
    }
}