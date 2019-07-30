#pragma once

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Winmm.lib")
#pragma comment(lib, "DbgHelp.Lib")

#include <WinSock2.h>
#include <process.h>
#include <WS2tcpip.h>
#include <string>
#include <Windows.h>
#include <psapi.h>
#include <DbgHelp.h>
#include <strsafe.h>
#include "StreamQ.h"
#include "LockFreeStack.h"
#include "LockFreeQueue.h"

namespace NetLib {

#define WSA_BUFF_SIZE 200
	#pragma pack(push, 1)
	struct header
	{
		short len;
	};

	#pragma pack(pop)

	class PacketBuffer;
	class StreamQ;

	union SESSIONID
	{
		__int64 fullSessionID;
		struct StructSessionID
		{
			int byte4;
			u_short byte2;
			WORD arrPos;
		};
		StructSessionID structSessionID;

		/*SESSIONID& operator=(const SESSIONID& rhs)
		{
			this->fullSessionID = rhs.fullSessionID;
			return *this;
		}*/
	};

	inline bool operator<(const SESSIONID& lhs, const SESSIONID& rhs)
	{
		return lhs.fullSessionID < rhs.fullSessionID;
	}
	/*
	inline bool operator==(const SESSIONID& lhs, const SESSIONID& rhs)
	{
		return lhs.fullSessionID == rhs.fullSessionID;
	}*/


	class Session
	{
		friend class LanServer;
	public:
	protected:

	private:
		OVERLAPPED recvOverlapped;
		OVERLAPPED sendOverlapped;
		WSABUF sendBuf[WSA_BUFF_SIZE];
		int sendBufCount;
		int sendBufSendCount;
		SOCKET sock;
		LockFreeQueue<PacketBuffer *> sendQ;
		LockFreeQueue<PacketBuffer *> sendingQ;
		StreamQ recvQ;
		long IOCount;
		long isRelease;
		long sending;
		SESSIONID sessionID;

		WCHAR ipv4Addr[32];
		u_short port;
	};


	class LanServer
	{
	public:
		LanServer();
		~LanServer();

		bool Start(WCHAR * szIp, short port, int workerThreadCount, int concurrentThreadCount, bool nagleOption, int maximumConnectUser);
		void Stop();
		int GetClientCount() {
			return connectClient;
		}

		bool Disconnect(SESSIONID SessionID);
		bool SendPacket(SESSIONID SessionID, PacketBuffer* packet);

		virtual bool OnConnectionRequest(WCHAR* szIP, u_short port) = 0;
		virtual void OnClientJoin(SESSIONID sessionID) = 0;
		virtual void OnClientLeave(SESSIONID sessionID) = 0;

		virtual void OnRecv(SESSIONID SessionID, PacketBuffer* packet) = 0;
		virtual void OnSend(SESSIONID SessionID, DWORD SendSize) = 0;

		virtual void OnWorkerThreadBegin() = 0;
		virtual void OnWorkerThreadEnd() = 0;

		virtual void OnError(int ErrorCode, WCHAR* ErrorMessage) = 0;

	protected:
		static unsigned int WINAPI AcceptThread(void *arg);
		static unsigned int WINAPI WorkerThread(void *arg);

		void RecvPost(Session* session);
		void SendPost(Session* session);

		void StartFail();
		void SessionRelease(Session* session);

		Session* GetSessionPtr(SESSIONID sessionID);
		u_short GetSessionPos(SESSIONID sessionID);

		struct SessionArray{
			Session session;
			long isUsing;
		};
		int connectClient;

	private:
		WSADATA wsa;
		HANDLE hcp;
		SOCKET listenSock;
		SOCKADDR_IN serverAddr;
		SessionArray* pSessionArr;
		LockFreeStack<int> emptySessionStack;

		int maximumConnectUser;
		int workerThreadCount;

		bool serverStatus;

		__int64 lastSessionId;
	};


#define NULLErrorMessage const_cast<WCHAR *>(L"")

#define NoMessageError(errorCode) OnError(errorCode, NULLErrorMessage)

#define WSASTARTUP_FAIL 1
#define CreateIoCompletionPort_FAIL 2
#define _beginthreadex_FAIL 3
#define socket_FAIL 4
#define bind_FAIL 5
#define listen_FAIL 6
#define Session_NEW_FAIL 7
#define Accept_FAIL 8
#define GQCS_FAIL 9
#define setsockopt_FAIL 10
#define recvQ_Dequeue_FAIL 11
#define recvQ_Enqueue_FAIL 12
#define sendQ_Dequeue_FAIL 13
#define sendQ_Enqueue_FAIL 14
#define transferred_Zero 15
#define PacketBuffer_Free_FAIL 16
}