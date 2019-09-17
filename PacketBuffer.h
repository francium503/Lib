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

		// 패킷 포인터 파괴
		void Release(void);

		// 패킷 버퍼 청소
		void Clear(void);

		// 버퍼 사이즈 얻기
		int GetBufferSize(void) {
			return m_iBufferSize;
		}

		// 패킷 사이즈 얻기
		int GetDataSize(void);

		// 버퍼 포인터 얻기
		unsigned char* GetBufferPtr(void) {
			return m_chpBuffer;
		}

		//읽는 버퍼 위치 얻기
		unsigned char* GetReadBufferPtr(void) {
			return &m_chpBuffer[m_iReadPos];
		}

		//쓰는 버퍼 위치 얻기
		unsigned char* GetWriteBufferPtr(void) {
			return &m_chpBuffer[m_iWritePos];
		}

		// 버퍼 포인터 위치 이동용 함수
		int MoveWritePos(int iPos);
		int MoveReadPos(int iPos);

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
		PacketBuffer& operator<<(WORD wrhs);


		// 빼기 연산자 오버로딩
		PacketBuffer& operator>>(BYTE &brhs);
		PacketBuffer& operator>>(char &chrhs);
		PacketBuffer& operator>>(short &shrhs);
		PacketBuffer& operator>>(int &irhs);
		PacketBuffer& operator>>(float &frhs);
		PacketBuffer& operator>>(double &drhs);
		PacketBuffer& operator>>(__int64 &i64rhs);
		PacketBuffer& operator>>(WORD &wrhs);

		// 직접 빼기
		int GetData(char *chpDest, int iGetSize);

		// 직접 넣기
		int PutData(char *chpSrc, int iPutSize);

		// 헤더 크기 2byte
		void SetHeader(short *header);
		void SetHeader(unsigned char code, unsigned char randKey, unsigned char hardKey);
		void SetLen();
		short GetLen();

		//헤더 암호화 관련
		bool DecryptPacket(unsigned char hardKey);
		unsigned char* GetHeaderPtr(void);


		//헤더 풀 사용 관련
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