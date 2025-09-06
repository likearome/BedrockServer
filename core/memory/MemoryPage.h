#pragma once
#include <cstddef>
#include <atomic>

namespace BedrockServer::Core::Memory
{
    // Forward declaration to avoid circular dependency.
    class ThreadHeap;

    // Represents a single block of memory that can be allocated.
    // IT's a node in a singly-linked list of free blocks.
    struct PageFreeBlock
    {
        PageFreeBlock* pNext;
    };

    // Represents a page of memory (e.g. 16KB) that is carved up into blocks of a fixed size.
    // Each page is owned by a specific thread.
    class MemoryPage
    {
    public:
        // Constructor to initialize the const members.
        MemoryPage(uint32_t blockSize, uint32_t numBlocks, uint32_t sizeClassIndex)
            : SizeClassIndex(sizeClassIndex), BlockSize(blockSize), NumBlocks(numBlocks) 
        {}

        const uint32_t SizeClassIndex;

        ThreadHeap* pOwnerHeap = nullptr; // Pointer to the thread heap that owns this page.

        // Page state and info
        std::atomic<uint32_t> UsedBlocks = 0;   // Number of blocks currently allocated from this page.
        const uint32_t BlockSize;           // The size of each block in this page.
        const uint32_t NumBlocks;        // Total number of blocks in this page.

        // Links for Managing lists of pages
        MemoryPage* pNextPage = nullptr;    // For linking pages in a central pool.
        MemoryPage* pLocalNext = nullptr;   // For linking pages in a thread's local cache.

        // The head of the free list for blocks within this page
        std::atomic<PageFreeBlock*> pFreeList;
    };
}