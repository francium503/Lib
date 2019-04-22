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

	// ��Ŷ ������ �ı�
	void Release(void);

	// ��Ŷ ���� û��
	void Clear(void);

	// ���� ������ ���
	int GetBufferSize(void);

	// ��Ŷ ������ ���
	int GetDataSize(void);

	// ���� ������ ���
	char* GetBufferPtr(void);


	// ���� ������ ��ġ �̵��� �Լ�
	int MoveWritePos(unsigned int iPos);
	int MoveReadPos(unsigned int iPos);

	// ���� ������ �����ε�
	PacketBuffer& operator =(PacketBuffer &rhs);


	// ���� ������ �����ε�
	PacketBuffer& operator<<(BYTE brhs);
	PacketBuffer& operator<<(char chrhs);
	PacketBuffer& operator<<(short shrhs);
	PacketBuffer& operator<<(int irhs);
	PacketBuffer& operator<<(float frhs);
	PacketBuffer& operator<<(double drhs);
	PacketBuffer& operator<<(__int64 i64rhs);


	// ���� ������ �����ε�
	PacketBuffer& operator>>(BYTE &brhs);
	PacketBuffer& operator>>(char &chrhs);
	PacketBuffer& operator>>(short &shrhs);
	PacketBuffer& operator>>(int &irhs);
	PacketBuffer& operator>>(float &frhs);
	PacketBuffer& operator>>(double &drhs);
	PacketBuffer& operator>>(__int64 &i64rhs);

	// ���� ����
	int GetData(char *chpDest, int iGetSize);

	// ���� �ֱ�
	int PutData(char *chpSrc, int iPutSize);


protected:
	int m_iBufferSize;
	int m_iDataSize;

	int m_iWritePos;
	int m_iReadPos;

	char* m_chpBuffer;
};

