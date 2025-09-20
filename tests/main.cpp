#include "gtest/gtest.h"
#include "memory/MemoryManager.h" // Now include MemoryManager
#include "memory/MemoryTracker.h" // Include MemoryTracker
#include <cstdlib> // For std::atexit

// The LeakReporter is no longer needed, as we now have explicit shutdown control.

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    
    int result = RUN_ALL_TESTS();

    BedrockServer::Core::Memory::MemoryTracker::GetInstance().ReportLeaks();

    return result;
}