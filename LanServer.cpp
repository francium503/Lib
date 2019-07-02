#include "pch.h"
#include "LanServer.h"
#include "StreamQ.h"
#include "PacketBuffer.h"
#include "Log.h"
#include "MiniDump.h"

NetLib::LanServer::LanServer()
{
	timeBeginPeriod(1);

	hcp = NULL;
	listenSock = INVALID_SOCKET;
	pSessionArr = nullptr;
	
	lastSessionId = 0;
	connectClient = 0;
	serverStatus = false;
}


NetLib::LanServer::~LanServer()
{
	Stop();
	timeEndPeriod(1);
}

bool NetLib::LanServer::Start(WCHAR * szIp, short port, int workerThreadCount, int concurrentThreadCount, bool nagleOption, int maximumConnectUser)
{
	serverStatus = true;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		NoMessageError(WSASTARTUP_FAIL);
		return false;
	}

	unsigned int threadId;
	this->workerThreadCount = workerThreadCount;
	this->maximumConnectUser = maximumConnectUser;

	hcp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, concurrentThreadCount);

	if (hcp == NULL) {
		NoMessageError(CreateIoCompletionPort_FAIL);
		return false;
	}
 
	HANDLE hThread = NULL;
	
	for (int i = 0; i < workerThreadCount; ++i) {
		hThread = (HANDLE)_beginthreadex(NULL, 0, WorkerThread, this, 0, &threadId);
		if (hThread == NULL) {
			NoMessageError(_beginthreadex_FAIL);
			return false;
		}
		CloseHandle(hThread);
	}

	listenSock = socket(AF_INET, SOCK_STREAM, 0);

	if (listenSock == INVALID_SOCKET) {
		NoMessageError(socket_FAIL);
		return false;
	}

	int retval = setsockopt(listenSock, IPPROTO_TCP, TCP_NODELAY, (char *)&nagleOption, sizeof(nagleOption));

	if (retval == SOCKET_ERROR) {
		NoMessageError(setsockopt_FAIL);
		return false;
	}

	ZeroMemory(&serverAddr, sizeof(serverAddr));

	serverAddr.sin_family = AF_INET;
	InetPton(AF_INET, szIp, &serverAddr.sin_addr);
	serverAddr.sin_port = htons(port);

	retval = bind(listenSock, (SOCKADDR *)&serverAddr, sizeof(serverAddr));

	if (retval == SOCKET_ERROR) {
		NoMessageError(bind_FAIL);
		return false;
	}

	retval = listen(listenSock, SOMAXCONN);

	if (retval == SOCKET_ERROR) {
		NoMessageError(listen_FAIL);
		return false;
	}

	pSessionArr = new SessionArray[maximumConnectUser];

	if (pSessionArr == nullptr) {
		NoMessageError(Session_NEW_FAIL);
		closesocket(listenSock);
		return false;
	}

	for (int i = 0; i < maximumConnectUser; ++i) {
		pSessionArr[i].isUsing = FALSE;
		emptySessionStack.Push(i);
	}

	hThread = (HANDLE)_beginthreadex(NULL, 0, AcceptThread, this, 0, &threadId);

	if (hThread == NULL) {
		NoMessageError(_beginthreadex_FAIL);
		CrashDump::Crash();
		return false;
	}

	return true;
}

void NetLib::LanServer::Stop()
{
	serverStatus = false;

	closesocket(listenSock);

	for (int i = 0; i < maximumConnectUser; ++i) {
		if (pSessionArr[i].isUsing) {
			SessionRelease(&pSessionArr[i].session);
		}
	}

	for (int i = 0; i < workerThreadCount; ++i) {
		PostQueuedCompletionStatus(hcp, 0, NULL, NULL);
	}
	delete[] pSessionArr;

	CloseHandle(hcp);

	hcp = NULL;
	listenSock = INVALID_SOCKET;
	pSessionArr = nullptr;

	connectClient = 0;
}

unsigned int WINAPI NetLib::LanServer::AcceptThread(void * arg)
{
	SOCKET clientSock;
	SOCKADDR_IN clientAddr;
	int addrLen;
	WCHAR ipv4[32];
	u_short port;
	LanServer *lanServer = static_cast<LanServer *>(arg);

	while (1) {
		addrLen = sizeof(clientAddr);
		clientSock = accept(lanServer->listenSock, (SOCKADDR *)&clientAddr, &addrLen);

		if (clientSock == INVALID_SOCKET) {
			if (lanServer->serverStatus == false) {
				break;
			}
			else {
				int error = WSAGetLastError();
				WCHAR errorCode[10];
				_itow_s(error, errorCode, 10);
				
				lanServer->OnError(Accept_FAIL, errorCode);

				break;
			}
		}

		port = ntohs(clientAddr.sin_port);
		InetNtopW(clientAddr.sin_family, &clientAddr.sin_addr, ipv4, sizeof(ipv4));

		lanServer->OnConnectionRequest(ipv4, port);

		if (clientSock == INVALID_SOCKET) {
			lanServer->NoMessageError(Accept_FAIL);
			continue;
		}

		int index = -1;
		bool result = lanServer->emptySessionStack.Pop(&index);
		Session* acceptSession = &lanServer->pSessionArr[index].session;

		if(!result)
		{
			shutdown(clientSock, SD_BOTH);
			Log::GetInstance()->SysLog(const_cast<WCHAR *>(L"LanServer"), Log::eLogLevel::eLogWarning, const_cast<WCHAR *>(L"Connect FULL Connect count - %d\n"), lanServer->connectClient);
			CrashDump::Crash();
		}

		if (index == -1)
		{
			Log::GetInstance()->SysLog(const_cast<WCHAR *>(L"LanServer"), Log::eLogLevel::eLogSystem, const_cast<WCHAR *>(L"Find Session Arr Pos Error"));
			CrashDump::Crash();
		}

		if(InterlockedCompareExchange(&lanServer->pSessionArr[index].isUsing, TRUE, FALSE))
		{
			Log::GetInstance()->SysLog(const_cast<WCHAR *>(L"LanServer"), Log::eLogLevel::eLogSystem, const_cast<WCHAR *>(L"Get Using SessionIndex"));
			CrashDump::Crash();
		}

		acceptSession->sock = clientSock;

		acceptSession->recvOverlapped = new OVERLAPPED;
		ZeroMemory(acceptSession->recvOverlapped, sizeof(OVERLAPPED));
		acceptSession->sendOverlapped = new OVERLAPPED;
		ZeroMemory(acceptSession->sendOverlapped, sizeof(OVERLAPPED));

		acceptSession->recvQ = new StreamQ(15000);

		acceptSession->sendBuf = nullptr;
		acceptSession->sendBufCount = 0;
		acceptSession->sendBufSendCount = 0;

		acceptSession->IOCount = 0;
		acceptSession->sending = FALSE;
		CreateIoCompletionPort((HANDLE)acceptSession->sock, lanServer->hcp, (ULONG_PTR)acceptSession, 0);

		acceptSession->port = ntohs(clientAddr.sin_port);
		InetNtop(clientAddr.sin_family, &clientAddr.sin_addr, acceptSession->ipv4Addr, sizeof(acceptSession->ipv4Addr));

		acceptSession->sessionID.fullSessionID = ++lanServer->lastSessionId;
		acceptSession->sessionID.structSessionID.arrPos = index;

		++(lanServer->connectClient);

		lanServer->RecvPost(acceptSession);
		lanServer->OnClientJoin(acceptSession->sessionID);
	}

	return 0;
}

unsigned int WINAPI NetLib::LanServer::WorkerThread(void * arg)
{
	DWORD transferred = 0;
	Session *session = nullptr;
	OVERLAPPED *overlapped = nullptr;
	LanServer *lanServer = static_cast<LanServer *>(arg);


	do {
		transferred = 0;
		session = nullptr;
		overlapped = nullptr;

		int retval = GetQueuedCompletionStatus(lanServer->hcp, &transferred, (PULONG_PTR)&session, (LPOVERLAPPED *)&overlapped, INFINITE);

		lanServer->OnWorkerThreadBegin();

		if (session == NULL && overlapped == NULL && transferred == 0) {
			//NULL NULL 0으로 스레드 종료
			break;
		}
		
		if (overlapped == NULL) {
			lanServer->OnError(GetLastError(), const_cast<WCHAR *>(L" - GetLastError value\n"));
		}
		else if (transferred == 0) {
			// 0은 종료 처리
			lanServer->Disconnect(session->sessionID);
		}
		else if (session->recvOverlapped == overlapped) {
			session->recvQ->MoveRear(transferred);

			while (true) {
				header h;

				if (session->recvQ->GetUseSize() < 2)
				{
					break;
				}

				if (!session->recvQ->Peek((char *)&h, sizeof(h)))
				{
					Log::GetInstance()->SysLog(const_cast<WCHAR *>(L"LanServer"), Log::eLogLevel::eLogSystem, const_cast<WCHAR *>(L"Header Peek error\n"));
					lanServer->NoMessageError(recvQ_Dequeue_FAIL);
				}

				if (session->recvQ->GetUseSize() < h.len) {
					break;
				}

				session->recvQ->MoveFront(sizeof(h));

				PacketBuffer* packet = PacketBuffer::Alloc();

				if(!session->recvQ->Dequeue((char *)packet->GetBufferPtr(), h.len))
				{
					Log::GetInstance()->SysLog(const_cast<WCHAR *>(L"LanServer"), Log::eLogLevel::eLogSystem, const_cast<WCHAR *>(L"recvQ Dequeue error\n"));
					lanServer->NoMessageError(recvQ_Dequeue_FAIL);
				}

				packet->MoveWritePos(h.len);

				lanServer->OnRecv(session->sessionID, packet);

				PacketBuffer::Free(packet);
			}

			lanServer->RecvPost(session);
		}
		else if (session->sendOverlapped == overlapped) {
			int sendSize = transferred;
			int lenSize = sendSize;
			for (int i = 0; i < session->sendBufSendCount; ++i) {
				PacketBuffer *packet;

				if (!session->sendingQ.Dequeue(packet))
				{
					lanServer->NoMessageError(sendQ_Dequeue_FAIL);
				}
				int packetSize = packet->GetDataSize();

				sendSize -= packetSize + 2;
				lenSize -= session->sendBuf[i].len;
				lanServer->OnSend(session->sessionID, packetSize);

				PacketBuffer::Free(packet);
			}

			if (sendSize != 0 || lenSize != sendSize)
				CrashDump::Crash();
			
			delete session->sendBuf;

			InterlockedCompareExchange(&session->sending, FALSE, TRUE);


			if (session->sendQ.GetSize() > 0) {
				lanServer->SendPost(session);
			}
		}

		InterlockedDecrement(&session->IOCount);

		if (session->IOCount == 0) {
			lanServer->SessionRelease(session);
		}

		lanServer->OnWorkerThreadEnd();
	} while (true);

	return 0;
}

void NetLib::LanServer::RecvPost(Session * session)
{
	WSABUF wsaBuf[2];
	DWORD flags = 0;
	int retval = 0;

	InterlockedIncrement(&session->IOCount);

	ZeroMemory(session->recvOverlapped, sizeof(OVERLAPPED));

	int putSize = session->recvQ->GetNotBrokenPutSize();
	int bufCount = 0;
	int freeSize = session->recvQ->GetFreeSize();
	if (putSize < freeSize) {
		wsaBuf[0].buf = session->recvQ->GetRearBufferPtr();
		wsaBuf[0].len = putSize;
		wsaBuf[1].buf = session->recvQ->GetBufferStartPtr();
		wsaBuf[1].len = freeSize - putSize;

		bufCount = 2;
	}
	else {
		wsaBuf[0].buf = session->recvQ->GetRearBufferPtr();
		wsaBuf[0].len = putSize;

		bufCount = 1;
	}

	retval = WSARecv(session->sock, wsaBuf, bufCount, NULL, &flags, session->recvOverlapped, NULL);

	if (retval == SOCKET_ERROR) {
		int error = WSAGetLastError();
		if (error != WSA_IO_PENDING) {
			if (error == WSAENOBUFS) {
				OnError(socket_FAIL, const_cast<WCHAR *>(L"WSAE NO BUF"));
			}
			InterlockedDecrement(&session->IOCount);

			if (session->IOCount == 0) {
				SessionRelease(session);
			}
		}
	}
}

void NetLib::LanServer::SendPost(Session * session)
{
	GOTO_SEND:
	if (InterlockedCompareExchange(&session->sending, TRUE, FALSE)) {
		return;
	}
	
	DWORD flags = 0;
	int retval = 0;
	int bufCount = 0;
	int getSize = session->sendQ.GetSize();

	//if (useSize == 0) {
	if( getSize == 0){
		InterlockedCompareExchange(&session->sending, FALSE, TRUE);

		if(session->sendQ.GetSize() == 0){
			return;
		}

		goto GOTO_SEND;
	}

	InterlockedIncrement(&session->IOCount);
	ZeroMemory(session->sendOverlapped, sizeof(OVERLAPPED));

	session->sendBuf = new WSABUF[getSize];

	for(int i=0;i< getSize; ++i)
	{
		PacketBuffer* packet;
		if(!session->sendQ.Dequeue(packet))
		{
			if (session->sendQ.GetSize() == 0) {

				break;
			}
			else {
				CrashDump::Crash();
			}
		}

		session->sendBuf[i].buf = (char *)packet->GetHeaderPtr();
		//TODO 헤더 사이즈 생각해서 +2 만큼 len에 해줌
		session->sendBuf[i].len = packet->GetDataSize() + 2;

		if(!session->sendingQ.Enqueue(packet))
		{
			CrashDump::Crash();
		}
	}

	session->sendBufSendCount = getSize;

	retval = WSASend(session->sock, session->sendBuf, session->sendBufSendCount, NULL, flags, session->sendOverlapped, NULL);

	if (retval == SOCKET_ERROR) {
		int error = WSAGetLastError();
		if (error != WSA_IO_PENDING) {
			if (error == WSAENOBUFS) {
				OnError(socket_FAIL, const_cast<WCHAR *>(L"WSAE NO BUF"));
			}
			InterlockedDecrement(&session->IOCount);

			if (session->IOCount == 0) {
				SessionRelease(session);
			}
		}
	}

}

void NetLib::LanServer::StartFail()
{
	if (pSessionArr != nullptr)
		delete[] pSessionArr;

	if (listenSock != INVALID_SOCKET)
		closesocket(listenSock);


	WSACleanup();
}

void NetLib::LanServer::SessionRelease(Session * session)
{
	SESSIONID sessionId;
	delete session->recvOverlapped;
	delete session->sendOverlapped;

	delete session->recvQ;

	closesocket(session->sock);

	for (int i = 0; i < maximumConnectUser; ++i) {
		if (&pSessionArr[i].session == session) {
			sessionId = pSessionArr[i].session.sessionID;
			InterlockedCompareExchange(&pSessionArr[i].isUsing, FALSE, TRUE);
			if(!emptySessionStack.Push(sessionId.structSessionID.arrPos))
			{
				CrashDump::Crash();
			}
		}
	}

	OnClientLeave(sessionId);
}

NetLib::Session* NetLib::LanServer::GetSessionPtr(SESSIONID sessionID)
{
	return &pSessionArr[sessionID.structSessionID.arrPos].session;
}

u_short NetLib::LanServer::GetSessionPos(SESSIONID sessionID)
{
	return sessionID.structSessionID.arrPos;
}


bool NetLib::LanServer::Disconnect(SESSIONID SessionID)
{
	u_short sessionPos = GetSessionPos(SessionID);

	if (pSessionArr[sessionPos].isUsing == FALSE) {
		CrashDump::Crash();
		return false;
	}

	shutdown(pSessionArr[sessionPos].session.sock, SD_RECEIVE);
	--connectClient;
	return true;
}

bool NetLib::LanServer::SendPacket(SESSIONID SessionID, PacketBuffer * packet)
{
	short len = packet->GetDataSize();
	packet->SetHeader(&len);

	Session *pSession = GetSessionPtr(SessionID);
	packet->AddRef();

	if (!pSession->sendQ.Enqueue(packet)) {
		Log::GetInstance()->SysLog(const_cast<WCHAR *>(L"LanServer"), Log::eLogLevel::eLogSystem, const_cast<WCHAR *>(L"sendQ packet Enqueue fail\n"));
		PacketBuffer::Free(packet);
		NoMessageError(sendQ_Enqueue_FAIL);
		return false;
	}

	SendPost(pSession);
	
	return true;
}
