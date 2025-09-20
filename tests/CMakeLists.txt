# List all test source files here.
set(TEST_SOURCES
    main.cpp
    MemoryTracker_Test.cpp
    CentralHeap_Test.cpp
    MemoryManager_Test.cpp
)

# Create the test runner executable, now including the global allocator source file.
# The path must be relative from the 'tests' directory.
add_executable(run_tests 
    ${TEST_SOURCES}
    ../core/memory/GlobalAllocators.cpp 
)

# Allow test files to include headers from the core library.
target_include_directories(run_tests PRIVATE ../core)

# Link the test runner against gtest_main (which includes gtest) and our core library.
# This automatically sets up include paths and library links for the test executable.
target_link_libraries(run_tests PRIVATE GTest::gtest_main bedrock_core)

target_compile_options(run_tests PRIVATE -Wno-interference-size)