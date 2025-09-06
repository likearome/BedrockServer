add_executable(run_tests
    MemoryTracker_Test.cpp
    MemoryManager_Test.cpp
    MemoryManager_Thread_Test.cpp
    MemoryManager_PMR_Test.cpp
    CentralHeap_Thread_Test.cpp
)

target_link_libraries(run_tests
    PRIVATE
    gtest
    gtest_main
    bedrock_core
    atomic
)

target_compile_options(run_tests PRIVATE -Wno-interference-size)

target_compile_definitions(run_tests PRIVATE BEDROCK_TESTING)

include(GoogleTest)
gtest_discover_tests(run_tests)