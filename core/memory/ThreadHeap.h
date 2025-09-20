#pragma once
#include "MemoryPage.h"
#include "common/ServerConfig.h"
#include "common/ConcurrentQueue.h"
#include <array>

namespace BedrockServer::Core::Memory
{
    // A thread-local heap that manages pages of memory for a specific thread.
    // This is the core of the mimalloc-style design.
    class ThreadHeap
    {
    public:
        ThreadHeap();
        ~ThreadHeap();

        void* Allocate(std::size_t size);
        void Deallocate(void* pPayload);
        void ProcessDeferredFrees();
    private:
        void FreeBlockInternal(void* pPayload);
        void ReturnPageIfEmpty(MemoryPage* pPage);

    private:
        // Calculate the number of size classes at compile time.
        static constexpr size_t NumSizeClasses = ServerConfig::MAX_SMALL_OBJECT_SIZE / ServerConfig::POOL_ALIGNMENT;

        // Caches pages for various small object size classes.
        // The index corresponds to the size class.
        std::array<MemoryPage*, NumSizeClasses> Pages; 

        // A queue for deferred frees from other threads.
        Common::ConcurrentQueue<void*> DeferredFreeQueue; 
    };

    // Forward declaration of the helper function defined in ThreadHeap.cpp
    ThreadHeap* GetCurrentThreadHeap();
}