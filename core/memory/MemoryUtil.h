#pragma once
#include "common/ServerConfig.h"
#include "memory/MemoryPage.h"
#include <bit> // For std::has_single_bit in C++20

namespace BedrockServer::Core::Memory
{
    // A common utility function to get the page start address from any pointer within that page.
    // This is a crucial piece of logic for the memory manager.
    // It's marked inline constexpr for maximum optimization, as it will be called frequently.
    inline constexpr MemoryPage* GetPageFromPointer(void* p)
    {
        // Ensure that PAGE_SIZE is a power of two at compile time for the bitmask to work.
        static_assert(std::has_single_bit(ServerConfig::PAGE_SIZE), "PAGE_SIZE must be a power of two.");

        return reinterpret_cast<MemoryPage*>(
            reinterpret_cast<uintptr_t>(p) & ~(static_cast<uintptr_t>(ServerConfig::PAGE_SIZE) - 1)
        );
    }
}