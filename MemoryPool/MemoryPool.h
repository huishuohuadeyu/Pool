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

    void checkForLeaks() const; // ����ڴ�й©

private:
    size_t blockSize;
    size_t poolSize;
    std::vector<char> pool;
    std::vector<void*> freeBlocks;
    std::unordered_set<void*> allocatedBlocks; // ���ڸ��ٷ���Ŀ�
    std::mutex poolMutex;

    void initializePool();
};

#endif // MEMORYPOOL_H