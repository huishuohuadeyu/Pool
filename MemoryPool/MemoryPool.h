#ifndef MEMORYPOOL_H
#define MEMORYPOOL_H

#include <cstddef>
#include <vector>
#include <set>
#include <mutex>
#include <unordered_set>

class MemoryPool {
public:
    MemoryPool(size_t blockSize, size_t poolSize);
    ~MemoryPool();

    void* allocate();
    void deallocate(void* ptr);

    void checkForLeaks() const; // 检查内存泄漏

private:
    size_t blockSize;
    size_t poolSize;
    std::vector<char> pool;
    std::vector<void*> freeBlocks;
    std::unordered_set<void*> allocatedBlocks; // 用于跟踪分配的块
    std::mutex poolMutex;

    void initializePool();
};

#endif // MEMORYPOOL_H