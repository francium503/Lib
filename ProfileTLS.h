#pragma once

#include <windows.h>

#define MAX_PROFILE_TAG 255
#define MAX_PROFILE_THREAD 10

#ifdef PROFILEING
	#define PRO_BEGIN(x) NetLib::ProfileTLS::GetInstance()->TLSProfileBegin(x)
	#define PRO_END(x) NetLib::ProfileTLS::GetInstance()->TLSProfileEnd(x)
	#define PRO_RESULT(x) NetLib::ProfileTLS::GetInstance()->TLSProfileDataOutText(x)
#else
	#define PRO_BEGIN(x)
	#define PRO_END(x)
	#define PRO_RESULT(x)
#endif

namespace NetLib {
	class ProfileTLS {
	public:
		struct ProfileDataTLS
		{
			long lFlag = 0;
			WCHAR szName[64];
			LARGE_INTEGER liStartTime;
			__int64 iTotalTime = 0;
			__int64 iMin[2] = { _I64_MAX, _I64_MAX };
			__int64 iMax[2] = { _I64_MIN, _I64_MIN };
			__int64 iCall = 0;
		};

		struct ThreadProfileData
		{
			DWORD dwThreadId;
			ProfileDataTLS profileData[MAX_PROFILE_TAG];
			long lFlag = 0;
		};

	private:
		ProfileTLS();
		~ProfileTLS();

	public:
		static ProfileTLS* GetInstance(void);
		void TLSProfileBegin(WCHAR *szName);
		void TLSProfileEnd(WCHAR *szName);

		void TLSProfileDataOutText(WCHAR *szFileName);
		void TLSProfileReset(void);

	protected:
		ProfileDataTLS* GetThreadProfileDataTLS(DWORD threadId);

	protected:
		ThreadProfileData *threadProfileData;
		int iMaxProfileThreadCount;

	};
}