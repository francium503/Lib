#include "pch.h"
#include "NetServer.h"
#include "StreamQ.h"
#include "PacketBuffer.h"
#include "Log.h"
#include "MiniDump.h"

NetLib::NetServer::NetServer()
{
	timeBeginPeriod(1);

	hcp = NULL;
	listenSock = INVALID_SOCKET;
	pSessionArr = nullptr;
	
	lastSessionId = 0;
	connectClient = 0;
	serverStatus = false;
}


NetLib::NetServer::~NetServer()
{
	Stop();
	timeEndPeriod(1);
}

bool NetLib::NetServer::Start(WCHAR * szIp, short port, int workerThreadCount, int concurrentThreadCount, bool nagleOption, int maximumConnectUser)
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

void NetLib::NetServer::Stop()
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

unsigned int WINAPI NetLib::NetServer::AcceptThread(void * arg)
{
	SOCKET clientSock;
	SOCKADDR_IN clientAddr;
	int addrLen;
	WCHAR ipv4[32];
	u_short port;
	NetServer *netServer = static_cast<NetServer *>(arg);

	while (1) {
		addrLen = sizeof(clientAddr);
		clientSock = accept(netServer->listenSock, (SOCKADDR *)&clientAddr, &addrLen);

		if (clientSock == INVALID_SOCKET) {
			if (netServer->serverStatus == false) {
				break;
			}
			else {
				int error = WSAGetLastError();
				WCHAR errorCode[10];
				_itow_s(error, errorCode, 10);
				
				netServer->OnError(Accept_FAIL, errorCode);

				break;
			}
		}

		tcp_keepalive tcpkl;
		tcpkl.onoff = 1;
		tcpkl.keepaliveinterval = 1000;
		tcpkl.keepalivetime = 30000;

		DWORD dwRet;
		WSAIoctl(clientSock, SIO_KEEPALIVE_VALS, &tcpkl, sizeof(tcp_keepalive), 0, 0, &dwRet, NULL, NULL);

		port = ntohs(clientAddr.sin_port);
		InetNtopW(clientAddr.sin_family, &clientAddr.sin_addr, ipv4, sizeof(ipv4));

		netServer->OnConnectionRequest(ipv4, port);

		if (clientSock == INVALID_SOCKET) {
			netServer->NoMessageError(Accept_FAIL);
			continue;
		}

		int index = -1;
		bool result = netServer->emptySessionStack.Pop(&index);
		Session* acceptSession = &netServer->pSessionArr[index].session;

		if(!result)
		{
			shutdown(clientSock, SD_BOTH);
			Log::GetInstance()->SysLog(const_cast<WCHAR *>(L"LanServer"), Log::eLogLevel::eLogWarning, const_cast<WCHAR *>(L"Connect FULL Connect count - %d\n"), netServer->connectClient);
			CrashDump::Crash();
		}

		if (index == -1)
		{
			Log::GetInstance()->SysLog(const_cast<WCHAR *>(L"LanServer"), Log::eLogLevel::eLogSystem, const_cast<WCHAR *>(L"Find Session Arr Pos Error"));
			CrashDump::Crash();
		}

		if(InterlockedCompareExchange(&netServer->pSessionArr[index].isUsing, TRUE, FALSE))
		{
			Log::GetInstance()->SysLog(const_cast<WCHAR *>(L"LanServer"), Log::eLogLevel::eLogSystem, const_cast<WCHAR *>(L"Get Using SessionIndex"));
			CrashDump::Crash();
		}

		acceptSession->sock = clientSock;

		ZeroMemory(&acceptSession->recvOverlapped, sizeof(OVERLAPPED));
		ZeroMemory(&acceptSession->sendOverlapped, sizeof(OVERLAPPED));

		acceptSession->sendBufCount = 0;
		acceptSession->sendBufSendCount = 0;

		acceptSession->IOCount = 0;
		acceptSession->isRelease = FALSE;
		acceptSession->sending = FALSE;

		acceptSession->port = ntohs(clientAddr.sin_port);
		InetNtop(clientAddr.sin_family, &clientAddr.sin_addr, acceptSession->ipv4Addr, sizeof(acceptSession->ipv4Addr));

		acceptSession->sessionID.fullSessionID = ++netServer->lastSessionId;
		acceptSession->sessionID.structSessionID.arrPos = index;

		InterlockedIncrement(&netServer->connectClient);

		CreateIoCompletionPort((HANDLE)acceptSession->sock, netServer->hcp, (ULONG_PTR)acceptSession, 0);
		netServer->OnClientJoin(acceptSession->sessionID);
		netServer->RecvPost(acceptSession);
	}

	return 0;
}

unsigned int WINAPI NetLib::NetServer::WorkerThread(void * arg)
{
	DWORD transferred = 0;
	Session *session = nullptr;
	OVERLAPPED *overlapped = nullptr;
	NetServer *netServer = static_cast<NetServer *>(arg);


	do {
		transferred = 0;
		session = nullptr;
		overlapped = nullptr;

		int retval = GetQueuedCompletionStatus(netServer->hcp, &transferred, (PULONG_PTR)&session, (LPOVERLAPPED *)&overlapped, INFINITE);

		netServer->OnWorkerThreadBegin();

		if (session == NULL && overlapped == NULL && transferred == 0) {
			//NULL NULL 0으로 스레드 종료
			break;
		}
		
		if (overlapped == NULL) {
			netServer->OnError(GetLastError(), const_cast<WCHAR *>(L" - GetLastError value\n"));
		}
		else if (transferred == 0) {
			// 0은 종료 처리
			netServer->Disconnect(session->sessionID);
		}
		else if (&session->recvOverlapped == overlapped) {
			session->recvQ.MoveRear(transferred);

			while (true) {
				header h;

				if (session->recvQ.GetUseSize() < sizeof(header))
				{
					break;
				}

				if (!session->recvQ.Peek((char *)&h, sizeof(header)))
				{
					Log::GetInstance()->SysLog(const_cast<WCHAR *>(L"LanServer"), Log::eLogLevel::eLogSystem, const_cast<WCHAR *>(L"Header Peek error\n"));
					netServer->NoMessageError(recvQ_Dequeue_FAIL);
				}

				if (session->recvQ.GetUseSize() < h.len + sizeof(h)) {
					break;
				}

				//session->recvQ->MoveFront(sizeof(h));

				PacketBuffer* packet = PacketBuffer::Alloc();

				if(packet->GetDataSize() != 0)
				{
					CrashDump::Crash();
				}

				if(!session->recvQ.Dequeue((char *)packet->GetHeaderPtr(), sizeof(header) + h.len))
				{
					Log::GetInstance()->SysLog(const_cast<WCHAR *>(L"LanServer"), Log::eLogLevel::eLogSystem, const_cast<WCHAR *>(L"recvQ Dequeue error\n"));
					netServer->NoMessageError(recvQ_Dequeue_FAIL);
				}
				packet->MoveWritePos(h.len);

				//NETSERVER 용 DecryptPacket 부분 추가
				bool result = packet->DecryptPacket(0x32);

				if(!result)
				{
					CrashDump::Crash();
				}

				netServer->OnRecv(session->sessionID, packet);

				PacketBuffer::Free(packet);
			}

			netServer->RecvPost(session);
		}
		else if (&session->sendOverlapped == overlapped) {
			int packetSize;
			for (int i = 0; i < session->sendBufSendCount; ++i) {
				PacketBuffer *packet;

				if (!session->sendingQ.Dequeue(packet))
				{
					netServer->NoMessageError(sendQ_Dequeue_FAIL);
				}
				packetSize = packet->GetDataSize();

				if(session->sendBuf[i].len != packetSize + sizeof(header))
				{
					CrashDump::Crash();
				}


				if(session->sessionID.structSessionID.arrPos >= netServer->maximumConnectUser || session->sessionID.structSessionID.arrPos < 0)
				{
					CrashDump::Crash();
				}

				netServer->OnSend(session->sessionID, session->sendBuf[i].len);

				bool result = PacketBuffer::Free(packet);

				if(!result)
				{
					CrashDump::Crash();
				}
			}

			InterlockedCompareExchange(&session->sending, FALSE, TRUE);

			if (session->sendQ.GetSize() > 0) {
				netServer->SendPost(session);
			}
		}

		if (InterlockedDecrement(&session->IOCount) == 0) {
			netServer->SessionRelease(session);
		}

		netServer->OnWorkerThreadEnd();
	} while (true);

	return 0;
}

void NetLib::NetServer::RecvPost(Session * session)
{
	WSABUF wsaBuf[2];
	DWORD flags = 0;
	int retval = 0;

	InterlockedIncrement(&session->IOCount);

	ZeroMemory(&session->recvOverlapped, sizeof(OVERLAPPED));

	int putSize = session->recvQ.GetNotBrokenPutSize();
	int bufCount = 0;
	int freeSize = session->recvQ.GetFreeSize();
	if (putSize < freeSize) {
		wsaBuf[0].buf = session->recvQ.GetRearBufferPtr();
		wsaBuf[0].len = putSize;
		wsaBuf[1].buf = session->recvQ.GetBufferStartPtr();
		wsaBuf[1].len = freeSize - putSize;

		bufCount = 2;
	}
	else {
		wsaBuf[0].buf = session->recvQ.GetRearBufferPtr();
		wsaBuf[0].len = putSize;

		bufCount = 1;
	}

	retval = WSARecv(session->sock, wsaBuf, bufCount, NULL, &flags, &session->recvOverlapped, NULL);

	if (retval == SOCKET_ERROR) {
		int error = WSAGetLastError();
		if (error != WSA_IO_PENDING) {
			if (error == WSAENOBUFS) {
				OnError(socket_FAIL, const_cast<WCHAR *>(L"WSAE NO BUF"));
			}

			if (InterlockedDecrement(&session->IOCount) == 0) {
				SessionRelease(session);
			}
		}
	}
}

void NetLib::NetServer::SendPost(Session * session)
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
	ZeroMemory(&session->sendOverlapped, sizeof(OVERLAPPED));

	for(int i=0;i< min(getSize, WSA_BUFF_SIZE); ++i)
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
		session->sendBuf[i].len = packet->GetDataSize() + sizeof(header);

		if(!session->sendingQ.Enqueue(packet))
		{
			CrashDump::Crash();
		}
	}

	session->sendBufSendCount = min(getSize, WSA_BUFF_SIZE);

	retval = WSASend(session->sock, session->sendBuf, session->sendBufSendCount, NULL, flags, &session->sendOverlapped, NULL);

	if (retval == SOCKET_ERROR) {
		int error = WSAGetLastError();
		if (error != WSA_IO_PENDING) {
			if (error == WSAENOBUFS) {
				OnError(socket_FAIL, const_cast<WCHAR *>(L"WSAE NO BUF"));
			}

			if (InterlockedDecrement(&session->IOCount) == 0) {
				SessionRelease(session);
			}
		}
	}

}

void NetLib::NetServer::StartFail()
{
	if (pSessionArr != nullptr)
		delete[] pSessionArr;

	if (listenSock != INVALID_SOCKET)
		closesocket(listenSock);

	WSACleanup();
}

void NetLib::NetServer::SessionRelease(Session * session)
{
	if(InterlockedCompareExchange(&session->isRelease, TRUE, FALSE))
	{
		return;
	}

	closesocket(session->sock);

	SESSIONID sessionId;

	PacketBuffer *p;
	while (session->sendQ.Dequeue(p))
	{
		PacketBuffer::Free(p);
	}
	while (session->sendingQ.Dequeue(p))
	{
		PacketBuffer::Free(p);
	}
	session->recvQ.ClearBuffer();

	int position = GetSessionPos(session->sessionID);

	if(position == -1)
	{
		CrashDump::Crash();
	}

	if (&pSessionArr[position].session == session) {
		sessionId = pSessionArr[position].session.sessionID;
		InterlockedCompareExchange(&pSessionArr[position].isUsing, FALSE, TRUE);
		if (!emptySessionStack.Push(sessionId.structSessionID.arrPos))
		{
			CrashDump::Crash();
		}
	}

	InterlockedDecrement(&connectClient);

	OnClientLeave(sessionId);
}

NetLib::Session* NetLib::NetServer::GetSessionPtr(SESSIONID sessionID)
{
	if (pSessionArr[sessionID.structSessionID.arrPos].session.sessionID.fullSessionID == sessionID.fullSessionID) {

		return &pSessionArr[sessionID.structSessionID.arrPos].session;
	}

	return nullptr;
}

u_short NetLib::NetServer::GetSessionPos(SESSIONID sessionID)
{
	if (pSessionArr[sessionID.structSessionID.arrPos].session.sessionID.fullSessionID == sessionID.fullSessionID)
		return sessionID.structSessionID.arrPos;
	else
		return (u_short)-1;
}


bool NetLib::NetServer::Disconnect(SESSIONID SessionID)
{
	Session* pSession = GetSessionPtr(SessionID);

	if (pSession == nullptr)
		return false;

	if(InterlockedIncrement(&pSession->IOCount) == 1)
	{
		InterlockedDecrement(&pSession->IOCount);

		shutdown(pSession->sock, SD_RECEIVE);

		SessionRelease(pSession);
		return true;
	}

	shutdown(pSession->sock, SD_RECEIVE);

	if(InterlockedDecrement(&pSession->IOCount) == 0)
	{
		SessionRelease(pSession);
	}

	return true;
}

bool NetLib::NetServer::SendPacket(SESSIONID SessionID, PacketBuffer * packet)
{
	/*
	short len = packet->GetDataSize();
	packet->SetHeader(&len);
	*/
	static char randkey = 0;
	
	packet->SetHeader(0x77, randkey++, 0x32);

	Session *pSession = GetSessionPtr(SessionID);

	if (pSession == nullptr) {
		return true;
	}

	if (InterlockedIncrement(&pSession->IOCount) == 1)
	{
		InterlockedDecrement(&pSession->IOCount);

		SessionRelease(pSession);
		return true;
	}

	if(pSession == nullptr)
	{
		CrashDump::Crash();
	}

	if(pSession->sessionID.fullSessionID != SessionID.fullSessionID)
	{
		return true;
	}

	packet->AddRef();
	if (!pSession->sendQ.Enqueue(packet)) {
		Log::GetInstance()->SysLog(const_cast<WCHAR *>(L"LanServer"), Log::eLogLevel::eLogSystem, const_cast<WCHAR *>(L"sendQ packet Enqueue fail\n"));
		PacketBuffer::Free(packet);
		NoMessageError(sendQ_Enqueue_FAIL);
		return false;
	}

	SendPost(pSession);
	 
	if(InterlockedDecrement(&pSession->IOCount) == 0)
	{
		SessionRelease(pSession);
	}

	return true;
}
