#pragma once


class StreamQ {
public:
	enum eBuffer {
		eBuffer_DEFAULT = 1000
	};

	StreamQ(void);
	StreamQ(int iBufferSize);

	~StreamQ();

	void ReSize(int iSize);
	int GetBufferSize(void);

	//쓰는 공간
	int GetUseSize(void);
	//남은 공간
	int GetFreeSize(void);

	//원형큐의 끊어진 부분을 통과하지 않고 Get 할 수 있는 사이즈
	int GetNotBrokenGetSize(void);
	//원형큐의 끊어진 부분을 통과하지 않고 Put 할 수 있는 사이즈
	int GetNotBrokenPutSize(void);

	// 넣고 성공여부 리턴
	BOOL Enqueue(char *chpData, int iSize);
	// 꺼내고 성공여부 리턴
	BOOL Dequeue(char *chpDest, int iSize);
	// Peek 하고 성공여부 리턴
	BOOL Peek(char* chpDest, int iSize);

	int MoveRear(int iSize);
	int MoveFront(int iSize);

	// 멤버변수 이동으로 버퍼 초기화
	void ClearBuffer(void);

	// 직접 접근용 버퍼 포인터 리턴
	char* GetFrontBufferPtr(void);
	// 직접 접근용 버퍼 포인터 리턴
	char* GetRearBufferPtr(void);
	// 직접 접근용 버퍼 시작 포인터 리턴
	char* GetBufferStartPtr(void);

	// 큐 내부 크리티컬 섹션 락 함수
	void Lock(void);
	// 큐 내부 크리티컬 섹션 언락 함수
	void Release(void);

private:
	CRITICAL_SECTION m_streamQCS;
	char *m_pBuffer;
	int m_front;
	int m_rear;
	int m_bufferSize;
};