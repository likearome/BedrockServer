#include "gtest/gtest.h"
#include "memory/MemoryManager.h" // The component we are testing.
#include <cstdint> // For uintptr_t

// Test fixture for MemoryManager tests.
class MemoryManagerTest : public ::testing::Test {
protected:
    // You can set up shared resources here if needed.
    void SetUp() override {}

    // You can clean up shared resources here.
    void TearDown() override {}
};

// Test struct with a specific alignment requirement.
struct alignas(32) AlignedStruct
{
    int a;
    char b;
    // Padded to 32 bytes by the compiler.
};

TEST_F(MemoryManagerTest, AlignedAllocationAndDeallocation)
{
    // This 'new' call should trigger your 'operator new(size_t, align_val_t)' overload.
    AlignedStruct* p = new AlignedStruct();

    // 1. Check if allocation was successful.
    ASSERT_NE(p, nullptr);

    // 2. Check if the returned address is actually 32-byte aligned.
    //    Cast the pointer to an integer to check its value.
    uintptr_t p_addr = reinterpret_cast<uintptr_t>(p);
    EXPECT_EQ(p_addr % 32, 0) << "Address " << p << " is not 32-byte aligned.";

    // This 'delete' should trigger your custom deallocation logic.
    // If it works without crashing, and MemoryTracker reports no leaks, it's a success.
    delete p;

    // The test passes if all assertions above are met.
}