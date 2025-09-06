#pragma once

#include <atomic>
#include <memory>
#include <thread>

namespace BedrockServer::Core::Common
{
    // A lock-free, thread-safe, multi-producer, single-consumer queue.
    // Implemented as a lock-free stack (LIFO order)
    template<typename T>
    class ConcurrentQueue
    {
    public:
        struct Node
        {
            T Data;
            Node* pNext;
        };

    public:
        ConcurrentQueue() = default;
        ~ConcurrentQueue()
        {
            // Clean up any remaining nodes on destruction.
            Node* pCurrent = pHead.load();
            while (pCurrent != nullptr)
            {
                Node* pNext = pCurrent->pNext;
                delete pCurrent;
                pCurrent = pNext;
            }
        }

        // Non-copyable and non-movable.
        ConcurrentQueue(const ConcurrentQueue&) = delete;
        ConcurrentQueue& operator=(const ConcurrentQueue&) = delete;

        // Pushed an item to the queue (thread-safe for multiple producers).
        void Push(T item)
        {
            Node* pNewNode = new Node{std::move(item), nullptr};
            
            constexpr int SPIN_COUNT_BEFORE_YIELD = 5000;
            int spinCount = 0;

            Node* pCurrentHead = pHead.load();
            do {
                pNewNode->pNext = pCurrentHead;

                // Add adaptive spinning
                if (spinCount++ > SPIN_COUNT_BEFORE_YIELD) {
                    std::this_thread::yield();
                }

            } while (!pHead.compare_exchange_weak(pCurrentHead, pNewNode));
        }

        // Pops all items from the queue at once (only for the single consumer).
        // Returns the head of the reversed list of items.
        Node* PopAll()
        {
            // Atomically take the entire list.
            Node* pListHead = pHead.exchange(nullptr);
            
            // The list is in LIFO order (stack), so we reverse it to get FIFO order.]
            Node* pPrev = nullptr;
            Node* pCurrent = pListHead;
            while (pCurrent != nullptr)
            {
                Node* pNext = pCurrent->pNext;
                pCurrent->pNext = pPrev;
                pPrev = pCurrent;
                pCurrent = pNext;
            }
            return pPrev; // pPrev is now the new head of the reversed list.
        }

    private:
        // The head of the intrusive linked list, protected by std::atomic.
        std::atomic<Node*> pHead = nullptr;
    };
}