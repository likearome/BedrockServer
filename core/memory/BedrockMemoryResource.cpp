#include "BedrockMemoryResource.h"
#include "MemoryManager.h"

namespace BedrockServer::Core::Memory
{
    void* BedrockMemoryResource::do_allocate(size_t bytes, size_t alignment)
    {
        // Check if the requested alignment is greater than the default.
        // __STDCPP_DEFAULT_NEW_ALIGNMENT__ is a standard macro for the default alignment.
        if (alignment > __STDCPP_DEFAULT_NEW_ALIGNMENT__)
        {
            // If so, call our aligned allocation function.
            return MemoryManager::GetInstance().Allocate(bytes, std::align_val_t{alignment});
        }
        else
        {
            // Otherwise, use the standard allocation function.
            return MemoryManager::GetInstance().Allocate(bytes);
        }
    }

    void BedrockMemoryResource::do_deallocate(void* p, size_t bytes, size_t alignment)
    {
        // Our MemoryManager::Deallocate is smart enough to figure out the details
        // by reading the header from the memory block.
        // So, we don't need the 'bytes' and 'alignment' parameters here.
        MemoryManager::GetInstance().Deallocate(p);
    }

    bool BedrockMemoryResource::do_is_equal(const std::pmr::memory_resource& other) const noexcept
    {
        // Since our underlying MemoryManager is a singleton, any two instances of
        // BedrockMemoryResource are effectively equal because they use the same single source of memory.
        // We can check if 'other' is also of our type using dynamic_cast.
        return dynamic_cast<const BedrockMemoryResource*>(&other) != nullptr;
    }
}