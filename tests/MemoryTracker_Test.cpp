#include "gtest/gtest.h"
#include "memory/MemoryManager.h"
#include "memory/MemoryTracker.h"
#include "memory/GlobalAllocators.h"

// A macro to explicitly use our tracked allocation via placement new.
#define bedrock_new new(__FILE__, __LINE__)

namespace
{
    struct LeakTestObject
    {
        int a;
        char buffer[128];
    };
}

TEST(MemoryTrackerTest, DetectsIntentionalLeak)
{
    // Now the compiler knows what `new(__FILE__, __LINE__)` refers to.
    LeakTestObject* pLeakedObject = bedrock_new LeakTestObject();

    (void)pLeakedObject; 

    // This 'delete' must be commented out to actually create a leak for the test.
    // delete pLeakedObject;
}