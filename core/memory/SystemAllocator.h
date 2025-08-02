#pragma once
#include <cstdlib>

namespace BedrockServer::Core::Memory
{
    template<typename T>
    class SystemAllocator
    {
    public:
        using value_type = T;

        SystemAllocator() = default;
        template<typename U>
        constexpr SystemAllocator(const SystemAllocator<U>&) noexcept {}

        T* allocate(std::size_t n)
        {
            return static_cast<T*>(std::malloc(n * sizeof(T)));
        }

        void deallocate(T* p, std::size_t n) noexcept
        {
            std::free(p);
        }
    };
}