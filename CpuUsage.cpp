#include "pch.h"
#include "CpuUsage.h"


NetLib::CpuUsage::CpuUsage()
{
	hProcess = GetCurrentProcess();

	SYSTEM_INFO si;

	GetSystemInfo(&si);

	iNumberofProcessors = si.dwNumberOfProcessors;

	fProcessorTotal = 0;
	fProcessorUser = 0;
	fProcessorKernel = 0;

	fProcessTotal = 0;
	fProcessUser = 0;
	fProcessKernel = 0;

	ftProcessor_LastKernel.QuadPart = 0;
	ftProcessor_LastUser.QuadPart = 0;
	ftProcessor_LastIdle.QuadPart = 0;

	ftProcess_LastUser.QuadPart = 0;
	ftProcess_LastKernel.QuadPart = 0;
	ftProcess_LastTime.QuadPart = 0;

	UpdateCpuTime();
}

NetLib::CpuUsage::CpuUsage(HANDLE hProcess)
{
	hProcess = hProcess;

	SYSTEM_INFO si;

	GetSystemInfo(&si);

	iNumberofProcessors = si.dwNumberOfProcessors;

	fProcessorTotal = 0;
	fProcessorUser = 0;
	fProcessorKernel = 0;

	fProcessTotal = 0;
	fProcessUser = 0;
	fProcessKernel = 0;

	ftProcessor_LastKernel.QuadPart = 0;
	ftProcessor_LastUser.QuadPart = 0;
	ftProcessor_LastIdle.QuadPart = 0;

	ftProcess_LastUser.QuadPart = 0;
	ftProcess_LastKernel.QuadPart = 0;
	ftProcess_LastTime.QuadPart = 0;

	UpdateCpuTime();
}

void NetLib::CpuUsage::UpdateCpuTime(void)
{
	ULARGE_INTEGER idle;
	ULARGE_INTEGER kernel;
	ULARGE_INTEGER user;

	if (GetSystemTimes((PFILETIME)&idle, (PFILETIME)&kernel, (PFILETIME)&user) == false) {
		return;
	}

	ULONGLONG kernelDiff = kernel.QuadPart - ftProcessor_LastKernel.QuadPart;
	ULONGLONG userDiff = user.QuadPart - ftProcessor_LastUser.QuadPart;
	ULONGLONG idleDiff = idle.QuadPart - ftProcessor_LastIdle.QuadPart;

	ULONGLONG total = kernelDiff + userDiff;
	ULONGLONG timeDiff;

	if (total == 0) {
		fProcessorUser = 0.0f;
		fProcessorKernel = 0.0f;
		fProcessorTotal = 0.0f;
	}
	else {
		fProcessorTotal = (float)((double)(total - idleDiff) / total * 100.0f);
		fProcessorUser = (float)((double)userDiff / total * 100.0f);
		fProcessorKernel = (float)((double)(kernelDiff - idleDiff) / total * 100.0f);
	}

	ftProcessor_LastKernel = kernel;
	ftProcessor_LastUser = user;
	ftProcessor_LastIdle = idle;

	ULARGE_INTEGER none;
	ULARGE_INTEGER nowTime;

	GetSystemTimeAsFileTime((LPFILETIME)&nowTime);

	GetProcessTimes(hProcess, (LPFILETIME)&none, (LPFILETIME)&none, (LPFILETIME)&kernel, (LPFILETIME)&user);

	timeDiff = nowTime.QuadPart - ftProcess_LastTime.QuadPart;
	userDiff = user.QuadPart - ftProcess_LastUser.QuadPart;
	kernelDiff = kernel.QuadPart - ftProcess_LastKernel.QuadPart;

	total = kernelDiff + userDiff;

	fProcessTotal = (float)(total / (double)iNumberofProcessors / (double)timeDiff * 100.0f);
	fProcessKernel = (float)(kernelDiff / (double)iNumberofProcessors / (double)timeDiff * 100.0f);
	fProcessUser = (float)(userDiff / (double)iNumberofProcessors / (double)timeDiff * 100.0f);

	ftProcess_LastTime = nowTime;
	ftProcess_LastKernel = kernel;
	ftProcess_LastUser = user;

}


NetLib::CpuUsage::~CpuUsage()
{
}
