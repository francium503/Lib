#pragma once
#include "LockFreeStack.h"
#include "MiniDump.h"

namespace NetLib {
	template <class T>
	class ObjectFreeList
	{
		struct st_BLOCK_NODE
		{
			T data;
			char chChecksum;
		};

	public:
		ObjectFreeList(bool bPlacementNew = false, char checksum = 0x12);
		virtual ~ObjectFreeList();

		// alloc 해주기
		T *Alloc(void);

		// 메모리 반환
		bool Free(T *pData);

		// alloc 전체 갯수 반환
		int GetAllocCount(void) { return m_iAllocCount; }

		// 사용중인 블럭 갯수 반환
		int GetUseCount(void) { return m_iUseCount; }

	protected:
		LockFreeStack<st_BLOCK_NODE *> pFreeStack;
		long m_iAllocCount;
		long m_iUseCount;
		bool m_bPlacementNew;
		char m_chChecksum;
	public:
	};

	template<class T>
	ObjectFreeList<T>::ObjectFreeList(bool bPlacementNew, char checksum)
	{
		m_iAllocCount = 0;
		m_iUseCount = 0;
		m_bPlacementNew = bPlacementNew;
		m_chChecksum = checksum;
	}

	template<class T>
	ObjectFreeList<T>::~ObjectFreeList()
	{
		st_BLOCK_NODE *tmp;
		while(pFreeStack.Pop(&tmp))
		{
			delete tmp;
		}
	}

	template<class T>
	T * ObjectFreeList<T>::Alloc(void)
	{
		st_BLOCK_NODE *tmp = nullptr;

		if (!pFreeStack.Pop(&tmp)) {
			tmp = new st_BLOCK_NODE;
			tmp->chChecksum = m_chChecksum;

			InterlockedIncrement(&m_iAllocCount);
		}
		else {
			if (m_bPlacementNew) {
				new (&tmp->data) T;
			}
		}

		InterlockedIncrement(&m_iUseCount);

		return (T *)tmp;
	}

	template<class T>
	bool ObjectFreeList<T>::Free(T * pData)
	{
		st_BLOCK_NODE *tmp = (st_BLOCK_NODE *)pData;

		if (tmp->chChecksum != m_chChecksum) {
			return false;
		}

		if (m_bPlacementNew) {
			tmp->data.~T();
		}

		if(!pFreeStack.Push(tmp))
		{
			CrashDump::Crash();
			return false;
		}

		InterlockedDecrement(&m_iUseCount);
		return true;
	}

}