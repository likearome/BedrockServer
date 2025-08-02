add_executable(run_tests
    MemoryTracker_Test.cpp
)

target_link_libraries(run_tests
    PRIVATE
    gtest
    gtest_main
    bedrock_core
)

include(GoogleTest)
gtest_discover_tests(run_tests)