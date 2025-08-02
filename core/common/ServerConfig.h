#pragma once
#include <cstdint>

namespace BedrockServer::Core
{
    // Holds server-wide config constants
    struct ServerConfig
    {
        static constexpr uint32_t MAX_THREADS = 256;
    };
}