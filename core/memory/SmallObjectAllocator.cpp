#include "SmallObjectAllocator.h"
#include "common/Assert.h"
#include "common/ServerConfig.h"
#include <cstdlib>
#include <new>

namespace
{
    // POOL CONFIGURATION
    
    // All allocation sizes are aligned to this value.
    constexpr std::size_t POOL_ALIGNMENT = 8;
    // How many blocks for each pool. e.g. 8-byte pool has 1024 blocks.
    constexpr std::size_t NUM_BLOCKS_PER_POOL = 1024;
}

namespace BedrockServer::Core::Memory
{
    // Calculate the number of pools at compile time.
    constexpr std::size_t NUM_POOLS = ServerConfig::MAX_SMALL_OBJECT_SIZE / POOL_ALIGNMENT;
    
    SmallObjectAllocator::SmallObjectAllocator()
    {
        // Use std::malloc to avoid calling our global `new` during initialization.
        void* pMemory = std::malloc(NUM_POOLS * sizeof(PoolAllocator));
        CHECK(pMemory != nullptr);
        
        pPools = static_cast<PoolAllocator*>(pMemory);

        // Construct PoolAllocator objects in-place using placement new.
        for (std::size_t i = 0; i < NUM_POOLS; ++i)
        {
            std::size_t payloadSize = (i + 1) * POOL_ALIGNMENT;
            new (&pPools[i]) PoolAllocator(payloadSize, NUM_BLOCKS_PER_POOL);
        }
    }

    SmallObjectAllocator::~SmallObjectAllocator()
    {
        // Manually call destructors for objects created with placement new.
        for (std::size_t i = 0; i < NUM_POOLS; ++i)
        {
            pPools[i].~PoolAllocator();
        }
        
        // Free the raw memory.
        std::free(pPools);
    }

    void* SmallObjectAllocator::Allocate(std::size_t size)
    {
        // It cannot allocate to use SmallObjectAllocator
        if (0 == size || size > ServerConfig::MAX_SMALL_OBJECT_SIZE)
        {
            return nullptr;
        }

        // Calculate which pool to use from size
        // ex) size 1~8  -> pPools[0] (8bytes)
        // ex) size 9~16 -> pPools[1] (16bytes)
        std::size_t index = ((size + POOL_ALIGNMENT - 1) / POOL_ALIGNMENT) - 1;
        CHECK(index < NUM_POOLS);

        return pPools[index].Allocate();
    }

    void SmallObjectAllocator::Deallocate(void* pPayload, std::size_t size)
    {
        if (nullptr == pPayload || 0 == size || size > ServerConfig::MAX_SMALL_OBJECT_SIZE)
        {
            return;
        }

        std::size_t index = ((size + POOL_ALIGNMENT - 1) / POOL_ALIGNMENT) - 1;
        CHECK(index < NUM_POOLS);
        pPools[index].Deallocate(pPayload);
    }
}