#include "pch.h"
#include "PacketBuffer.h"
#include "MiniDump.h"
#include "Log.h"

NetLib::ObjectFreeList<NetLib::PacketBuffer> NetLib::PacketBuffer::m_freeList = NetLib::ObjectFreeList<NetLib::PacketBuffer>();

NetLib::PacketBuffer::PacketBuffer()
{
	m_startBuffer = new unsigned char[eBuffer_DEFAULT + eHeader_Size];
	m_chpBuffer = &m_startBuffer[eHeader_Size];
	m_iBufferSize = eBuffer_DEFAULT;
	m_iReadPos = 0;
	m_iWritePos = 0;
	m_refCount = 0;
	m_isSet = true;
	m_bHeader = false;
}

NetLib::PacketBuffer::PacketBuffer(int iBuffSize)
{
	m_startBuffer = new unsigned char[iBuffSize + eHeader_Size];
	m_chpBuffer = &m_startBuffer[eHeader_Size];
	m_iBufferSize = iBuffSize;
	m_iReadPos = 0;
	m_iWritePos = 0;
	m_refCount = 0;
	m_isSet = true;
	m_bHeader = false;
}


NetLib::PacketBuffer::~PacketBuffer()
{
	m_isSet = false;
	delete m_startBuffer;
}

void NetLib::PacketBuffer::Release(void)
{
	m_iReadPos = 0;
	m_iWritePos = 0;
}

void NetLib::PacketBuffer::Clear(void)
{
	m_iReadPos = 0;
	m_iWritePos = 0;
	m_bHeader = false;
}

int NetLib::PacketBuffer::GetDataSize()
{
	return m_iWritePos - m_iReadPos;
}

int NetLib::PacketBuffer::MoveWritePos(int iPos)
{
	if (m_iWritePos + iPos >= m_iBufferSize) {
		int tmp = m_iBufferSize - m_iWritePos - 1;
		m_iWritePos = m_iBufferSize - 1;
		return tmp;
	}

	m_iWritePos += iPos;
	return iPos;
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
	this->m_iWritePos = rhs.m_iWritePos;
	this->m_iReadPos = rhs.m_iReadPos;

	delete[] m_chpBuffer;

	m_chpBuffer = new unsigned char[m_iBufferSize];

	memcpy_s(m_chpBuffer, m_iBufferSize, rhs.m_chpBuffer, m_iBufferSize);

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

NetLib::PacketBuffer& NetLib::PacketBuffer::operator<<(WORD wrhs)
{
	int size = sizeof(WORD);
	if (m_iWritePos + size >= m_iBufferSize)
		return *this;

	memcpy_s(&m_chpBuffer[m_iWritePos], size, &wrhs, size);
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

NetLib::PacketBuffer & NetLib::PacketBuffer::operator>>(WORD& wrhs)
{
	int size = sizeof(WORD);
	if (m_iReadPos + size > m_iWritePos)
		return *this;

	memcpy_s(&wrhs, size, &m_chpBuffer[m_iReadPos], size);
	m_iReadPos += size;

	return *this;
}


int NetLib::PacketBuffer::GetData(char * chpDest, int iGetSize)
{
	if (m_iReadPos + iGetSize > m_iWritePos)
		return -1;

	memcpy_s(chpDest, iGetSize, &m_chpBuffer[m_iReadPos], iGetSize);

	m_iReadPos += iGetSize;

	return iGetSize;
}

int NetLib::PacketBuffer::PutData(char * chpSrc, int iPutSize)
{
	if (m_iWritePos + iPutSize >= m_iBufferSize)
		return -1;

	memcpy_s(chpSrc, iPutSize, &m_chpBuffer[m_iWritePos], iPutSize);
	m_iWritePos += iPutSize;

	return iPutSize;
}

void NetLib::PacketBuffer::SetHeader(short* header)
{
	short *tmp = (short *)&m_startBuffer[3];

	*tmp = *header;
}

void NetLib::PacketBuffer::SetHeader(unsigned char code, unsigned char randKey, unsigned char hardKey)
{
	if (m_bHeader)
		return;

	m_bHeader = true;

	m_startBuffer[0] = code;
	m_startBuffer[3] = randKey;

	unsigned char p = 0;
	unsigned int checksum = 0;
	for (int i = 0; i < m_iWritePos - m_iReadPos; ++i)
	{
		checksum += m_chpBuffer[i];
	}
	m_startBuffer[4] = (unsigned char)(checksum % 256);

	for (int i = 0; i <= m_iWritePos - m_iReadPos; ++i)
	{
		p = m_startBuffer[4 + i] ^ (randKey + p + (i + 1));
		if (i == 0)
			m_startBuffer[4 + i] = p ^ (hardKey + (i + 1));
		else
			m_startBuffer[4 + i] = p ^ (hardKey + m_startBuffer[4 + i - 1] + (i + 1));
	}

	short *tmp = (short *)&m_startBuffer[1];

	*tmp = m_iWritePos - m_iReadPos;

}

void NetLib::PacketBuffer::SetLen()
{
	short *tmp = (short *)&m_startBuffer[1];

	*tmp = m_iWritePos - m_iReadPos;
}

short NetLib::PacketBuffer::GetLen()
{
	return *(short *)&m_startBuffer[1];
}

bool NetLib::PacketBuffer::DecryptPacket(unsigned char hardKey)
{
	unsigned char p = 0;
	unsigned char oldP = 0;
	unsigned char randKey = m_startBuffer[3];
	unsigned char oldData;
	int checksum = 0;


	for (int i = 0; i <= m_iWritePos - m_iReadPos; ++i)
	{
		if (i == 0) {
			p = m_startBuffer[4 + i] ^ (hardKey + i + 1);
			oldData = m_startBuffer[4 + i];
			m_startBuffer[4 + i] = p ^ (randKey + 1);
			oldP = p;
		}
		else {
			p = m_startBuffer[4 + i] ^ (oldData + hardKey + i + 1);
			oldData = m_startBuffer[4 + i];
			m_startBuffer[4 + i] = p ^ (randKey + (i + 1) + oldP);
			oldP = p;
			checksum += m_startBuffer[4 + i];
		}

	}

	if ((unsigned char)(checksum % 256) != m_startBuffer[4])
	{
		return false;
	}


	return true;
}

unsigned char* NetLib::PacketBuffer::GetHeaderPtr()
{
	//return &m_startBuffer[3];
	return m_startBuffer;
}

void NetLib::PacketBuffer::InitializePacketBuffer(int iBuffSize)
{
	m_startBuffer = new unsigned char[iBuffSize + eHeader_Size];
	m_chpBuffer = &m_startBuffer[eHeader_Size];
	m_iBufferSize = iBuffSize;
	m_iReadPos = 0;
	m_iWritePos = 0;
	m_refCount = 0;
	m_isSet = true;
	m_bHeader = false;
}

NetLib::PacketBuffer* NetLib::PacketBuffer::Alloc()
{
	PacketBuffer* p = m_freeList.Alloc();

	if (p->m_refCount != 0)
		CrashDump::Crash();

	p->Clear();
	InterlockedIncrement(&p->m_refCount);

	return p;
}

bool NetLib::PacketBuffer::Free(PacketBuffer* pPacket)
{
	long refCount = InterlockedDecrement(&pPacket->m_refCount);
	if(refCount == 0)
	{

		if(!m_freeList.Free(pPacket))
		{
			CrashDump::Crash();
		}
	} else if (refCount > 2 || refCount < 0)
		CrashDump::Crash();

	return true;
}

void NetLib::PacketBuffer::AddRef()
{
	InterlockedIncrement(&m_refCount);
}

