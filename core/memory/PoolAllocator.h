#pragma once
#include <cstddef>

// Enable this to allow memory pools to shrink by releasing unused memory chunks back to the OS.
// This adds a small memory overhead (one pointer per block) but can improve overall system memory usage.
#define BEDROCK_ENABLE_MEMORY_SHRINK

namespace BedrockServer::Core::Memory
{
    class PoolAllocator
    {
    private:
        // A node in a singly-linked list of free memory blocks.
        struct FreeNode 
        { 
            FreeNode* pNext; 
        };

#ifdef BEDROCK_ENABLE_MEMORY_SHRINK
        // Forward declaration.
        struct MemoryChunk;

        // Header stored at the beginning of each memory block.
        struct BlockHeader
        {
            MemoryChunk* pOwnerChunk; // Pointer back to the chunk that owns this block.
        };

        // Represents a single large chunk of memory obtained from the OS.
        struct MemoryChunk
        {
            MemoryChunk* pNext = nullptr;
            // Number of blocks currently allocated (not in the free list) from this chunk.
            std::size_t AllocCount = 0; 
            // The head of this chunk's own free list.
            FreeNode* pFreeList = nullptr;
        };
#else
        // Without shrink capability, we use a simpler chunk structure.
        struct MemoryChunk 
        { 
            MemoryChunk* pNext = nullptr; 
        };
#endif

    public:
        PoolAllocator(std::size_t payloadSize, std::size_t blockCount);
        ~PoolAllocator();

        // Rule of five: non-copyable, but movable.
        PoolAllocator(const PoolAllocator& other) = delete;
        PoolAllocator& operator=(const PoolAllocator& other) = delete;
        PoolAllocator(PoolAllocator&& other) noexcept;
        PoolAllocator& operator=(PoolAllocator&& other) noexcept;

        void* Allocate();
        void Deallocate(void* pPayload);

#ifdef BEDROCK_ENABLE_MEMORY_SHRINK
        // Releases completely unused memory chunks back to the OS.
        void Shrink();
#endif

    private:
        void AddNewChunk();
        
        MemoryChunk* ChunkListHead = nullptr; // Head of the linked list of memory chunks.

#ifndef BEDROCK_ENABLE_MEMORY_SHRINK
        FreeNode* FreeListHead = nullptr; // Head of the global free list for all chunks.
#endif
        
        std::size_t BlockSize = 0;      // Size of a single block, including headers/fences.
        std::size_t BlockCount = 0;     // Number of blocks per chunk.
        std::size_t PayloadSize = 0;    // The actual size requested by the user.
    };
}