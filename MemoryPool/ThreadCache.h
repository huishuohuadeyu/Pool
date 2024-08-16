#pragma once

#include"Common.h"

class ThreadCache
{
public:
	// �߳�����size��С�Ŀռ�
	void* Allocate(size_t size);

	// �����߳��д�СΪsize��obj�ռ�
	void Deallocate(void* obj, size_t size);

	// ThreadCache�пռ䲻��ʱ����CentralCache����ռ�Ľӿ�
	void* FetchFromCentralCache(size_t index, size_t alignSize);

	// tc��cc�黹�ռ�ListͰ�еĿռ�
	void ListTooLong(FreeList& list, size_t size);
private:
	FreeList _freeLists[FREE_LIST_NUM]; // ��ϣ��ÿ��Ͱ��ʾһ����������
};

