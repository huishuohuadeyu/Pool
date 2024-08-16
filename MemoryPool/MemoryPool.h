#ifndef MEMORYPOOL_H
#define MEMORYPOOL_H

#include <cstddef>
#include <vector>
#include <mutex>

class MemoryPool {
public:
    MemoryPool(size_t blockSize, size_t poolSize);
    ~MemoryPool();

    void* allocate();
    void deallocate(void* ptr);

private:
    size_t blockSize;
    size_t poolSize;
    std::vector<char> pool;
    std::vector<void*> freeBlocks;
    std::mutex poolMutex;

    void initializePool();
};

#endif // MEMORYPOOL_H