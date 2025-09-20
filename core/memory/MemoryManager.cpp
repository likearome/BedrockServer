#include "memory/MemoryManager.h"
#include "memory/ThreadHeap.h"
#include "memory/CentralHeap.h"
#include "memory/MemoryUtil.h"
#include "common/ServerConfig.h"
#include <mutex>
#include <sys/mman.h>
#include <cstddef>
#include <cstdlib>
#include <new>

namespace BedrockServer::Core::Memory
{
    struct LargeAllocHeader
    {
        static constexpr uint64_t MAGIC = 0xDEADBEEFABADCAFE;
        uint64_t Magic;
        std::size_t TotalSize;
    };
    
    std::once_flag g_initFlag;
    CentralHeap* g_pCentralHeap = nullptr;

    void MemoryManager::Initialize()
    {
        void* pMemory = std::malloc(sizeof(CentralHeap));
        if (pMemory == nullptr) return;
        
        g_pCentralHeap = new (pMemory) CentralHeap();
        CentralHeap::SetInstance(g_pCentralHeap);
    }

    void MemoryManager::Shutdown()
    {
        // This is intentionally left empty to prevent shutdown crashes.
        // The OS will reclaim the memory of the CentralHeap singleton.
    }
    
    void EnsureInitialized()
    {
        std::call_once(g_initFlag, MemoryManager::Initialize);
    }

    MemoryManager& MemoryManager::GetInstance()
    {
        EnsureInitialized();
        static MemoryManager instance;
        return instance;
    }

    void* MemoryManager::Allocate(std::size_t userSize)
    {
        EnsureInitialized();

        if (userSize == 0) return nullptr;

        if (userSize <= ServerConfig::MAX_SMALL_OBJECT_SIZE)
        {
            return GetCurrentThreadHeap()->Allocate(userSize);
        }

        const std::size_t totalSize = userSize + sizeof(LargeAllocHeader);
#ifdef __SANITIZE_THREAD__
        void* pBlock = std::malloc(totalSize);
#else
        void* pBlock = mmap(nullptr, totalSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
#endif
        if (pBlock == MAP_FAILED) return nullptr;

        LargeAllocHeader* pHeader = static_cast<LargeAllocHeader*>(pBlock);
        pHeader->Magic = LargeAllocHeader::MAGIC;
        pHeader->TotalSize = totalSize;
        
        return static_cast<std::byte*>(pBlock) + sizeof(LargeAllocHeader);
    }

    void* MemoryManager::Allocate(std::size_t userSize, std::align_val_t al)
    {
        EnsureInitialized();
        const std::size_t alignment = static_cast<std::size_t>(al);
        const std::size_t totalSize = userSize + sizeof(LargeAllocHeader) + alignment;
        
#ifdef __SANITIZE_THREAD__
        void* pBlock = std::malloc(totalSize);
#else
        void* pBlock = mmap(nullptr, totalSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
#endif
        if (pBlock == MAP_FAILED) return nullptr;
        
        void* pPayload = reinterpret_cast<void*>(
            (reinterpret_cast<uintptr_t>(pBlock) + sizeof(LargeAllocHeader) + (alignment - 1)) & ~(alignment - 1)
        );
        
        LargeAllocHeader* pHeader = reinterpret_cast<LargeAllocHeader*>(static_cast<std::byte*>(pPayload) - sizeof(LargeAllocHeader));
        pHeader->Magic = LargeAllocHeader::MAGIC;
        pHeader->TotalSize = totalSize;

        return pPayload;
    }

    void MemoryManager::Deallocate(void* pPayload)
    {
        EnsureInitialized();
        if (pPayload == nullptr) return;

        LargeAllocHeader* pHeader = reinterpret_cast<LargeAllocHeader*>(
            static_cast<std::byte*>(pPayload) - sizeof(LargeAllocHeader));

        if (pHeader->Magic == LargeAllocHeader::MAGIC)
        {
            void* pBlock = static_cast<void*>(pHeader);
            const std::size_t totalSize = pHeader->TotalSize;
            pHeader->Magic = 0; // Prevent double-free
#ifdef __SANITIZE_THREAD__
            std::free(pBlock);
#else
            munmap(pBlock, totalSize);
#endif
            return;
        }

        MemoryPage* pPage = GetPageFromPointer(pPayload);

        // This is the crucial ID check.
        // We verify if the page this pointer belongs to is a valid page we created.
        if (pPage->Magic == MemoryPage::MAGIC)
        {
            // If it is our page, proceed with deallocation.
            if (pPage->pOwnerHeap != nullptr)
            {
                pPage->pOwnerHeap->Deallocate(pPayload);
            }
            return;
        }
        
        // If the magic number does not match, this is a "foreign" pointer
        // from a source like the standard library.
        // To prevent a crash, we must not process it. We simply ignore it.
    }
}