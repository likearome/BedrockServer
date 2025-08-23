#include"memory/MemoryManager.h"
#include"common/ServerConfig.h"
#include"common/ThreadId.h"
#include<cstdlib>
#include<cstring>

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
            // Get the ID of the current thread.
            uint32_t threadId = BedrockServer::Core::Common::GetThreadId();
            // Use the thread-specific allocator from the array.
            pBlock = static_cast<std::byte*>(thread_allocators_[threadId].Allocate(totalSize));
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
    void* MemoryManager::Allocate(std::size_t userSize, std::align_val_t al)
    {
        if (userSize == 0) return nullptr;

        // For aligned requests, we bypass the small object allocator and go to the system.
        std::size_t alignment = static_cast<std::size_t>(al);
        std::size_t totalSize = userSize + ALLOC_HEADER_SIZE;
        
        void* pBlockRaw = nullptr;
        // posix_memalign is the standard way to get aligned memory on Linux/Debian.
        if (posix_memalign(&pBlockRaw, alignment, totalSize) != 0)
        {
            return nullptr; // Allocation failed.
        }
        std::byte* pBlock = static_cast<std::byte*>(pBlockRaw);

        if (pBlock == nullptr) return nullptr;

        // Write the original user size into the header.
        *reinterpret_cast<std::size_t*>(pBlock) = userSize;

        // Return the address right after the header to the user.
        return pBlock + ALLOC_HEADER_SIZE;
    }

    void MemoryManager::Deallocate(void* pPayload)
    {
        if (pPayload == nullptr) return;

        // Get the original block address by moving the pointer back.
        std::byte* pBlock = static_cast<std::byte*>(pPayload) - ALLOC_HEADER_SIZE;

        // Read the original user size from the header.
        std::size_t userSize; 
        std::memcpy(&userSize, pBlock, sizeof(userSize));

        if (userSize + ALLOC_HEADER_SIZE <= ServerConfig::MAX_SMALL_OBJECT_SIZE)
        {
            // Get the ID of the current thread.
            uint32_t threadId = BedrockServer::Core::Common::GetThreadId();
            // Route the deallocation back to the small object allocator.
            thread_allocators_[threadId].Deallocate(pBlock, userSize + ALLOC_HEADER_SIZE);
        }
        else
        {
            // Route the deallocation back to the system's free().
            std::free(pBlock);
        }
    }
}