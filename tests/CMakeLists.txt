add_executable(run_tests
    MemoryTracker_Test.cpp
    MemoryManager_Test.cpp
    MemoryManager_Thread_Test.cpp
    MemoryManager_PMR_Test.cpp
)

target_link_libraries(run_tests
    PRIVATE
    gtest
    gtest_main
    bedrock_core
)

include(GoogleTest)
gtest_discover_tests(run_tests)