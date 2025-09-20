#pragma once
#include <atomic>
#include <cstdint>

namespace BedrockServer::Core::Memory {
    class ThreadHeap;
    struct PageFreeBlock;
}

namespace BedrockServer::Core::Memory
{
    struct MemoryPage
    {
        static constexpr uint64_t MAGIC = 0xBADDECAFDEADBEEF;
        uint64_t Magic;

        ThreadHeap* pOwnerHeap;
        MemoryPage* pLocalNext;
        
        // Pointer for CentralHeap's singly linked list of free pages.
        MemoryPage* pNextPageInCentralHeap;

        std::atomic<PageFreeBlock*> pFreeList;
        std::atomic<uint32_t> UsedBlocks;
        size_t SizeClassIndex;

        MemoryPage(uint32_t blockSize, size_t sizeClassIndex)
            : Magic(MAGIC),
              pOwnerHeap(nullptr),
              pLocalNext(nullptr),
              pNextPageInCentralHeap(nullptr),
              pFreeList(nullptr),
              UsedBlocks(0),
              SizeClassIndex(sizeClassIndex)
        {
        }

        void InitFreeList(uint32_t numBlocks, uint32_t blockSize);
    };

    struct PageFreeBlock
    {
        PageFreeBlock* pNext;
    };
}