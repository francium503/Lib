#pragma once

#include <Windows.h>

#include "ObjectFreeList.h"

namespace NetLib
{
	template<class T>
	class LockFreeQueue
	{
		struct Node
		{
			T data;
			Node* pNextNode;
		};

	public:
		LockFreeQueue()
		{
			_size = 0;
			pHeadNode = new Node;
			pHeadNode->pNextNode = nullptr;
			pTailNode = pHeadNode;
		}

		bool Enqueue(T data)
		{
			Node* node = memoryPool.Alloc();
			node->data = data;
			node->pNextNode = nullptr;

			while(true)
			{
				Node* tail = pTailNode;
				Node* next = tail->pNextNode;

				if (tail == pTailNode) {
					if (next == nullptr)
					{
						if (InterlockedCompareExchangePointer((PVOID *)&tail->pNextNode, node, next) == next)
						{
							InterlockedCompareExchangePointer((PVOID *)&pTailNode, node, tail);
							break;
						}
					}else
					{
						InterlockedCompareExchangePointer((PVOID *)&pTailNode, next, tail);
					}
				}
			}

			InterlockedIncrement(&_size);
			InterlockedIncrement(&enqueueCount);

			return true;
		}

		bool Dequeue(T& data)
		{
			if (_size == 0)
				return false;

			while(true)
			{
				Node* head = pHeadNode;
				Node* next = pHeadNode->pNextNode;
				Node* tail = pTailNode;

				if (head == pHeadNode) {

					if (head == tail) {
						if (next == nullptr)
						{
							return false;
						}
					}
					else {
						data = next->data;
						
						if (InterlockedCompareExchangePointer((PVOID *)&pHeadNode, next, head) == head)
						{
							memoryPool.Free(head);
							break;
						}
					}
				}
			}

			InterlockedDecrement(&_size);
			InterlockedIncrement(&dequeueCount);
			
			return true;
		}

		int GetSize()
		{
			return _size;
		}

	private:
		long _size;
		Node* pHeadNode;
		Node* pTailNode;
		long enqueueCount = 0;
		long dequeueCount = 0;

		ObjectFreeList<Node> memoryPool = ObjectFreeList<Node>(false, 0x12);
	};
}