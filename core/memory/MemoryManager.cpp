#include"memory/MemoryManager.h"
#include"common/ServerConfig.h"
#include<cstdlib>

namespace BedrockServer::Core::Memory
{
    namespace 
    {
        constexpr std::size_t ALLOC_HEADER_SIZE = sizeof(std::size_t);
    }

    MemoryManager& MemoryManager::GetInstance()
    {
        static MemoryManager instance;
        return instance;
    }

    void* MemoryManager::Allocate(std::size_t userSize)
    {
        if (userSize == 0) return nullptr;

        // Calculate the total size needed, including our header.
        std::size_t totalSize = userSize + ALLOC_HEADER_SIZE;
        std::byte* pBlock = nullptr;

        if (totalSize <= ServerConfig::MAX_SMALL_OBJECT_SIZE)
        {
            // The total block size fits in one of our small object pools.
            pBlock = static_cast<std::byte*>(small_obj_alloc_.Allocate(totalSize));
        }
        else
        {
            // Fallback to the system allocator for large allocations.
            pBlock = static_cast<std::byte*>(std::malloc(totalSize));
        }

        if (pBlock == nullptr) return nullptr;

        *reinterpret_cast<std::size_t*>(pBlock) = userSize;
        
        // return after header
        return pBlock + ALLOC_HEADER_SIZE;
    }

    void MemoryManager::Deallocate(void* pPayload)
    {
        if (pPayload == nullptr) return;

        // Get the original block address by moving the pointer back.
        std::byte* pBlock = static_cast<std::byte*>(pPayload) - ALLOC_HEADER_SIZE;

        // Read the original user size from the header.
        std::size_t userSize = *reinterpret_cast<std::size_t*>(pBlock);

        if (userSize + ALLOC_HEADER_SIZE <= ServerConfig::MAX_SMALL_OBJECT_SIZE)
        {
            // Route the deallocation back to the small object allocator.
            small_obj_alloc_.Deallocate(pPayload, userSize);
        }
        else
        {
            // Route the deallocation back to the system's free().
            std::free(pBlock);
        }
    }
}