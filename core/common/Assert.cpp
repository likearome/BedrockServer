#include"Assert.h"
#include<iostream>

namespace BedrockServer::Core
{
    void HandleAssertionFailure(std::string_view condition, std::string_view file, int line)
    {
        // it's temporary implementation.
        // this will be replaced by CrashManager implementation.
        std::cerr   << "Assertion failed: " << condition
                    << "File: " << file
                    << "Line: " << line;
    }
}