#pragma once

#include "ObjectFreeList.h"

namespace NetLib {

	class PacketBuffer
	{
	public:
		friend ObjectFreeList<PacketBuffer>;
		enum ePACKET {
			eBuffer_DEFAULT = 1500
		};
		enum eHEADERSIZE
		{
			eHeader_Size = 5
		};

	private:
		PacketBuffer();
		PacketBuffer(int iBuffSize);

	public:
		virtual ~PacketBuffer();

		// ��Ŷ ������ �ı�
		void Release(void);

		// ��Ŷ ���� û��
		void Clear(void);

		// ���� ������ ���
		int GetBufferSize(void) {
			return m_iBufferSize;
		}

		// ��Ŷ ������ ���
		int GetDataSize(void);

		// ���� ������ ���
		unsigned char* GetBufferPtr(void) {
			return m_chpBuffer;
		}

		//�д� ���� ��ġ ���
		unsigned char* GetReadBufferPtr(void) {
			return &m_chpBuffer[m_iReadPos];
		}

		//���� ���� ��ġ ���
		unsigned char* GetWriteBufferPtr(void) {
			return &m_chpBuffer[m_iWritePos];
		}

		// ���� ������ ��ġ �̵��� �Լ�
		int MoveWritePos(int iPos);
		int MoveReadPos(int iPos);

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
		PacketBuffer& operator<<(WORD wrhs);


		// ���� ������ �����ε�
		PacketBuffer& operator>>(BYTE &brhs);
		PacketBuffer& operator>>(char &chrhs);
		PacketBuffer& operator>>(short &shrhs);
		PacketBuffer& operator>>(int &irhs);
		PacketBuffer& operator>>(float &frhs);
		PacketBuffer& operator>>(double &drhs);
		PacketBuffer& operator>>(__int64 &i64rhs);
		PacketBuffer& operator>>(WORD &wrhs);

		// ���� ����
		int GetData(char *chpDest, int iGetSize);

		// ���� �ֱ�
		int PutData(char *chpSrc, int iPutSize);

		// ��� ũ�� 2byte
		void SetHeader(short *header);
		void SetHeader(unsigned char code, unsigned char randKey, unsigned char hardKey);
		void SetLen();
		short GetLen();

		//��� ��ȣȭ ����
		bool DecryptPacket(unsigned char hardKey);
		unsigned char* GetHeaderPtr(void);


		//��� Ǯ ��� ����
		void InitializePacketBuffer(int iBuffSize);
		void AddRef();
		static PacketBuffer* Alloc();
		static bool Free(PacketBuffer* pPacket);

	private:


	protected:
		int m_iBufferSize;

		int m_iWritePos;
		int m_iReadPos;

		unsigned char* m_chpBuffer;
		unsigned char* m_startBuffer;

		long m_refCount;
		bool m_isSet;
		bool m_bHeader;

	public:
		static ObjectFreeList<PacketBuffer> m_freeList;
	};

}