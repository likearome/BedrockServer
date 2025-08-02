#include"common/ThreadRegistry.h"

namespace BedrockServer::Core::Common
{
    ThreadRegistry& ThreadRegistry::GetInstance()
    {
        static ThreadRegistry instance;
        return instance;
    }

    uint32_t ThreadRegistry::GetNewThreadId()
    {
        return next_thread_id_.fetch_add(1);
    }
}