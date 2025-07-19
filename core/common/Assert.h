#pragma once
#include <string_view>

// forward declaration for handler
namespace BedrockServer::Core
{
    void HandleAssertionFailure(std::string_view condition, std::string_view file, int line);
}

// CHECK: Always enabled. for critical checks that must pass in all builds
#define CHECK(condition) \
    do  \
    {   \
        if (!(condition))   \
        {   \
            BedrockServer::Core::HandleAssertionFailure(#condition, __FILE__, __LINE__);    \
            __builtin_trap(); /* GCC/Clang intrinsic */     \
        }   \
    } while(false)

// DCHECK: Debug-only check.
#ifndef NDEBUG
    #define DCHECK(condition) \
        do  \
        {   \
            if (!(condition))   \
            {   \
                BedrockServer::Core::HandleAssertionFailure(#condition, __FILE__, __LINE__);    \
                __builtin_trap(); /* GCC/Clang intrinsic */     \
            }   \
        } while(false)
#else
    #define DCHECK(condition)  ((void)0)
#endif