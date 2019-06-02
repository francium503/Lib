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
		char* GetBufferPtr(void) {
			return m_chpBuffer;
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

		// ��� ũ�� 2byte
		void SetHeader(short *header);
		char* GetHeaderPtr(void);
		void InitializePacketBuffer(int iBuffSize);
		void AddRef();

		static PacketBuffer* Alloc();
		static bool Free(PacketBuffer* pPacket);

	protected:
		int m_iBufferSize;

		int m_iWritePos;
		int m_iReadPos;

		char* m_chpBuffer;
		char* m_startBuffer;

		long m_refCount;
		bool m_isSet;

		int FreethreadId = 0;
		int allocThreadId = 0;

	public:
		int FreePos = 0;
		int AllocPos = 0;
		static ObjectFreeList<PacketBuffer> m_freeList;
	};

}