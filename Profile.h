#pragma once
#include <windows.h>

#define MAX_PROFILE_TAG 255

#ifdef PROFILING
	#define PRO_BEGIN(x) ProfileBegin(x)
	#define PRO_END(x) ProfileEnd(x)
#else
	#define PRO_BEGIN(x)
	#define PRO_END(x)
#endif

struct ProfileData
{
	long lFlag = 0;
	WCHAR szName[64];
	LARGE_INTEGER liStartTime;
	__int64 iTotalTime = 0;
	__int64 iMin[2] = { _I64_MAX, _I64_MAX };
	__int64 iMax[2] = { _I64_MIN,_I64_MIN };
	__int64 iCall = 0;
};

void ProfileBegin(WCHAR *szName);
void ProfileEnd(WCHAR *szName);

void ProfileDataOutText(WCHAR *szFileName);
void ProfileReset(void);