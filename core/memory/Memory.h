#pragma once

#ifdef _DEBUG
    #include <cstddef>

    // Declare placement new operators to be used by the macro.
    // The definitions are in GlobalAllocators.cpp.
    void* operator new(std::size_t size, const char* file, int line);
    void* operator new[](std::size_t size, const char* file, int line);

    // Any call to "new" will be replaced by this, automatically capturing file and line.
    #define new new(__FILE__, __LINE__)

#endif // _DEBUG