#pragma once

#include"Common.h"

class PageCache
{
public:
	// 饿汉单例
	static PageCache* GetInstance()
	{
		return &_sInst;
	}

	
	Span* NewSpan(size_t k);

	
	Span* MapObjectToSpan(void* obj);

	
	void ReleaseSpanToPageCache(Span* span);

private:
	SpanList _spanLists[PAGE_NUM]; 

	
	TCMalloc_PageMap1<32 - PAGE_SHIFT> _idSpanMap;

	ObjectPool<Span> _spanPool; 
public:
	
	std::mutex _pageMtx; 

private: 
	PageCache()
	{}

	PageCache(const PageCache& pc) = delete;
	PageCache& operator = (const PageCache& pc) = delete;

	static PageCache _sInst; // 单例类对象
};