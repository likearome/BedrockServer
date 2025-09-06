# Minimum CMake version requirement
cmake_minimum_required(VERSION 3.16)
cmake_policy(SET CMP0135 NEW)

# Set project name and C++ version.
project(BedrockServer VERSION 1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")

# append this line for include core/memory to build
add_subdirectory(core)

# Make executable named "BedrockServer" 
add_executable(BedrockServer src/main.cpp)

target_sources(BedrockServer
    PRIVATE
    core/memory/GlobalAllocators.cpp
)

# link bedrock_memory library to BedrockServer executable
target_link_libraries(BedrockServer PRIVATE bedrock_core)

# Add _DEBUG definition for Debug builds
target_compile_definitions(BedrockServer PRIVATE $<$<CONFIG:Debug>:_DEBUG>)
target_compile_definitions(bedrock_core PRIVATE $<$<CONFIG:Debug>:_DEBUG>)

# Add NDEBUG definition for Release builds to disable asserts
target_compile_definitions(BedrockServer PRIVATE $<$<CONFIG:Release>:NDEBUG>)
target_compile_definitions(bedrock_core PRIVATE $<$<CONFIG:Release>:NDEBUG>)

target_compile_options(BedrockServer PRIVATE -Wno-interference-size)
target_compile_options(bedrock_core PRIVATE -Wno-interference-size)

# Add an option to control memory fencing (default is ON for debug builds)
option(ENABLE_MEMORY_FENCING "Enable memory fencing check for debugging" OFF)

if(ENABLE_MEMORY_FENCING)
    # Pass the definition to all targets
    add_compile_definitions(BEDROCK_ENABLE_MEMORY_FENCING)
endif()

# BedrockServer


# =======================================================
# Unit Testing Setup using Google Test
# =======================================================
enable_testing()

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)

FetchContent_MakeAvailable(googletest)

# Add tests directory
add_subdirectory(tests)