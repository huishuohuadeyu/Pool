#include <iostream>
#include "MemoryPool.h"

int main() {
    const size_t blockSize = 64; // ÿ���ڴ�Ĵ�С�����ֽ�Ϊ��λ��
    const size_t poolSize = 10;  // �ڴ���еĿ���

    MemoryPool pool(blockSize, poolSize);

    try {
        // �����ڴ��
        void* ptr1 = pool.allocate();
        void* ptr2 = pool.allocate();

        std::cout << "Allocated memory blocks at addresses: " 
                  << ptr1 << " and " << ptr2 << std::endl;

        // ���ⲻ�ͷ�ptr2��ģ���ڴ�й©
        pool.deallocate(ptr1);
        // pool.deallocate(ptr2); // ȡ��ע���Է�ֹй©

        std::cout << "Deallocated memory block at address: " << ptr1 << std::endl;
    } catch (const std::bad_alloc& e) {
        std::cerr << "Memory allocation failed: " << e.what() << std::endl;
    }

    // �ڳ������ʱ���Զ�������������������ڴ�й©
    return 0;
}