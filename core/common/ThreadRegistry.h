#pragma once
#include<atomic>
#include<cstdint>

namespace BedrockServer::Core::Common
{
    /// @class ThreadRegistry
    /// @brief A singleton to issue unique, sequential IDs to threads.
    class ThreadRegistry
    {
    public:
        static ThreadRegistry& GetInstance();
        uint32_t GetNewThreadId();
    private:
        ThreadRegistry() = default;
        ~ThreadRegistry() = default;

        // Non-copyable, non-movable singleton
        ThreadRegistry(const ThreadRegistry&) = delete;
        ThreadRegistry& operator=(const ThreadRegistry&) = delete;
        ThreadRegistry(ThreadRegistry &&) = delete;
        ThreadRegistry& operator=(ThreadRegistry&&) = delete;

        std::atomic<uint32_t> next_thread_id_ = 0;
    };
}