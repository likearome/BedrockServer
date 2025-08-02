#include"memory/MemoryTracker.h"
#include"common/Assert.h"
#include<iostream>

namespace BedrockServer::Core::Memory
{
    MemoryTracker& MemoryTracker::GetInstance()
    {
        static MemoryTracker instance;
        return instance;
    }

    void MemoryTracker::Track(uint32_t threadId, void* p, std::size_t size, const char* file, int line)
    {
        CHECK(threadId < ServerConfig::MAX_THREADS);
        if(nullptr == p)
        {
            return;
        }

        auto& allocations = thread_data_[threadId].allocations;
        CHECK(allocations.find(p) == allocations.end());
        allocations[p] = {size, file, line};
    }
    void MemoryTracker::Untrack(uint32_t threadId, void* p)
    {
        CHECK(threadId < ServerConfig::MAX_THREADS);
        if(nullptr == p)
        {
            return;
        }

        auto& allocations = thread_data_[threadId].allocations;
        CHECK(allocations.erase(p) == 1);
    }
    void MemoryTracker::ReportLeaks()
    {
        std::cout << "==================================================" << std::endl;
        std::cout << "            MEMORY LEAK REPORT                  " << std::endl;
        std::cout << "==================================================" << std::endl;
        
        std::size_t totalLeaks = 0;
        std::size_t totalLeakedBytes = 0;

        //Iterate through the data for all possible threads.
        for(uint32_t i = 0; i < ServerConfig::MAX_THREADS; ++i)
        {
            const auto& allocations = thread_data_[i].allocations;
            if(allocations.empty())
            {
                continue;
            }

            for(const auto& [p, info] : allocations)
            {
                std::cout << "Leak detected: " << info.size << " bytes at " << p << std::endl;
                std::cout << "  -> Thread: " << i << ", Allocated at: " << info.file << " (Line: " << info.line << ")" << std::endl;
                totalLeakedBytes += info.size;
                totalLeaks++;
            }
        }

        if(0 == totalLeaks)
        {
            std::cout << "No memory leaks detected." << std::endl;
        }
        else
        {
            std::cout << "--------------------------------------------------" << std::endl;
            std::cout << "Total Leaked: " << totalLeaks << " allocations, " << totalLeakedBytes << " bytes" << std::endl;
        }
        std::cout << "==================================================" << std::endl;
    }
}