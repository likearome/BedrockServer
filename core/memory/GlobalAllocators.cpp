#include "memory/MemoryManager.h"
#include "memory/MemoryTracker.h"
#include "common/ThreadId.h"
#include <new>
#include <cstdlib>

// A thread-local guard to prevent infinite recursion.
thread_local bool G_IsTracking = true;

// --- Helper Macros to reduce code duplication ---
#define ALLOC_BODY(size, file, line) \
    if (!G_IsTracking) { return std::malloc(size); } \
    G_IsTracking = false; \
    void* p = BedrockServer::Core::Memory::MemoryManager::GetInstance().Allocate(size); \
    BedrockServer::Core::Memory::MemoryTracker::GetInstance().Track( \
        BedrockServer::Core::Common::GetThreadId(), p, size, file, line); \
    G_IsTracking = true; \
    return p;

#define DEALLOC_BODY(p) \
    if (p == nullptr) return; \
    if (!G_IsTracking) { std::free(p); return; } \
    G_IsTracking = false; \
    BedrockServer::Core::Memory::MemoryTracker::GetInstance().Untrack( \
        BedrockServer::Core::Common::GetThreadId(), p); \
    BedrockServer::Core::Memory::MemoryManager::GetInstance().Deallocate(p); \
    G_IsTracking = true;


// --- Standard new/delete (Always available) ---

void* operator new(std::size_t size)
{
    void* p = BedrockServer::Core::Memory::MemoryManager::GetInstance().Allocate(size);
#ifdef _DEBUG
    if (G_IsTracking)
    {
        G_IsTracking = false;
        BedrockServer::Core::Memory::MemoryTracker::GetInstance().Track(
            BedrockServer::Core::Common::GetThreadId(), p, size, "Unknown", 0);
        G_IsTracking = true;
    }
#endif
    return p;
}

void operator delete(void* p) noexcept
{
    if (p == nullptr) return;
#ifdef _DEBUG
    if (G_IsTracking)
    {
        G_IsTracking = false;
        BedrockServer::Core::Memory::MemoryTracker::GetInstance().Untrack(
            BedrockServer::Core::Common::GetThreadId(), p);
        G_IsTracking = true;
    }
#endif
    BedrockServer::Core::Memory::MemoryManager::GetInstance().Deallocate(p);
}

void* operator new[](std::size_t size)
{
    return ::operator new(size);
}

void operator delete[](void* p) noexcept
{
    ::operator delete(p);
}


// --- Placement new/delete for tracking (Debug only) ---

#ifdef _DEBUG

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

void operator delete(void* p, const char* file, int line) noexcept
{
    (void)file; (void)line;
    ::operator delete(p);
}

void* operator new[](std::size_t size, const char* file, int line)
{
    return ::operator new(size, file, line);
}

void operator delete[](void* p, const char* file, int line) noexcept
{
    (void)file; (void)line;
    ::operator delete[](p);
}

#endif // _DEBUG