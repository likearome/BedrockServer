#pragma once
#include "memory/PoolAllocator.h"
#include <vector>

namespace BedrockServer::Core::Memory
{
    /// @class SmallObjectAllocator
    /// @brief Manages multiple pools to handle allocations for various small object sizes.
    /// Acts as a facade over a series of PoolAllocator instances.
    class SmallObjectAllocator
    {
    public:
        SmallObjectAllocator();
        ~SmallObjectAllocator();
        
        // Non-copyable
        SmallObjectAllocator(const SmallObjectAllocator& other) = delete;
        SmallObjectAllocator& operator=(const SmallObjectAllocator& other) = delete;

        void* Allocate(std::size_t size);
        void Deallocate(void* pBlock, std::size_t size);

    private:
        // We will have pools for 8 * n = 8, 16, 24, 32, 40, 48, ...
        PoolAllocator* pools_ = nullptr;
        std::size_t num_pools_ = 0;
    };
}