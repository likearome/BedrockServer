#pragma once
#include <cstddef> // For std::size_t
#include <new>     // For std::align_val_t

namespace BedrockServer::Core::Memory
{
    // Forward declare ThreadHeap to avoid circular dependencies.
    class ThreadHeap;

    /// @class MemoryManager
    /// @brief A singleton that serves as the entry point for all memory allocations.
    /// It delegates small allocations to a thread-local ThreadHeap and handles
    /// large allocations by directly calling the OS (mmap).
    class MemoryManager
    {
        // Note: All allocations from this manager will now be prefixed with a header
        // to distinguish them from other memory sources.
    public:
        /// @brief Gets the singleton instance of the MemoryManager.
        static MemoryManager& GetInstance();
        
        // This is a singleton, so all copy/move operations are deleted.
        MemoryManager(const MemoryManager&) = delete;
        MemoryManager& operator=(const MemoryManager&) = delete;
        MemoryManager(MemoryManager&&) = delete;
        MemoryManager& operator=(MemoryManager&&) = delete;

        /// @brief Public methods to control the memory system's lifecycle.
        static void Initialize();
        static void Shutdown();

        /// @brief Allocates a block of memory of the given size.
        void* Allocate(std::size_t size);

        /// @brief Allocates a block of memory with the given size and alignment.
        void* Allocate(std::size_t size, std::align_val_t al);

        /// @brief Deallocates a previously allocated block of memory.
        void Deallocate(void* p);

    private:
        // Private constructor and destructor for the singleton pattern.
        MemoryManager() = default;
        ~MemoryManager() = default;

        // The MemoryManager is now mostly stateless.
        // The array of SmallObjectAllocators has been removed.
        // Thread-local heaps are managed via a thread_local pointer in ThreadHeap.cpp.
    };
}