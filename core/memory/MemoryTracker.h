#pragma once
#include <map>
#include <mutex>
#include <string_view>
#include <cstdint>
#include <array>
#include "common/ServerConfig.h"
#include "memory/SystemAllocator.h"

namespace BedrockServer::Core::Memory
{
    /// @struct AllocateInfo
    /// @brief Holds metadata for a single memory allocation.
    struct AllocationInfo
    {
        std::size_t size = 0;
        const char* file = nullptr; 
        int line = 0;
    };

    /// @class Memory Tracker
    /// @brief Singleton class to track all memory allocations for leak detection.
    class MemoryTracker
    {
    private:
        // The map now uses our SystemAllocator, breaking the recursion.
        using AllocationMap = std::map<
            void*, 
            AllocationInfo, 
            std::less<void*>, 
            SystemAllocator<std::pair<void* const, AllocationInfo>>
        >;
        /// @struct PerThreadData
        /// @brief Holds the allocation map for a single thread
        /// using alignas to prevent false sharing
        struct alignas(64) PerThreadData
        {
            //std::map<void*, AllocationInfo> allocations;
            AllocationMap allocations;
        };
    public:
        // Get the singleton instance
        static MemoryTracker& GetInstance();

        // Non-copyable, non-movable singleton
        MemoryTracker(const MemoryTracker&) = delete;
        MemoryTracker& operator=(const MemoryTracker&) = delete;
        MemoryTracker(MemoryTracker&&) = delete;
        MemoryTracker& operator=(MemoryTracker&&) = delete;

        void Track(uint32_t threadId, void* p, std::size_t size, const char* file, int line);
        void Untrack(uint32_t threadId, void* p);
        void ReportLeaks();
    private:
        MemoryTracker() = default;
        ~MemoryTracker() = default;
        
        std::array<PerThreadData, ServerConfig::MAX_THREADS> thread_data_;
    };
}