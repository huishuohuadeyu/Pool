#include "MemoryPool.h"
#include <iostream>
#include <cassert>

MemoryPool::MemoryPool(size_t blockSize, size_t poolSize)
    : blockSize(blockSize), poolSize(poolSize), pool(blockSize * poolSize) {
    initializePool();
}

MemoryPool::~MemoryPool() {
    // No dynamic memory to release, pool is managed by vector
}

void MemoryPool::initializePool() {
    std::lock_guard<std::mutex> lock(poolMutex);
    for (size_t i = 0; i < poolSize; ++i) {
        freeBlocks.push_back(&pool[i * blockSize]);
    }
}

void* MemoryPool::allocate() {
    std::lock_guard<std::mutex> lock(poolMutex);
    if (freeBlocks.empty()) {
        throw std::bad_alloc(); // No available blocks
    }
    void* block = freeBlocks.back();
    freeBlocks.pop_back();
    return block;
}

void MemoryPool::deallocate(void* ptr) {
    std::lock_guard<std::mutex> lock(poolMutex);
    freeBlocks.push_back(ptr);
}