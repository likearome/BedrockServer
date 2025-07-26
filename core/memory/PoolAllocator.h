#pragma once
#include <cstddef>

namespace BedrockServer::Core::Memory
{
    /// @class PoolAllocator
    /// @brief A simple, non-thread-safe pool allocator for fixed-size memory blocks.
    class PoolAllocator
    {
    private:
        /// @struct FreeNode
        /// @brief A node in the singly-linked list of free memory blocks.
        /// Its memory footprint is the same as a single pointer
        struct FreeNode
        {
            FreeNode* pNext;
        };
    public:
        // PLEASE DO NOT SET DEFAULT VALUE FOR blockCount !
        PoolAllocator(std::size_t blockSize, std::size_t blockCount);
        ~PoolAllocator();

        // --- START: Rule of Five Implementation ---
        
        // Non-copyable.
        PoolAllocator(const PoolAllocator& other) = delete;
        PoolAllocator& operator=(const PoolAllocator& other) = delete;

        // Movable.
        PoolAllocator(PoolAllocator&& other) noexcept;
        PoolAllocator& operator=(PoolAllocator&& other) noexcept;

        // --- END: Rule of Five Implementation ---

        void* Allocate();
        void Deallocate(void* pBlock);

    private:
        std::byte* memory_chunk_ = nullptr;  // Owns the pre-allocated pool memory.
        FreeNode* free_list_head_ = nullptr; // Head of the free list.
        std::size_t block_size_ = 0;         // Size of a single block.
        std::size_t block_count_ = 0;        // Number of blocks in the pool.
        std::size_t payload_size_ = 0;       // The actual size requested by the user.
    };
}