#pragma once

namespace NetLib {

	class CpuUsage
	{
	public:
		CpuUsage();
		CpuUsage(HANDLE hProcess);

		void UpdateCpuTime(void);

		float ProcessorTotal(void) { return fProcessorTotal; }
		float ProcessorUser(void) { return fProcessorUser; }
		float ProcessorKernel(void) { return fProcessorKernel; }

		float ProcessTotal(void) { return fProcessTotal; }
		float ProcessUser(void) { return fProcessUser; }
		float ProcessKernel(void) { return fProcessKernel; }

		~CpuUsage();

	private:
		HANDLE hProcess;
		int iNumberofProcessors;

		float fProcessorTotal;
		float fProcessorUser;
		float fProcessorKernel;

		float fProcessTotal;
		float fProcessUser;
		float fProcessKernel;

		ULARGE_INTEGER ftProcessor_LastKernel;
		ULARGE_INTEGER ftProcessor_LastUser;
		ULARGE_INTEGER ftProcessor_LastIdle;

		ULARGE_INTEGER ftProcess_LastKernel;
		ULARGE_INTEGER ftProcess_LastUser;
		ULARGE_INTEGER ftProcess_LastTime;
	};

}