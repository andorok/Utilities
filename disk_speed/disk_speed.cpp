
//#include <windows.h>
#include <stdio.h>
#include <conio.h>

#include "gipcy.h"

char driveName[MAX_PATH] = "\\\\.\\I:";
char fileName[MAX_PATH] = "data.bin";
ULONG bBufSize;
int bufCnt, bufStat;
int drvFlg = 0;

void readIniFile()
{
	char Buffer[128];
	char iniFilePath[MAX_PATH];
	GetCurrentDirectoryA(sizeof(iniFilePath), iniFilePath);
	lstrcatA(iniFilePath, "\\disk_speed.ini");

	IPC_getPrivateProfileString("Option", "DriveName", "\\\\.\\H:", driveName, sizeof(driveName), iniFilePath);
	IPC_getPrivateProfileString("Option", "DriveFlag", "0", Buffer, sizeof(Buffer), iniFilePath);
	drvFlg = atoi(Buffer);
	IPC_getPrivateProfileString("Option", "FullFileName", "data.bin", fileName, sizeof(fileName), iniFilePath);
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

int main()
{
	readIniFile();

	IPC_initKeyboard();

	if(drvFlg)
	{
		printf("Data of drive %s will be DESTROYED!!! Continue ?\n", driveName);
		IPC_getch();
	}
	else
		printf("File name is %s\n", fileName);

	LARGE_INTEGER Frequency;
	IPC_TIMEVAL StartPerformCount;
	IPC_TIMEVAL StopPerformCount;
	IPC_TIMEVAL StartCount;
	IPC_TIMEVAL StopCount;

	int bHighRes = QueryPerformanceFrequency (&Frequency);

	void* pBuffer = IPC_virtAlloc(bBufSize);
	if(!pBuffer)
	{
		printf("VirtualAlloc() is error!!!\n");
		IPC_getch();
		return -1; // error
	}
	double* pTimes = NULL;
	if(bufStat)
		pTimes = new double[bufCnt];

	IPC_handle hfile = 0;

	if(drvFlg)
	{
		hfile = IPC_openFileEx(driveName, IPC_OPEN_FILE | IPC_FILE_WRONLY, 0);
	}
	else
	{
		//hfile = IPC_openFile(fileName, IPC_CREATE_FILE | IPC_FILE_WRONLY, FILE_ATTRIBUTE_NORMAL);
		hfile = IPC_openFileEx(fileName, IPC_CREATE_FILE | IPC_FILE_WRONLY, IPC_FILE_NOBUFFER);
	}
	if(hfile == INVALID_HANDLE_VALUE)
	{
		printf("CreateFile() is error!!!\n");
		IPC_getch();
		return -1;
	}
	//ULONG nNumberOfWriteBytes;
	double ms_time;
	double max_time = 0.;

	IPC_getTime(&StartPerformCount);

	for(int i = 0; i < bufCnt; i++)
	{
		IPC_getTime(&StartCount);
		IPC_writeFile(hfile, pBuffer, bBufSize);
		IPC_getTime(&StopCount);
		ms_time = IPC_getDiffTime(&StartCount, &StopCount);
		if(bufStat)
			pTimes[i] = ms_time;
		if(ms_time > max_time)
			max_time = ms_time;
		printf("Buffer number is %d\r", i);
	}
	IPC_getTime(&StopPerformCount);
	double msTime = IPC_getDiffTime(&StartPerformCount, &StopPerformCount);
	printf("Hard drive write speed is %f Mbytes/sec\n", ((double)bBufSize * bufCnt / msTime)/1000.);

	printf("Min write speed is %f Mbytes/sec\n", ((double)bBufSize / max_time)/1000.);

	IPC_closeFile(hfile);

	if(bufStat)
	{
		//wr_time(pTimes);
		delete[] pTimes;
	}
	IPC_virtFree(pBuffer);

	IPC_getch();
	IPC_cleanupKeyboard();
	return 0;
}
