
//#include <windows.h>
#include <stdio.h>
#include <conio.h>

#include "gipcy.h"

char driveName[4][MAX_PATH] = { "\\\\.\\I:", NULL, NULL, NULL};
char fileName[4][MAX_PATH] = { "data.bin", NULL, NULL, NULL };

ULONG bBufSize;
int bufCnt, bufStat;
int drvFlg = 0;

typedef struct _TIME_STAT {
	volatile int dev;
	volatile double	wrtime;
} TIME_STAT, *PTIME_STAT;

TIME_STAT* pTimes = NULL;
volatile int iTimes = 0;
HANDLE hMutex;
int driveCnt = 1;

void readIniFile()
{
	char Buffer[128];
	char iniFilePath[MAX_PATH];
	GetCurrentDirectoryA(sizeof(iniFilePath), iniFilePath);
	lstrcatA(iniFilePath, "\\multidisk_speed.ini");

	IPC_getPrivateProfileString("Option", "DriveCnt", "1", Buffer, sizeof(Buffer), iniFilePath);
	driveCnt = atoi(Buffer);

	for (int i = 0; i < driveCnt; i++)
	{
		sprintf(Buffer, "DriveName%d", i);
		IPC_getPrivateProfileString("Option", Buffer, "\\\\.\\H:", driveName[i], sizeof(driveName[i]), iniFilePath);
	}

	IPC_getPrivateProfileString("Option", "DriveFlag", "0", Buffer, sizeof(Buffer), iniFilePath);
	drvFlg = atoi(Buffer);

	for (int i = 0; i < driveCnt; i++)
	{
		sprintf(Buffer, "FullFileName%d", i);
		IPC_getPrivateProfileString("Option", Buffer, "data.bin", fileName[i], sizeof(fileName[i]), iniFilePath);
	}

	IPC_getPrivateProfileString("Option", "BufferSizeInMb", "1", Buffer, sizeof(Buffer), iniFilePath);
	bBufSize = atoi(Buffer) * 1024 * 1024;
	IPC_getPrivateProfileString("Option", "BufferNum", "1", Buffer, sizeof(Buffer), iniFilePath);
	bufCnt = atoi(Buffer);
	IPC_getPrivateProfileString("Option", "BufferStatistic", "0", Buffer, sizeof(Buffer), iniFilePath);
	bufStat = atoi(Buffer);
}

void wr_time(TIME_STAT* pTimes)
{
	FILE *fs;
	//errno_t err = _wfopen_s(&fs, L"Statistic.txt", L"w+t");

	//char strBuf[48] = "";
	//for(int i = 0; i < bufCnt; i++)
	//{
	//	sprintf_s(strBuf, 48, "%.2f", ((double)bBufSize / pTimes[i])/1000.);

	//	fprintf(fs, "%s\n", strBuf);
	//}
 //   fclose(fs);

	errno_t err = _wfopen_s(&fs, L"Statistic.txt", L"w+b");

	int bufCntTotal = bufCnt * driveCnt;
	for(int i = 0; i < bufCntTotal; i++)
	{
		int val = (int)((double)bBufSize / pTimes[i].wrtime)/1000.;
		fprintf(fs, "%d %d %d\n", pTimes[i].dev & 0xF, pTimes[i].dev >> 4, val);

		//_write(fs,&val,8);
	}
	fclose(fs);

	printf("File Statistic.txt is written\n");
}

typedef struct _THREAD_PARAM {
	HANDLE file_handle;
	void*	pBufData;
	int idx;
} THREAD_PARAM, *PTHREAD_PARAM;

volatile TIME_STAT max_time;

thread_value __IPC_API FileWritingThread(void* pParams)
{
	int status = 0;
	IPC_TIMEVAL StartCount;
	IPC_TIMEVAL StopCount;
	double ms_time;
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
	//int prior = GetThreadPriority(GetCurrentThread());
	//BRDC_printf(_BRDC("Thread Priority = %d\n"), prior);

	PTHREAD_PARAM pThreadParam = (PTHREAD_PARAM)pParams;
	HANDLE hfile = pThreadParam->file_handle;
	int idx = pThreadParam->idx;
	void* pBuffer = pThreadParam->pBufData;

	SetThreadIdealProcessor(GetCurrentThread(), idx + 2);

	unsigned long writesize;
	OVERLAPPED ovl;
	ovl.hEvent = NULL;
	ovl.Offset = 0;
	ovl.OffsetHigh = 0;
	unsigned __int64 fullOffset = 0;

	for (int iBuf = 0; iBuf < bufCnt; iBuf++)
	{
		IPC_getTime(&StartCount);
		//status = IPC_writeFile(hfile, pBuffer, bBufSize);
		status = WriteFile(hfile, pBuffer, bBufSize, &writesize, &ovl);
		WaitForSingleObject(hfile, INFINITE);
		IPC_getTime(&StopCount);
		fullOffset += bBufSize;
		ovl.Offset = fullOffset & 0xFFFFFFFF;
		ovl.OffsetHigh = (fullOffset >> 32) & 0xFFFFFFFF;
		ms_time = IPC_getDiffTime(&StartCount, &StopCount);
		WaitForSingleObject(hMutex, INFINITE);
		if (bufStat)
		{
			pTimes[iTimes].dev = (iBuf << 4) + idx;
			pTimes[iTimes++].wrtime = ms_time;
		}
		if (ms_time > max_time.wrtime)
		{
			max_time.wrtime = ms_time;
			max_time.dev = (iBuf << 4) + idx;
		}
		ReleaseMutex(hMutex);
		printf("Device %d: Buffer number is %d\r", idx, iBuf);
		//printf("Buffer number is %d\r", i);
	}

	return (thread_value)status;
}
	
int main()
{
	readIniFile();

	IPC_initKeyboard();

	if(drvFlg)
	{
		for (int i = 0; i < driveCnt; i++)
		{
			printf("Data of drive %s will be DESTROYED!!! Continue ?\n", driveName[i]);
			IPC_getch();
		}
	}
	else
	{
		for (int i = 0; i < driveCnt; i++)
		{
			printf("File name is %s\n", fileName[i]);
		}
	}

	printf("Buffer size is %d Mbytes, buffer count is %d\n", bBufSize / 1024 / 1024, bufCnt);

	LARGE_INTEGER Frequency;
	IPC_TIMEVAL StartPerformCount;
	IPC_TIMEVAL StopPerformCount;
	//IPC_TIMEVAL StartCount;
	//IPC_TIMEVAL StopCount;

	int bHighRes = QueryPerformanceFrequency (&Frequency);

	void* pBuffer[4];
	for (int i = 0; i < driveCnt; i++)
	{
		pBuffer[i] = IPC_virtAlloc(bBufSize);
		if (!pBuffer[i])
		{
			printf("VirtualAlloc() is error!!!\n");
			IPC_getch();
			return -1; // error
		}
	}

	if(bufStat)
		pTimes = new TIME_STAT[bufCnt * driveCnt];

	HANDLE hfile[4];

	if(drvFlg)
	{
		for (int i = 0; i < driveCnt; i++)
		{
			//hfile[i] = IPC_openFileEx(driveName[i], IPC_OPEN_FILE | IPC_FILE_WRONLY, 0);
			hfile[i] = CreateFile(driveName[i], GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);
		}
	}
	else
	{
		for (int i = 0; i < driveCnt; i++)
	{
			//hfile = IPC_openFile(fileName, IPC_CREATE_FILE | IPC_FILE_WRONLY, FILE_ATTRIBUTE_NORMAL);
			//hfile[i] = IPC_openFileEx(fileName[i], IPC_CREATE_FILE | IPC_FILE_WRONLY, IPC_FILE_NOBUFFER);
			hfile[i] = CreateFile(fileName[i], GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_FLAG_NO_BUFFERING | FILE_FLAG_OVERLAPPED, NULL);
		}
	}
	for (int i = 0; i < driveCnt; i++)
		if (hfile[i] == INVALID_HANDLE_VALUE)
		{
			printf("CreateFile() is error!!!\n");
			IPC_getch();
			return -1;
		}

	max_time.dev = -1;
	max_time.wrtime = 0.;
	hMutex = CreateMutex(NULL, FALSE, NULL);

	//ULONG nNumberOfWriteBytes;
	
	//double ms_time;
	//IPC_getTime(&StartPerformCount);

	//for(int i = 0; i < bufCnt; i++)
	//{
	//	IPC_getTime(&StartCount);
	//	for (int i = 0; i < driveCnt; i++)
	//	{
	//		IPC_writeFile(hfile[i], pBuffer[i], bBufSize);
	//	}

	//	IPC_getTime(&StopCount);
	//	ms_time = IPC_getDiffTime(&StartCount, &StopCount);
	//	if(bufStat)
	//		pTimes[i] = ms_time;
	//	if(ms_time > max_time)
	//		max_time = ms_time;
	//	printf("Buffer number is %d\r", i);
	//}
	//IPC_getTime(&StopPerformCount);

	IPC_handle hThread[4];
	THREAD_PARAM thread_par[4];

	IPC_getTime(&StartPerformCount);

	for (int i = 0; i < driveCnt; i++)
	{
		thread_par[i].file_handle = hfile[i];
		thread_par[i].idx = i;
		thread_par[i].pBufData = pBuffer[i];
		hThread[i] = IPC_createThread(("WriteFileThread"), &FileWritingThread, &thread_par[i]);

	}
	// Wait until threads terminates
	for (int i = 0; i < driveCnt; i++)
	{
		IPC_waitThread(hThread[i], INFINITE);// Wait until threads terminates
		IPC_deleteThread(hThread[i]);
	}

	IPC_getTime(&StopPerformCount);

	double msTime = IPC_getDiffTime(&StartPerformCount, &StopPerformCount);
	printf("Hard drive(s) write speed is %f(%f) Mbytes/sec\n", ((double)bBufSize * bufCnt / msTime) / 1000.,
									((double)bBufSize * bufCnt * driveCnt / msTime)/1000.);

	printf("Min write (%d device, %d buffer) speed is %f Mbytes/sec\n", max_time.dev & 0xF, max_time.dev >> 4, ((double)bBufSize / max_time.wrtime)/1000.);

	for (int i = 0; i < driveCnt; i++)
	{
		CloseHandle(hfile[i]);
		IPC_virtFree(pBuffer[i]);
	}

	CloseHandle(hMutex);

	if (bufStat)
	{
		wr_time(pTimes);
		delete[] pTimes;
	}

	IPC_getch();
	IPC_cleanupKeyboard();
	return 0;
}
