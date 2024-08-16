#pragma once
#include"ThreadCache.h"
#include"PageCache.h"

static void* ConcurrentAlloc(size_t size)
{
	// 如果申请空间超过256KB，就直接找下层的去要
	if (size > MAX_BYTES)
	{
		size_t alignSize = SizeClass::RoundUp(size); // 先按照页大小对齐
		size_t k = alignSize >> PAGE_SHIFT; // 算出来对齐之后需要多少页

		PageCache::GetInstance()->_pageMtx.lock(); // 对pc中的span进行操作，加锁
		Span* span = PageCache::GetInstance()->NewSpan(k); // 直接向pc要
		span->_objSize = size; // 统计大于256KB的页
		PageCache::GetInstance()->_pageMtx.unlock(); // 解锁

		void* ptr = (void*)(span->_pageID << PAGE_SHIFT); // 通过获得到的span来提供空间
		return ptr;
	}
	else 
	{
		
		if (pTLSThreadCache == nullptr)
		{
			
			static ObjectPool<ThreadCache> objPool; // 静态的，一直存在
			objPool._poolMtx.lock();
			pTLSThreadCache = objPool.New();
			objPool._poolMtx.unlock();
		}

		

		return pTLSThreadCache->Allocate(size);
	}
}

// 线程调用这个函数用来回收空间
static void ConcurrentFree(void* ptr)
{			
	assert(ptr);
	
	
	Span* span = PageCache::GetInstance()->MapObjectToSpan(ptr);
	size_t size = span->_objSize; // 通过映射来的span获取ptr所指空间大小

	
	if (size > MAX_BYTES)
	{
		PageCache::GetInstance()->_pageMtx.lock(); // 记得加锁解锁
		PageCache::GetInstance()->ReleaseSpanToPageCache(span); // 直接通过span释放空间
		PageCache::GetInstance()->_pageMtx.unlock(); // 记得加锁解锁
	}
	else 
	{
		pTLSThreadCache->Deallocate(ptr, size);
	}
}

