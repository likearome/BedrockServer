#include "MemoryPage.h"
#include "common/ServerConfig.h"
#include <cstddef> // For std::byte

namespace BedrockServer::Core::Memory
{
    void MemoryPage::InitFreeList(uint32_t numBlocks, uint32_t blockSize)
    {
        // This function takes a raw page and formats it into a singly linked list
        // of free blocks for the ThreadHeap to use.

        // The memory available for blocks starts right after the MemoryPage header.
        std::byte* const pBlockStart = reinterpret_cast<std::byte*>(this) + sizeof(MemoryPage);
        
        PageFreeBlock* pHead = nullptr;
        for (uint32_t i = 0; i < numBlocks; ++i)
        {
            // Calculate the address of the current block.
            void* pBlockMem = pBlockStart + (i * blockSize);
            PageFreeBlock* pBlock = static_cast<PageFreeBlock*>(pBlockMem);

            // Push it to the front of our temporary list.
            pBlock->pNext = pHead;
            pHead = pBlock;
        }

        // Atomically set the head of the lock-free list.
        pFreeList.store(pHead, std::memory_order_relaxed);
    }
}