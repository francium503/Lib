#include "pch.h"
#include "LanServer.h"
#include "StreamQ.h"
#include "PacketBuffer.h"

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

	for (int i = 0; i < maximumConnectUser; ++i) {
		pSessionArr[i].isUsing = false;
	}

	if (pSessionArr == nullptr) {
		NoMessageError(Session_NEW_FAIL);
		closesocket(listenSock);
		return false;
	}

	hThread = (HANDLE)_beginthreadex(NULL, 0, AcceptThread, this, 0, &threadId);

	if (hThread == NULL) {
		NoMessageError(_beginthreadex_FAIL);
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
	short port;
	NetLib::LanServer *lanServer = static_cast<LanServer *>(arg);

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

		for (int i = 0; i < lanServer->maximumConnectUser; ++i) {
			if (!InterlockedCompareExchange(&lanServer->pSessionArr[i].isUsing, TRUE, FALSE)) {
				lanServer->pSessionArr[i].session.sock = clientSock;
				
				lanServer->pSessionArr[i].session.recvOverlapped = new OVERLAPPED;
				ZeroMemory(lanServer->pSessionArr[i].session.recvOverlapped, sizeof(OVERLAPPED));
				lanServer->pSessionArr[i].session.sendOverlapped = new OVERLAPPED;
				ZeroMemory(lanServer->pSessionArr[i].session.sendOverlapped, sizeof(OVERLAPPED));

				lanServer->pSessionArr[i].session.recvQ = new StreamQ(15000);
				lanServer->pSessionArr[i].session.sendQ = new StreamQ(15000);

				lanServer->pSessionArr[i].session.IOCount = 0;
				lanServer->pSessionArr[i].session.sending = FALSE;
				CreateIoCompletionPort((HANDLE)lanServer->pSessionArr[i].session.sock, lanServer->hcp, (ULONG_PTR)&lanServer->pSessionArr[i].session, 0);

				lanServer->pSessionArr[i].session.port = ntohs(clientAddr.sin_port);
				InetNtop(clientAddr.sin_family, &clientAddr.sin_addr, lanServer->pSessionArr[i].session.ipv4Addr, sizeof(lanServer->pSessionArr[i].session.ipv4Addr));

				lanServer->pSessionArr[i].session.sessionID = ++lanServer->lastSessionId;

				++(lanServer->connectClient);

				lanServer->OnClientJoin(lanServer->pSessionArr[i].session.sessionID);

				lanServer->RecvPost(&lanServer->pSessionArr[i].session);

				break;
			}
		}
	}

	return 0;
}

unsigned int WINAPI NetLib::LanServer::WorkerThread(void * arg)
{
	DWORD transferred = 0;
	Session *session = nullptr;
	OVERLAPPED *overlapped = nullptr;
	NetLib::LanServer *lanServer = static_cast<LanServer *>(arg);

	do {
		transferred = 0;
		session = nullptr;
		overlapped = nullptr;

		int retval = GetQueuedCompletionStatus(lanServer->hcp, &transferred, (PULONG_PTR)&session, (LPOVERLAPPED *)&overlapped, INFINITE);

		lanServer->OnWorkerThreadBegin();

		if (session == NULL && overlapped == NULL && transferred == 0) {
			//NULL NULL 0���� ������ ����
			break;
		}
		else if (overlapped == NULL) {
			lanServer->OnError(GetLastError(), const_cast<WCHAR *>(L" - GetLastError value\n"));
		}
		else if (transferred == 0) {
			// 0�� ���� ó��
			lanServer->Disconnect(session->sessionID);
		}
		else if (session->recvOverlapped == overlapped) {
			//TODO ��Ŷ �˻� �߰� �ʿ�
			PacketBuffer *packet = new PacketBuffer(transferred + 10);

			session->recvQ->MoveRear(transferred);

			BOOL result = session->recvQ->Dequeue(packet->GetBufferPtr(), transferred);
			packet->MoveWritePos(transferred);

			if (result != TRUE) {
				lanServer->NoMessageError(recvQ_Dequeue_FAIL);
				delete packet;
			}
			else {
				lanServer->OnRecv(session->sessionID, packet);
				delete packet;
			}

			lanServer->RecvPost(session);
		}
		else if (session->sendOverlapped == overlapped) {
			session->sendQ->MoveFront(transferred);

			InterlockedCompareExchange(&session->sending, FALSE, TRUE);

			if (session->sendQ->GetUseSize() > 0) {
				lanServer->SendPost(session);
			}

			lanServer->OnSend(session->sessionID, transferred);
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

	WSABUF wsaBuf[2];
	DWORD flags = 0;
	int retval = 0;
	int bufCount = 0;
	int getSize = session->sendQ->GetNotBrokenGetSize();
	int useSize = session->sendQ->GetUseSize();

	if (useSize == 0) {
		InterlockedCompareExchange(&session->sending, FALSE, TRUE);

		if (session->sendQ->GetUseSize() == 0) {
			return;
		}

		goto GOTO_SEND;
	}

	InterlockedIncrement(&session->IOCount);

	ZeroMemory(session->sendOverlapped, sizeof(OVERLAPPED));

	if (getSize < useSize) {
		wsaBuf[0].buf = session->sendQ->GetFrontBufferPtr();
		wsaBuf[0].len = getSize;
		wsaBuf[1].buf = session->sendQ->GetBufferStartPtr();
		wsaBuf[1].len = useSize - getSize;

		bufCount = 2;
	}
	else {
		wsaBuf[0].buf = session->sendQ->GetFrontBufferPtr();
		wsaBuf[0].len = getSize;

		bufCount = 1;
	}
	retval = WSASend(session->sock, wsaBuf, bufCount, NULL, flags, session->sendOverlapped, NULL);

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
	delete session->sendQ;

	closesocket(session->sock);

	for (int i = 0; i < maximumConnectUser; ++i) {
		if (&pSessionArr[i].session == session) {
			sessionId = pSessionArr[i].session.sessionID;
			InterlockedCompareExchange(&pSessionArr[i].isUsing, FALSE, TRUE);
		}
	}

	OnClientLeave(sessionId);
}

bool NetLib::LanServer::Disconnect(SESSIONID SessionID)
{
	for (int i = 0; i < maximumConnectUser; ++i) {
		if (pSessionArr[i].session.sessionID == SessionID) {
			if (pSessionArr[i].isUsing == FALSE)
				return false;

			shutdown(pSessionArr[i].session.sock, SD_BOTH);
			--connectClient;
			break;
		}
	}
	return true;
}

bool NetLib::LanServer::SendPacket(SESSIONID SessionID, PacketBuffer * packet)
{
	for (int i = 0; i < maximumConnectUser; ++i) {
		if (pSessionArr[i].session.sessionID == SessionID) {
			if (!pSessionArr[i].session.sendQ->Enqueue(packet->GetBufferPtr(), packet->GetDataSize())) {
				NoMessageError(sendQ_Enqueue_FAIL);
				return false;
			}

			SendPost(&pSessionArr[i].session);

			break;
		}
	}

	return true;
}