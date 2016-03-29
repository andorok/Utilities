
#include <windows.h>
#include <stdio.h>
#include <conio.h>

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

	GetPrivateProfileStringA("Option", "DriveName", "\\\\.\\I:", driveName, sizeof(driveName), iniFilePath);
	GetPrivateProfileStringA("Option", "DriveFlag", "0", Buffer, sizeof(Buffer), iniFilePath);
	drvFlg = atoi(Buffer);
	GetPrivateProfileStringA("Option", "FullFileName", "data.bin", fileName, sizeof(fileName), iniFilePath);
	GetPrivateProfileStringA("Option", "BufferSizeInMb", "1", Buffer, sizeof(Buffer), iniFilePath);
	bBufSize = atoi(Buffer) * 1024 * 1024;
	GetPrivateProfileStringA("Option", "BufferNum", "1", Buffer, sizeof(Buffer), iniFilePath);
	bufCnt = atoi(Buffer);
	GetPrivateProfileStringA("Option", "BufferStatistic", "0", Buffer, sizeof(Buffer), iniFilePath);
	bufStat = atoi(Buffer);
}

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



int main()
{
	readIniFile();

	if(drvFlg)
	{
		printf("Data of drive %s will be DESTROYED!!! Continue ?\n", driveName);
		_getch();
	}
	else
		printf("File name is %s\n", fileName);

	LARGE_INTEGER Frequency;
	LARGE_INTEGER StartPerformCount;
	LARGE_INTEGER StopPerformCount;
	LARGE_INTEGER StartCount;
	LARGE_INTEGER StopCount;
	int bHighRes = QueryPerformanceFrequency (&Frequency);

	void* pBuffer = VirtualAlloc(NULL, bBufSize, MEM_COMMIT, PAGE_READWRITE);
	if(!pBuffer)
	{
		printf("VirtualAlloc() is error!!!\n");
		_getch();
		return -1; // error
	}
	double* pTimes = NULL;
	if(bufStat)
		pTimes = new double[bufCnt];

	HANDLE hfile;
	if(drvFlg)
	{
		hfile = CreateFileA(	driveName,
								GENERIC_WRITE,
//								FILE_SHARE_WRITE | FILE_SHARE_READ,
								0,
								NULL,
								OPEN_EXISTING,
								0,
								NULL);
	}
	else
	{
		hfile = CreateFileA(	fileName,
								GENERIC_WRITE,
//								FILE_SHARE_WRITE | FILE_SHARE_READ,
								0,
								NULL,
								CREATE_ALWAYS,
//								FILE_ATTRIBUTE_NORMAL,
							    FILE_FLAG_NO_BUFFERING,         
								NULL);
	}
	if(hfile == INVALID_HANDLE_VALUE)
	{
		printf("CreateFile() is error!!!\n");
		_getch();
		return -1;
	}
	ULONG nNumberOfWriteBytes;
	double ms_time;
	double max_time = 0.;

	QueryPerformanceCounter(&StartPerformCount);

	for(int i = 0; i < bufCnt; i++)
	{
		QueryPerformanceCounter(&StartCount);
		WriteFile(hfile, pBuffer, bBufSize, &nNumberOfWriteBytes, NULL);
		QueryPerformanceCounter(&StopCount);
		ms_time = (double)(StopCount.QuadPart - StartCount.QuadPart) / (double)Frequency.QuadPart * 1.E3;
		if(bufStat)
			pTimes[i] = ms_time;
		if(ms_time > max_time)
			max_time = ms_time;
		printf("Buffer number is %d\r", i);
	}
	QueryPerformanceCounter (&StopPerformCount);
	double msTime = (double)(StopPerformCount.QuadPart - StartPerformCount.QuadPart) / (double)Frequency.QuadPart * 1.E3;
	printf("Hard drive write speed is %f Mbytes/sec\n", ((double)bBufSize * bufCnt / msTime)/1000.);

	printf("Min write speed is %f Mbytes/sec\n", ((double)bBufSize / max_time)/1000.);

	CloseHandle(hfile);

	if(bufStat)
	{
		wr_time(pTimes);
		delete[] pTimes;
	}
	VirtualFree(pBuffer, 0, MEM_RELEASE);

	_getch();
	return 0;
}
