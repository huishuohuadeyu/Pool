#include"PageCache.h"

PageCache PageCache::_sInst; // ��������


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

		
		for (PageID i = 0; i < span->_n; ++i) // ע��iҪPageID���ͣ���Ȼ��64λ�º�_pageID��ӻᱨ����
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
			Span* kSpan = _spanPool.New(); // �ö����ڴ�ؿ��ռ�

			
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

	void* ptr = SystemAlloc(PAGE_NUM - 1); // PAGE_NUMΪ129
	
	Span* bigSpan = _spanPool.New(); // �ö����ڴ�ؿ��ռ�

	
	bigSpan->_pageID = ((PageID)ptr) >> PAGE_SHIFT;
	bigSpan->_n = PAGE_NUM - 1;

	
	_spanLists[PAGE_NUM - 1].PushFront(bigSpan);

	
	return NewSpan(k);  
}

Span* PageCache::MapObjectToSpan(void* obj)
{ 
	
  // ͨ�����ַ�ҵ�ҳ��
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
	// ͨ��span�ж��ͷŵĿռ�ҳ���Ƿ����128ҳ���������128ҳ��ֱ�ӻ���os
	if (span->_n > PAGE_NUM - 1)
	{
		void* ptr = (void*)(span->_pageID << PAGE_SHIFT); // ��ȡ��Ҫ�ͷŵĵ�ַ
		SystemFree(ptr); // ֱ�ӵ���ϵͳ�ӿ��ͷſռ�
		//delete span; // �ͷŵ�span
		_spanPool.Delete(span); // �ö����ڴ��ɾ��span

		return;
	}

	
	
	while (1)
	{
		PageID leftID = span->_pageID - 1; // �õ��������ҳ
		auto ret = _idSpanMap.get(leftID); // ͨ������ҳӳ�����Ӧspan
		
		// û������span��ֹͣ�ϲ�
		if (ret == nullptr)
		{
			break;
		}

		Span* leftSpan = (Span*)ret; // ����span
		// ����span��cc�У�ֹͣ�ϲ�
		if (leftSpan->_isUse == true)
		{
			break;
		}

		// ����span�뵱ǰspan�ϲ��󳬹�128ҳ��ֹͣ�ϲ�
		if (leftSpan->_n + span->_n > PAGE_NUM - 1)
		{
			break;
		}

		// ��ǰspan������span���кϲ�
		span->_pageID = leftSpan->_pageID;
		span->_n += leftSpan->_n;

		_spanLists[leftSpan->_n].Erase(leftSpan);// ������span�����Ͱ��ɾ��
		//delete leftSpan;// ɾ��������span����
		_spanPool.Delete(leftSpan); // �ö����ڴ��ɾ��span
	}

	// ���Ҳ��Ϻϲ�
	while (1)
	{
		PageID rightID = span->_pageID + span->_n; // �ұߵ�����ҳ
		auto it = _idSpanMap.get(rightID); // ͨ������ҳ�ҵ���Ӧspanӳ���ϵ

		// û������span��ֹͣ�ϲ�
		if (it == nullptr)
		{
			break;
		}

		Span* rightSpan = (Span*)it; // �ұߵ�span
		// ����span��cc�У�ֹͣ�ϲ�
		if (rightSpan->_isUse == true)
		{
			break;
		}

		// ����span�뵱ǰspan�ϲ��󳬹�128ҳ��ֹͣ�ϲ�
		if (rightSpan->_n + span->_n > PAGE_NUM - 1)
		{
			break;
		}

		// ��ǰspan������span���кϲ�
		span->_n += rightSpan->_n; 
								   

		// ��Ͱ�����spanɾ��
		_spanLists[rightSpan->_n].Erase(rightSpan);
		//delete rightSpan; // ɾ���ұ�span����Ŀռ�
		_spanPool.Delete(rightSpan); // �ö����ڴ��ɾ��span
	}

	// �ϲ���ϣ�����ǰspan�ҵ���ӦͰ��
	_spanLists[span->_n].PushFront(span);
	span->_isUse = false; // ��cc���ص�pc��isUse�ĳ�false

	// ӳ�䵱ǰspan�ı�Եҳ�����������Զ����span�ϲ�
	/*_idSpanMap[span->_pageID] = span; 
	_idSpanMap[span->_pageID + span->_n - 1] = span;*/
	_idSpanMap.set(span->_pageID, span);
	_idSpanMap.set(span->_pageID + span->_n - 1, span);
}
