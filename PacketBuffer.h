#pragma once

class PacketBuffer
{
public:
	enum ePACKET {
		eBuffer_DEFAULT = 1400
	};

	PacketBuffer();
	PacketBuffer(int iBuffSize);

	virtual ~PacketBuffer();

	// 패킷 포인터 파괴
	void Release(void);

	// 패킷 버퍼 청소
	void Clear(void);

	// 버퍼 사이즈 얻기
	int GetBufferSize(void);

	// 패킷 사이즈 얻기
	int GetDataSize(void);

	// 버퍼 포인터 얻기
	char* GetBufferPtr(void);


	// 버퍼 포인터 위치 이동용 함수
	int MoveWritePos(unsigned int iPos);
	int MoveReadPos(unsigned int iPos);

	// 대입 연산자 오버로딩
	PacketBuffer& operator =(PacketBuffer &rhs);


	// 삽입 연산자 오버로딩
	PacketBuffer& operator<<(BYTE brhs);
	PacketBuffer& operator<<(char chrhs);
	PacketBuffer& operator<<(short shrhs);
	PacketBuffer& operator<<(int irhs);
	PacketBuffer& operator<<(float frhs);
	PacketBuffer& operator<<(double drhs);
	PacketBuffer& operator<<(__int64 i64rhs);


	// 빼기 연산자 오버로딩
	PacketBuffer& operator>>(BYTE &brhs);
	PacketBuffer& operator>>(char &chrhs);
	PacketBuffer& operator>>(short &shrhs);
	PacketBuffer& operator>>(int &irhs);
	PacketBuffer& operator>>(float &frhs);
	PacketBuffer& operator>>(double &drhs);
	PacketBuffer& operator>>(__int64 &i64rhs);

	// 직접 빼기
	int GetData(char *chpDest, int iGetSize);

	// 직접 넣기
	int PutData(char *chpSrc, int iPutSize);


protected:
	int m_iBufferSize;
	int m_iDataSize;

	int m_iWritePos;
	int m_iReadPos;

	char* m_chpBuffer;
};

