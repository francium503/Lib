#pragma once

namespace NetLib {

	class Log {

		enum class eLogLevel
		{
			eLogDebug,
			eLogWarning,
			eLogError,
			eLogSystem,
		};

	public:
		Log();
		Log(WCHAR *szDir);
		~Log();

		void LogDir(WCHAR *szDir);
		void SysLog(WCHAR *szType, eLogLevel logLevel, WCHAR* szStringFormat, ...);
		void SysLogHex(WCHAR *szType, eLogLevel logLevel, WCHAR *szLog, BYTE *pByte, int iByteLen);

	private:
		WCHAR *_szDir;
		BOOL _consoleLog;
		eLogLevel _logLevel;
		SRWLOCK _srwLock;


	};
}