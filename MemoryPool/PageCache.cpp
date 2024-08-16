#include"PageCache.h"

PageCache PageCache::_sInst; // 单例对象


Span* PageCache::NewSpan(size_t k)
{
	
	assert(k > 0);

	
	if (k > PAGE_NUM - 1) 
	{
		void* ptr = SystemAlloc(k); 
		//Span* span = new Span; 
		Span* span = _spanPool.New(); 
		
		span->_pageID = ((PageID)ptr >> PAGE_SHIFT); 
		span->_n = k; 
		
		
		_idSpanMap.set(span->_pageID, span);
		

		return span;
	}

	
	if (!_spanLists[k].Empty())
	{ 
		Span* span = _spanLists[k].PopFront();

		
		for (PageID i = 0; i < span->_n; ++i) // 注意i要PageID类型，不然在64位下和_pageID相加会报警告
		{ 
			_idSpanMap.set(span->_pageID + i, span);
		}

		return span;
	}

	
	for (int i = k + 1; i < PAGE_NUM; ++i)
	{ 
		if (!_spanLists[i].Empty())
		{ 
			
			
			Span* nSpan = _spanLists[i].PopFront();

			;
			Span* kSpan = _spanPool.New(); // 用定长内存池开空间

			
			kSpan->_pageID = nSpan->_pageID;
			kSpan->_n = k;

			
			nSpan->_pageID += k;
			nSpan->_n -= k;

			
			_spanLists[nSpan->_n].PushFront(nSpan);

			
			_idSpanMap.set(nSpan->_pageID, nSpan);
			_idSpanMap.set(nSpan->_pageID + nSpan->_n - 1, nSpan);

			
			for (PageID i = 0; i < kSpan->_n; ++i) 
			{ 
				_idSpanMap.set(kSpan->_pageID + i, kSpan);
			}

			return kSpan;
		}
	}

	void* ptr = SystemAlloc(PAGE_NUM - 1); // PAGE_NUM为129
	
	Span* bigSpan = _spanPool.New(); // 用定长内存池开空间

	
	bigSpan->_pageID = ((PageID)ptr) >> PAGE_SHIFT;
	bigSpan->_n = PAGE_NUM - 1;

	
	_spanLists[PAGE_NUM - 1].PushFront(bigSpan);

	
	return NewSpan(k);  
}

Span* PageCache::MapObjectToSpan(void* obj)
{ 
	
  // 通过块地址找到页号
	PageID id = (((PageID)obj) >> PAGE_SHIFT);

	
	auto ret = _idSpanMap.get(id);
	
	if (ret != nullptr)
	{ 
		return (Span*)ret;
	}
	else
	{
		assert(false);
		return nullptr;
	}
}

void PageCache::ReleaseSpanToPageCache(Span* span)
{
	// 通过span判断释放的空间页数是否大于128页，如果大于128页就直接还给os
	if (span->_n > PAGE_NUM - 1)
	{
		void* ptr = (void*)(span->_pageID << PAGE_SHIFT); // 获取到要释放的地址
		SystemFree(ptr); // 直接调用系统接口释放空间
		//delete span; // 释放掉span
		_spanPool.Delete(span); // 用定长内存池删除span

		return;
	}

	
	
	while (1)
	{
		PageID leftID = span->_pageID - 1; // 拿到左边相邻页
		auto ret = _idSpanMap.get(leftID); // 通过相邻页映射出对应span
		
		// 没有相邻span，停止合并
		if (ret == nullptr)
		{
			break;
		}

		Span* leftSpan = (Span*)ret; // 相邻span
		// 相邻span在cc中，停止合并
		if (leftSpan->_isUse == true)
		{
			break;
		}

		// 相邻span与当前span合并后超过128页，停止合并
		if (leftSpan->_n + span->_n > PAGE_NUM - 1)
		{
			break;
		}

		// 当前span与相邻span进行合并
		span->_pageID = leftSpan->_pageID;
		span->_n += leftSpan->_n;

		_spanLists[leftSpan->_n].Erase(leftSpan);// 将相邻span对象从桶中删掉
		//delete leftSpan;// 删除掉相邻span对象
		_spanPool.Delete(leftSpan); // 用定长内存池删除span
	}

	// 向右不断合并
	while (1)
	{
		PageID rightID = span->_pageID + span->_n; // 右边的相邻页
		auto it = _idSpanMap.get(rightID); // 通过相邻页找到对应span映射关系

		// 没有相邻span，停止合并
		if (it == nullptr)
		{
			break;
		}

		Span* rightSpan = (Span*)it; // 右边的span
		// 相邻span在cc中，停止合并
		if (rightSpan->_isUse == true)
		{
			break;
		}

		// 相邻span与当前span合并后超过128页，停止合并
		if (rightSpan->_n + span->_n > PAGE_NUM - 1)
		{
			break;
		}

		// 当前span与相邻span进行合并
		span->_n += rightSpan->_n; 
								   

		// 把桶里面的span删掉
		_spanLists[rightSpan->_n].Erase(rightSpan);
		//delete rightSpan; // 删掉右边span对象的空间
		_spanPool.Delete(rightSpan); // 用定长内存池删除span
	}

	// 合并完毕，将当前span挂到对应桶中
	_spanLists[span->_n].PushFront(span);
	span->_isUse = false; // 从cc返回到pc，isUse改成false

	// 映射当前span的边缘页，后续还可以对这个span合并
	/*_idSpanMap[span->_pageID] = span; 
	_idSpanMap[span->_pageID + span->_n - 1] = span;*/
	_idSpanMap.set(span->_pageID, span);
	_idSpanMap.set(span->_pageID + span->_n - 1, span);
}
