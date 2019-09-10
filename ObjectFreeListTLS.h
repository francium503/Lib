#pragma once

#include "ObjectFreeList.h"

namespace NetLib
{
#define Chunk_SIZE 2000

	template <class T>
	class ObjectFreeListTLS{

	public:
		ObjectFreeListTLS(bool bPlacementNew = false, char checksum = 0x12);
		~ObjectFreeListTLS();
		T* Alloc();
		bool Free(T* pData);

	protected:
		template <class T>
		class Chunk;

		template <class T>
		struct NodeT
		{
			T data;
			Chunk<T>* pChunk;
		};

		template <class T>
		class Chunk
		{
			friend ObjectFreeListTLS;
		public:
			
			NodeT<T> data[Chunk_SIZE];
			int index;
			int freeCount;
			ObjectFreeList<Chunk<T>>* pMemPool;
		public:
			Chunk()
			{
				index = 0;
				freeCount = 0;
				pMemPool = nullptr;
			}

			T* Alloc()
			{
				data[index].pChunk = this;

				return &data[index++].data;
			}

			bool Free(NodeT<T>* pData)
			{
				++freeCount;

				if(freeCount == Chunk_SIZE)
				{
					if(!pMemPool->Free(this))
					{
						return false;
					}
				}

				return true;
			}

			long GetSize()
			{
				return (Chunk_SIZE - index);
			}

			void Initialize(ObjectFreeList<Chunk<T>>* pMemPool)
			{
				index = 0;
				freeCount = 0;
				this->pMemPool = pMemPool;
			}
		};

	private:
		static DWORD tlsIndex;
		ObjectFreeList<Chunk<T>>* memoryPool;
		bool placementNew;
	};

	template <class T>
	ObjectFreeListTLS<T>::ObjectFreeListTLS(bool bPlacementNew, char checksum)
	{
		if (tlsIndex == TLS_OUT_OF_INDEXES)
		{
			tlsIndex = TlsAlloc();

			if (tlsIndex == TLS_OUT_OF_INDEXES)
			{
				DWORD error = GetLastError();
				CrashDump::Crash();
			}
		}

		memoryPool = new ObjectFreeList<Chunk<T>>(false, checksum);

		if(memoryPool == nullptr)
		{
			CrashDump::Crash();
		}
		placementNew = bPlacementNew;

		Chunk<T>* pChunk = memoryPool->Alloc();
		pChunk->Initialize(memoryPool);

		TlsSetValue(tlsIndex, (LPVOID)pChunk);
	}

	template <class T>
	ObjectFreeListTLS<T>::~ObjectFreeListTLS()
	{
		delete memoryPool;
	}

	template <class T>
	T* ObjectFreeListTLS<T>::Alloc()
	{
		Chunk<T>* pChunk = static_cast<Chunk<T> *>(TlsGetValue(tlsIndex));

		if(Chunk_SIZE - pChunk->index == 0)
		{
			pChunk = memoryPool->Alloc();
			pChunk->index = 0;
			pChunk->freeCount = 0;
			pChunk->pMemPool = memoryPool;

			TlsSetValue(tlsIndex, (LPVOID)pChunk);
		}

		pChunk->data[pChunk->index].pChunk = pChunk;
		T* pT = &pChunk->data[pChunk->index++].data;

		if(placementNew)
		{
			new (pT) T;
		}

		return pT;
	}

	template <class T>
	bool ObjectFreeListTLS<T>::Free(T* pData)
	{
		NodeT<T>*pTmp = (NodeT<T>*)pData;

		if(placementNew)
		{
			pTmp->data.~T();
		}

		return pTmp->pChunk->Free(pTmp);
	}

	template <class T>
	DWORD ObjectFreeListTLS<T>::tlsIndex = TlsAlloc();
}
