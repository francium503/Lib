#include "pch.h"
#include "StreamQ.h"

StreamQ::StreamQ(void)
{
	m_pBuffer = new char[eBuffer_DEFAULT];
	m_front = 0;
	m_rear = 1;
	m_bufferSize = eBuffer_DEFAULT;

	InitializeCriticalSection(&m_streamQCS);
}

StreamQ::StreamQ(int iBufferSize)
{
	m_pBuffer = new char[iBufferSize];
	m_front = 0;
	m_rear = 1;
	m_bufferSize = iBufferSize;

	InitializeCriticalSection(&m_streamQCS);
}

StreamQ::~StreamQ()
{
	delete[] m_pBuffer;

	DeleteCriticalSection(&m_streamQCS);
}

void StreamQ::ReSize(int iSize)
{
	delete[] m_pBuffer;

	m_pBuffer = new char[iSize];

	m_front = 0;
	m_rear = 1;
	m_bufferSize = iSize;
}

int StreamQ::GetBufferSize(void)
{
	return m_bufferSize;
}

int StreamQ::GetUseSize(void)
{
	if (m_rear < m_front) {
		return (m_rear)+(m_bufferSize - (m_front + 1));
	}
	else {
		return m_rear - m_front - 1;
	}
}

int StreamQ::GetFreeSize(void)
{
	if (m_rear < m_front) {
		return m_front - m_rear;
	}
	else {
		return m_front + (m_bufferSize - m_rear);
	}
}

int StreamQ::GetNotBrokenGetSize(void)
{
	if (m_rear > m_front) {
		return m_rear - m_front - 1;
	}
	else {
		if (m_front + 1 == m_bufferSize) {
			return m_rear;
		}
		else {
			return m_bufferSize - ((m_front + 1) % m_bufferSize);
		}
	}
}

int StreamQ::GetNotBrokenPutSize(void)
{
	if (m_rear > m_front) {
		return m_bufferSize - m_rear;
	}
	else {
		return m_front - m_rear;
	}
}

BOOL StreamQ::Enqueue(char * chpData, int iSize)
{
	int front = m_front;
	int rear = m_rear;
	int bufferSize = m_bufferSize;
	int putSize;
	char* pRear = &m_pBuffer[(rear) % bufferSize];

	if (rear > front) {
		putSize = bufferSize - rear;
	}
	else {
		putSize = front - rear;
	}

	if (putSize >= iSize) {
		memcpy_s(pRear, iSize, chpData, iSize);
		m_rear = (rear + iSize) % bufferSize;

		return TRUE;
	}
	else {
		memcpy_s(pRear, putSize, chpData, putSize);

		int tmpRear = (rear + putSize) % bufferSize;

		int dataSize = iSize - putSize;
		pRear = &m_pBuffer[(tmpRear) % m_bufferSize];

		if (tmpRear > front) {
			putSize = bufferSize - tmpRear;
		}
		else {
			putSize = front - tmpRear;
		}

		if (putSize >= dataSize) {
			memcpy_s(pRear, dataSize, &chpData[iSize - dataSize], dataSize);
			m_rear = (rear + iSize) % bufferSize;
			

			return TRUE;
		}
		else {
			return FALSE;
		}
	}
}

BOOL StreamQ::Dequeue(char * chpDest, int iSize)
{
	int front = m_front;
	int rear = m_rear;
	int bufferSize = m_bufferSize;
	int getSize;

	if (rear > front) {
		getSize = rear - front - 1;
	}
	else {
		if (front + 1 == bufferSize) {
			getSize = rear;
		}
		else {
			getSize = bufferSize - ((front + 1) % bufferSize);
		}
	}


	char* pFront = &m_pBuffer[(front + 1) % bufferSize];

	if (getSize >= iSize) {
		memcpy_s(chpDest, iSize, pFront, iSize);
		m_front = (front + iSize) % bufferSize;

		return TRUE;
	}
	else {
		memcpy_s(chpDest, getSize, pFront, getSize);

		int tmpFront = (front + getSize) % bufferSize;

		int dataSize = iSize - getSize;

		pFront = &m_pBuffer[(tmpFront + 1) % bufferSize];

		if (rear > tmpFront) {
			getSize = rear - tmpFront - 1;
		}
		else {
			if (tmpFront + 1 == bufferSize) {
				getSize = rear;
			}
			else {
				getSize = bufferSize - ((tmpFront + 1) % bufferSize);
			}
		}

		if (getSize >= dataSize) {
			memcpy_s(&chpDest[iSize - dataSize], dataSize, pFront, dataSize);
			m_front = (front + iSize) % bufferSize;

			return TRUE;
		}
		else {
			return FALSE;
		}
	}
}

int StreamQ::Peek(char * chpDest, int iSize)
{
	int getSize = GetNotBrokenGetSize();
	char* pFront = &m_pBuffer[(m_front + 1) % m_bufferSize];
	int tmpFront = m_front;

	if (getSize > iSize) {
		memcpy_s(chpDest, iSize, pFront, iSize);

		return iSize;
	}
	else {
		memcpy_s(chpDest, getSize, pFront, getSize);
		MoveFront(getSize);

		int dataSize = iSize - getSize;

		pFront = &m_pBuffer[(m_front + 1) % m_bufferSize];

		getSize = GetNotBrokenGetSize();

		if (getSize > dataSize) {
			memcpy_s(&chpDest[iSize - dataSize], dataSize, pFront, dataSize);
			MoveFront(dataSize);
			m_front = tmpFront;

			return iSize;
		}
		else {
			memcpy_s(&chpDest[iSize - dataSize], getSize, pFront, getSize);
			MoveFront(getSize);
			m_front = tmpFront;

			return iSize - dataSize + getSize;
		}
	}
}

int StreamQ::MoveRear(int iSize)
{
	m_rear = (m_rear + iSize) % m_bufferSize;

	return iSize;
}

int StreamQ::MoveFront(int iSize)
{
	m_front = (m_front + iSize) % m_bufferSize;

	return iSize;
}

void StreamQ::ClearBuffer(void)
{
	m_front = 0;
	m_rear = 1;
}

char * StreamQ::GetFrontBufferPtr(void)
{
	return &m_pBuffer[(m_front + 1) % m_bufferSize];
}

char * StreamQ::GetRearBufferPtr(void)
{
	return &m_pBuffer[(m_rear) % m_bufferSize];
}

char * StreamQ::GetBufferStartPtr(void)
{
	return m_pBuffer;
}

void StreamQ::Lock(void)
{
	EnterCriticalSection(&m_streamQCS);
}

void StreamQ::Release(void)
{
	LeaveCriticalSection(&m_streamQCS);
}
