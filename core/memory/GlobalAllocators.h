#pragma once
#include <cstddef> // For size_t

// --- Placement New for Memory Tracking ---

// This is the DECLARATION of our custom placement new.
// It tells the compiler that a function with this signature exists somewhere else.
void* operator new(size_t size, const char* file, int line);

// This is the DECLARATION for the corresponding placement delete.
void operator delete(void* p, const char* file, int line) noexcept;