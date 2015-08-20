#include <stdio.h>
#include <windows.h>

#define COMPKEY_JOBOBJECT  ((UINT_PTR) 2)

#define COMPKEY_TERMINATE  ((UINT_PTR) 0)

HANDLE hIOCP;

typedef unsigned(__stdcall *PTHREAD_START) (void *);

#define chBEGINTHREADEX(psa, cbStackSize, pfnStartAddr, \
	pvParam, dwCreateFlags, pdwThreadId)                 \
	((HANDLE)_beginthreadex(\
	(void *)(psa), \
	(unsigned)(cbStackSize), \
	(PTHREAD_START)(pfnStartAddr), \
	(void *)(pvParam), \
	(unsigned)(dwCreateFlags), \
	(unsigned *)(pdwThreadId)))


DWORD JobNotify(PVOID d) {
	DWORD dwBytesXferred;
	ULONG_PTR CompKey;
	LPOVERLAPPED po;

	while (1){
		GetQueuedCompletionStatus(hIOCP,
			&dwBytesXferred, &CompKey, &po, INFINITE);

		/* accept that will exit */
		if (COMPKEY_TERMINATE == CompKey)
			break;

		if (CompKey == COMPKEY_JOBOBJECT) {
			switch (dwBytesXferred) {
			case JOB_OBJECT_MSG_EXIT_PROCESS:
				printf("child proccess shutdown\n");
				break;

			case JOB_OBJECT_MSG_NEW_PROCESS:
				printf("child proccess new\n");
				break;
			}

		}
	}
	return 0;
}

int main()
{
	/* check If the current process already in a job */
	BOOL bInJob = FALSE;
	IsProcessInJob(GetCurrentProcess(), NULL, &bInJob);
	if (bInJob) {
		fprintf(stderr, "Process already in a job\n");
		return -1;
	}

	/* Create a Job Object */
	HANDLE myjob = CreateJobObjectA(NULL, "myjob");
	if (myjob == NULL) {
		fprintf(stderr, "CreateJobObjectA failed\n");
		return -1;
	}

	/* Create and add child process */
	int i = 0;
	PROCESS_INFORMATION pi;
	for (i = 0; i < 1; i++) {

		/* Create a process group */
		STARTUPINFO si = { sizeof(si) };
		BOOL cc = FALSE;
		//D:\\ubuntu_share\\developing\\4_0_working\\base\\example\\bin\\ttest.exe
		cc = CreateProcessA(NULL, "D:\\ubuntu_share\\developing\\4_0_working\\base\\example\\bin\\ttest.exe", NULL, NULL,
			FALSE, CREATE_NEW_PROCESS_GROUP | CREATE_SUSPENDED, NULL, NULL, &si, &pi);	/* Note the flag */
		if (cc == FALSE) {
			fprintf(stderr, "CreateProcessA failed, Error Code <%d>\n", GetLastError());
			return -1;
		}

		/* Add the process to job */
		AssignProcessToJobObject(myjob, pi.hProcess);
		ResumeThread(pi.hThread);
	}

	// Create the completion port that receives job notifications
	hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

	// Create a thread that waits on the completion port
	HANDLE hThreadIOCP = chBEGINTHREADEX(NULL, 0, JobNotify, NULL, 0, NULL);

	JOBOBJECT_ASSOCIATE_COMPLETION_PORT joacp = { (PVOID)COMPKEY_JOBOBJECT, hIOCP };
	SetInformationJobObject(myjob,
		JobObjectAssociateCompletionPortInformation, &joacp, sizeof(joacp));

	/* generrate the signal, but how can we get the process ID. */
	DWORD pid = GetProcessId(pi.hProcess);
//	Sleep(1000);
	GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, pid);

	PostQueuedCompletionStatus(hIOCP, 0, COMPKEY_TERMINATE, NULL);

	WaitForSingleObject(hThreadIOCP, INFINITE);

	return 0;
}