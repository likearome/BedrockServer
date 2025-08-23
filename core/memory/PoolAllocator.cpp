#include "memory/PoolAllocator.h"
#include "common/Assert.h"
#include <new>
#include <cstdint>
#include <sys/mman.h>

#ifdef BEDROCK_ENABLE_MEMORY_FENCING
namespace
{
    // A pattern written before and after the user's payload to detect buffer overruns.
    constexpr uint32_t FENCE_PATTERN = 0xDEADBEEF;
    constexpr std::size_t FENCE_SIZE = sizeof(FENCE_PATTERN);
}
#endif

namespace BedrockServer::Core::Memory
{

#ifdef BEDROCK_ENABLE_MEMORY_SHRINK

    // =================================================================
    // VERSION WITH SHRINK ENABLED
    // =================================================================
    
    void PoolAllocator::AddNewChunk()
    {
        // Calculate the actual size needed for one block, including our custom header.
        const std::size_t blockSizeWithHeader = sizeof(BlockHeader) + BlockSize;
        // Calculate the total size for the entire chunk, including the chunk's own header.
        const std::size_t totalChunkSize = sizeof(MemoryChunk) + (blockSizeWithHeader * BlockCount);
        
        void* pMem = mmap(nullptr, totalChunkSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        CHECK(pMem != MAP_FAILED);

        // Construct the chunk header at the beginning of the allocated memory.
        MemoryChunk* newChunk = new (pMem) MemoryChunk();
        newChunk->pNext = ChunkListHead;
        ChunkListHead = newChunk;

        // Carve up the rest of the chunk into blocks and build the chunk's local free list.
        std::byte* pBlockStart = reinterpret_cast<std::byte*>(newChunk) + sizeof(MemoryChunk);
        for (std::size_t i = 0; i < BlockCount; ++i) {
            std::byte* pBlock = pBlockStart + (i * blockSizeWithHeader);
            
            // Write the header into the block, pointing back to the owner chunk.
            BlockHeader* pHeader = reinterpret_cast<BlockHeader*>(pBlock);
            pHeader->pOwnerChunk = newChunk;
            
            // The free node starts right after our block header.
            FreeNode* pFreeNode = reinterpret_cast<FreeNode*>(pBlock + sizeof(BlockHeader));
            
            // Add the new free node to this chunk's local free list.
            pFreeNode->pNext = newChunk->pFreeList;
            newChunk->pFreeList = pFreeNode;
        }
    }

    PoolAllocator::PoolAllocator(std::size_t payloadSize, std::size_t blockCount)
        : PayloadSize(payloadSize), BlockCount(blockCount)
    {
    #ifdef BEDROCK_ENABLE_MEMORY_FENCING
        BlockSize = PayloadSize + (2 * FENCE_SIZE);
    #else
        BlockSize = PayloadSize;
    #endif
        // The actual payload area must be large enough for a FreeNode.
        CHECK(PayloadSize >= sizeof(FreeNode));
        CHECK(BlockCount > 0);
        AddNewChunk(); // Allocate the first chunk.
    }

    PoolAllocator::~PoolAllocator()
    {
        MemoryChunk* pCurrent = ChunkListHead;
        while (pCurrent != nullptr) {
            MemoryChunk* pNext = pCurrent->pNext;
            const std::size_t blockSizeWithHeader = sizeof(BlockHeader) + BlockSize;
            const std::size_t totalChunkSize = sizeof(MemoryChunk) + (blockSizeWithHeader * BlockCount);
            munmap(pCurrent, totalChunkSize);
            pCurrent = pNext;
        }
    }

    void* PoolAllocator::Allocate()
    {
        // 1. Find a chunk that has free blocks.
        MemoryChunk* pChunk = ChunkListHead;
        while (pChunk != nullptr && pChunk->pFreeList == nullptr) {
            pChunk = pChunk->pNext;
        }

        // 2. If no chunk has free blocks, create a new one.
        if (pChunk == nullptr) {
            AddNewChunk();
            pChunk = ChunkListHead;
            if (pChunk == nullptr || pChunk->pFreeList == nullptr) return nullptr; // Out of memory
        }

        // 3. Allocate from the found chunk's free list.
        FreeNode* pFreeNode = pChunk->pFreeList;
        pChunk->pFreeList = pFreeNode->pNext;
        pChunk->AllocCount++;
        
        void* pPayload = reinterpret_cast<void*>(pFreeNode);

    #ifdef BEDROCK_ENABLE_MEMORY_FENCING
        // Fencing is written around the user payload, which starts where the FreeNode was.
        std::byte* pPayloadBytes = static_cast<std::byte*>(pPayload);
        *reinterpret_cast<uint32_t*>(pPayloadBytes) = FENCE_PATTERN; // Front fence
        *reinterpret_cast<uint32_t*>(pPayloadBytes + FENCE_SIZE + PayloadSize) = FENCE_PATTERN; // Back fence
        return pPayloadBytes + FENCE_SIZE; // Return pointer after the front fence
    #else
        return pPayload;
    #endif
    }

    void PoolAllocator::Deallocate(void* pPayload)
    {
        if (pPayload == nullptr) return;
        
        std::byte* pRealPayload = static_cast<std::byte*>(pPayload);
    #ifdef BEDROCK_ENABLE_MEMORY_FENCING
        // The user pointer is after the front fence, so step back to get the real payload start.
        pRealPayload = pRealPayload - FENCE_SIZE;
        std::byte* pBlockStart = pRealPayload - sizeof(BlockHeader);
        
        // Check fences for corruption.
        DCHECK(*reinterpret_cast<uint32_t*>(pRealPayload) == FENCE_PATTERN);
        DCHECK(*reinterpret_cast<uint32_t*>(pRealPayload + FENCE_SIZE + PayloadSize) == FENCE_PATTERN);
    #endif

        // The BlockHeader is located right before the real payload area.
        std::byte* pBlockBytes = pRealPayload - sizeof(BlockHeader);
        BlockHeader* pHeader = reinterpret_cast<BlockHeader*>(pBlockBytes);
        MemoryChunk* pOwnerChunk = pHeader->pOwnerChunk;

        // Place the FreeNode at the real payload's address and add it to the owner chunk's free list.
        FreeNode* pNewHead = new (pRealPayload) FreeNode{pOwnerChunk->pFreeList};
        pOwnerChunk->pFreeList = pNewHead;
        pOwnerChunk->AllocCount--;
    }

    void PoolAllocator::Shrink()
    {
        MemoryChunk* pCurrent = ChunkListHead;
        MemoryChunk* pPrev = nullptr;
        while (pCurrent != nullptr) {
            MemoryChunk* pNext = pCurrent->pNext;
            // We can only free a chunk if it's completely empty AND it's not the last one.
            if (pCurrent->AllocCount == 0 && ChunkListHead != pCurrent) {
                if (pPrev != nullptr) {
                    pPrev->pNext = pNext;
                } else { // This means we are removing the head
                    ChunkListHead = pNext;
                }
                const std::size_t blockSizeWithHeader = sizeof(BlockHeader) + BlockSize;
                const std::size_t totalChunkSize = sizeof(MemoryChunk) + (blockSizeWithHeader * BlockCount);
                munmap(pCurrent, totalChunkSize);
            } else {
                pPrev = pCurrent;
            }
            pCurrent = pNext;
        }
    }

    PoolAllocator::PoolAllocator(PoolAllocator&& other) noexcept
        : ChunkListHead(other.ChunkListHead),
          BlockSize(other.BlockSize),
          BlockCount(other.BlockCount),
          PayloadSize(other.PayloadSize)
    {
        // Take ownership, leave other in a valid but empty state.
        other.ChunkListHead = nullptr;
    }

    PoolAllocator& PoolAllocator::operator=(PoolAllocator&& other) noexcept
    {
        if (this != &other) {
            // Free own resources by calling the destructor.
            this->~PoolAllocator();

            // Steal other's resources.
            ChunkListHead = other.ChunkListHead;
            BlockSize = other.BlockSize;
            BlockCount = other.BlockCount;
            PayloadSize = other.PayloadSize;
            
            // Leave other in a valid, empty state.
            other.ChunkListHead = nullptr;
        }
        return *this;
    }

#else

    // ===============================================================
    // VERSION WITH SHRINK DISABLED
    // ===============================================================
    
    void PoolAllocator::AddNewChunk()
    {
        const std::size_t totalSize = sizeof(MemoryChunk) + (BlockSize * BlockCount);
        void* pMem = mmap(nullptr, totalSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        CHECK(pMem != MAP_FAILED);

        MemoryChunk* newChunk = new (pMem) MemoryChunk();
        newChunk->pNext = ChunkListHead;
        ChunkListHead = newChunk;

        std::byte* pBlockStart = reinterpret_cast<std::byte*>(newChunk) + sizeof(MemoryChunk);
        for (std::size_t i = 0; i < BlockCount; ++i) {
            std::byte* pBlock = pBlockStart + (i * BlockSize);
            // Add the new free block to the allocator's global free list.
            FreeNode* pNewHead = new (pBlock) FreeNode{FreeListHead};
            FreeListHead = pNewHead;
        }
    }

    PoolAllocator::PoolAllocator(std::size_t payloadSize, std::size_t blockCount)
        : PayloadSize(payloadSize), BlockCount(blockCount)
    {
    #ifdef BEDROCK_ENABLE_MEMORY_FENCING
        BlockSize = PayloadSize + (2 * FENCE_SIZE);
    #else
        BlockSize = PayloadSize;
    #endif
        CHECK(PayloadSize >= sizeof(FreeNode));
        CHECK(BlockCount > 0);
        AddNewChunk();
    }

    PoolAllocator::~PoolAllocator()
    {
        MemoryChunk* pCurrent = ChunkListHead;
        while (pCurrent != nullptr) {
            MemoryChunk* pNext = pCurrent->pNext;
            munmap(pCurrent, sizeof(MemoryChunk) + (BlockSize * BlockCount));
            pCurrent = pNext;
        }
    }

    void* PoolAllocator::Allocate()
    {
        if (nullptr == FreeListHead) {
            AddNewChunk();
        }
        if (nullptr == FreeListHead) {
            return nullptr;
        }

        FreeNode* pBlock = FreeListHead;
        FreeListHead = pBlock->pNext;

    #ifdef BEDROCK_ENABLE_MEMORY_FENCING
        std::byte* pBlockBytes = reinterpret_cast<std::byte*>(pBlock);
        *reinterpret_cast<uint32_t*>(pBlockBytes) = FENCE_PATTERN;
        *reinterpret_cast<uint32_t*>(pBlockBytes + FENCE_SIZE + PayloadSize) = FENCE_PATTERN;
        return pBlockBytes + FENCE_SIZE;
    #else
        return pBlock;
    #endif
    }

    void PoolAllocator::Deallocate(void* pPayload)
    {
        if (nullptr == pPayload) return;

        std::byte* pBlockToReturn = static_cast<std::byte*>(pPayload);
    #ifdef BEDROCK_ENABLE_MEMORY_FENCING
        pBlockToReturn = pBlockToReturn - FENCE_SIZE;
        DCHECK(*reinterpret_cast<uint32_t*>(pBlockToReturn) == FENCE_PATTERN);
        DCHECK(*reinterpret_cast<uint32_t*>(pBlockToReturn + FENCE_SIZE + PayloadSize) == FENCE_PATTERN);
    #endif
        
        FreeNode* pNewHead = new (pBlockToReturn) FreeNode{FreeListHead};
        FreeListHead = pNewHead;
    }

    PoolAllocator::PoolAllocator(PoolAllocator&& other) noexcept
        : ChunkListHead(other.ChunkListHead),
          FreeListHead(other.FreeListHead),
          BlockSize(other.BlockSize),
          BlockCount(other.BlockCount),
          PayloadSize(other.PayloadSize)
    {
        other.ChunkListHead = nullptr;
        other.FreeListHead = nullptr;
    }

    PoolAllocator& PoolAllocator::operator=(PoolAllocator&& other) noexcept
    {
        if (this != &other)
        {
            this->~PoolAllocator(); // Free own resources

            ChunkListHead = other.ChunkListHead;
            FreeListHead = other.FreeListHead;
            BlockSize = other.BlockSize;
            BlockCount = other.BlockCount;
            PayloadSize = other.PayloadSize;

            other.ChunkListHead = nullptr;
            other.FreeListHead = nullptr;
        }
        return *this;
    }

#endif // BEDROCK_ENABLE_MEMORY_SHRINK
}