#include "pch.h"
#include "Log.h"

void NetLib::Log::SysLog(WCHAR * szType, eLogLevel logLevel, WCHAR * szStringFormat, ...)
{
	if (logLevel < _logLevel) {
		return;
	}

	WCHAR LogString[1024];
	va_list vaList;
	va_list value;

	va_start(vaList, szStringFormat);
	va_copy(value, vaList);

	HRESULT result = StringCbPrintf(LogString, 1024 * sizeof(WCHAR), szStringFormat, );
}
