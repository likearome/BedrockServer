#include "common/ThreadId.h"
#include "common/ThreadRegistry.h"

namespace BedrockServer::Core::Common
{
    uint32_t GetThreadId()
    {
        // This is the thread-local cache for the ID
        static thread_local uint32_t G_ThreadId = ThreadRegistry::GetInstance().GetNewThreadId();
        return G_ThreadId;
    }
}