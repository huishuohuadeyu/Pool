#include "MemoryPool.h"
#include <iostream>
#include <cassert>

MemoryPool::MemoryPool(size_t blockSize, size_t poolSize)
    : blockSize(blockSize), poolSize(poolSize), pool(blockSize * poolSize) {
    initializePool();
}

MemoryPool::~MemoryPool() {
    checkForLeaks(); // 析构时检查内存泄漏
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
        throw std::bad_alloc(); // 无可用块
    }
    void* block = freeBlocks.back();
    freeBlocks.pop_back();
    allocatedBlocks.insert(block); // 记录分配的块
    return block;
}

void MemoryPool::deallocate(void* ptr) {
    std::lock_guard<std::mutex> lock(poolMutex);
    auto it = allocatedBlocks.find(ptr);
    if (it != allocatedBlocks.end()) {
        allocatedBlocks.erase(it); // 从已分配集合中移除
        freeBlocks.push_back(ptr); // 将块返回到空闲列表
    } else {
        std::cerr << "Warning: Attempted to deallocate memory that was not allocated by this pool." << std::endl;
    }
}

void MemoryPool::checkForLeaks() const {
    if (!allocatedBlocks.empty()) {
        std::cerr << "Memory leak detected! " << allocatedBlocks.size() << " block(s) not deallocated." << std::endl;
        for (void* ptr : allocatedBlocks) {
            std::cerr << "Leaked block at address: " << ptr << std::endl;
        }
    } else {
        std::cout << "No memory leaks detected." << std::endl;
    }
}