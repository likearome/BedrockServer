#include "memory/SmallObjectAllocator.h"
#include "common/Assert.h"
#include <iostream>
#include <cstdint> // For uint32_t

// This pattern must be the same as the one in PoolAllocator.cpp
constexpr uint32_t FENCE_PATTERN = 0xDEADBEEF;

int main()
{
    std::cout << "--- Graceful Memory Fencing Test ---" << std::endl;

#ifdef BEDROCK_ENABLE_MEMORY_FENCING
    std::cout << "[STATUS] Memory Fencing is ENABLED." << std::endl;
    std::cout << "Verifying fence patterns..." << std::endl;

    BedrockServer::Core::Memory::SmallObjectAllocator soa;
    
    const std::size_t payloadSize = 8;
    void* pPayload = soa.Allocate(payloadSize);
    CHECK(pPayload != nullptr);

    // Get the memory addresses of the fences
    // The prefix fence is located right before our payload pointer.
    uint32_t* pPrefixFence = reinterpret_cast<uint32_t*>(static_cast<std::byte*>(pPayload) - sizeof(FENCE_PATTERN));
    
    // The suffix fence is located right after our payload.
    uint32_t* pSuffixFence = reinterpret_cast<uint32_t*>(static_cast<std::byte*>(pPayload) + payloadSize);

    // Check if the allocator correctly placed the fences.
    if (*pPrefixFence == FENCE_PATTERN && *pSuffixFence == FENCE_PATTERN)
    {
        std::cout << "[RESULT] SUCCESS: Both fences are correctly placed." << std::endl;
    }
    else
    {
        std::cout << "[RESULT] FAILURE: Fences are missing or incorrect." << std::endl;
    }

    soa.Deallocate(pPayload, payloadSize);

#else
    std::cout << "[STATUS] Memory Fencing is DISABLED." << std::endl;
    std::cout << "[RESULT] Test skipped." << std::endl;
#endif

    return 0;
}