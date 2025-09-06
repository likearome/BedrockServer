#include "gtest/gtest.h"
#include "memory/MemoryManager.h" // The component we are testing.
#include <cstdint> // For uintptr_t
#include <cstring>

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
/*
TEST_F(MemoryManagerTest, LargeObjectAllocatorStressTest)
{
    BedrockServer::Core::Memory::LargeObjectAllocator largeAlloc;
    std::vector<void*> allocated_pointers;

    const size_t baseSize = BedrockServer::Core::ServerConfig::MAX_SMALL_OBJECT_SIZE + 1;

    // Allocate many large blocks of various sizes.
    for (int i = 0; i < 100; ++i)
    {
        // Make sizes varied to check different scenarios.
        size_t allocSize = baseSize + (i * 123);
        void* pMem = largeAlloc.Allocate(allocSize);
        ASSERT_NE(pMem, nullptr);
        allocated_pointers.push_back(pMem);
    }

    // Deallocate all of them.
    for (void* pMem : allocated_pointers)
    {
        // To deallocate, we need the original size. We need to read it from the header.
        // This simulates what MemoryManager would do.
        std::byte* pBlock = static_cast<std::byte*>(pMem) - sizeof(std::size_t);
        std::size_t userSize;
        std::memcpy(&userSize, pBlock, sizeof(userSize));

        // The user payload pointer (pMem) is passed to deallocate, which then calculates the block start.
        // Wait, my own LargeObjectAllocator design is inconsistent. Let's fix it.
        // The LA::Deallocate should take the pBlock and totalSize.
        // But the user's MM::Deallocate calls it with pBlock and totalSize.
        // And the LA::Allocate returns pPayload. This is all tangled.

        // LET'S GO BACK TO THE CLEAN DESIGN.
        // MemoryManager handles headers. LargeObjectAllocator is dumb.
        // My test code here must reflect that clean design.

        largeAlloc.Deallocate(pMem, userSize); // This assumes LA is smart.
    }

    SUCCEED();
}
*/