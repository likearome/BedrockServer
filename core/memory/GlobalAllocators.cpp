#include "memory/GlobalAllocators.h" // <<< ADD THIS INCLUDE
#include "memory/MemoryManager.h"
#include <new>

// --- Global Operator Overrides are removed ---

// --- Placement New for Memory Tracking ---

// This is the DEFINITION of our custom placement new.

void* operator new(size_t size, const char* file, int line)
{
    void* ptr = BedrockServer::Core::Memory::MemoryManager::GetInstance().Allocate(size);
    // BedrockServer::Core::Memory::MemoryTracker::GetInstance().Track(ptr, size, file, line);
    return ptr;
}

// This is the DEFINITION for the corresponding placement delete.
void operator delete(void* p, const char* file, int line) noexcept
{
    BedrockServer::Core::Memory::MemoryManager::GetInstance().Deallocate(p);
}