#include "pch.h"
#include "ProfileTLS.h"
#include "MiniDump.h"


NetLib::ProfileTLS::ProfileTLS()
{
	this->iMaxProfileThreadCount = MAX_PROFILE_THREAD;
	threadProfileData = new ThreadProfileData[iMaxProfileThreadCount];
}

NetLib::ProfileTLS::~ProfileTLS()
{
	delete[] threadProfileData;
}

NetLib::ProfileTLS* NetLib::ProfileTLS::GetInstance()
{
	static ProfileTLS pTls;
	return &pTls;
}

void NetLib::ProfileTLS::TLSProfileBegin(WCHAR* szName)
{
	ProfileDataTLS *threadList = nullptr;
	ProfileDataTLS *pD = nullptr;
	DWORD threadId = GetCurrentThreadId();

	threadList = GetThreadProfileDataTLS(threadId);

	if(threadList == nullptr)
	{
		for(int i=0;i<iMaxProfileThreadCount; ++i)
		{
			if(threadProfileData[i].lFlag == 0)
			{
				threadProfileData[i].dwThreadId = threadId;
				threadProfileData[i].lFlag = 1;

				threadList = threadProfileData[i].profileData;
				break;
			}
		}
	}

	if (threadList == nullptr) {
		//최대치 초과
		return;
	}

	for(int i=0;i<MAX_PROFILE_TAG;++i)
	{
		if(wcscmp(threadList[i].szName, szName) == 0)
		{
			pD = &threadList[i];
			break;
		}
	}

	if(pD == nullptr)
	{
		for(int i=0;i<MAX_PROFILE_TAG; ++i)
		{
			if(threadList[i].lFlag == 0)
			{
				pD = &threadList[i];

				wcscpy_s(pD->szName, szName);

				break;
			}
		}
	}

	if(pD== nullptr)
	{
		return;
	}

	pD->lFlag = 1;

	QueryPerformanceCounter(&pD->liStartTime);
}

void NetLib::ProfileTLS::TLSProfileEnd(WCHAR* szName)
{
	LARGE_INTEGER tmp;
	QueryPerformanceCounter(&tmp);
	
	ProfileDataTLS *pD = nullptr;
	ProfileDataTLS *threadList = nullptr;
	DWORD error;
	DWORD threadId = GetCurrentThreadId();

	threadList = GetThreadProfileDataTLS(threadId);

	if (threadList == nullptr)
	{
		return;
	}

	for (int i = 0; i < MAX_PROFILE_TAG; ++i)
	{
		if (wcscmp(threadList[i].szName, szName) == 0)
		{
			pD = &threadList[i];
			break;
		}
	}

	if (pD == nullptr)
	{
		return;
	}

	if(pD->lFlag == 0)
	{
		return;
	}

	__int64 time = tmp.QuadPart - pD->liStartTime.QuadPart;
	pD->iCall++;
	pD->iTotalTime += time;

	if (pD->iMin[0] > time)
	{
		if (pD->iMin[1] > time)
			pD->iMin[1] = time;
		else
			pD->iMin[0] = time;
	}

	if (pD->iMax[0] < time)
	{
		if (pD->iMax[1] < time)
			pD->iMax[1] = time;
		else
			pD->iMax[0] = time;
	}

	pD->liStartTime.QuadPart = 0;
}

void NetLib::ProfileTLS::TLSProfileDataOutText(WCHAR* szFileName)
{
	FILE* fp = nullptr;
	LARGE_INTEGER liFre;
	ProfileDataTLS *pTlsData;

	QueryPerformanceFrequency(&liFre);

	errno_t err = _wfopen_s(&fp, szFileName, L"a");

	if (err != 0)
	{
		CrashDump::Crash();
		return;
	}

	fwprintf(fp, L"-------------------------------------------------------------------------------\n");

	for (int i = 0; i < iMaxProfileThreadCount; ++i) {
		if (threadProfileData[i].lFlag == 0)
			continue;

		pTlsData = threadProfileData[i].profileData;
		
		fwprintf(fp, L"-----------------------------ThreadID : %10d---------------------------\n", threadProfileData[i].dwThreadId);
		fwprintf(fp, L"-------------------------------------------------------------------------------\n");
		fwprintf(fp, L"            Name  |      Average  |         Min   |         Max   |       Call |\n");
		fwprintf(fp, L"-------------------------------------------------------------------------------\n");

		for (int j = 0; j < MAX_PROFILE_TAG; ++j)
		{
			if (pTlsData[j].lFlag == 0)
				continue;

			fwprintf(fp, L"  %16s | %12.4lf | %12.4lf | %12.4lf | %9I64d |\n",
				pTlsData[j].szName, (pTlsData[j].iTotalTime - pTlsData[j].iMax[0] - pTlsData[j].iMax[1] - pTlsData[j].iMin[0] - pTlsData[j].iMin[1]) / (pTlsData[j].iCall - 4) / static_cast<double>(liFre.QuadPart) * 1000000,
				pTlsData[j].iMin[1] / static_cast<double>(liFre.QuadPart) * 1000000, pTlsData[j].iMax[1] / static_cast<double>(liFre.QuadPart) * 1000000, pTlsData[j].iCall);
		}
		fwprintf(fp, L"-------------------------------------------------------------------------------\n");
	}
	fclose(fp);
}

void NetLib::ProfileTLS::TLSProfileReset()
{
	ProfileDataTLS *pTlsData = nullptr;

	for(int i=0;i<iMaxProfileThreadCount; ++i)
	{
		threadProfileData[i].lFlag = 0;

		pTlsData = threadProfileData[i].profileData;
		
		for (int i = 0; i < MAX_PROFILE_TAG; ++i)
		{
			pTlsData[i].liStartTime.QuadPart = 0;
			pTlsData[i].iCall = 0;
			pTlsData[i].iMin[0] = _I64_MAX;
			pTlsData[i].iMin[1] = _I64_MAX;
			pTlsData[i].iMax[0] = _I64_MIN;
			pTlsData[i].iMax[1] = _I64_MIN;
			pTlsData[i].iTotalTime = 0;
			pTlsData[i].szName[0] = '\0';
			pTlsData[i].lFlag = 0;
		}
	}
	
}

NetLib::ProfileTLS::ProfileDataTLS* NetLib::ProfileTLS::GetThreadProfileDataTLS(DWORD threadId)
{
	for(int i=0;i<iMaxProfileThreadCount;++i)
	{
		if (threadProfileData[i].dwThreadId == threadId)
			return threadProfileData[i].profileData;
	}

	return nullptr;
}


