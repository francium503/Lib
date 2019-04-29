#include "pch.h"
#include "Log.h"

NetLib::Log::Log() {
	_szDir = new WCHAR[_MAX_DIR];
	_consoleLog = TRUE;
	_logLevel = eLogLevel::eLogDebug;
	InitializeSRWLock(&_srwLock);
}

NetLib::Log::~Log()
{
	delete[] _szDir;
}

NetLib::Log * NetLib::Log::GetInstance(void)
{
	static NetLib::Log Log;
	return &Log;
}

void NetLib::Log::LogDir(WCHAR * szDir)
{
	wcscpy_s(_szDir, _MAX_DIR, szDir);
}

void NetLib::Log::SetLogLevel(eLogLevel logLevel)
{
	_logLevel = logLevel;
}

void NetLib::Log::SetConsole(BOOL consoleWrite)
{
	_consoleLog = consoleWrite;
}

void NetLib::Log::SysLog(WCHAR * szType, eLogLevel logLevel, WCHAR * szStringFormat, ...)
{
	if (logLevel < _logLevel) {
		return;
	}

	WCHAR LogString[eLogStringLength];
	va_list vaList;

	va_start(vaList, szStringFormat);
	HRESULT result = StringCbVPrintf(LogString, eLogStringLength * sizeof(WCHAR), szStringFormat, vaList);

	if (result != S_OK) {
		NetLib::Log::GetInstance()->SysLog(const_cast<WCHAR *>(L"SYSTEM"), eLogLevel::eLogError, const_cast<WCHAR *>(L"StringCbVPrintf Error %s"), LogString);
		return;
	}

	WCHAR formatedLog[eFormatedLogStringLength];
	SYSTEMTIME stNowTime;
	
	GetLocalTime(&stNowTime);
	switch (logLevel) {
	case eLogLevel::eLogDebug:
		result = StringCchPrintf(formatedLog, eFormatedLogStringLength, L"[%d-%02d-%02d %02d:%02d:%02d] %s\t\t|%s\t\t|%s", stNowTime.wYear, stNowTime.wMonth, stNowTime.wDay, stNowTime.wHour, stNowTime.wMinute, stNowTime.wSecond, szType, L"DEBUG", LogString);
		break;

	case eLogLevel::eLogWarning:
		result = StringCchPrintf(formatedLog, eFormatedLogStringLength, L"[%d-%02d-%02d %02d:%02d:%02d] %s\t\t|%s\t\t|%s", stNowTime.wYear, stNowTime.wMonth, stNowTime.wDay, stNowTime.wHour, stNowTime.wMinute, stNowTime.wSecond, szType, L"WARNING", LogString);
		break;

	case eLogLevel::eLogError:
		result = StringCchPrintf(formatedLog, eFormatedLogStringLength, L"[%d-%02d-%02d %02d:%02d:%02d] %s\t\t|%s\t\t|%s", stNowTime.wYear, stNowTime.wMonth, stNowTime.wDay, stNowTime.wHour, stNowTime.wMinute, stNowTime.wSecond, szType, L"ERROR", LogString);
		break;

	case eLogLevel::eLogSystem:
		result = StringCchPrintf(formatedLog, eFormatedLogStringLength, L"[%d-%02d-%02d %02d:%02d:%02d] %s\t\t|%s\t\t|%s", stNowTime.wYear, stNowTime.wMonth, stNowTime.wDay, stNowTime.wHour, stNowTime.wMinute, stNowTime.wSecond, szType, L"SYSTEM", LogString);
		break;

	default:
		result = StringCchPrintf(formatedLog, eFormatedLogStringLength, L"[%d-%02d-%02d %02d:%02d:%02d] %s\t\t|%d\t\t|%s", stNowTime.wYear, stNowTime.wMonth, stNowTime.wDay, stNowTime.wHour, stNowTime.wMinute, stNowTime.wSecond, szType, logLevel, LogString);
		break;
	}

	if (result != S_OK) {
		NetLib::Log::GetInstance()->SysLog(const_cast<WCHAR *>(L"SYSTEM"), eLogLevel::eLogError, const_cast<WCHAR *>(L"StringCchPrintf Error %s"), formatedLog);
		return;
	}
	
	int retval = _wmkdir(_szDir);

	FILE *fp = nullptr;
	WCHAR fileName[_MAX_DIR];

	StringCchPrintf(fileName, _MAX_DIR, L"%s\\%s_%d%02d.txt", _szDir, szType, stNowTime.wYear, stNowTime.wMonth);

	AcquireSRWLockExclusive(&_srwLock);

	do {
		_wfopen_s(&fp, fileName, L"a");

		if (fp == nullptr) {
			NetLib::Log::GetInstance()->SysLog(const_cast<WCHAR *>(L"SYSTEM"), eLogLevel::eLogError, const_cast<WCHAR *>(L"fopen Error"));
			break;
		}

		fwprintf(fp, L"%s\n", formatedLog);

		fclose(fp);
	} while (0);

	ReleaseSRWLockExclusive(&_srwLock);

	if (_consoleLog) {
		wprintf(L"%s\n", formatedLog);
	}

	return;
}

void NetLib::Log::SysLogHex(WCHAR * szType, eLogLevel logLevel, WCHAR * szLog, BYTE * pByte, int iByteLen)
{
	if (logLevel < _logLevel) {
		return;
	}

	WCHAR formatedLog[eFormatedLogStringLength];
	SYSTEMTIME stNowTime;

	GetLocalTime(&stNowTime);
	HRESULT result;
	switch (logLevel) {
	case eLogLevel::eLogDebug:
		result = StringCchPrintf(formatedLog, eFormatedLogStringLength, L"[%d-%02d-%02d %02d:%02d:%02d] %s\t\t|%s\t\t|0x", stNowTime.wYear, stNowTime.wMonth, stNowTime.wDay, stNowTime.wHour, stNowTime.wMinute, stNowTime.wSecond, szType, L"DEBUG");
		break;

	case eLogLevel::eLogWarning:
		result = StringCchPrintf(formatedLog, eFormatedLogStringLength, L"[%d-%02d-%02d %02d:%02d:%02d] %s\t\t|%s\t\t|0x", stNowTime.wYear, stNowTime.wMonth, stNowTime.wDay, stNowTime.wHour, stNowTime.wMinute, stNowTime.wSecond, szType, L"WARNING");
		break;

	case eLogLevel::eLogError:
		result = StringCchPrintf(formatedLog, eFormatedLogStringLength, L"[%d-%02d-%02d %02d:%02d:%02d] %s\t\t|%s\t\t|0x", stNowTime.wYear, stNowTime.wMonth, stNowTime.wDay, stNowTime.wHour, stNowTime.wMinute, stNowTime.wSecond, szType, L"ERROR");
		break;

	case eLogLevel::eLogSystem:
		result = StringCchPrintf(formatedLog, eFormatedLogStringLength, L"[%d-%02d-%02d %02d:%02d:%02d] %s\t\t|%s\t\t|0x", stNowTime.wYear, stNowTime.wMonth, stNowTime.wDay, stNowTime.wHour, stNowTime.wMinute, stNowTime.wSecond, szType, L"SYSTEM");
		break;

	default:
		result = StringCchPrintf(formatedLog, eFormatedLogStringLength, L"[%d-%02d-%02d %02d:%02d:%02d] %s\t\t|%d\t\t|0x", stNowTime.wYear, stNowTime.wMonth, stNowTime.wDay, stNowTime.wHour, stNowTime.wMinute, stNowTime.wSecond, szType, logLevel);
		break;
	}

	if (result != S_OK) {
		NetLib::Log::GetInstance()->SysLog(const_cast<WCHAR *>(L"SYSTEM"), eLogLevel::eLogError, const_cast<WCHAR *>(L"StringCchPrintf Error %s"), formatedLog);
		return;
	}

	int retval = _wmkdir(_szDir);

	FILE *fp = nullptr;
	WCHAR fileName[_MAX_DIR];

	StringCchPrintf(fileName, _MAX_DIR, L"%s\\%s_%d%02d.txt", _szDir, szType, stNowTime.wYear, stNowTime.wMonth);

	AcquireSRWLockExclusive(&_srwLock);

	do {
		_wfopen_s(&fp, fileName, L"a");

		if (fp == nullptr) {
			NetLib::Log::GetInstance()->SysLog(const_cast<WCHAR *>(L"SYSTEM"), eLogLevel::eLogError, const_cast<WCHAR *>(L"fopen Error"));
			break;
		}

		fwprintf(fp, L"%s", formatedLog);
		for (int i = 0; i < iByteLen; ++i) {
			fwprintf(fp, L"%X", pByte[i]);
		}
		fwprintf(fp, L"\n");

		fclose(fp);
	} while (0);

	ReleaseSRWLockExclusive(&_srwLock);

	if (_consoleLog) {

		wprintf(L"%s", formatedLog);
		for (int i = 0; i < iByteLen; ++i) {
			wprintf(L"%X", pByte[i]);
		}
		wprintf(L"\n");
	}

	return;
}
