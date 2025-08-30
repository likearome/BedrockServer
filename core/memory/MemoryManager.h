#pragma once
#include<array>
#include"memory/SmallObjectAllocator.h"
#include"memory/LargeObjectAllocator.h"
#include"common/ServerConfig.h"

namespace BedrockServer::Core::Memory
{
    /// @class MemoryManager
    /// @brief the central manager for all memory allocations.
    class MemoryManager
    {
    public:
        static MemoryManager& GetInstance();
        
        // Non-copyable, non-movable singleton
        MemoryManager(const MemoryManager&) = delete;
        MemoryManager& operator=(const MemoryManager&) = delete;
        MemoryManager(MemoryManager&&) = delete;
        MemoryManager& operator=(MemoryManager&&) = delete;

        void* Allocate(std::size_t size);
        void* Allocate(std::size_t size, std::align_val_t al);
        void Deallocate(void* p);

    private:
        MemoryManager() = default;
        ~MemoryManager() = default;

        // alignas(64) prevents false sharing between threads on different cores.
        // We now have an array of allocators, one for each thread.
        alignas(64) std::array<SmallObjectAllocator, ServerConfig::MAX_THREADS> ThreadAllocator;

        // Add an allocator for large objects.
        // It is thread-safe because the class is stateless and its underlying OS calls are gueranteed to be safe.
        LargeObjectAllocator LargeObjectAlloc;
    };
}