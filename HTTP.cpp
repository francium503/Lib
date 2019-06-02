#include "pch.h"
#include "HTTP.h"



NetLib::HTTP::HTTP()
{
	this->connectTimeOut = 10000;
	this->requestTimeOut = 10000;
	this->responseTimeOut = 10000;
	
	this->stringLength = 256;

	WSADATA wsa;

	WSAStartup(MAKEWORD(2, 2), &wsa);
}

NetLib::HTTP::HTTP(int connectTimeOut, int requestTimeOut, int responseTimeOut) {
	this->connectTimeOut = connectTimeOut;
	this->requestTimeOut = requestTimeOut;
	this->responseTimeOut = responseTimeOut;

	this->stringLength = 256;

	WSADATA wsa;
	
	WSAStartup(MAKEWORD(2, 2), &wsa);
}


NetLib::HTTP::~HTTP()
{
	delete[] URI;
}

void NetLib::HTTP::SetURI(char * szURI, int URILength)
{
	URI = new char[URILength];
	this->URILength = URILength;

	strcpy_s(URI, URILength, szURI);
}

int NetLib::HTTP::Request(const char * buffer, int bufferSize, char * response, int * responseSize)
{
	int responseCode = 0;
	fd_set writeSet, exceptionSet;
	timeval tV;
	int retval;
	char* host = new char[stringLength];
	short port;
	char* file = new char[stringLength];

	retval = URIParse(URI, host, &port, file);

	if (retval == 0) {
		delete[] host;
		delete[] file;
		return -1;
	}

	tV.tv_sec = connectTimeOut / 1000;
	tV.tv_usec = connectTimeOut & 1000;

	sock = socket(AF_INET, SOCK_STREAM, 0);

	if (sock == INVALID_SOCKET) {
		delete[] host;
		delete[] file;
		return -1;
	}

	// 논블록으로 connect 하기 위해 논블록 전환
	u_long on = 1;
	ioctlsocket(sock, FIONBIO, &on);
	if (sock == SOCKET_ERROR) {
		delete[] host;
		delete[] file;
		return -1;
	}

	retval = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&responseTimeOut, sizeof(responseTimeOut));
	if (retval == SOCKET_ERROR) {
		delete[] host;
		delete[] file;
		return -1;
	}

	retval = setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&requestTimeOut, sizeof(requestTimeOut));
	if (retval == SOCKET_ERROR) {
		delete[] host;
		delete[] file;
		return -1;
	}

	FD_ZERO(&writeSet);
	FD_ZERO(&exceptionSet);

	FD_SET(sock, &writeSet);
	FD_SET(sock, &exceptionSet);
	
	addrinfo hint;
	addrinfo *list;
	sockaddr_in conAddr;
	
	ZeroMemory(&conAddr, sizeof(conAddr));
	ZeroMemory(&hint, sizeof(hint));

	hint.ai_family = AF_INET;
	hint.ai_socktype = SOCK_STREAM;

	if (getaddrinfo(host, NULL, &hint, &list)) {
		delete[] host;
		delete[] file;
		return -1;
	}

	//conAddr = *(sockaddr_in *)list->ai_addr;

	inet_pton(AF_INET, "127.0.0.1", &conAddr.sin_addr);
	conAddr.sin_port = htons(80);
	conAddr.sin_family = AF_INET;
	
	freeaddrinfo(list);

	connect(sock, (sockaddr *)&conAddr, sizeof(conAddr));

	retval = select(0, NULL, &writeSet, &exceptionSet, &tV);

	// 소켓 에러 
	if (retval == SOCKET_ERROR) {
		delete[] host;
		delete[] file;
		return -1;
	}

	// 타임 아웃
	if (retval == 0) {
		delete[] host;
		delete[] file;
		return -2;
	}

	// 연결 성공 or 연결 실패해서 exceptionSet에 세팅
	if (FD_ISSET(sock, &exceptionSet)) {
		return -3;
	} else if (FD_ISSET(sock, &writeSet)) {
		
		std::string protocol;
		char* sizeTmp = new char[256];
		char* recvBuff = new char[1024];

		int protocolSize;
		do {

			protocol += "POST /";
			protocol.append(file);
			protocol += " HTTP/1.1\r\n";
			protocol += "User-Agent: console\r\n";
			protocol += "Host: ";
			protocol.append(host);
			protocol += "\r\n";
			protocol += "Content-Type: application/x-www-form-urlencoded\r\n";
			protocol += "Content-Length: ";
			//protocol += _itoa(bufferSize - 1, sizeTmp, 10);
			protocol += _itoa_s(bufferSize - 1, sizeTmp, 256, 10);
			protocol += "\r\nConnection: Close\r\n\r\n";
			protocol.append(buffer);

			on = 0;

			ioctlsocket(sock, FIONBIO, &on);

			if (sock == SOCKET_ERROR) {
				delete[] sizeTmp;
				delete[] host;
				delete[] file;
				return WSAGetLastError();
			}

			protocolSize = (int)protocol.size() - 1;
			retval = send(sock, protocol.c_str(), protocolSize, 0);

			if (retval != protocolSize) {
				delete[] sizeTmp;
				delete[] host;
				delete[] file;
				return -1;
			}
			std::string responseString;

			while (1) {
				memset(recvBuff, 0, 1024);

				retval = recv(sock, recvBuff, 1024, 0);

				if (retval == 0) {
					break;
				}
				else if (retval > 0) {
					responseString += recvBuff;
				}
				else {
					delete[] recvBuff;
					delete[] sizeTmp;
					delete[] host;
					delete[] file;

					return WSAGetLastError();
				}
			}


			responseString += '\0';
			char* pResponse = new char[responseString.size()];

			strcpy_s(pResponse, responseString.size(), responseString.c_str());

			char* tmp = strchr(pResponse, 0x20);
			char* tmp2 = strchr(tmp + 1, 0x20);
			*tmp2 = 0;

			responseCode = atoi(tmp + 1);

			if (responseCode != 200) {
				break;
			}

			*tmp2 = 0x20;

			tmp = strstr(pResponse, "Content-Length:");
			tmp += 16;
			tmp2 = strchr(tmp, 0x0d);

			*tmp2 = 0;
			*responseSize = atoi(tmp);
			*tmp2 = 0x0d;

			tmp = strstr(pResponse, "\r\n\r\n");
			tmp += 4;
			
			ZeroMemory(response, *responseSize + 1);

			strcpy_s(response, *responseSize + 1, tmp);

			delete[] pResponse;

		} while (false);

		delete[] sizeTmp;
		delete[] recvBuff;
	}

	delete[] host;
	delete[] file;

	return responseCode;
}

int NetLib::HTTP::URIParse(char * URI, char * host, short * port, char * file)
{
	char* tmp = nullptr;
	char* tmp2 = nullptr;
	char* tmp3 = nullptr;

	tmp = strstr(URI, "://");

	if (tmp == NULL) {
		tmp = strstr(URI, ":");

		if (tmp == NULL) {
			tmp = strstr(URI, "/");

			if (tmp == NULL) {
				std::cout << "error" << std::endl;
			}
			else {
				*tmp = '\0';

				strcpy_s(host, stringLength, URI);
				*tmp = '/';

				strcpy_s(file, stringLength, tmp + 1);
			}
		}
		else {
			*tmp = '\0';

			strcpy_s(host, stringLength, URI);
			*tmp = ':';

			tmp += 1;
			tmp2 = strstr(tmp, "/");

			if (tmp2 == NULL) {
				return 0;
			}
			else {
				*tmp2 = '\0';

				*port = atoi(tmp);

				*tmp2 = '/';

				strcpy_s(file, stringLength, tmp2 + 1);
			}
		}
	}
	else {
		tmp += 3;
		tmp2 = strstr(tmp, ":");

		if (tmp2 == NULL) {
			tmp2 = strstr(tmp, "/");

			if (tmp2 == NULL) {
				std::cout << "error" << std::endl;
			}
			else {
				*tmp2 = '\0';

				strcpy_s(host, stringLength, tmp);
				*tmp2 = '/';
				tmp2 += 1;

				strcpy_s(file, stringLength, tmp2);
			}
		}
		else {
			*tmp2 = '\0';

			strcpy_s(host, stringLength, tmp);
			*tmp2 = ':';
			tmp2 += 1;

			tmp3 = strstr(tmp2, "/");

			if (tmp3 == NULL) {
				return 0;
			}
			else {
				*tmp3 = '\0';

				*port = atoi(tmp2);

				*tmp3 = '/';

				strcpy_s(file, stringLength, tmp3 + 1);
			}
		}
	}

	return 1;
}

