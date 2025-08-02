#pragma once
#include <cstdint>

namespace BedrockServer::Core::Common
{
    // Returns the unique ID for the calling thread.
    uint32_t GetThreadId();
}