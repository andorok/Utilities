
//#include <windows.h>
#include <stdio.h>
#include <conio.h>

#include "gipcy.h"

char driveName[4][MAX_PATH] = { "\\\\.\\I:", NULL, NULL, NULL};
char fileName[4][MAX_PATH] = { "data.bin", NULL, NULL, NULL };

ULONG bBufSize;
int bufCnt, bufStat;
int drvFlg = 0;

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
/*
void wr_time(double* pTimes)
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

	for(int i = 0; i < bufCnt; i++)
	{
		int val = (int)((double)bBufSize / pTimes[i])/1000.;
		fprintf(fs, "%d\n", val);

		//_write(fs,&val,8);
	}
	fclose(fs);

}
*/

typedef struct _THREAD_PARAM {
	IPC_handle file_handle;
	void*	pBufData;
	int idx;
} THREAD_PARAM, *PTHREAD_PARAM;

double max_time = 0.;

thread_value __IPC_API FileWritingThread(void* pParams)
{
	int status = 0;
	IPC_TIMEVAL StartCount;
	IPC_TIMEVAL StopCount;
	double ms_time;
	//SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
	//int prior = GetThreadPriority(GetCurrentThread());
	//BRDC_printf(_BRDC("Thread Priority = %d\n"), prior);

	PTHREAD_PARAM pThreadParam = (PTHREAD_PARAM)pParams;
	IPC_handle hfile = pThreadParam->file_handle;
	int idx = pThreadParam->idx;
	void* pBuffer = pThreadParam->pBufData;

	for (int i = 0; i < bufCnt; i++)
	{
		IPC_getTime(&StartCount);
		int status = IPC_writeFile(hfile, pBuffer, bBufSize);
		IPC_getTime(&StopCount);
		ms_time = IPC_getDiffTime(&StartCount, &StopCount);
		//if (bufStat)
		//	pTimes[i] = ms_time;
		if (ms_time > max_time)
			max_time = ms_time;
		printf("Buffer number is %d\r", i);
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

	double* pTimes = NULL;
	if(bufStat)
		pTimes = new double[bufCnt];

	IPC_handle hfile[4];

	if(drvFlg)
	{
		for (int i = 0; i < driveCnt; i++)
		{
			hfile[i] = IPC_openFileEx(driveName[i], IPC_OPEN_FILE | IPC_FILE_WRONLY, 0);
		}
	}
	else
	{
		for (int i = 0; i < driveCnt; i++)
		{
			//hfile = IPC_openFile(fileName, IPC_CREATE_FILE | IPC_FILE_WRONLY, FILE_ATTRIBUTE_NORMAL);
			hfile[i] = IPC_openFileEx(fileName[i], IPC_CREATE_FILE | IPC_FILE_WRONLY, IPC_FILE_NOBUFFER);
		}
	}
	for (int i = 0; i < driveCnt; i++)
		if (hfile[i] == INVALID_HANDLE_VALUE)
		{
			printf("CreateFile() is error!!!\n");
			IPC_getch();
			return -1;
		}

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
	printf("Hard drive write speed is %f Mbytes/sec\n", ((double)bBufSize * bufCnt / msTime)/1000.);

	printf("Min write speed is %f Mbytes/sec\n", ((double)bBufSize / max_time)/1000.);

	for (int i = 0; i < driveCnt; i++)
	{
		IPC_closeFile(hfile[i]);
		IPC_virtFree(pBuffer[i]);
	}

	if (bufStat)
	{
		//wr_time(pTimes);
		delete[] pTimes;
	}

	IPC_getch();
	IPC_cleanupKeyboard();
	return 0;
}
