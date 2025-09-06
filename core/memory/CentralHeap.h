#pragma once
#include "MemoryPage.h"
#include "common/NonCopyable.h"
#include "common/ServerConfig.h"
#include <array>
#include <mutex>
#include <atomic> // ❗️atomic 헤더 추가

namespace BedrockServer::Core::Memory
{
    /// @class CentralHeap
    /// @brief A thread-safe central repository for free MemoryPages.
    /// All public methods are protected by a single mutex to guarantee thread safety.
    class CentralHeap : public Common::NonCopyable
    {
    private:
        // Calculate the number of size classes at compile time.
        static constexpr size_t NumSizeClasses = 
            ServerConfig::MAX_SMALL_OBJECT_SIZE / ServerConfig::POOL_ALIGNMENT;

    public:
        /// @struct PageStats
        /// @brief A simple struct to hold diagnostic information, required by tests.
        struct PageStats 
        { 
            size_t FreePageCount = 0; 
        };

        /// @brief Gets the singleton instance of the CentralHeap.
        static CentralHeap& GetInstance();
        
        /// @brief Retrieves a free page. If none are available, allocates a new batch from the OS.
        MemoryPage* GetPage(size_t sizeClassIndex);
        
        /// @brief Returns a single page to the free list.
        void ReturnPage(MemoryPage* pPage);
        
        /// @brief Gets diagnostic statistics for a given size class.
        PageStats GetStats(size_t sizeClassIndex);
            
#ifndef NDEBUG
        void ResetDebugCounters() { m_DebugPagesCreated = 0; }
        size_t GetDebugPagesCreated() const { return m_DebugPagesCreated.load(); }
#endif

    private:
        // Private constructor and destructor for the singleton pattern.
        CentralHeap();
        ~CentralHeap();

        // An array of simple pointers to the heads of the free lists.
        std::array<MemoryPage*, NumSizeClasses> FreePages;
        
        // A single mutex to protect all concurrent access to the FreePages array.
        alignas(std::hardware_destructive_interference_size) std::mutex m_Mutex;

        // counters for whole pages created by mmap
        std::atomic<size_t> m_DebugPagesCreated = 0;
    };
}