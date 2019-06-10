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


	public:
		int allocPos;
		int allocThread;
		__int64 allocSessionID;
		long allocCount;
		int freePos;
		int freeThread;
		__int64 freeSessionID;
		long freeCount;
		int RealFreeThread;
		__int64 RealFreeSessionID;
		int RealFreePos;
		int addRefPos;
		int addRefThread;
		__int64 addRefSessionID;
		long addRefCount;

		static ObjectFreeList<PacketBuffer> m_freeList;
	};

}