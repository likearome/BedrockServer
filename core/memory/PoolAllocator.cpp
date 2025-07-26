#include"memory/PoolAllocator.h"
#include"common/Assert.h"
#include<new>
#include<cstdint>

namespace
{
    constexpr uint32_t FENCE_PATTERN = 0xDEADBEEF;
}

namespace BedrockServer::Core::Memory
{
    PoolAllocator::PoolAllocator(std::size_t payloadSize, std::size_t blockCount)
        : payload_size_(payloadSize), block_count_(blockCount)
    {
        // the actual block size that user required needs space for the user payload + double-sided fences. 
        block_size_ = payload_size_ + (2 * sizeof(FENCE_PATTERN));

        // critical pre-conditions. These must be true in all builds.
        CHECK(payload_size_ >= sizeof(FreeNode));
        CHECK(block_count_ > 0);

        memory_chunk_ = new std::byte[block_size_ * block_count_];
        CHECK(memory_chunk_ != nullptr);

        // initialize the free list using placement new
        // constructs a FreeNode at the start of memory_chunk_
        free_list_head_ = new (memory_chunk_) FreeNode{nullptr};
        
        FreeNode* pCurrent = free_list_head_;
        for(std::size_t i = 1; i < block_count_; ++i)
        {
            // iterate blocks in memory_chunk
            // because type of memory_chunk is std::byte*, so type of pNextBlock is also std::byte*.
            std::byte* pNextBlock = memory_chunk_ + (i * block_size_);
            // pCurrent->pNext sets using placement new
            // the location is first area of pNextBlock.
            pCurrent->pNext = new (pNextBlock) FreeNode{nullptr};
            // move next
            pCurrent = pCurrent->pNext;
        }
    }

    PoolAllocator::~PoolAllocator()
    {
        delete[] memory_chunk_;
    }

    void* PoolAllocator::Allocate()
    {
        if(nullptr == free_list_head_)
        {
            return nullptr;
        }

        FreeNode* pBlock = static_cast<FreeNode*>(free_list_head_);
        free_list_head_ = pBlock->pNext;

        // install fence around the payload area.
        std::byte* pBlockBytes = reinterpret_cast<std::byte*>(pBlock);
        *reinterpret_cast<uint32_t*>(pBlockBytes) = FENCE_PATTERN;
        *reinterpret_cast<uint32_t*>(pBlockBytes + sizeof(FENCE_PATTERN) + payload_size_) = FENCE_PATTERN;

        return pBlockBytes + sizeof(FENCE_PATTERN);
    }

    void PoolAllocator::Deallocate(void* pPayload)
    {
        if(nullptr == pPayload)
        {
            return ;
        }

        std::byte* pPayloadBytes = static_cast<std::byte*>(pPayload);
        std::byte* pBlockBytes = pPayloadBytes - sizeof(FENCE_PATTERN);

        DCHECK(*reinterpret_cast<uint32_t*>(pBlockBytes) == FENCE_PATTERN);
        DCHECK(*reinterpret_cast<uint32_t*>(pBlockBytes + sizeof(FENCE_PATTERN) + payload_size_) == FENCE_PATTERN);

        // Construct a new FreeNode at pBlock, making it point to the old head.
        FreeNode* pNewHead = new (pBlockBytes) FreeNode{free_list_head_};
        free_list_head_ = pNewHead;
    }

    PoolAllocator::PoolAllocator(PoolAllocator&& other) noexcept
    : memory_chunk_(other.memory_chunk_),
      free_list_head_(other.free_list_head_),
      block_size_(other.block_size_),
      block_count_(other.block_count_),
      payload_size_(other.payload_size_)
    {
        // Null out the other object to prevent double-free.
        other.memory_chunk_ = nullptr;
        other.free_list_head_ = nullptr;
        other.block_size_ = 0;
        other.block_count_ = 0;
        other.payload_size_ = 0;
    }

    PoolAllocator& PoolAllocator::operator=(PoolAllocator&& other) noexcept
    {
        if (this != &other)
        {
            // Release our own resource first.
            delete[] memory_chunk_;

            // Steal resources from the other object.
            memory_chunk_ = other.memory_chunk_;
            free_list_head_ = other.free_list_head_;
            block_size_ = other.block_size_;
            block_count_ = other.block_count_;
            payload_size_ = other.payload_size_;

            // Null out the other object.
            other.memory_chunk_ = nullptr;
            other.free_list_head_ = nullptr;
            other.block_size_ = 0;
            other.block_count_ = 0;
            other.payload_size_ = 0;
        }
        return *this;
    }
}