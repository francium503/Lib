#pragma once

namespace NetLib {
	template <class T>
	class ObjectFreeList
	{
		struct st_BLOCK_NODE
		{
			T data;
			st_BLOCK_NODE *pNextNode;
			char chChecksum;

			st_BLOCK_NODE()
			{
				pNextNode = nullptr;
			}
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
		st_BLOCK_NODE * volatile m_pFreeNode;
		long m_iAllocCount;
		long m_iUseCount;
		bool m_bPlacementNew;
		char m_chChecksum;
		SRWLOCK srwLock;
	};

	template<class T>
	ObjectFreeList<T>::ObjectFreeList(bool bPlacementNew, char checksum)
	{
		m_pFreeNode = nullptr;
		m_iAllocCount = 0;
		m_iUseCount = 0;
		m_bPlacementNew = bPlacementNew;
		m_chChecksum = checksum;
		InitializeSRWLock(&srwLock);
	}

	template<class T>
	ObjectFreeList<T>::~ObjectFreeList()
	{
		for (int i = 0; i < m_iAllocCount - m_iUseCount; ++i) {
			if (m_pFreeNode == nullptr)
				break;

			if (m_pFreeNode->chChecksum == m_chChecksum) {
				st_BLOCK_NODE *tmp = m_pFreeNode->pNextNode;

				delete m_pFreeNode;

				m_pFreeNode = tmp;
			}
			else {
				throw (100);
			}
		}
	}

	template<class T>
	T * ObjectFreeList<T>::Alloc(void)
	{
		AcquireSRWLockExclusive(&srwLock);
		st_BLOCK_NODE *tmp = nullptr;

		if (m_pFreeNode == nullptr) {
			tmp = new st_BLOCK_NODE;
			tmp->chChecksum = m_chChecksum;

			InterlockedIncrement(&m_iAllocCount);
		}
		else {
			if (m_bPlacementNew) {
				new (&tmp->data) T;
			}
			tmp = m_pFreeNode;
			m_pFreeNode = tmp->pNextNode;
			tmp->pNextNode = nullptr;
		}

		InterlockedIncrement(&m_iUseCount);

		ReleaseSRWLockExclusive(&srwLock);

		return (T *)tmp;
	}

	template<class T>
	bool ObjectFreeList<T>::Free(T * pData)
	{
		AcquireSRWLockExclusive(&srwLock);
		st_BLOCK_NODE *tmp = (st_BLOCK_NODE *)pData;

		if (tmp->chChecksum != m_chChecksum) {
			ReleaseSRWLockExclusive(&srwLock);
			return false;
		}

		if (m_bPlacementNew) {
			tmp->data.~T();
		}

		tmp->pNextNode = m_pFreeNode;
		m_pFreeNode = tmp;

		InterlockedDecrement(&m_iUseCount);
		ReleaseSRWLockExclusive(&srwLock);
		return true;
	}

}