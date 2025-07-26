#include "memory/SmallObjectAllocator.h"
#include "common/Assert.h"
#include <iostream>
#include <vector>

int main()
{
    std::cout << "--- SmallObjectAllocator Test ---" << std::endl;

    BedrockServer::Core::Memory::SmallObjectAllocator soa;

    // Test various allocation sizes
    void* p1 = soa.Allocate(7);    // Should use 8-byte pool
    void* p2 = soa.Allocate(15);   // Should use 16-byte pool
    void* p3 = soa.Allocate(32);   // Should use 32-byte pool
    void* p4 = soa.Allocate(250);  // Should use 256-byte pool

    CHECK(p1 != nullptr);
    CHECK(p2 != nullptr);
    CHECK(p3 != nullptr);
    CHECK(p4 != nullptr);

    std::cout << "Allocated 7 bytes at: " << p1 << std::endl;
    std::cout << "Allocated 15 bytes at: " << p2 << std::endl;
    std::cout << "Allocated 32 bytes at: " << p3 << std::endl;
    std::cout << "Allocated 250 bytes at: " << p4 << std::endl;

    // Test deallocation
    soa.Deallocate(p1, 7);
    soa.Deallocate(p2, 15);
    soa.Deallocate(p3, 32);
    soa.Deallocate(p4, 250);

    std::cout << "All blocks deallocated." << std::endl;

    // Test reallocation to see if we get the same blocks back
    void* p5 = soa.Allocate(7);
    std::cout << "Re-allocated 7 bytes at: " << p5 << std::endl;
    CHECK(p5 == p1); // Because we are the only user, should get the same memory back.

    soa.Deallocate(p5, 7);
    std::cout << "Test finished successfully." << std::endl;

    return 0;
}