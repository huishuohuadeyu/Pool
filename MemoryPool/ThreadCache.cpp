#include"ThreadCache.h"
#include"CentralCache.h"

// �߳���tc����size��С�Ŀռ�
void* ThreadCache::Allocate(size_t size)
{
	assert(size <= MAX_BYTES); // tc�е���ֻ�����벻����256KB�Ŀռ�

	size_t alignSize = SizeClass::RoundUp(size); 
	size_t index = SizeClass::Index(size); 

	if (!_freeLists[index].Empty())
	{ 
		return _freeLists[index].Pop();
	}
	else
	{ 
		return FetchFromCentralCache(index, alignSize); 
	}
}

// �����߳��д�СΪsize��obj�ռ�
void ThreadCache::Deallocate(void* obj, size_t size)
{
	assert(obj); // ���տռ䲻��Ϊ��
	assert(size <= MAX_BYTES); // ���տռ��С���ܳ���256KB

	size_t index = SizeClass::Index(size); // �ҵ�size��Ӧ����������
	_freeLists[index].Push(obj); // �ö�Ӧ����������տռ�

	// ��ǰͰ�еĿ������ڵ��ڵ��������������ʱ��黹�ռ�
	if (_freeLists[index].Size() >= _freeLists[index].MaxSize())
	{
		ListTooLong(_freeLists[index], size);
	}
}

// ThreadCache�пռ䲻��ʱ����CentralCache����ռ�Ľӿ�
void* ThreadCache::FetchFromCentralCache(size_t index, size_t alignSize)
{
#ifdef WIN32
	// ͨ��MaxSize��NumMoveSie�����Ƶ�ǰ��tc�ṩ���ٿ�alignSize��С�Ŀռ�
	size_t batchNum = min(_freeLists[index].MaxSize(), SizeClass::NumMoveSize(alignSize));
		
#else
	// ����ϵͳ�е���std
	size_t batchNum = std::min(_freeLists[index].MaxSize(), SizeClass::NumMoveSize(alignSize));
#endif // WIN32

	if (batchNum == _freeLists[index].MaxSize())
	{ //���û�е������ޣ����´����������ռ��ʱ����Զ�����һ��
		_freeLists[index].MaxSize()++; 
		
	}

	

	// ����Ͳ���������֮��Ľ������tc��Ҫ�Ŀռ�
	void* start = nullptr;
	void* end = nullptr;

	// ����ֵΪʵ�ʻ�ȡ���Ŀ���
	size_t actulNum = CentralCache::GetInstance()->FetchRangeObj(start, end, batchNum, alignSize);
	
	assert(actulNum >= 1); //actualNumһ���Ǵ��ڵ���1�ģ�����FetchRangeObj�ܱ�֤��

	if (actulNum == 1)
	{ 
		assert(start == end);
		return start;
	}
	else
	{ 
		_freeLists[index].PushRange(ObjNext(start), end, actulNum - 1);

		
		return start;
	}
}

// tc��cc�黹�ռ�
void ThreadCache::ListTooLong(FreeList& list, size_t size)
{ 
	void* start = nullptr;
	void* end = nullptr;

	// ��ȡMaxSize��ռ�
	list.PopRange(start, end, list.MaxSize());

	// �黹�ռ�
	CentralCache::GetInstance()->ReleaseListToSpans(start, size);
}
