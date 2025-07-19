#include"memory/PoolAllocator.h"
#include"common/Assert.h"

namespace BedrockServer::Core::Memory
{
    PoolAllocator::PoolAllocator(std::size_t blockSize, std::size_t blockCount)
        : block_size_(blockSize), block_count_(blockCount)
    {
        // critical pre-conditions. These must be true in all builds.
        CHECK(block_size_ >= sizeof(void*));
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
            // because type of memory_chunk is std::byte*, type of pNextBlock is also std::byte*.
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

        FreeNode* pBlock = free_list_head_;
        free_list_head_ = pBlock->pNext;

        return pBlock;
    }

    void PoolAllocator::Deallocate(void* pBlock)
    {
        if(nullptr == pBlock)
        {
            return ;
        }
        DCHECK
        (
            (static_cast<std::byte*>(pBlock) >= memory_chunk_) &&
            (static_cast<std::byte*>(pBlock) < memory_chunk_ + (block_size_ * block_count_))
        );

        // Construct a new FreeNode at pBlock, making it point to the old head.
        FreeNode* pNewHead = new (pBlock) FreeNode{free_list_head_};
        free_list_head_ = pNewHead;
    }
}