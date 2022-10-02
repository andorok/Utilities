
//#include <stdio.h>

//#include <numa.h>
//#include <sched.h>

//#include <math.h>

#ifdef _WIN32
#include "windows.h"
//#include <conio.h>
//#include <process.h> 
#else
#include <unistd.h> 
//#include <sys/types.h>
//#include <sys/stat.h>
//#include <fcntl.h>
//#include <sys/ioctl.h>
//#include <errno.h>
//#include <linux/hdreg.h>
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

#else

//char drive_name[MAX_PATH] = "\\\\.\\G:";
//char file_name[MAX_PATH] = "G:/data";
char file_name[MAX_PATH] = "F:/32G/data";

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

int file_write_direct(char* fname, void *buf, size_t size);
int file_write(char* fname, void *buf, size_t size);
int file_read_direct(char* fname, void *buf, size_t size);
int file_read(char* fname, void *buf, size_t size);
//int write2drive(void *buf, char* fname, int num);
//int read_drive(char* fname, int num, uint32_t *read_buf, uint32_t *cmp_buf);

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
				}

				wrMutex->lock();

				wrfile_num = idx;
					
				ipc_time_t start_time, stop_time;
				start_time = ipc_get_time();

				if (g_direct)
				{
					//printf("THREAD: DIRECT & SYNC writing buffer on disk!\r");
					int ret = file_write_direct(wrfname, wrBuf, writesize);
					if (ret < 0)
					{
						wrMutex->unlock();
#ifdef __linux__
//						pthread_mutex_unlock(&mutex_write);
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
					int ret = file_write(wrfname, wrBuf, writesize);
					if (ret < 0)
					{
						wrMutex->unlock();
#ifdef __linux__
//						pthread_mutex_unlock(&mutex_write);
						return (void*)-1;
#else
//						ReleaseMutex(g_hMutex_write);
						return ret;
#endif
					}
				}

				stop_time = ipc_get_time();
				wr_time = ipc_get_nano_difftime(start_time, stop_time) / 1000000.;

				wrMutex->unlock();

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
			}

			read_mutex.lock();

			rdfile_num = idx;

			ipc_time_t start_time, stop_time;
			start_time = ipc_get_time();

			if (g_direct)
			{
				//printf("READ (MAIN) THREAD: DIRECT & SYNC reading buffer from disk!\n");
				int ret = file_read_direct(rdfname, rdBuf, readsize);
				if (ret < 0)
				{
					read_mutex.unlock();
					return ret;
				}
			}
			else
			{
				//printf("READ (MAIN) THREAD: Reading buffer from disk by fopen()!\n");
				int ret = file_read(rdfname, rdBuf, readsize);
				if (ret < 0)
				{
					read_mutex.unlock();
					return -1;
				}
			}

			stop_time = ipc_get_time();
			rd_time = ipc_get_nano_difftime(start_time, stop_time) / 1000000.;

			read_mutex.unlock();

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

	virtFree(rdBuf);
	virtFree(wrBuf);

	return 0;
}

