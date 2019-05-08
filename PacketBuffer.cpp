#include "pch.h"
#include "PacketBuffer.h"


NetLib::PacketBuffer::PacketBuffer()
{
	m_chpBuffer = new char[eBuffer_DEFAULT];
	m_iBufferSize = eBuffer_DEFAULT;
	m_iDataSize = 0;
	m_iReadPos = 0;
	m_iWritePos = 0;
}

NetLib::PacketBuffer::PacketBuffer(int iBuffSize)
{
	m_chpBuffer = new char[iBuffSize];
	m_iBufferSize = iBuffSize;
	m_iDataSize = 0;
	m_iReadPos = 0;
	m_iWritePos = 0;
}


NetLib::PacketBuffer::~PacketBuffer()
{
	delete[] m_chpBuffer;
}

void NetLib::PacketBuffer::Release(void)
{
	delete[] m_chpBuffer;
}

void NetLib::PacketBuffer::Clear(void)
{
	m_iReadPos = 0;
	m_iWritePos = 0;
	m_iDataSize = 0;
}

int NetLib::PacketBuffer::MoveWritePos(int iPos)
{
	if (m_iWritePos + iPos >= m_iBufferSize) {
		int tmp = m_iBufferSize - m_iWritePos - 1;
		m_iWritePos = m_iBufferSize - 1;
		return tmp;
	}
	else {
		m_iWritePos += iPos;
		return iPos;
	}
}

int NetLib::PacketBuffer::MoveReadPos(int iPos)
{
	if (m_iReadPos + iPos >= m_iBufferSize) {
		int tmp = m_iBufferSize - m_iReadPos - 1;
		m_iReadPos = m_iBufferSize - 1;
		return tmp;
	}
	else {
		m_iReadPos += iPos;
		return iPos;
	}
}

NetLib::PacketBuffer & NetLib::PacketBuffer::operator=(PacketBuffer & rhs)
{
	this->m_iBufferSize = rhs.m_iBufferSize;
	this->m_iDataSize = rhs.m_iDataSize;
	this->m_iWritePos = rhs.m_iWritePos;
	this->m_iReadPos = rhs.m_iReadPos;

	delete[] m_chpBuffer;

	m_chpBuffer = new char[m_iBufferSize];

	memcpy_s(m_chpBuffer, m_iDataSize, rhs.m_chpBuffer, m_iDataSize);

	return *this;
}

NetLib::PacketBuffer & NetLib::PacketBuffer::operator<<(BYTE brhs)
{
	int size = sizeof(BYTE);
	if (m_iWritePos + size >= m_iBufferSize)
		return *this;

	memcpy_s(&m_chpBuffer[m_iWritePos], size, &brhs, size);
	m_iWritePos += size;

	return *this;
}

NetLib::PacketBuffer & NetLib::PacketBuffer::operator<<(char chrhs)
{
	int size = sizeof(char);
	if (m_iWritePos + size >= m_iBufferSize)
		return *this;

	memcpy_s(&m_chpBuffer[m_iWritePos], size, &chrhs, size);
	m_iWritePos += size;

	return *this;
}

NetLib::PacketBuffer & NetLib::PacketBuffer::operator<<(short shrhs)
{
	int size = sizeof(short);
	if (m_iWritePos + size >= m_iBufferSize)
		return *this;

	memcpy_s(&m_chpBuffer[m_iWritePos], size, &shrhs, size);
	m_iWritePos += size;

	return *this;
}
NetLib::PacketBuffer & NetLib::PacketBuffer::operator<<(int irhs)
{
	int size = sizeof(int);
	if (m_iWritePos + size >= m_iBufferSize)
		return *this;

	memcpy_s(&m_chpBuffer[m_iWritePos], size, &irhs, size);
	m_iWritePos += size;

	return *this;
}

NetLib::PacketBuffer & NetLib::PacketBuffer::operator<<(float frhs)
{
	int size = sizeof(float);
	if (m_iWritePos + size >= m_iBufferSize)
		return *this;

	memcpy_s(&m_chpBuffer[m_iWritePos], size, &frhs, size);
	m_iWritePos += size;

	return *this;
}

NetLib::PacketBuffer & NetLib::PacketBuffer::operator<<(double drhs)
{
	int size = sizeof(double);
	if (m_iWritePos + size >= m_iBufferSize)
		return *this;

	memcpy_s(&m_chpBuffer[m_iWritePos], size, &drhs, size);
	m_iWritePos += size;

	return *this;
}

NetLib::PacketBuffer & NetLib::PacketBuffer::operator<<(__int64 i64rhs)
{
	int size = sizeof(__int64);
	if (m_iWritePos + size >= m_iBufferSize)
		return *this;

	memcpy_s(&m_chpBuffer[m_iWritePos], size, &i64rhs, size);
	m_iWritePos += size;

	return *this;
}

NetLib::PacketBuffer & NetLib::PacketBuffer::operator>>(BYTE & brhs)
{
	int size = sizeof(BYTE);
	if (m_iReadPos + size > m_iWritePos)
		return *this;

	memcpy_s(&brhs, size , &m_chpBuffer[m_iReadPos], size);
	m_iReadPos += size;

	return *this;
}

NetLib::PacketBuffer & NetLib::PacketBuffer::operator>>(char & chrhs)
{
	int size = sizeof(char);
	if (m_iReadPos + size > m_iWritePos)
		return *this;

	memcpy_s(&chrhs, size, &m_chpBuffer[m_iReadPos], size);
	m_iReadPos += size;

	return *this;
}

NetLib::PacketBuffer & NetLib::PacketBuffer::operator>>(short & shrhs)
{
	int size = sizeof(short);
	if (m_iReadPos + size > m_iWritePos)
		return *this;

	memcpy_s(&shrhs, size, &m_chpBuffer[m_iReadPos], size);
	m_iReadPos += size;

	return *this;
}

NetLib::PacketBuffer & NetLib::PacketBuffer::operator>>(int & irhs)
{
	int size = sizeof(int);
	if (m_iReadPos + size > m_iWritePos)
		return *this;

	memcpy_s(&irhs, size, &m_chpBuffer[m_iReadPos], size);
	m_iReadPos += size;

	return *this;
}

NetLib::PacketBuffer & NetLib::PacketBuffer::operator>>(float & frhs)
{
	int size = sizeof(float);
	if (m_iReadPos + size > m_iWritePos)
		return *this;

	memcpy_s(&frhs, size, &m_chpBuffer[m_iReadPos], size);
	m_iReadPos += size;

	return *this;
}

NetLib::PacketBuffer & NetLib::PacketBuffer::operator>>(double & drhs)
{
	int size = sizeof(double);
	if (m_iReadPos + size > m_iWritePos)
		return *this;

	memcpy_s(&drhs, size, &m_chpBuffer[m_iReadPos], size);
	m_iReadPos += size;

	return *this;
}

NetLib::PacketBuffer & NetLib::PacketBuffer::operator>>(__int64 & i64rhs)
{
	int size = sizeof(__int64);
	if (m_iReadPos + size > m_iWritePos)
		return *this;

	memcpy_s(&i64rhs, size, &m_chpBuffer[m_iReadPos], size);
	m_iReadPos += size;

	return *this;
}


int NetLib::PacketBuffer::GetData(char * chpDest, int iGetSize)
{
	if (m_iReadPos + iGetSize > m_iWritePos)
		return -1;

	memcpy_s(chpDest, iGetSize, &m_chpBuffer[m_iReadPos], iGetSize);

	return iGetSize;
}

int NetLib::PacketBuffer::PutData(char * chpSrc, int iPutSize)
{
	if (m_iWritePos + iPutSize >= m_iBufferSize)
		return -1;

	memcpy_s(chpSrc, iPutSize, &m_chpBuffer[m_iWritePos], iPutSize);

	return iPutSize;
}

