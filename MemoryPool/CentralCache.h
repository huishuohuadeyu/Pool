#pragma once
#include"Common.h"

class CentralCache
{
public:
	// �����ӿ�
	static CentralCache* GetInstance()
	{
		return &_sInst;
	}

	// cc���Լ���_spanLists��Ϊtc�ṩtc����Ҫ�Ŀ�ռ�
	size_t FetchRangeObj(void*& start, void*& end, size_t batchNum, size_t size);
		/*start��end��ʾcc�ṩ�Ŀռ�Ŀ�ʼ��β������Ͳ���*/
		/*batchNum��ʾtc��Ҫ���ٿ�size��С�Ŀռ�*/
		/*size��ʾtc��Ҫ�ĵ���ռ�Ĵ�С*/
		/*����ֵ��ccʵ���ṩ��С��ռ����*/

	// ��ȡһ������ռ䲻Ϊ�յ�span
	Span* GetOneSpan(SpanList& list, size_t size);

	// ��tc�������Ķ��ռ�ŵ�span��
	void ReleaseListToSpans(void* start, size_t size);

private:
	// ������ȥ�����졢�����Ϳ���
	CentralCache()
	{}

	CentralCache(const CentralCache& copy) = delete;
	CentralCache& operator =(const CentralCache& copy) = delete;

private:
	SpanList _spanLists[FREE_LIST_NUM]; // ��ϣͰ�йҵ���һ��һ����Span
	static CentralCache _sInst; // ����ģʽ����һ��CentralCache
};