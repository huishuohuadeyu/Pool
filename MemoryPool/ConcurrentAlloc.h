#pragma once
#include"ThreadCache.h"
#include"PageCache.h"

static void* ConcurrentAlloc(size_t size)
{
	// �������ռ䳬��256KB����ֱ�����²��ȥҪ
	if (size > MAX_BYTES)
	{
		size_t alignSize = SizeClass::RoundUp(size); // �Ȱ���ҳ��С����
		size_t k = alignSize >> PAGE_SHIFT; // ���������֮����Ҫ����ҳ

		PageCache::GetInstance()->_pageMtx.lock(); // ��pc�е�span���в���������
		Span* span = PageCache::GetInstance()->NewSpan(k); // ֱ����pcҪ
		span->_objSize = size; // ͳ�ƴ���256KB��ҳ
		PageCache::GetInstance()->_pageMtx.unlock(); // ����

		void* ptr = (void*)(span->_pageID << PAGE_SHIFT); // ͨ����õ���span���ṩ�ռ�
		return ptr;
	}
	else 
	{
		
		if (pTLSThreadCache == nullptr)
		{
			
			static ObjectPool<ThreadCache> objPool; // ��̬�ģ�һֱ����
			objPool._poolMtx.lock();
			pTLSThreadCache = objPool.New();
			objPool._poolMtx.unlock();
		}

		

		return pTLSThreadCache->Allocate(size);
	}
}

// �̵߳�����������������տռ�
static void ConcurrentFree(void* ptr)
{			
	assert(ptr);
	
	
	Span* span = PageCache::GetInstance()->MapObjectToSpan(ptr);
	size_t size = span->_objSize; // ͨ��ӳ������span��ȡptr��ָ�ռ��С

	
	if (size > MAX_BYTES)
	{
		PageCache::GetInstance()->_pageMtx.lock(); // �ǵü�������
		PageCache::GetInstance()->ReleaseSpanToPageCache(span); // ֱ��ͨ��span�ͷſռ�
		PageCache::GetInstance()->_pageMtx.unlock(); // �ǵü�������
	}
	else 
	{
		pTLSThreadCache->Deallocate(ptr, size);
	}
}

