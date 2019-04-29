#include "pch.h"
#include "Log.h"

void NetLib::Log::SysLog(WCHAR * szType, eLogLevel logLevel, WCHAR * szStringFormat, ...)
{
	if (logLevel < _logLevel) {
		return;
	}

	WCHAR LogString[1024];
	va_list vaList;

	va_start(vaList, szStringFormat);
	vswprintf_s(LogString, szStringFormat, vaList);


}
