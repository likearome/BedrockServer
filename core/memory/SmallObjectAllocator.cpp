#include"SmallObjectAllocator.h"
#include"common/Assert.h"

namespace
{
    // POOL CONFIGURATION
    
    // All allocation sizes are aligned to this value.
    constexpr std::size_t POOL_ALIGNMENT = 8;
    // If requested size is bigger than this, use general allocator.
    constexpr std::size_t MAX_SMALL_OBJECT_SIZE = 256;
    // How many blocks for each pool. e.g. 8-byte pool has 1024 blocks.
    constexpr std::size_t NUM_BLOCKS_PER_POOL = 1024;
}

namespace BedrockServer::Core::Memory
{
    SmallObjectAllocator::SmallObjectAllocator()
    {
        // up to MAX_SMALL_OBJECT_SIZE
        const std::size_t numPools = MAX_SMALL_OBJECT_SIZE / POOL_ALIGNMENT;
        pools_.reserve(numPools);

        for(std::size_t i = 1; i <= numPools; ++i)
        {
            // fill pools by i * POOL_ALIGNMENT;
            std::size_t payloadSize = i * POOL_ALIGNMENT;
            pools_.emplace_back(payloadSize, NUM_BLOCKS_PER_POOL);
        }
    }

    SmallObjectAllocator::~SmallObjectAllocator() = default;

    void* SmallObjectAllocator::Allocate(std::size_t size)
    {
        // It cannot allocate to use SmallObjectAllocator
        if(0 == size || size > MAX_SMALL_OBJECT_SIZE)
        {
            return nullptr;
        }

        // Calculate which pool to use from size
        // ex) size 1~8  -> pools_[0] (8bytes)
        // ex) size 9~16 -> pools_[1] (16bytes)
        std::size_t index = ((size + POOL_ALIGNMENT - 1) / POOL_ALIGNMENT) - 1; // ((size + 7) / 8) - 1
        CHECK(index < pools_.size());

        return pools_[index].Allocate();
    }

    void SmallObjectAllocator::Deallocate(void* pBlock, std::size_t size)
    {
        if(nullptr == pBlock  || 0 == size || size > MAX_SMALL_OBJECT_SIZE)
        {
            return ;
        }

        std::size_t index = ((size + POOL_ALIGNMENT - 1) / POOL_ALIGNMENT) - 1; // ((size + 7) / 8) - 1
        CHECK(index < pools_.size());

        pools_[index].Deallocate(pBlock);
    }
}