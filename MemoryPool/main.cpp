#include <iostream>
#include "MemoryPool.h"

int main() {
    const size_t blockSize = 64; // Size of each block in bytes
    const size_t poolSize = 10;  // Number of blocks

    MemoryPool pool(blockSize, poolSize);

    try {
        // Allocate memory blocks
        void* ptr1 = pool.allocate();
        void* ptr2 = pool.allocate();

        std::cout << "Allocated memory blocks at addresses: " 
                  << ptr1 << " and " << ptr2 << std::endl;

        // Deallocate memory blocks
        pool.deallocate(ptr1);
        pool.deallocate(ptr2);

        std::cout << "Deallocated memory blocks." << std::endl;
    } catch (const std::bad_alloc& e) {
        std::cerr << "Memory allocation failed: " << e.what() << std::endl;
    }

    return 0;
}