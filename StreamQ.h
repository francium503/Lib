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

	//���� ����
	int GetUseSize(void);
	//���� ����
	int GetFreeSize(void);

	//����ť�� ������ �κ��� ������� �ʰ� Get �� �� �ִ� ������
	int GetNotBrokenGetSize(void);
	//����ť�� ������ �κ��� ������� �ʰ� Put �� �� �ִ� ������
	int GetNotBrokenPutSize(void);

	// �ְ� �������� ����
	BOOL Enqueue(char *chpData, int iSize);
	// ������ �������� ����
	BOOL Dequeue(char *chpDest, int iSize);
	// Peek �ϰ� �������� ����
	BOOL Peek(char* chpDest, int iSize);

	int MoveRear(int iSize);
	int MoveFront(int iSize);

	// ������� �̵����� ���� �ʱ�ȭ
	void ClearBuffer(void);

	// ���� ���ٿ� ���� ������ ����
	char* GetFrontBufferPtr(void);
	// ���� ���ٿ� ���� ������ ����
	char* GetRearBufferPtr(void);
	// ���� ���ٿ� ���� ���� ������ ����
	char* GetBufferStartPtr(void);

	// ť ���� ũ��Ƽ�� ���� �� �Լ�
	void Lock(void);
	// ť ���� ũ��Ƽ�� ���� ��� �Լ�
	void Release(void);

private:
	CRITICAL_SECTION m_streamQCS;
	char *m_pBuffer;
	int m_front;
	int m_rear;
	int m_bufferSize;
};