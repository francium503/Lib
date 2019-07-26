#pragma once

#pragma comment(lib, "./mysql/lib/vs14/mysqlclient.lib")

#include "./mysql/include/mysql.h"
#include "./mysql/include/errmsg.h"
#include <strsafe.h>

namespace NetLib
{

	class DBConnector
	{
	public:

		enum en_DB_CONNECTOR
		{
			eQUERY_MAX_LEN = 2048

		};

		DBConnector(WCHAR *szDBIP, WCHAR *szUser, WCHAR *szPassword, WCHAR *szDBName, int iDBPort);
		virtual ~DBConnector();

		//////////////////////////////////////////////////////////////////////
		// MySQL DB 연결
		//////////////////////////////////////////////////////////////////////
		bool Connect(void);

		//////////////////////////////////////////////////////////////////////
		// MySQL DB 끊기
		//////////////////////////////////////////////////////////////////////
		bool Disconnect(void);


		//////////////////////////////////////////////////////////////////////
		// 쿼리 날리고 결과셋 임시 보관
		//
		//////////////////////////////////////////////////////////////////////
		bool Query(WCHAR *szStringFormat, ...);
		bool Query_Save(WCHAR *szStringFormat, ...); // DBWriter 스레드의 Save 쿼리 전용
		// 결과셋을 저장하지 않음.

		//////////////////////////////////////////////////////////////////////
		// 쿼리를 날린 뒤에 결과 뽑아오기.
		//
		// 결과가 없다면 NULL 리턴.
		//////////////////////////////////////////////////////////////////////
		MYSQL_ROW FetchRow(void);

		//////////////////////////////////////////////////////////////////////
		// 한 쿼리에 대한 결과 모두 사용 후 정리.
		//////////////////////////////////////////////////////////////////////
		void FreeResult(void);


		//////////////////////////////////////////////////////////////////////
		// Error 얻기.한 쿼리에 대한 결과 모두 사용 후 정리.
		//////////////////////////////////////////////////////////////////////
		int GetLastError(void) { return _iLastError; };
		WCHAR *GetLastErrorMsg(void) { return _szLastErrorMsg; }


	private:

		//////////////////////////////////////////////////////////////////////
		// mysql 의 LastError 를 맴버변수로 저장한다.
		//////////////////////////////////////////////////////////////////////
		void SaveLastError(void);

	private:



		//-------------------------------------------------------------
		// MySQL 연결객체 본체
		//-------------------------------------------------------------
		MYSQL _MySQL;

		//-------------------------------------------------------------
		// MySQL 연결객체 포인터. 위 변수의 포인터임. 
		// 이 포인터의 null 여부로 연결상태 확인.
		//-------------------------------------------------------------
		MYSQL *_pMySQL;

		//-------------------------------------------------------------
		// 쿼리를 날린 뒤 Result 저장소.
		//
		//-------------------------------------------------------------
		MYSQL_RES *_pSqlResult;

		WCHAR _szDBIP[16];
		WCHAR _szDBUser[64];
		WCHAR _szDBPassword[64];
		WCHAR _szDBName[64];
		int _iDBPort;


		WCHAR _szQuery[eQUERY_MAX_LEN];
		char _szQueryUTF8[eQUERY_MAX_LEN];

		int _iLastError;
		WCHAR _szLastErrorMsg[128];

	};
}