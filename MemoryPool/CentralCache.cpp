#include"CentralCache.h"
#include"PageCache.h"

CentralCache CentralCache::_sInst; // CentralCache�Ķ�������

// cc��һ������ռ�ǿյ�span���ó�һ��batchNum��size��С�Ŀ�ռ�
size_t CentralCache::FetchRangeObj(void*& start, void*& end, size_t batchNum, size_t size)
{
	// ��ȡ��size��Ӧ��һ��SpanList
	size_t index = SizeClass::Index(size);
	
	// ��cc�е�SpanList����ʱҪ����
	_spanLists[index]._mtx.lock();

	// ��ȡ��һ������ռ�ǿյ�span
	Span* span = GetOneSpan(_spanLists[index], size);
	assert(span); // ����һ��span��Ϊ��
	assert(span->_freeList); // ����һ��span����Ŀռ䲻��Ϊ��

	// �����ָ��_freeList����end����������
	start = end = span->_freeList;
	size_t actualNum = 1; // ����ʵ�ʵķ���ֵ

	// ��end��next��Ϊ�յ�ǰ���£���end��batchNum - 1��
	size_t i = 0;
	while (i < batchNum - 1 && ObjNext(end) != nullptr)
	{
		end = ObjNext(end);
		++actualNum; // ��¼end�߹��˶��ٲ�
		++i; // ��������
	}

	// ��[start, end]���ظ�ThreadCache�󣬵���Span��_freeList
	span->_freeList = ObjNext(end);
	span->use_count += actualNum; // ��tc���˶��پ͸�useCount�Ӷ���
	// ����һ�οռ䣬��Ҫ��ԭ��Span��_freeList�еĿ�����
	ObjNext(end) = nullptr; 

	_spanLists[index]._mtx.unlock();

	return actualNum;
}

// ��ȡһ������ռ�ǿյ�Span
Span* CentralCache::GetOneSpan(SpanList& list, size_t size)
{
	// ����cc����һ����û�й���ռ�ǿյ�span
	Span* it = list.Begin();
	while (it != list.End())
	{
		if (it->_freeList != nullptr) // �ҵ�����ռ�ǿյ�span
			return it;
		else // û�ҵ�����������
			it = it->_next;
	}

	// ���Ͱ�������������ccͰ���в������߳����õ���
	list._mtx.unlock();

	// �ߵ������cc��û���ҵ�����ռ�ǿյ�span
	
	// ��sizeת����ƥ���ҳ�����Թ�pc�ṩһ�����ʵ�span
	size_t k = SizeClass::NumMovePage(size);

	// ��������ķ��������ڵ���NewSpan�ĵط�����
	PageCache::GetInstance()->_pageMtx.lock();	// ����
	// ����NewSpan��ȡһ��ȫ��span
	Span* span = PageCache::GetInstance()->NewSpan(k);
	span->_isUse = true; // cc��ȡ����pc�е�span���ĳ�����ʹ��
	PageCache::GetInstance()->_pageMtx.unlock(); // ����

	/* ����Ҫǿתһ�£���Ϊ_pageID��PageID����(size_t����
	 unsigned long long)�ģ�����ֱ�Ӹ�ֵ��ָ��*/
	char* start = (char*)(span->_pageID << PAGE_SHIFT);
	char* end = (char*)(start + (span->_n << PAGE_SHIFT));

	span->_objSize = size; // ��¼span���зֵĿ��ж��

	// ��ʼ�з�span����Ŀռ�

	span->_freeList = start;// ����Ŀռ�ŵ�span->_freeList��

	void* tail = start; // �����tailָ��start
	start += size; // start������һ�飬�������ѭ��

	int i = 0;
	// ���Ӹ�����
	while (start < end)
	{
		++i;
		ObjNext(tail) = start;
		start += size;
		tail = ObjNext(tail);
	}
	ObjNext(tail) = nullptr; // �ǵ�Ҫ�����һλ�ÿ�

	// �к�span�Ժ���Ҫ��span�ҵ�cc��Ӧ�±��Ͱ����ȥ	
	list._mtx.lock(); // span����ȥ֮ǰ����
	list.PushFront(span);

	return span;
}


// ��tc�������Ķ��ռ�ŵ�span��
void CentralCache::ReleaseListToSpans(void* start, size_t size)
{
	// ��ͨ��size�ҵ���Ӧ��Ͱ������
	size_t index = SizeClass::Index(size);

	// ����Ҫ��cc�е�span���в���������Ҫ����cc��Ͱ��
	_spanLists[index]._mtx.lock();

	// ����start����������ŵ���Ӧҳ��span�������_freeList��
	while (start) // startΪ��ʱֹͣ
	{
		// ��¼һ��start��һλ
		void* next = ObjNext(start);
		
		// �ҵ���Ӧspan
		Span* span = PageCache::GetInstance()->MapObjectToSpan(start);

		// �ѵ�ǰ����뵽��Ӧspan��
		ObjNext(start) = span->_freeList;
		span->_freeList = start;

		// ������һ��ռ䣬��Ӧspan��useCountҪ��1
		span->use_count--;
		if (span->use_count == 0) // ���span���������ҳ��������
		{ // �����span����pc����
			
			// �Ƚ�span��cc��ȥ��
			_spanLists[index].Erase(span);
			span->_freeList = nullptr; // һЩ������
			span->_next = nullptr;
			span->_prev = nullptr;

			// �黹span�������ǰͰ��
			_spanLists[index]._mtx.unlock();

			// �黹span������page��������
			PageCache::GetInstance()->_pageMtx.lock();
			PageCache::GetInstance()->ReleaseSpanToPageCache(span);
			PageCache::GetInstance()->_pageMtx.unlock();

			// �黹��ϣ��ټ��ϵ�ǰͰ��Ͱ��
			_spanLists[index]._mtx.lock();
		}

		// ����һ����
		start = next;
	}

	_spanLists[index]._mtx.unlock(); // ��Ͱ��
}