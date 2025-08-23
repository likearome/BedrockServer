#pragma once
#include <cstdint>

namespace BedrockServer::Core
{
    // Holds server-wide config constants
    struct ServerConfig
    {
        static constexpr uint32_t MAX_THREADS = 256;
        static constexpr std::size_t MAX_SMALL_OBJECT_SIZE = 256;
    };
}

// Enable it if you want to enable memory shrinking
//#define BEDROCK_ENABLE_MEMORY_SHRINK