#include"SmallObjectAllocator.h"
#include"common/Assert.h"
#include"common/ServerConfig.h"
#include<cstdlib>
#include<new>
#include<sys/mman.h>

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
    SmallObjectAllocator::SmallObjectAllocator()
    {
        num_pools_ = ServerConfig::MAX_SMALL_OBJECT_SIZE / POOL_ALIGNMENT;

        void* pMemory = mmap(
            nullptr,                               // Let the kernel choose the address
            num_pools_ * sizeof(PoolAllocator),    // Size to allocate
            PROT_READ | PROT_WRITE,                // We want to read and write to this memory
            MAP_PRIVATE | MAP_ANONYMOUS,           // Not backed by a file, not shared
            -1,                                    // File descriptor (none for anonymous)
            0                                      // Offset (none for anonymous)
        );
        CHECK(pMemory != nullptr);
        
        pools_ = static_cast<PoolAllocator*>(pMemory);

        // Construct PoolAllocator objects in-place using placement new.
        for (std::size_t i = 0; i < num_pools_; ++i)
        {
            std::size_t payloadSize = (i + 1) * POOL_ALIGNMENT;
            new (&pools_[i]) PoolAllocator(payloadSize, NUM_BLOCKS_PER_POOL);
        }
    }

    SmallObjectAllocator::~SmallObjectAllocator()
    {
        // Manually call destructors for objects created with placement new.
        for (std::size_t i = 0; i < num_pools_; ++i)
        {
            pools_[i].~PoolAllocator();
        }
        
        // Free the raw memory.
        munmap(pools_, num_pools_ * sizeof(PoolAllocator));
    }

    void* SmallObjectAllocator::Allocate(std::size_t size)
    {
        // It cannot allocate to use SmallObjectAllocator
        if(0 == size || size > ServerConfig::MAX_SMALL_OBJECT_SIZE)
        {
            return nullptr;
        }

        // Calculate which pool to use from size
        // ex) size 1~8  -> pools_[0] (8bytes)
        // ex) size 9~16 -> pools_[1] (16bytes)
        std::size_t index = ((size + POOL_ALIGNMENT - 1) / POOL_ALIGNMENT) - 1;
        CHECK(index < num_pools_);

        return pools_[index].Allocate();
    }

    void SmallObjectAllocator::Deallocate(void* pPayload, std::size_t size)
    {
        if(nullptr == pPayload  || 0 == size || size > ServerConfig::MAX_SMALL_OBJECT_SIZE)
        {
            return ;
        }

        std::size_t index = ((size + POOL_ALIGNMENT - 1) / POOL_ALIGNMENT) - 1;
        CHECK(index < num_pools_);
        pools_[index].Deallocate(pPayload);
    }
}