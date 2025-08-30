#pragma once

#include <cstddef>

namespace BedrockServer::Core::Memory
{
    // Handles memory allocations that are too large for the SmallObjectAllocator.
    // This class is a thin wrapper around the OS's memory allocation calls (e.g., mmap).
    class LargeObjectAllocator
    {
    public:
        LargeObjectAllocator() = default;
        ~LargeObjectAllocator() = default;

        // Non-copyable to prevent accidental copying of ownership.
        LargeObjectAllocator(const LargeObjectAllocator&) = delete;
        LargeObjectAllocator& operator=(const LargeObjectAllocator&) = delete;

        void* Allocate(std::size_t size);
        void Deallocate(void* pPayload, std::size_t size);
    };
}