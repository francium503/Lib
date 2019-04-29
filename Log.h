#pragma once

namespace NetLib {

	class Log {
	public:
		enum class eLogLevel
		{
			eLogDebug,
			eLogWarning,
			eLogError,
			eLogSystem,
		};

		enum LogStringLength {
			eLogStringLength = 1024,
			eFormatedLogStringLength = 2048,
		};

	private:
		Log();
		~Log();

	public:
		static Log* GetInstance(void);
		void LogDir(WCHAR *szDir);
		void SetLogLevel(eLogLevel logLevel);
		void SetConsole(BOOL consoleWrite);
		void SysLog(WCHAR *szType, eLogLevel logLevel, WCHAR* szStringFormat, ...);
		void SysLogHex(WCHAR *szType, eLogLevel logLevel, WCHAR *szLog, BYTE *pByte, int iByteLen);

	private:
		WCHAR *_szDir;
		BOOL _consoleLog;
		eLogLevel _logLevel;
		SRWLOCK _srwLock;

	};
}