#pragma once

#include <new>

template <class T>
class ObjectFreeList
{
	struct st_BLOCK_NODE
	{
		st_BLOCK_NODE()
		{
			pNextNode = nullptr;
		}

		T* data;
		st_BLOCK_NODE *pNextNode;
		char chChecksum;
	};


public:
	ObjectFreeList(int iBlockNum, bool bPlacementNew = false, char checksum = 0x12);
	virtual ~ObjectFreeList() throw();

	// alloc 해주기
	T *Alloc(void);

	// 메모리 반환
	bool Free(T *pData);

	// alloc 전체 갯수 반환
	int GetAllocCount(void) { return m_iAllocCount; }

	// 사용중인 블럭 갯수 반환
	int GetUseCount(void) { return m_iUseCount; }

protected:
	st_BLOCK_NODE *m_pFreeNode;
	int m_iAllocCount;
	int m_iUseCount;
	bool m_bPlacementNew;
	char m_chChecksum;
};

template<class T>
inline ObjectFreeList<T>::ObjectFreeList(int iBlockNum, bool bPlacementNew, char checksum)
{
	m_pFreeNode = nullptr;
	m_iAllocCount = 0;
	m_iUseCount = 0;
	m_bPlacementNew = bPlacementNew;
	m_chChecksum = checksum;

	for (int i = 0; i < iBlockNum; ++i) {
		if (m_pFreeNode != nullptr) {
			st_BLOCK_NODE *tmp = new st_BLOCK_NODE;
			tmp->chChecksum = checksum;
			tmp->pNextNode = m_pFreeNode;
			m_pFreeNode = tmp;
			m_iAllocCount++;
		}
		else {
			m_pFreeNode = new st_BLOCK_NODE;
			m_pFreeNode->chChecksum = checksum;
			m_iAllocCount++;
		}
	}
}

template<class T>
inline ObjectFreeList<T>::~ObjectFreeList()
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
inline T * ObjectFreeList<T>::Alloc(void)
{
	st_BLOCK_NODE *tmp;

	if (m_pFreeNode == nullptr) {
		if (m_bPlacementNew) {
			tmp = new st_BLOCK_NODE;
			tmp->data = new T;
			tmp->chChecksum = m_chChecksum;
		}
		else {
			tmp = new st_BLOCK_NODE;
			tmp->data = (T*)malloc(sizeof(T));
			tmp->chChecksum = m_chChecksum;
		}
		m_iAllocCount++;
	}
	else {
		if (m_bPlacementNew) {
			m_pFreeNode->data = new (m_pFreeNode->data)T;
		}

		tmp = m_pFreeNode;
		m_pFreeNode = m_pFreeNode->pNextNode;
	}
	m_iUseCount++;
	return (T*)tmp;
}

template<class T>
inline bool ObjectFreeList<T>::Free(T * pData)
{
	st_BLOCK_NODE *tmp = (st_BLOCK_NODE *)pData;

	if (tmp->chChecksum != m_chChecksum) {
		return false;
	}

	if (m_bPlacementNew) {
		tmp->data->~T();
	}

	tmp->pNextNode = m_pFreeNode;

	m_pFreeNode = tmp;
	m_iUseCount--;
	return true;
}

