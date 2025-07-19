# Minimum CMake version requirement
cmake_minimum_required(VERSION 3.16)

# Set project name and C++ version.
project(BedrockServer VERSION 1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# append this line for include core/memory to build
add_subdirectory(core)

# Make executable named "BedrockServer" 
add_executable(BedrockServer src/main.cpp)

# link bedrock_memory library to BedrockServer executable
target_link_libraries(BedrockServer PRIVATE bedrock_core)
# BedrockServer
