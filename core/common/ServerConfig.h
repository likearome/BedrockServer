#pragma once
#include <cstdint>

namespace BedrockServer::Core
{
    // Holds server-wide config constants
    struct ServerConfig
    {
        static constexpr uint32_t MAX_THREADS = 256;

        //For game server, it is critical to optimize frequent allocations for packets up to 256 bytes.
        static constexpr std::size_t MAX_SMALL_OBJECT_SIZE = 256;
    
        // All allocation sizes are aligned to this value.
        static constexpr std::size_t POOL_ALIGNMENT = 8;

        // This should match CentralHeap's allocation size.
        static constexpr std::size_t PAGE_SIZE = 16 * 1024;
    };
}

// Enable it if you want to enable memory shrinking
//#define BEDROCK_ENABLE_MEMORY_SHRINK