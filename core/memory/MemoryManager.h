#pragma once
#include "memory/SmallObjectAllocator.h"

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
        void Deallocate(void* p);

    private:
        MemoryManager() = default;
        ~MemoryManager() = default;

        SmallObjectAllocator small_obj_alloc_;
    };
}