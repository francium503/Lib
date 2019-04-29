#pragma once

namespace NetLib {
	class HTTP
	{
	public:
		HTTP();
		HTTP(int connectTimeOut, int requestTimeOut, int responseTimeOut);
		~HTTP();

		void SetURI(char* szURI, int URILength);

		int Request(const char* buffer, int bufferSize, char* response, int* responseSize);
		int URIRequest(WCHAR* szURIAddr, const char* buffer, int bufferSize, char* response, int* responseSize);

	private:
		SOCKET sock;
		char* URI;
		int URILength;
		int stringLength;

		int connectTimeOut;
		int requestTimeOut;
		int responseTimeOut;

		int URIParse(char* URI, char* host, short* port, char* file);
	};

}