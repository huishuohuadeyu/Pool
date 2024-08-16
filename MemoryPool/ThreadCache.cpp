#include"ThreadCache.h"
#include"CentralCache.h"

// 线程向tc申请size大小的空间
void* ThreadCache::Allocate(size_t size)
{
	assert(size <= MAX_BYTES); // tc中单次只能申请不超过256KB的空间

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

// 回收线程中大小为size的obj空间
void ThreadCache::Deallocate(void* obj, size_t size)
{
	assert(obj); // 回收空间不能为空
	assert(size <= MAX_BYTES); // 回收空间大小不能超过256KB

	size_t index = SizeClass::Index(size); // 找到size对应的自由链表
	_freeLists[index].Push(obj); // 用对应自由链表回收空间

	// 当前桶中的块数大于等于单批次申请块数的时候归还空间
	if (_freeLists[index].Size() >= _freeLists[index].MaxSize())
	{
		ListTooLong(_freeLists[index], size);
	}
}

// ThreadCache中空间不够时，向CentralCache申请空间的接口
void* ThreadCache::FetchFromCentralCache(size_t index, size_t alignSize)
{
#ifdef WIN32
	// 通过MaxSize和NumMoveSie来控制当前给tc提供多少块alignSize大小的空间
	size_t batchNum = min(_freeLists[index].MaxSize(), SizeClass::NumMoveSize(alignSize));
		
#else
	// 其他系统中的用std
	size_t batchNum = std::min(_freeLists[index].MaxSize(), SizeClass::NumMoveSize(alignSize));
#endif // WIN32

	if (batchNum == _freeLists[index].MaxSize())
	{ //如果没有到达上限，那下次再申请这块空间的时候可以多申请一块
		_freeLists[index].MaxSize()++; 
		
	}

	

	// 输出型参数，返回之后的结果就是tc想要的空间
	void* start = nullptr;
	void* end = nullptr;

	// 返回值为实际获取到的块数
	size_t actulNum = CentralCache::GetInstance()->FetchRangeObj(start, end, batchNum, alignSize);
	
	assert(actulNum >= 1); //actualNum一定是大于等于1的，这是FetchRangeObj能保证的

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

// tc向cc归还空间
void ThreadCache::ListTooLong(FreeList& list, size_t size)
{ 
	void* start = nullptr;
	void* end = nullptr;

	// 获取MaxSize块空间
	list.PopRange(start, end, list.MaxSize());

	// 归还空间
	CentralCache::GetInstance()->ReleaseListToSpans(start, size);
}
