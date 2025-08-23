#include "gtest/gtest.h"
#include "memory/BedrockMemoryResource.h" // The component we are testing
#include "memory/Memory.h"                 // To enable the new/delete override macro
#include <vector>

// This test verifies that STL containers can use our custom memory resource.
TEST(MemoryManagerPMRTest, WorksWithStdVector)
{
    // Create an instance of our memory resource.
    BedrockServer::Core::Memory::BedrockMemoryResource bedrock_resource;

    // Create a std::pmr::vector that uses our memory resource.
    // All memory for this vector's elements will now be allocated by your MemoryManager.
    std::pmr::vector<int> my_vector(&bedrock_resource);
    
    // Add many elements to trigger (re)allocations through your manager.
    for (int i = 0; i < 1000; ++i) {
        my_vector.push_back(i);
    }
    
    ASSERT_EQ(my_vector.size(), 1000);
    ASSERT_EQ(my_vector.get_allocator().resource(), &bedrock_resource);

    // When my_vector goes out of scope, its memory will be deallocated
    // via your MemoryManager. The MemoryTracker should report no leaks at the end.
    SUCCEED();
}