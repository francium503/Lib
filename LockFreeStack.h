#pragma once

#include <Windows.h>

namespace NetLib {

	template<class T>
	class LockFreeStack
	{
		struct Node
		{
			T data;
			Node* pNextNode;
		};
	public:
		LockFreeStack();
		~LockFreeStack();

		bool Push(T data);
		bool Pop(T* data);


	protected:
		__declspec(align(16)) LONG64 stackInfo[2];
	};

#define popCounter stackInfo[0]
#define _top (Node *)stackInfo[1]

	template <class T>
	LockFreeStack<T>::LockFreeStack()
	{
		popCounter = 0;
		stackInfo[1] = NULL;
	}

	template <class T>
	LockFreeStack<T>::~LockFreeStack()
	{
		while(_top != NULL)
		{
			Node *tmp = _top;
			stackInfo[1] = (LONG64)(_top)->pNextNode;

			delete tmp;
		}
	}

	template <class T>
	bool LockFreeStack<T>::Push(T data)
	{
		LONG64 counter;
		Node* top;
		Node* tmp = nullptr;
		LONG64 arr[2];
		do{
			counter = popCounter;
			top = _top;
			if(tmp == nullptr)
				tmp = new Node;

			tmp->data = data;
			tmp->pNextNode = top;

			arr[0] = counter;
			arr[1] = (LONG64)top;
		} while (InterlockedCompareExchange128(stackInfo, (LONG64)tmp, counter + 1, arr) != 1);

		return true;
	}

	template <class T>
	bool LockFreeStack<T>::Pop(T* data)
	{
		LONG64 counter;
		Node* popNode;
		Node* newTop;
		LONG64 arr[2];

		/*if(_top == nullptr)
		{
			return false;
		}*/

		do
		{
			counter = popCounter;
			popNode = _top;

			if(popNode == nullptr)
			{
				return false;
			}

			newTop = popNode->pNextNode;
	
			arr[0] = counter;
			arr[1] = (LONG64)popNode;
		} while (InterlockedCompareExchange128(stackInfo, (LONG64)newTop, counter + 1, arr) != 1);

		*data = popNode->data;
		delete popNode;

		return true;
	}

#undef _top
#undef popCounter
}
