#include <iostream>
#include<vector>
#include "memory/MemoryTracker.h"
#include "memory/Memory.h" // The ONLY memory header we need now.

struct Vec3 { float x, y, z; };

int main()
{
    std::cout << "--- Final Integration Test ---" << std::endl;

    // This 'new' will be replaced by our macro: new(__FILE__, __LINE__)
    // It will call our placement new, which tracks the allocation.
    int* my_int = new int(5);
    Vec3* my_vec = new Vec3{1.0f, 2.0f, 3.0f};

    // This one will be intentionally leaked.
    int* leaked_int = new int(100);

    std::cout << "Allocated an int at " << my_int << std::endl;
    std::cout << "Allocated a Vec3 at " << my_vec << std::endl;
    std::cout << "Allocated a leaked_int at " << leaked_int << std::endl;

    // These deletes will also be handled by our system.
    delete my_int;
    delete my_vec;

    // Report leaks at the end of the program.
    BedrockServer::Core::Memory::MemoryTracker::GetInstance().ReportLeaks();

    return 1;
}