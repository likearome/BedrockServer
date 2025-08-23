#pragma once

#include "MemoryManager.h"
#include <memory_resource>

namespace BedrockServer::Core::Memory
{
    // This class adapts our MemoryManager to the standard C++17 memory_resource interface.
    class BedrockMemoryResource : public std::pmr::memory_resource
    {
    public:
        // This is a virtual destructor, required for polymorphic base classes.
        ~BedrockMemoryResource() override = default;

    private:
        // When this resource is used to allocate memory, it calls our MemoryManager.
        void* do_allocate(size_t bytes, size_t alignment) override;

        // When this resource is used to deallocate memory, it calls our MemoryManager.
        void do_deallocate(void* p, size_t bytes, size_t alignment) override;

        // Comparison for memory_resource equality.
        bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override;
    };
}