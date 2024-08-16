#include <iostream>
#include "MemoryPool.h"

int main() {
    const size_t blockSize = 64; // 每块内存的大小（以字节为单位）
    const size_t poolSize = 10;  // 内存池中的块数

    MemoryPool pool(blockSize, poolSize);

    try {
        // 分配内存块
        void* ptr1 = pool.allocate();
        void* ptr2 = pool.allocate();

        std::cout << "Allocated memory blocks at addresses: " 
                  << ptr1 << " and " << ptr2 << std::endl;

        // 有意不释放ptr2以模拟内存泄漏
        pool.deallocate(ptr1);
        // pool.deallocate(ptr2); // 取消注释以防止泄漏

        std::cout << "Deallocated memory block at address: " << ptr1 << std::endl;
    } catch (const std::bad_alloc& e) {
        std::cerr << "Memory allocation failed: " << e.what() << std::endl;
    }

    // 在程序结束时会自动调用析构函数并检查内存泄漏
    return 0;
}