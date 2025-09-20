#pragma once

#include <atomic>
#include <utility>
#include <cstdlib>

namespace BedrockServer::Core::Common
{
    // A correct, lock-free, multi-producer, single-consumer (MPSC) queue
    // based on the Michael-Scott algorithm principles.
    template<typename T>
    class ConcurrentQueue
    {
    public:
        struct Node
        {
            T Data;
            std::atomic<Node*> pNext;

            // Isolate node memory management to prevent use-after-free data races.
            static void* operator new(size_t size) { return std::malloc(size); }
            static void operator delete(void* p) { std::free(p); }
        };

    public:
        ConcurrentQueue()
        {
            // Start with a dummy/sentinel node to simplify the algorithm.
            // Both head and tail point to this sentinel.
            Node* dummyNode = new Node{ T{}, nullptr };
            pHead.store(dummyNode, std::memory_order_relaxed);
            pTail.store(dummyNode, std::memory_order_relaxed);
        }

        ~ConcurrentQueue()
        {
            // Clean up any remaining nodes, including the dummy node.
            Node* pCurrent = pTail.load(std::memory_order_relaxed);
            while (pCurrent != nullptr)
            {
                Node* pNext = pCurrent->pNext.load(std::memory_order_relaxed);
                delete pCurrent;
                pCurrent = pNext;
            }
        }

        ConcurrentQueue(const ConcurrentQueue&) = delete;
        ConcurrentQueue& operator=(const ConcurrentQueue&) = delete;

        // Pushed by producers. Producers only ever interact with pHead.
        void Push(T item)
        {
            Node* pNewNode = new Node{std::move(item), nullptr};
            Node* pOldHead = pHead.exchange(pNewNode, std::memory_order_acq_rel);
            pOldHead->pNext.store(pNewNode, std::memory_order_relaxed);
        }

        // Popped by the single consumer. The consumer only ever interacts with pTail.
        Node* PopAll()
        {
            Node* pCurrentTail = pTail.load(std::memory_order_relaxed);
            Node* pCurrentHead = pHead.load(std::memory_order_acquire);

            if (pCurrentTail == pCurrentHead) {
                return nullptr; // Queue is empty.
            }

            // The list to return starts from the node *after* our tail sentinel.
            Node* pListToReturn = pCurrentTail->pNext.load(std::memory_order_relaxed);
            
            // The new tail is the current head.
            pTail.store(pCurrentHead, std::memory_order_relaxed);
            
            // The old tail (our sentinel) must be deleted.
            delete pCurrentTail;

            // And a new sentinel must be created for the next cycle.
            Node* newDummyNode = new Node{ T{}, nullptr };
            pTail.load()->pNext.store(newDummyNode, std::memory_order_relaxed);


            // Reverse the list to get FIFO order.
            Node* pPrev = nullptr;
            Node* pCurrent = pListToReturn;
            while (pCurrent != pCurrentHead)
            {
                Node* pNext = pCurrent->pNext.load(std::memory_order_relaxed);
                pCurrent->pNext.store(pPrev, std::memory_order_relaxed);
                pPrev = pCurrent;
                pCurrent = pNext;
            }
            pCurrent->pNext.store(pPrev, std::memory_order_relaxed);

            return pCurrent;
        }

    private:
        // Producers push to the head.
        std::atomic<Node*> pHead;

        // Consumer pops from the tail.
        std::atomic<Node*> pTail;
    };
}