#include"LargeObjectAllocator.h"
#include"common/Assert.h"
#include<sys/mman.h>
#include<cstring>

namespace
{
    // A header is placed at the beginning of each allocation to store the original size.
    constexpr std::size_t ALLOC_HEADER_SIZE = sizeof(std::size_t);
}

namespace BedrockServer::Core::Memory
{
    void* LargeObjectAllocator::Allocate(std::size_t userSize)
    {
        if(userSize == 0) return nullptr;

        // Request memory directly from the OS.
        void *pBlock = mmap(nullptr, userSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

        // Just allocate and return the raw block.
        return (pBlock == MAP_FAILED) ? nullptr : pBlock;
    }

    void LargeObjectAllocator::Deallocate(void* pPayload, std::size_t size)
    {
        if(pPayload == nullptr) return;

        // Just unmap the raw block with the given size.
        munmap(pPayload, size);
    }
}