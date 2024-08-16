#pragma once

#include<cassert>

#include<iostream>
#include<thread>
#include<mutex>

#include<unordered_map>
#include<vector>


using std::vector;
using std::cout;
using std::endl;

static const size_t FREE_LIST_NUM = 208; 
static const size_t MAX_BYTES = 256 * 1024; 
static const size_t PAGE_NUM = 129; 
static const size_t PAGE_SHIFT = 13; 


typedef size_t PageID;




#ifdef _WIN32
	#include<Windows.h> // Windows�µ�ͷ�ļ�

#endif // _WIN32

// ֱ��ȥ���ϰ�ҳ����ռ�
inline static void* SystemAlloc(size_t kpage)
{
#ifdef _WIN32 // Windows�µ�ϵͳ���ýӿ�
	void* ptr = VirtualAlloc(0, kpage << PAGE_SHIFT, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
	// linux��brk mmap��
#endif

	if (ptr == nullptr)
		throw std::bad_alloc();

	return ptr;
}

// ֱ��ȥ�����ͷſռ�
inline static void SystemFree(void* ptr)
{
#ifdef _WIN32
	VirtualFree(ptr, 0, MEM_RELEASE);
#else
	// sbrk unmmap��
#endif
}

static void*& ObjNext(void* obj) // obj��ͷ4/8���ֽ�
{ 
	return *(void**)obj;
}

class SizeClass; 

#include"ObjectPool.h"
#include"PageMap.h"

class FreeList // ThreadCache�е���������
{
public:
	// ��ȡ��ǰͰ���ж��ٿ�ռ�
	size_t Size()
	{
		return _size;
	}

	// ɾ����Ͱ��n���飨ͷɾ��������ɾ���Ŀռ���Ϊ����Ͳ�������
	void PopRange(void*& start, void*& end, size_t n)
	{
		// ɾ���������ܳ���size��
		assert(n <= _size);

		start = end = _freeList;

		for (size_t i = 0; i < n - 1; ++i)
		{
			end = ObjNext(end);
		}

		_freeList = ObjNext(end);
		ObjNext(end) = nullptr;
		_size -= n;
	}

	// ������������ͷ�壬�Ҳ�����ռ�
	void PushRange(void* start, void* end, size_t size)
	{
		ObjNext(end) = _freeList;
		_freeList = start;

		_size += size;
	}

	bool Empty() // �жϹ�ϣͰ�Ƿ�Ϊ��
	{
		return _freeList == nullptr;
	}

	void Push(void* obj) // �������տռ��
	{ // ͷ��
		assert(obj); // ����ǿտռ�

		ObjNext(obj) = _freeList;
		_freeList = obj;

		++_size; // ����һ�飬size + 1
	}

	void* Pop() // �����ṩ�ռ��
	{ // ͷɾ
		assert(_freeList); // �ṩ�ռ��ǰ����Ҫ�пռ�

		void* obj = _freeList;
		_freeList = ObjNext(obj);

		--_size; // ȥ��һ�飬_size - 1

		return obj;
	}

	// FreeList��ǰδ������ʱ���ܹ����������ռ��Ƕ���
	size_t& MaxSize()
	{
		return _maxSize;
	}

private:
	void* _freeList = nullptr; // ����������ʼΪ��
	size_t _maxSize = 1; // ��ǰ������������δ�ﵽ����ʱ���ܹ����������ռ��Ƕ���
						 // ��ʼֵ��1����ʾ��һ��������ľ���1��
						 // ��������֮��_maxSize���ֵ��������
	size_t _size = 0; // ��ǰ�����������ж��ٿ�ռ�
};

struct Span // ��ҳΪ������λ�Ľṹ��
{
public:
	PageID _pageID = 0; // ҳ��
	size_t _n = 0; // ��ǰspan�����ҳ������
	size_t _objSize = 0; // span����ҳ���зֳɵĿ��ж��

	void* _freeList = nullptr; // ÿ��span����ҵ�С��ռ��ͷ���
	size_t use_count = 0; // ��ǰspan�����ȥ�˶��ٸ���ռ�


	Span* _prev = nullptr; // ǰһ���ڵ�
	Span* _next = nullptr; // ��һ���ڵ�

	bool _isUse = false; // �жϵ�ǰspan����cc�л�����pc��
};

class SpanList
{
public:
	// ɾ������һ��span
	Span* PopFront()
	{
		// �Ȼ�ȡ��_head����ĵ�һ��span
		Span* front = _head->_next;
		// ɾ�������span��ֱ�Ӹ���Erase
		Erase(front);

		// ����ԭ���ĵ�һ��span
		return front;
	}

	// �п�
	bool Empty()
	{ // ��ͷ˫��ѭ���յ�ʱ��_headָ���Լ�
		return _head == _head->_next;
	}

	// ͷ��
	void PushFront(Span* span)
	{
		Insert(Begin(), span);
	}

	// ͷ���
	Span* Begin()
	{
		return _head->_next;
	}

	// β���
	Span* End()
	{
		return _head;
	}

	void Erase(Span* pos)
	{
		assert(pos); // pos��Ϊ��
		assert(pos != _head); // pos�������ڱ�λ

		Span* prev = pos->_prev;
		Span* next = pos->_next;

		prev->_next = next;
		next->_prev = prev;

		/*pos�ڵ㲻��Ҫ����deleteɾ������Ϊ
		pos�ڵ��Span��Ҫ���գ�������ֱ��ɾ��*/
		// ��������߼�
	}

	void Insert(Span* pos, Span* ptr)
	{ // ��posǰ�����ptr
		assert(pos); // pos��Ϊ��
		assert(ptr); // ptr��Ϊ��

		// ��������߼�
		Span* prev = pos->_prev;

		prev->_next = ptr;
		ptr->_prev = prev;

		ptr->_next = pos;
		pos->_prev = ptr;
	}

	SpanList()
	{ // ���캯���и��ڱ�λͷ���
		_head = new Span;

		// ��Ϊ��˫��ѭ���ģ����Զ�ָ��_head
		_head->_next = _head;
		_head->_prev = _head;
	}

private:
	Span* _head; // �ڱ�λͷ���
public:
	std::mutex _mtx; // ÿ��CentralCache�еĹ�ϣͰ��Ҫ��һ��Ͱ��
};

class SizeClass
{
	// �߳�����size�Ķ������������������10%���ҵ�����Ƭ�˷�
	//	size��Χ				������				��Ӧ��ϣͰ�±귶Χ
	 // [1,128]					8B ���� �0�2 �0�2 �0�2		freelist[0,16)
	 // [128+1,1024]			16B ���� �0�2			freelist[16,72)
	 // [1024+1,8*1024]			128B ���� �0�2			freelist[72,128)
	 // [8*1024+1,64*1024]		1024B ���� �0�2 �0�2		freelist[128,184)
	 // [64*1024+1,256*1024]	8*1024B ���� �0�2		freelist[184,208)
public:
	//// ����ÿ��������Ӧ�Ķ������ֽ���(��ͨд��)
	//static size_t _RoundUp(size_t size, size_t alignNum)
	//{									// alignNum��size��Ӧ�����Ķ�����
	//	size_t res = 0;
	//	if (size % alignNum != 0)
	//	{ // ��������Ҫ���һ�����룬����size = 3���������(3 / 8 + 1) * 8 = 8
	//		res = (size / alignNum + 1) * alignNum;
	//	}
	//	else
	//	{ // û��������������ܶ��룬����size = 8
	//		res = size;
	//	}

	//	return res;
	//}

	// ����ÿ��������Ӧ�Ķ������ֽ���(����д��)
	static size_t _RoundUp(size_t size, size_t alignNum)
	{									// alignNum��size��Ӧ�����Ķ�����
		return ((size + alignNum - 1) & ~(alignNum - 1));
	}

	static size_t RoundUp(size_t size) // ����������ֽ�����sizeΪ�߳�����Ŀռ��С
	{
		if (size <= 128)
		{ // [1,128] 8B
			return _RoundUp(size, 8);
		}
		else if (size <= 1024)
		{ // [128+1,1024] 16B
			return _RoundUp(size, 16);
		}
		else if (size <= 8 * 1024)
		{ // [1024+1,8*1024] 128B
			return _RoundUp(size, 128);
		}
		else if (size <= 64 * 1024)
		{ // [8*1024+1,64*1024] 1024B
			return _RoundUp(size, 1024);
		}
		else if (size <= 256 * 1024)
		{ // [64*1024+1,256*1024] 8 * 1024B
			return _RoundUp(size, 8 * 1024);
		}
		else
		{ // ��������ռ����256KB��ֱ�Ӱ���ҳ������
			return _RoundUp(size, 1 << PAGE_SHIFT);
			// ����ֱ�Ӹ�PAGE_SHIFT��Ȼ˵��ǰһ����8 * 1024KBһ������
			// ���������������Ҫ�޸�ҳ��С��ʱ���ֱ���޸�PAGE_SHIFT
			// �����ͺ�ǰ��Ĳ�һ����
		}
	}

	// ��size��Ӧ�ڹ�ϣ���е��±꣨����д����
	static inline size_t _Index(size_t size, size_t align_shift)
	{							/*����align_shift��ָ�������Ķ�����λ��������sizeΪ2��ʱ�������
								Ϊ8��8����2^3�����Դ�ʱalign_shift����3*/
		return ((size + (1 << align_shift) - 1) >> align_shift) - 1;
		//����_Index������ǵ�ǰsize��������ĵڼ����±꣬����Index�ķ���ֵ��Ҫ����ǰ����������Ĺ�ϣͰ�ĸ���
	}

	// ����ӳ�����һ����������Ͱ��tc��cc�ã�����ӳ�����һ����
	static inline size_t Index(size_t size)
	{
		assert(size <= MAX_BYTES);

		// ÿ�������ж��ٸ���
		static int group_array[4] = { 16, 56, 56, 56 };
		if (size <= 128)
		{ // [1,128] 8B -->8B����2^3B����Ӧ������λΪ3λ
			return _Index(size, 3); // 3��ָ�������Ķ�����λλ��������8B����2^3B�����Ծ���3
		}
		else if (size <= 1024)
		{ // [128+1,1024] 16B -->4λ
			return _Index(size - 128, 4) + group_array[0];
		}
		else if (size <= 8 * 1024)
		{ // [1024+1,8*1024] 128B -->7λ
			return _Index(size - 1024, 7) + group_array[1] + group_array[0];
		}
		else if (size <= 64 * 1024)
		{ // [8*1024+1,64*1024] 1024B -->10λ
			return _Index(size - 8 * 1024, 10) + group_array[2] + group_array[1]
				+ group_array[0];
		}
		else if (size <= 256 * 1024)
		{ // [64*1024+1,256*1024] 8 * 1024B  -->13λ
			return _Index(size - 64 * 1024, PAGE_SHIFT) + group_array[3] +
				group_array[2] + group_array[1] + group_array[0];
		}
		else
		{
			assert(false);
		}
		return -1;
	}

	// tc��cc���������ռ�����޿���
	static size_t NumMoveSize(size_t size)
	{
		assert(size > 0); // ��������0��С�Ŀռ�

		// MAX_BYTES���ǵ���������ռ䣬Ҳ����256KB
		int num = MAX_BYTES / size; // �����֮���ȼ򵥿���һ��

		if (num > 512)
		{
			/*����˵�����������8B��256KB����8B�õ�����һ��������
			������������������������̫���ˣ�ֱ�ӿ����������ܻ���
			�ɺܶ��˷ѵĿռ䣬��̫��ʵ�����Ը�Сһ��*/
			num = 512;
		}

		// ���˵����֮���ر�С����2С����ô�͵���2
		if (num < 2)
		{
			/*����˵�����������256KB���ǳ���1�����256KB����һֱ��1
			���������е�̫���ˣ������߳�Ҫ����4��256KB���ǽ�num�ĳ�2
			�Ϳ����ٵ��ü��Σ�Ҳ�ͻ��ټ��ο���������Ҳ����̫�࣬256KB
			�ռ��Ǻܴ�ģ�num̫���˲�̫��ʵ�����ܻ�����˷�*/
			num = 2;
		}
		
		// [2, 512]��һ�������ƶ����ٸ������(������)����ֵ
		// С����һ���������޸�
		// С����һ���������޵�

		return num;
	}

	// ��ҳƥ���㷨
	static size_t NumMovePage(size_t size)// size��ʾһ��Ĵ�С
	{ // ��cc��û��spanΪtc�ṩС��ռ�ʱ��cc����Ҫ��pc����һ��span����ʱ��Ҫ����һ��ռ�Ĵ�С��ƥ��
	  // ��һ��ά��ҳ�ռ��Ϊ���ʵ�span���Ա�֤spanΪsize�������˷ѻ��㹻����Ƶ��������ͬ��С��span

		// NumMoveSize�����tc��cc����size��С�Ŀ�ʱ�ĵ�������������
		size_t num = NumMoveSize(size);
		
		// num * size���ǵ����������ռ��С
		size_t npage = num * size;

		/* PAGE_SHIFT��ʾһҳҪռ�ö���λ������һҳ8KB����13λ��������
		 ����ʵ���ǳ���ҳ��С����������ǵ����������ռ��ж���ҳ*/
		npage >>= PAGE_SHIFT;
	
		/*��������Ϊ0���Ǿ�ֱ�Ӹ�1ҳ������˵sizeΪ8Bʱ��num����512��npage
		���������4KB�������һҳ8KB�������ֱ��Ϊ0�ˣ���˼���ǰ�ҳ�Ŀռ䶼
		��8B�ĵ�����������ռ��ˣ����Ƕ�������û��0.5������ֻ�ܸ�1ҳ*/
		if (npage == 0)
			npage = 1;
		
		return npage;
	}
};

