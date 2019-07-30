#include "pch.h"
#include "Profile.h"
#include <windows.h>
#include <iostream>

ProfileData profileData[MAX_PROFILE_TAG];

void ProfileBegin(WCHAR* szName)
{
	ProfileData *pD = nullptr;

	for (int i = 0; i < MAX_PROFILE_TAG; ++i)
	{
		if (wcscmp(profileData[i].szName, szName) == 0)
		{
			pD = &profileData[i];
			break;
		}
	}

	if (pD == nullptr)
	{
		for (int i = 0; i < MAX_PROFILE_TAG; ++i)
		{
			if (profileData[i].lFlag == 0)
			{
				pD = &profileData[i];

				wcscpy_s(pD->szName, szName);

				break;
			}
		}
	}

	if (pD == nullptr)
	{
		return;
	}

	pD->lFlag = 1;

	QueryPerformanceCounter(&pD->liStartTime);
}

void ProfileEnd(WCHAR* szName)
{
	LARGE_INTEGER tmp;

	QueryPerformanceCounter(&tmp);

	ProfileData *pD = nullptr;

	for (int i = 0; i < MAX_PROFILE_TAG; ++i)
	{
		if (wcscmp(profileData[i].szName, szName) == 0)
		{
			pD = &profileData[i];
			break;
		}
	}

	if (pD == nullptr)
	{
		return;
	}

	if (pD->lFlag == 0)
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

void ProfileDataOutText(WCHAR* szFileName)
{
	FILE* fp = nullptr;

	LARGE_INTEGER liFre;

	QueryPerformanceFrequency(&liFre);

	errno_t err = _wfopen_s(&fp, szFileName, L"w");

	if (err != 0)
	{
		//////// throw 가 들어가야 할거 같지만 일단 return
		return;
	}

	fwprintf(fp, L"-------------------------------------------------------------------------------\n");
	fwprintf(fp, L"           Name  |     Average  |        Min   |        Max   |      Call |\n");
	fwprintf(fp, L"-------------------------------------------------------------------------------\n");

	for (int i = 0; i < MAX_PROFILE_TAG; ++i)
	{
		if (profileData[i].lFlag == 0)
			continue;

		fwprintf(fp, L"%16s | %12.4lf | %12.4lf | %12.4lf | %9I64d |\n",
			profileData[i].szName, (profileData[i].iTotalTime - profileData[i].iMax[0] - profileData[i].iMax[1] - profileData[i].iMin[0] - profileData[i].iMin[1]) / (profileData[i].iCall - 4) / static_cast<double>(liFre.QuadPart) * 1000000,
			profileData[i].iMin[1] / static_cast<double>(liFre.QuadPart) * 1000000, profileData[i].iMax[1] / static_cast<double>(liFre.QuadPart) * 1000000, profileData[i].iCall);
	}
	fwprintf(fp, L"-------------------------------------------------------------------------------\n");
	fclose(fp);
}

void ProfileReset()
{
	for (int i = 0; i < MAX_PROFILE_TAG; ++i)
	{
		profileData[i].liStartTime.QuadPart = 0;
		profileData[i].iCall = 0;
		profileData[i].iMin[0] = _I64_MAX;
		profileData[i].iMin[1] = _I64_MAX;
		profileData[i].iMax[0] = _I64_MIN;
		profileData[i].iMax[1] = _I64_MIN;
		profileData[i].iTotalTime = 0;
		profileData[i].szName[0] = '\0';
		profileData[i].lFlag = 0;
	}
}