#pragma once
#include "common/NonCopyable.h"
#include "memory/MemoryPage.h"
#include "common/ServerConfig.h"
#include <array>
#include <mutex>
#include <atomic>

namespace BedrockServer::Core::Memory
{
    class MemoryManager; // Forward declaration

    class CentralHeap : public Common::NonCopyable
    {
        friend class MemoryManager;

    private:
        static constexpr size_t NumSizeClasses = 
            ServerConfig::MAX_SMALL_OBJECT_SIZE / ServerConfig::POOL_ALIGNMENT;

    public:
        struct PageStats { size_t FreePageCount = 0; };

        static CentralHeap* GetInstance();
        static void SetInstance(CentralHeap* instance);
        
        MemoryPage* GetPage(size_t sizeClassIndex);
        void ReturnPage(MemoryPage* pPage);
        PageStats GetStats(size_t sizeClassIndex);

    private:
        CentralHeap();
        ~CentralHeap();

        static CentralHeap* s_pInstance;

        // An array of head pointers to singly linked lists of MemoryPages.
        std::array<MemoryPage*, NumSizeClasses> FreePages;
        
        alignas(std::hardware_destructive_interference_size) std::mutex m_Mutex;
        std::atomic<size_t> m_DebugPagesCreated = 0;
    };
}