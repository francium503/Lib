#pragma once

#include <Windows.h>

#include "ObjectFreeList.h"
#include "ObjectFreeListTLS.h"

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

#define _pHeadNode (Node *)headInfo[0]
#define _pHeadCounter headInfo[1]
#define _pTailNode (Node *)tailInfo[0]
#define _pTailCounter tailInfo[1]

	public:
		LockFreeQueue()
		{
			_size = 0;
			headInfo[0] = (LONG64)new Node;
			Node* pH = _pHeadNode;
			pH->pNextNode = nullptr;
			_pHeadCounter = 0;
			tailInfo[0] = headInfo[0];
			_pTailCounter = 0;

		}

		bool Enqueue(T data)
		{
			Node* node = memoryPool.Alloc();
			node->data = data;
			node->pNextNode = nullptr;
			LONG64 arr[2];

			while (true)
			{
				LONG64 counter = _pTailCounter;
				Node* tail = _pTailNode;
				Node* next = tail->pNextNode;

				if (next == nullptr)
				{
					if (InterlockedCompareExchangePointer((PVOID *)&tail->pNextNode, node, next) == next) {
						arr[0] = (LONG64)tail;
						arr[1] = counter;

						InterlockedCompareExchange128(tailInfo, counter + 1, (LONG64)node, arr);
						break;
					}
				}
				else
				{
					InterlockedCompareExchangePointer((PVOID *)&tailInfo[0], next, tail);
				}
			}

			InterlockedIncrement(&_size);

			return true;
		}

		bool Dequeue(T& data)
		{
			while (true)
			{
				if (_size == 0)
					return false;

				LONG64 counter = _pHeadCounter;
				Node* head = _pHeadNode;
				Node* next = head->pNextNode;
				Node* tail = _pTailNode;
				LONG64 arr[2];

				if (head == tail) {
					if (next == nullptr) {
						return false;
					}
				}
				else {
					if (next != nullptr)
					{
						data = next->data;

						arr[0] = (LONG64)head;
						arr[1] = counter;

						if (InterlockedCompareExchange128(headInfo, counter + 1, (LONG64)next, arr) == 1) {
							memoryPool.Free(head);
							break;
						}
					}
				}
			}

			InterlockedDecrement(&_size);

			return true;
		}

		int GetSize()
		{
			return _size;
		}

	private:
		long _size;
		_declspec(align(16)) LONG64 tailInfo[2];
		_declspec(align(16)) LONG64 headInfo[2];

		ObjectFreeListTLS<Node> memoryPool = ObjectFreeListTLS<Node>(false, 0x12);
	};
}