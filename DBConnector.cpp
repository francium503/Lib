#include "pch.h"
#include "DBConnector.h"

NetLib::DBConnector::DBConnector(WCHAR* szDBIP, WCHAR* szUser, WCHAR* szPassword, WCHAR* szDBName, int iDBPort)
{
	memcpy_s(_szDBIP, sizeof(_szDBIP), szDBIP, sizeof(_szDBIP));
	memcpy_s(_szDBUser, sizeof(_szDBUser), szUser, sizeof(_szDBUser));
	memcpy_s(_szDBPassword, sizeof(_szDBPassword), szPassword, sizeof(_szDBPassword));
	memcpy_s(_szDBName, sizeof(_szDBName), szDBName, sizeof(_szDBName));
	_iDBPort = iDBPort;

	mysql_init(&_MySQL);
}

NetLib::DBConnector::~DBConnector()
{
	Disconnect();
}

bool NetLib::DBConnector::Connect()
{
	char* DBIP;
	char* DBUser;
	char* DBPassword;
	char* DBName;

	int reconnectCount = 0;

	int wcharSize = WideCharToMultiByte(CP_ACP, 0, _szDBIP, -1, NULL, 0, NULL, NULL);
	DBIP = new char[wcharSize];
	WideCharToMultiByte(CP_ACP, 0, _szDBIP, -1, DBIP, wcharSize, 0, 0);
	
	wcharSize = WideCharToMultiByte(CP_ACP, 0, _szDBUser, -1, NULL, 0, NULL, NULL);
	DBUser = new char[wcharSize];
	WideCharToMultiByte(CP_ACP, 0, _szDBUser, -1, DBUser, wcharSize, 0, 0);

	wcharSize = WideCharToMultiByte(CP_ACP, 0, _szDBPassword, -1, NULL, 0, NULL, NULL);
	DBPassword = new char[wcharSize];
	WideCharToMultiByte(CP_ACP, 0, _szDBPassword, -1, DBPassword, wcharSize, 0, 0);

	wcharSize = WideCharToMultiByte(CP_ACP, 0, _szDBName, -1, NULL, 0, NULL, NULL);
	DBName = new char[wcharSize];
	WideCharToMultiByte(CP_ACP, 0, _szDBName, -1, DBName, wcharSize, 0, 0);

	_pMySQL = mysql_real_connect(&_MySQL, DBIP, DBUser, DBPassword, DBName, _iDBPort, (char *)NULL, 0);

	while (_pMySQL == NULL){
		++reconnectCount;

		_pMySQL = mysql_real_connect(&_MySQL, DBIP, DBUser, DBPassword, DBName, _iDBPort, (char *)NULL, 0);

		if (_pMySQL != NULL)
			break;


		Sleep(100);

		if(reconnectCount >= 5)
		{
			SaveLastError();
			if (_iLastError == CR_SOCKET_CREATE_ERROR ||
				_iLastError == CR_CONNECTION_ERROR ||
				_iLastError == CR_CONN_HOST_ERROR ||
				_iLastError == CR_SERVER_GONE_ERROR ||
				_iLastError == CR_TCP_CONNECTION ||
				_iLastError == CR_SERVER_HANDSHAKE_ERR ||
				_iLastError == CR_SERVER_LOST ||
				_iLastError == CR_INVALID_CONN_HANDLE)
			{
				continue;
			}
			

			delete[] DBIP;
			delete[] DBUser;
			delete[] DBPassword;
			delete[] DBName;

			return false;
		}
	}

	mysql_set_character_set(_pMySQL, "utf8");

	delete[] DBIP;
	delete[] DBUser;
	delete[] DBPassword;
	delete[] DBName;

	return true;
}

bool NetLib::DBConnector::Disconnect()
{
	mysql_close(_pMySQL);

	return true;
}

bool NetLib::DBConnector::Query(WCHAR* szStringFormat, ...)
{
	va_list vaList;
	va_start(vaList, szStringFormat);

	StringCbVPrintf(_szQuery, eQUERY_MAX_LEN * sizeof(WCHAR), szStringFormat, vaList);
	WideCharToMultiByte(CP_ACP, 0, _szQuery, -1, _szQueryUTF8, eQUERY_MAX_LEN - 1, 0, 0);

	int query_stat = mysql_query(_pMySQL, _szQueryUTF8);

	if(query_stat != 0)
	{
		SaveLastError();
		return false;
	}

	_pSqlResult = mysql_store_result(_pMySQL);

	return true;
}

bool NetLib::DBConnector::Query_Save(WCHAR* szStringFormat, ...)
{
	va_list vaList;
	va_start(vaList, szStringFormat);

	StringCbVPrintf(_szQuery, eQUERY_MAX_LEN * sizeof(WCHAR), szStringFormat, vaList);
	WideCharToMultiByte(CP_ACP, 0, _szQuery, -1, _szQueryUTF8, eQUERY_MAX_LEN - 1, 0, 0);

	int query_stat = mysql_query(_pMySQL, _szQueryUTF8);

	if (query_stat != 0)
	{
		SaveLastError();
		return false;
	}

	return true;
}

MYSQL_ROW NetLib::DBConnector::FetchRow()
{
	return mysql_fetch_row(_pSqlResult);
}

void NetLib::DBConnector::FreeResult()
{
	mysql_free_result(_pSqlResult);
}

void NetLib::DBConnector::SaveLastError()
{
	_iLastError = mysql_errno(&_MySQL);
	const char *err = mysql_error(&_MySQL);
	MultiByteToWideChar(CP_ACP, 0, err, -1, _szLastErrorMsg, 127);
}