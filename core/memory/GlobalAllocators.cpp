#include "memory/MemoryManager.h"
#include "memory/MemoryTracker.h"
#include "common/ThreadId.h"
#include <new>
#include <cstdlib>

// A thread-local guard to prevent infinite recursion.
thread_local bool G_IsTracking = true;

// --- new ---
void* operator new(std::size_t size)
{
    if (!G_IsTracking) { return std::malloc(size); }

    G_IsTracking = false;
    void* p = BedrockServer::Core::Memory::MemoryManager::GetInstance().Allocate(size);
#ifndef NDEBUG
    BedrockServer::Core::Memory::MemoryTracker::GetInstance().Track(
        BedrockServer::Core::Common::GetThreadId(), p, size, "Unknown", 0);
#endif
    G_IsTracking = true;
    return p;
}

// --- delete ---
void operator delete(void* p) noexcept
{
    if (p == nullptr) return;
    if (!G_IsTracking) { std::free(p); return; }

    G_IsTracking = false;
// #ifndef NDEBUG
//     BedrockServer::Core::Memory::MemoryTracker::GetInstance().Untrack(
//         BedrockServer::Core::Common::GetThreadId(), p);
// #endif
    BedrockServer::Core::Memory::MemoryManager::GetInstance().Deallocate(p);
    G_IsTracking = true;
}

// --- array new/delete ---
void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete[](void* p) noexcept { ::operator delete(p); }

// =================================================================
// C++17 Aligned Allocation Overloads
// =================================================================

void* operator new(std::size_t size, std::align_val_t al)
{
    // For non-Windows (POSIX compliant systems like Debian), use posix_memalign.
    if (!G_IsTracking) {
        void* p = nullptr;
        // posix_memalign is the standard way to get aligned memory on Linux.
        if (posix_memalign(&p, static_cast<size_t>(al), size) != 0) {
            p = nullptr; // Failed to allocate
        }
        return p;
    }

    G_IsTracking = false;
    // Delegate aligned allocations to the MemoryManager.
    void* p = BedrockServer::Core::Memory::MemoryManager::GetInstance().Allocate(size, al);
#ifndef NDEBUG
    BedrockServer::Core::Memory::MemoryTracker::GetInstance().Track(
        BedrockServer::Core::Common::GetThreadId(), p, size, "Unknown (Aligned)", 0);
#endif
    G_IsTracking = true;
    return p;
}

void operator delete(void* p, std::align_val_t al) noexcept
{
    if (p == nullptr) return;
    
    // For pointers from posix_memalign, a standard free() is the correct way to deallocate.
    if (!G_IsTracking) {
        std::free(p);
        return;
    }

    G_IsTracking = false;
    // Delegate aligned deallocations to the MemoryManager.
    BedrockServer::Core::Memory::MemoryManager::GetInstance().Deallocate(p);
    G_IsTracking = true;
}

void* operator new[](std::size_t size, std::align_val_t al) 
{ 
    return ::operator new(size, al); 
}

void operator delete[](void* p, std::align_val_t al) noexcept 
{ 
    ::operator delete(p, al); 
}

#ifndef NDEBUG
// --- placement new/delete for tracking ---
void* operator new(std::size_t size, const char* file, int line)
{
    if (!G_IsTracking) { return std::malloc(size); }

    G_IsTracking = false;
    void* p = BedrockServer::Core::Memory::MemoryManager::GetInstance().Allocate(size);
    BedrockServer::Core::Memory::MemoryTracker::GetInstance().Track(
        BedrockServer::Core::Common::GetThreadId(), p, size, file, line);
    G_IsTracking = true;
    return p;
}

void* operator new[](std::size_t size, const char* file, int line) { return ::operator new(size, file, line); }
void operator delete(void* p, const char*, int) noexcept { ::operator delete(p); }
void operator delete[](void* p, const char*, int) noexcept { ::operator delete[](p); }
#endif