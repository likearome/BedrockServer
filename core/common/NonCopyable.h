#pragma once

namespace BedrockServer::Core::Common
{
    // Inheriting from this class makes a class non-copyable and non-movable.
    // This is a common C++ idiom to enforce unique ownership for classes
    // like singletons or resource managers.
    class NonCopyable
    {
    public:
        // Delete the copy constructor and copy assignment operator.
        NonCopyable(const NonCopyable&) = delete;
        NonCopyable& operator=(const NonCopyable&) = delete;
    
    protected:
        // Allow construction and destruction by derived classes.
        NonCopyable() = default;
        ~NonCopyable() = default;

        // Also delete move semantics to prevent accidental moves of singletons.
        NonCopyable(NonCopyable&&) = delete;
        NonCopyable& operator=(NonCopyable&&) = delete;
    };
}