
//#include <stdio.h>

//#include <numa.h>
//#include <sched.h>
#include <math.h>

#ifdef _WIN32
#include "windows.h"
#include <conio.h>
//#include <process.h> 
#else
#include <unistd.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
//#include <errno.h>
#include <linux/hdreg.h>
#endif

#include "time_func.h"
#include "mem_func.h"
#include "cthread.h"

#define SIZE_1G 1024*1024*1024
#define SIZE_32M 32*1024*1024
#define SIZE_16M 16*1024*1024
#define SIZE_1M 1024*1024

#include <csignal>
#include <cstring>
#include <tuple>

#ifdef __linux__
#define MAX_PATH          260
//char drive_name[MAX_PATH] = "/dev/sdc1";
char file_name[MAX_PATH] = "/mnt/diskG/data";
//char file_name[MAX_PATH] = "/mnt/f/32G/data";
//char file_name[MAX_PATH] = "/mnt/e/data";

//pthread_mutex_t mutex_read;
//pthread_mutex_t mutex_write;

#else
//char drive_name[MAX_PATH] = "\\\\.\\G:";
//char file_name[MAX_PATH] = "G:/data";
char file_name[MAX_PATH] = "F:/32G/data";

//char name_mutex_read[MAX_PATH] = "mutex_read_file";
//char name_mutex_write[MAX_PATH] = "mutex_write_file";
//HANDLE g_hMutex_read = NULL;
//HANDLE g_hMutex_write = NULL;

#endif // __linux__

int g_fcnt = 4;
int g_direct = 0;
int g_drive = 0;
//int g_read = 0;
//int g_thrd = 0;

static bool exit_app = false;
void signal_handler(int signo)
{
	fprintf(stderr, "\nSignal CTRL+C\n");
	signo = signo;
	exit_app = true;
}

// Вывести подсказку
void DisplayHelp()
{
	printf("Usage:\n");
	//printf("\n");
	printf("disk_test.exe [<swtches>]\n");
	printf("Switches:\n");
	printf(" -dir  - direct and sync writing\n");
	//printf(" -drv  - writing to drive without file system\n");
	//	printf(" -thr<N>  - use N threads\n");
	printf(" -n<N>  - writing N files\n");
	printf("\n");
}

// разобрать командную строку
void ParseCommandLine(int argc, char *argv[])
{
	int			ii;
	char		*pLin, *endptr;

	for (ii = 1; ii < argc; ii++)
	{
		// Вывести подсказку
		if (tolower(argv[ii][1]) == 'h')
		{
			DisplayHelp();
			exit(1);
		}

		// Указать режим небуферизированного синхронного вывода в файл
		if (!strcmp(argv[ii], "-dir"))
		{
			g_direct = 1;
			printf("Command line: -dir\n");
		}

		// Указать режим записи на диск без файловой системы
		//if (!strcmp(argv[ii], "-drv"))
		//{
		//	g_drive = 1;
		//	printf("Command line: -drv\n");
		//}

		// Выполнить чтение и проверку после записи
		//if (!strcmp(argv[ii], "-rd"))
		//{
		//	g_read = 1;
		//	printf("Command line: -rd\n");
		//}

		// Выполнить чтение в отдельном потоке во время записи
		//if (!strcmp(argv[ii], "-thrd"))
		//{
		//	g_thrd = 1;
		//	printf("Command line: -thrd\n");
		//}

		// Указать число записываемых файлов
		if (tolower(argv[ii][1]) == 'n')
		{
			pLin = &argv[ii][2];
			if (argv[ii][2] == '\0')
			{
				ii++;
				pLin = argv[ii];
			}
			g_fcnt = strtoul(pLin, &endptr, 0);
			printf("Command line: -n%d\n", g_fcnt);
		}

	}
}

//#ifdef __linux__
//
////! Функция выделяет страницы виртуальной памяти процесса
//void* virtAlloc(size_t nSize)
//{
//	void *ptr = NULL;
//	long pageSize = sysconf(_SC_PAGESIZE);
//	int  res = posix_memalign(&ptr, pageSize, nSize);
//
//	if ((res != 0) || !ptr)
//		return NULL;
//
//	return ptr;
//}
//
////! Функция освобождает страницы виртуальной памяти процесса
//void virtFree(void *ptr)
//{
//	free(ptr);
//}
//
//#else
//
////! Функция выделяет страницы виртуальной памяти процесса
//void* virtAlloc(size_t nSize)
//{
//	void* ptr = VirtualAlloc(NULL, nSize, MEM_COMMIT, PAGE_READWRITE);
//
//	return ptr;
//}
//
////! Функция освобождает страницы виртуальной памяти процесса
//void virtFree(void *ptr)
//{
//	VirtualFree(ptr, 0, MEM_RELEASE);
//
//}
//
//#endif

int file_write_direct(void *buf, char* fname)
{
#ifdef __linux__
	int sysflag = O_CREAT | O_TRUNC | O_WRONLY | O_DIRECT | O_SYNC;
	int hfile = open(fname, sysflag, 0666);
	if (hfile < 0)
	{
		printf("error: can not open %s\n", fname);
		return -1;
	}
	printf("Writing file %s ...                   \r", fname);
	size_t writesize = SIZE_1G;
	int res = write(hfile, buf, writesize);
	if (res < 0) {
		printf("error: can not write %s\n", fname);
		return -1;
	}
	close(hfile);
#else
	unsigned long amode = GENERIC_WRITE;
	unsigned long cmode = CREATE_ALWAYS;
	unsigned long fattr = FILE_FLAG_NO_BUFFERING;
	HANDLE  hfile = CreateFile(fname,
		amode,
		//						FILE_SHARE_WRITE | FILE_SHARE_READ,
		0,
		NULL,
		cmode,
		fattr,
		NULL);
	if (hfile == INVALID_HANDLE_VALUE)
	{
		printf("error: can not open %s\n", fname);
		return -1;
	}
	printf("Writing file %s ...                   \r", fname);
	unsigned long writesize;
	int ret = WriteFile(hfile, buf, SIZE_1G, &writesize, NULL);
	if (!ret)
	{
		printf("error: can not write %s\n", fname);
		return -1;
	}
	CloseHandle(hfile);
#endif // __linux__
	return 0;
}

int file_read_direct(void *buf, char* fname)
{
#ifdef __linux__
	int sysflag = O_RDONLY | O_DIRECT | O_SYNC;
	int hfile = open(fname, sysflag, 0666);
	if (hfile < 0)
	{
		printf("error: can not open %s\n", fname);
		return -1;
	}
	printf("Reading file %s ...                   \r", fname);
	size_t readsize = SIZE_1G;
	int res = read(hfile, buf, readsize);
	if (res < 0) {
		printf("error: can not read %s\n", fname);
		return -1;
	}
	close(hfile);
#else
	unsigned long amode = GENERIC_READ;
	unsigned long cmode = OPEN_EXISTING;
	unsigned long fattr = FILE_FLAG_NO_BUFFERING;
	HANDLE  hfile = CreateFile(fname,
		amode,
		//						FILE_SHARE_WRITE | FILE_SHARE_READ,
		0,
		NULL,
		cmode,
		fattr,
		NULL);
	if (hfile == INVALID_HANDLE_VALUE)
	{
		printf("error: can not open %s\n", fname);
		return -1;
	}
	printf("Reading file %s ...                   \r", fname);
	unsigned long readsize;
	int ret = ReadFile(hfile, buf, SIZE_1G, &readsize, NULL);
	if (!ret)
	{
		printf("error: can not read %s\n", fname);
		return -1;
	}
	CloseHandle(hfile);
#endif // __linux__
	return 0;
}

int write2drive(void *buf, char* fname, int num)
{
	double wr_time = 0.;
	double total_time = 0.;
	double max_time = 0.;
	int bsize = 4096;
	uint32_t *pBuf = (uint32_t *)buf;

#ifdef __linux__
	//O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH | S_IWOTH
	//int sysflag = O_WRONLY;
	int sysflag = O_RDWR | O_DIRECT | O_SYNC;
	int hfile = open(fname, sysflag, 0666);
	if (hfile < 0)
	{
		printf("error: can not open %s\n", fname);
		return -1;
	}

	static struct hd_driveid hd;
	if (!ioctl(hfile, HDIO_GET_IDENTITY, &hd))
		printf("S/N %.20s\n", hd.serial_no);
	else
		if (errno == -ENOMSG)
			printf("No serial number available\n");
		else
			perror("ERROR: HDIO_GET_IDENTITY");

	printf("Writing file %s ...                   \n", fname);
	size_t writesize = SIZE_1G;

	for (int idx = 0; idx < num; idx++)
	{
		for (int i = 0; i < bsize; i++)
			pBuf[i] = (idx << 16) + i;

		ipc_time_t start_time, stop_time;
		start_time = ipc_get_time();

		ssize_t res = write(hfile, buf, writesize);
		if (res < 0) {
			printf("error: can not write %s\n", fname);
            close(hfile);
			return -1;
		}
		if (SIZE_1G != res)
		{
			printf("error: disk %s is full\n", fname);
            close(hfile);
			return -1;
		}
		stop_time = ipc_get_time();
		wr_time = ipc_get_nano_difftime(start_time, stop_time) / 1000000.;

		total_time += wr_time;
		if (wr_time > max_time)
			max_time = wr_time;

		printf("Speed (%d): Avr %.4f Mb/s, Cur %.4f Mb/s, Min %.4f Mb/s\r", idx, ((double)writesize * (idx + 1) / total_time) / 1000., ((double)writesize / wr_time) / 1000., ((double)writesize / max_time) / 1000.);
		printf("\n");
	}
	close(hfile);
#else
	unsigned long amode = GENERIC_WRITE;
	unsigned long cmode = OPEN_EXISTING;
	unsigned long fattr = 0;
	HANDLE  hfile = CreateFile(fname,
		amode,
		//						FILE_SHARE_WRITE | FILE_SHARE_READ,
		0,
		NULL,
		cmode,
		fattr,
		NULL);
	if (hfile == INVALID_HANDLE_VALUE)
	{
		printf("error: can not open %s\n", fname);
		_getch();
		return -1;
	}
	printf("Writing file %s ...                   \n", fname);
	unsigned long writesize;
	for (int idx = 0; idx < num; idx++)
	{
		for (int i = 0; i < bsize; i++)
			pBuf[i] = (idx << 16) + i;

		ipc_time_t start_time, stop_time;
		start_time = ipc_get_time();

		int ret = WriteFile(hfile, buf, SIZE_1G, &writesize, NULL);
		if (writesize != SIZE_1G)
		{
			printf("error: disk %s is full\n", fname);
			CloseHandle(hfile);
			_getch();
			return -1;
		}
		if (!ret)
		{
			printf("error: can not write %s\n", fname);
			CloseHandle(hfile);
			_getch();
			return -1;
		}
		stop_time = ipc_get_time();
		wr_time = ipc_get_nano_difftime(start_time, stop_time) / 1000000.;

		total_time += wr_time;
		if (wr_time > max_time)
			max_time = wr_time;

		printf("Speed (%d): Avr %.4f Mb/s, Cur %.4f Mb/s, Min %.4f Mb/s\r", idx, ((double)writesize * (idx + 1) / total_time) / 1000., ((double)writesize / wr_time) / 1000., ((double)writesize / max_time) / 1000.);
		printf("\n");
	}
	CloseHandle(hfile);
	//printf("\nPress any key to quit of program\n");
	printf("\nPress any key....\n");
	_getch();
#endif // __linux__
	return 0;
}

int read_drive(char* fname, int num, uint32_t *read_buf, uint32_t *cmp_buf)
{
	double rd_time = 0.;
	double total_time = 0.;
	double max_time = 0.;
	int bsize = 4096;

#ifdef __linux__
	//O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH | S_IWOTH
	//int sysflag = O_WRONLY;
	int sysflag = O_RDWR | O_DIRECT | O_SYNC;
	int hfile = open(fname, sysflag, 0666);
	if (hfile < 0)
	{
		printf("error: can not open %s\n", fname);
		return -1;
	}

	static struct hd_driveid hd;
	if (!ioctl(hfile, HDIO_GET_IDENTITY, &hd))
	{
		printf("Serial number - %.20s\n", hd.serial_no);
		printf("Model - %s\n", hd.model);
		printf("Logical blocks - %d\n", hd.lba_capacity);
		printf("Cylinders - %d\n", hd.cyls);
		printf("Heads - %d\n", hd.heads);
		printf("Sectors - %d\n", hd.sectors);
	}
	else
		if (errno == -ENOMSG)
			printf("No serial number available\n");
		else
			perror("ERROR: HDIO_GET_IDENTITY");

	printf("Reading file %s ...                   \n", fname);
	size_t readsize = SIZE_1G;

	for (int idx = 0; idx < num; idx++)
	{
		for (int i = 0; i < bsize; i++)
			cmp_buf[i] = (idx << 16) + i;

		ipc_time_t start_time, stop_time;
		start_time = ipc_get_time();

		ssize_t res = read(hfile, read_buf, readsize);
		if (res < 0) {
			printf("error: can not read %s\n", fname);
			close(hfile);
			return -1;
		}
		stop_time = ipc_get_time();
		rd_time = ipc_get_nano_difftime(start_time, stop_time) / 1000000.;

		total_time += rd_time;
		if (rd_time > max_time)
			max_time = rd_time;

		printf("Speed (%d): Avr %.4f Mb/s, Cur %.4f Mb/s, Min %.4f Mb/s\r", idx, ((double)readsize * (idx + 1) / total_time) / 1000., ((double)readsize / rd_time) / 1000., ((double)readsize / max_time) / 1000.);
		printf("\n");

		uint32_t err_cnt = 0;
		for (int i = 0; i < bsize; i++)
			if (read_buf[i] != cmp_buf[i])
				err_cnt++;
		if (err_cnt)
		{
			printf(" Errors by reading: %d\n", err_cnt);
			break;
		}
		printf("Verifying is OK\n");
	}
	close(hfile);
#else
	unsigned long amode = GENERIC_READ;
	unsigned long cmode = OPEN_EXISTING;
	unsigned long fattr = 0;
	HANDLE  hfile = CreateFile(fname,
		amode,
		//						FILE_SHARE_WRITE | FILE_SHARE_READ,
		0,
		NULL,
		cmode,
		fattr,
		NULL);
	if (hfile == INVALID_HANDLE_VALUE)
	{
		printf("error: can not open %s\n", fname);
		_getch();
		return -1;
	}
	printf("Reading file %s ...                   \n", fname);
	unsigned long readsize;
	for (int idx = 0; idx < num; idx++)
	{
		for (int i = 0; i < bsize; i++)
			cmp_buf[i] = (idx << 16) + i;

		ipc_time_t start_time, stop_time;
		start_time = ipc_get_time();

		int ret = ReadFile(hfile, read_buf, SIZE_1G, &readsize, NULL);
		if (!ret)
		{
			printf("error: can not read %s\n", fname);
			CloseHandle(hfile);
			_getch();
			return -1;
		}
		stop_time = ipc_get_time();
		rd_time = ipc_get_nano_difftime(start_time, stop_time) / 1000000.;

		total_time += rd_time;
		if (rd_time > max_time)
			max_time = rd_time;

		printf("Speed (%d): Avr %.4f Mb/s, Cur %.4f Mb/s, Min %.4f Mb/s\r", idx, ((double)readsize * (idx + 1) / total_time) / 1000., ((double)readsize / rd_time) / 1000., ((double)readsize / max_time) / 1000.);
		printf("\n");

		uint32_t err_cnt = 0;
		for (int i = 0; i < bsize; i++)
			if (read_buf[i] != cmp_buf[i])
				err_cnt++;
		if (err_cnt)
		{
			printf(" Errors by reading: %d\n", err_cnt);
			break;
		}
		printf("Verifying is OK\n");

	}
	CloseHandle(hfile);
	//printf("\nPress any key to quit of program\n");
	printf("\nPress any key....\n");
	_getch();
#endif // __linux__
	return 0;
}

volatile int wrfile_num = -1;
volatile int rdfile_num = -1;

volatile int g_read_max = 1;

typedef struct {
	uint32_t *wr_buf;
	uint32_t size;
	CMutex *wrmutex;
	CMutex *rdmutex;
} THREAD_PARAM, *PTHREAD_PARAM;

#ifdef __linux__
	//pthread_t createThread(void* (*function)(void *), void* param);
	//int waitThread(pthread_t thread_id, int timeout);
	//int deleteThread(pthread_t thread_id);

void*	file_write_thread(void* pParams)
#else
unsigned int __stdcall file_write_thread(void* pParams)
#endif
{
#ifdef _WIN32
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
	//SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
	int prior = GetThreadPriority(GetCurrentThread());
	printf("Thread Priority = %d\n", prior);
#endif
	PTHREAD_PARAM pThreadParam = (PTHREAD_PARAM)pParams;
	
	int bsize = pThreadParam->size;
	uint32_t *wrBuf = pThreadParam->wr_buf;
	CMutex *wrMutex = pThreadParam->wrmutex;
	CMutex *rdMutex = pThreadParam->rdmutex;

	char wrfname[512];
	double wr_time = 0.;
	double total_time = 0.;
	double max_time = 0.;
	size_t writesize = SIZE_1G;

//	if (g_drive)
//	{
//		int ret = write2drive(wrBuf, drive_name, g_fcnt);
//	}
//	else
	{
		int count = 0;
		while (!exit_app)
		{
			for (int idx = 0; idx < g_fcnt; idx++)
			{
				for (int i = 0; i < bsize; i++)
					wrBuf[i] = (idx << 16) + i;

				sprintf(wrfname, "%s_%03d", file_name, idx);

				//while (rdfile_num == idx)
				//	ipc_delay(250);
				if (rdfile_num == idx)
				{
					rdMutex->lock();
					rdMutex->unlock();
//#ifdef __linux__
//					pthread_mutex_lock(&mutex_read);
//					pthread_mutex_unlock(&mutex_read);
//#else
//					//printf("THREAD: Waiting read mutex before writing (wr_num %d, rd_num %d, wr_idx %d)...                   \n", wrfile_num, rdfile_num, idx);
//					WaitForSingleObject(g_hMutex_read, INFINITE);
//					ReleaseMutex(g_hMutex_read);
//#endif
				}

				wrMutex->lock();
//#ifdef __linux__
//				pthread_mutex_lock(&mutex_write);
//#else
//				//printf("THREAD: Capturing write mutex (wr_num %d, rd_num %d, wr_idx %d)...                   \n", wrfile_num, rdfile_num, idx);
//				printf("WRITE CAPTURE (wr_idx %d)!\n", idx);
//				WaitForSingleObject(g_hMutex_write, INFINITE);
//#endif

				wrfile_num = idx;
					
				ipc_time_t start_time, stop_time;
				start_time = ipc_get_time();

				if (g_direct)
				{
					//printf("THREAD: DIRECT & SYNC writing buffer on disk!\r");
					int ret = file_write_direct(wrBuf, wrfname);
					if (ret < 0)
					{
						wrMutex->unlock();
#ifdef __linux__
//						pthread_mutex_unlock(&mutex_write);
						//return &ret;
						return (void*)-1;
#else
//						ReleaseMutex(g_hMutex_write);
						return ret;
#endif
					}
				}
				else
				{
					//printf("THREAD: Writing buffer on disk by fopen()!\r");
					FILE *fp = fopen(wrfname, "wb");
					if (!fp) {
						printf("error: can not open %s\n", wrfname);
						wrMutex->unlock();
#ifdef __linux__
//						pthread_mutex_unlock(&mutex_write);
						return (void*)-1;
#else
//						ReleaseMutex(g_hMutex_write);
						return -1;
#endif
					}

					//printf("THREAD: Writing file %s ...                   \r", wrfname);
					size_t ret = fwrite(wrBuf, 1, writesize, fp);
					if (ret != writesize)
					{
						printf("error: can not write %s\n", wrfname);
						wrMutex->unlock();
#ifdef __linux__
//						pthread_mutex_unlock(&mutex_write);
						return (void*)-1;
#else
//						ReleaseMutex(g_hMutex_write);
						return -1;
#endif
					}

					fclose(fp);
				}

				stop_time = ipc_get_time();
				wr_time = ipc_get_nano_difftime(start_time, stop_time) / 1000000.;

				wrMutex->unlock();
//#ifdef __linux__
//				pthread_mutex_unlock(&mutex_write);
//#else
//				//printf("THREAD: Release write mutex (wr_num %d, rd_num %d, wr_idx %d)...                   \n", wrfile_num, rdfile_num, idx);
//				printf("WRITE RELEASE (wr_idx %d)!\n", idx);
//				ReleaseMutex(g_hMutex_write);
//#endif

				total_time += wr_time;
				if (wr_time > max_time)
					max_time = wr_time;

				printf("WRITE THREAD: Time (%s): Total %.4f s, Cur %.4f s, Max %.4f s\n",
										wrfname, total_time / 1000., wr_time / 1000., max_time / 1000.);
				printf("WRITE THREAD: Speed (%s): Avr %.4f Mb/s, Cur %.4f Mb/s, Min %.4f Mb/s\n",
							wrfname, ((double)writesize * (++count) / total_time) / 1000., ((double)writesize / wr_time) / 1000., ((double)writesize / max_time) / 1000.);

			//	printf("\n");
				g_read_max = (count < g_fcnt) ? count : g_fcnt;

				if (exit_app)
					break;
			}
		}
	}
#ifdef __linux__
	return (void*)0;
#else
	return 0;
#endif
}

int main(int argc, char *argv[])
{
	// выход по Ctrl+C
	signal(SIGINT, signal_handler);

	//const int bsize = 4096;

	//uint32_t buf[bsize];

	//while (!exit_app)
	//{
	//	int idx = 2;
	//	printf("ENTER the file number for reading !\n");
	//	scanf("%d", &idx);
	//	printf("wup0_%03d\n", idx);
	//	if (exit_app)
	//	{
	//		printf("exit_app!\n");
	//		break;
	//	}
	//	printf("before cycle!\n");

	//	for (int i = 0; i < bsize; i++)
	//		buf[i] = (idx << 16) + i;
	//	printf("after cycle!\n");
	//}
	//printf("Complete program!\n");
	//_getch();
	//return 0;

	ParseCommandLine(argc, argv);

	// Выделяем память для записи в файл
	printf("Allocating memory for writing to file...              \n");
	uint32_t *wrBuf = (uint32_t *)virtAlloc(SIZE_1G);
	if (!wrBuf)
	{
		//printf("error: can not alloc %d Mbytes\n", SIZE_32M / SIZE_1M);
		printf("ERROR: can not alloc %d Mbytes for writing to file\n", SIZE_1G / SIZE_1M);
		return -1;
	}

	// Выделяем память для чтения из файла
	printf("Allocating memory for reading from file...              \n");
	uint32_t *rdBuf = (uint32_t *)virtAlloc(SIZE_1G);
	if (!rdBuf)
	{
		//printf("error: can not alloc %d Mbytes\n", SIZE_32M / SIZE_1M);
		printf("ERROR: can not alloc %d Mbytes for reading from file\n", SIZE_1G / SIZE_1M);
		return -1;
	}

	char rdfname[512];
	double rd_time = 0.;
	//double total_time = 0.;
	//double max_time = 0.;
	size_t readsize = SIZE_1G;
	const int bsize = SIZE_1G / sizeof(int32_t);

//#ifdef __linux__
//	pthread_mutex_init(&mutex_read, NULL);//Инициализация мьютекса
//	pthread_mutex_init(&mutex_write, NULL);//Инициализация мьютекса
//	//pthread_t thread_id;
//#else
//	g_hMutex_read = CreateMutex(NULL, FALSE, name_mutex_read);
//	g_hMutex_write = CreateMutex(NULL, FALSE, name_mutex_write);
//	//HANDLE hThread;
//#endif

	CMutex write_mutex;
	CMutex read_mutex;

	THREAD_PARAM thread_par;
	thread_par.wr_buf = wrBuf;
	thread_par.size = bsize;
	thread_par.wrmutex = &write_mutex;
	thread_par.rdmutex = &read_mutex;
	//thread_par.size = SIZE_1G / sizeof(int32_t);

	CThread write_thread;
	write_thread.create(&file_write_thread, &(thread_par));
//#ifdef __linux__
//	thread_id = createThread(&file_write_thread, &(thread_par));
//#else
//	unsigned threadID;
//	hThread = (HANDLE)_beginthreadex(NULL, 0, &file_write_thread, &(thread_par), 0, &(threadID));
//#endif

	while (!exit_app)
	{
		if (exit_app)
			break;

		int idx = 2;
		printf("ENTER the file number (0 - %d) for reading !\n", g_read_max-1);
		//printf("ENTER the file number for reading !\n");
		// выход из scanf по CTRL+D под Windows и по CTRL+Z под Linux 
		int inpch_cnt = scanf("%d", &idx);
		if (inpch_cnt != 1)
		{
			printf("scanf returned NOT 1 !\n");
			exit_app = 1;
		// break;
		}
		if (exit_app)
			break;

		if (idx >= g_read_max)
			continue;

		{
			sprintf(rdfname, "%s_%03d", file_name, idx);

			//while (wrfile_num == idx)
			//	ipc_delay(250);
			if (wrfile_num == idx)
			{
				write_mutex.lock();
				write_mutex.unlock();
//#ifdef __linux__
//				pthread_mutex_lock(&mutex_read);
//				pthread_mutex_unlock(&mutex_read);
//#else
//				//printf("MAIN: Waiting write mutex before reading (wr_num %d, rd_num %d, rd_idx %d)...                   \n", wrfile_num, rdfile_num, idx);
//				WaitForSingleObject(g_hMutex_write, INFINITE);
//				//printf("MAIN: Release read mutex (wr_num %d, rd_num %d, rd_idx %d)...                   \n", wrfile_num, rdfile_num, idx);
//				ReleaseMutex(g_hMutex_write);
//#endif
			}

			read_mutex.lock();
//#ifdef __linux__
//			pthread_mutex_lock(&mutex_read);
//#else
//			//printf("MAIN: Capturing read mutex (wr_num %d, rd_num %d, rd_idx %d)...                   \n", wrfile_num, rdfile_num, idx);
//			printf("READ CAPTURE (rd_idx %d)!\n", idx);
//			WaitForSingleObject(g_hMutex_read, INFINITE);
//#endif

			rdfile_num = idx;

			ipc_time_t start_time, stop_time;
			start_time = ipc_get_time();

			if (g_direct)
			{
				//printf("READ (MAIN) THREAD: DIRECT & SYNC reading buffer from disk!\n");
				int ret = file_read_direct(rdBuf, rdfname);
				if (ret < 0)
				{
					read_mutex.unlock();
//#ifdef __linux__
//					pthread_mutex_unlock(&mutex_read);
//#else
//					ReleaseMutex(g_hMutex_read);
//#endif
					return ret;
				}
			}
			else
			{
				//printf("READ (MAIN) THREAD: Reading buffer from disk by fopen()!\n");
				FILE *fp = fopen(rdfname, "rb");
				if (!fp) {
					printf("ERROR: can not open %s\n", rdfname);
					read_mutex.unlock();
//#ifdef __linux__
//					pthread_mutex_unlock(&mutex_read);
//#else
//					ReleaseMutex(g_hMutex_read);
//#endif
					return -1;
				}

				//printf("MAIN: Reading file %s ...                   \n", rdfname);
				size_t ret = fread(rdBuf, 1, readsize, fp);
				if (ret != readsize)
				{
					printf("ERROR: can not read %s\n", rdfname);
					read_mutex.unlock();
//#ifdef __linux__
//					pthread_mutex_unlock(&mutex_read);
//#else
//					ReleaseMutex(g_hMutex_read);
//#endif
					return -1;
				}

				//printf("MAIN: Closing file %s ...                   \n", rdfname);
				fclose(fp);
			}

			stop_time = ipc_get_time();
			rd_time = ipc_get_nano_difftime(start_time, stop_time) / 1000000.;

			read_mutex.unlock();
//#ifdef __linux__
//			pthread_mutex_unlock(&mutex_read);
//#else
//			//printf("MAIN: Release read mutex (wr_num %d, rd_num %d, rd_idx %d)...                   \n", wrfile_num, rdfile_num, idx);
//			printf("READ RELEASE (rd_idx %d)!\n", idx);
//			ReleaseMutex(g_hMutex_read);
//#endif

			//printf("READ THREAD: Time (%s): Total %.4f s, Cur %.4f s, Max %.4f s\n",
			//	rdfname, total_time / 1000., rd_time / 1000., max_time / 1000.);
			//printf("READ THREAD: Speed (%s): Avr %.4f Mb/s, Cur %.4f Mb/s, Min %.4f Mb/s\r",
			//	rdfname, ((double)readsize * (++count) / total_time) / 1000., ((double)readsize / rd_time) / 1000., ((double)readsize / max_time) / 1000.);
			printf("READ THREAD (%s): Cur Time %.4f s, Cur Speed %.4f Mb/s\n", 
								rdfname, rd_time / 1000., ((double)readsize / rd_time) / 1000.);

			uint32_t err_cnt = 0;
			for (int i = 0; i < bsize; i++)
			{
				uint32_t cmp_val = (idx << 16) + i;
				if (rdBuf[i] != cmp_val)
					err_cnt++;
			}
			if (err_cnt)
			{
				printf(" Errors by reading: %d\n", err_cnt);
				return -1;
			}
			printf("READ (MAIN) THREAD: Verifying file %s is OK                        \n", rdfname);

		}
	}

	write_thread.wait(INFINITE);
	write_thread.close();
//#ifdef __linux__
//	//waitThread(thread_id, -1);// Wait until threads terminates
//	//deleteThread(thread_id);
//	pthread_mutex_destroy(&mutex_read); //Уничтожение мьютекса
//	pthread_mutex_destroy(&mutex_write); //Уничтожение мьютекса
//#else
//	//WaitForSingleObject(hThread, INFINITE); // Wait until threads terminates
//	//CloseHandle(hThread);
//
//	CloseHandle(g_hMutex_read);
//	CloseHandle(g_hMutex_write);
//#endif

	virtFree(rdBuf);
	virtFree(wrBuf);

	return 0;
}

