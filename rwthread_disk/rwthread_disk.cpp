
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

#include "../lib_func/time_func.h"
#include "../lib_func/mem_func.h"
#include "../lib_func/cpu_func.h"
#include "../lib_func/cthread.h"
#include "../lib_func/cmdline.h"

#include "log4cpp/Category.hh"
#include "log4cpp/Appender.hh"
#include "log4cpp/FileAppender.hh"
#include "log4cpp/OstreamAppender.hh"
#include "log4cpp/Layout.hh"
#include "log4cpp/BasicLayout.hh"
#include "log4cpp/Priority.hh"
#include "log4cpp/RollingFileAppender.hh"
#include "log4cpp/PatternLayout.hh"

//#define SIZE_1G 1024*1024*1024
//#define SIZE_32M 32*1024*1024
//#define SIZE_16M 16*1024*1024
#define SIZE_1M 1024*1024

#include <csignal>
#include <cstring>
#include <tuple>

#ifdef __linux__

#define MAX_PATH          260
char drive_name[MAX_PATH] = "/dev/sdc1";
char file_name[MAX_PATH] = "/mnt/diskG/data";
//char file_name[MAX_PATH] = "/mnt/f/32G/data";
//char file_name[MAX_PATH] = "/mnt/e/data";

#else

//char drive_name[MAX_PATH] = "\\\\.\\C:";
//char file_name[MAX_PATH] = "C:/_WORKS/data";
//char drive_name[MAX_PATH] = "\\\\.\\G:";
char drive_name[MAX_PATH] = "\\\\.\\H:";
char file_name[MAX_PATH] = "H:/data";
//char drive_name[MAX_PATH] = "\\\\.\\PhysicalDrive3";

#endif // __linux__

int g_fcnt = 4;
int g_direct = 0;
int g_drive = 0;
double g_speed_limit = 1800.;
int g_fsize = 1024;

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
//	char		*pLin, *endptr;

	for (ii = 1; ii < argc; ii++)
	{
		// Вывести подсказку
		if (tolower(argv[ii][1]) == 'h')
		{
			DisplayHelp();
			exit(1);
		}

		// Указать режим небуферизированного синхронного вывода в файл
		g_direct = get_cmdl_arg(ii, argv, "-dir", ARG_TYPE_NOT_NUM, g_direct);

		// Указать число записываемых файлов
		g_fcnt = get_cmdl_arg(ii, argv, "-n", ARG_TYPE_CHR_NUM, g_fcnt);

		// Указать размер файла
		g_fsize = get_cmdl_arg(ii, argv, "-s", ARG_TYPE_CHR_NUM, g_fsize);
		g_fsize *= 1024 * 1024;
	}
}

int file_write_direct(char* fname, void *buf, size_t size);
int file_write(char* fname, void *buf, size_t size);
int file_read_direct(char* fname, void *buf, size_t size);
int file_read(char* fname, void *buf, size_t size);
//int write2drive(void *buf, char* fname, int num);
//int read_drive(char* fname, int num, uint32_t *read_buf, uint32_t *cmp_buf);
int get_info_drive(char* drvname);

volatile int wrfile_num = -1;
volatile int rdfile_num = -1;

//volatile int g_read_max = 1;

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

	SetAffinityCPU(3);

//#ifdef __linux__
//	// Set affinity mask to include CPU
//	cpu_set_t cpuset;
//	CPU_ZERO(&cpuset);
//	int cpu_num = 11;
//	CPU_SET(cpu_num, &cpuset);
//	pthread_t current_thread = pthread_self();
//	pthread_setaffinity_np(current_thread, sizeof(cpuset), &cpuset);
//	// Check the actual affinity mask assigned to the thread.
//	pthread_getaffinity_np(current_thread, sizeof(cpuset), &cpuset);
//	printf("Set returned by pthread_getaffinity_np() contained:\n");
//	for (int j = 0; j < CPU_SETSIZE; j++)
//		if (CPU_ISSET(j, &cpuset))
//			printf("    CPU %d\n", j);
//#else
//	HANDLE hCurThread = GetCurrentThread();
//	uint32_t cpu_mask = 0x8; // 3-й
//	SetThreadAffinityMask(hCurThread, cpu_mask);
//	//int cpu_num = 1;
//	//SetThreadIdealProcessor(hCurThread, cpu_num);
//#endif

	log4cpp::Category& _LOG = log4cpp::Category::getRoot();

	PTHREAD_PARAM pThreadParam = (PTHREAD_PARAM)pParams;
	
	int bsize = pThreadParam->size;
	uint32_t *wrBuf = pThreadParam->wr_buf;
	CMutex *wrMutex = pThreadParam->wrmutex;
	CMutex *rdMutex = pThreadParam->rdmutex;

	char wrfname[512];
	double wr_time = 0.;
	double total_time = 0.;
	double max_time = 0.;
	size_t writesize = g_fsize;

//	if (g_drive)
//	{
//		int ret = write2drive(wrBuf, drive_name, g_fcnt);
//	}
//	else
	{
		int count = 0;
		int count1500 = 0;
		int count1600 = 0;
		int count1700 = 0;
		int count1800 = 0;
		int count1900 = 0;
		int count2000 = 0;
		int cycle = 0;
		while (!exit_app)
		{
			for (int idx = 0; idx < g_fcnt; idx++)
			{
				for (int i = 0; i < bsize; i++)
					wrBuf[i] = (idx << 16) + i;

				sprintf(wrfname, "%s_%07d", file_name, idx);

				//while (rdfile_num == idx)
				//	ipc_delay(250);
				if (rdfile_num == idx)
				{
					rdMutex->sync();
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

				double speed_avr = ((double)writesize * (++count) / total_time) / 1000.;
				double speed_cur = ((double)writesize / wr_time) / 1000.;
				double speed_min = ((double)writesize / max_time) / 1000.;

				if (speed_cur < 1500.) count1500++;
				else if (speed_cur < 1600.) count1600++;
					else if (speed_cur < 1700.) count1700++;
						else if (speed_cur < 1800.) count1800++;
							else if (speed_cur < 1900.) count1900++;
								else if (speed_cur < 2000.) count2000++;

				//printf("WRITE THREAD: Time (%s): Total %.4f s, Cur %.4f s, Max %.4f s\n",
				//						wrfname, total_time / 1000., wr_time / 1000., max_time / 1000.);
				printf("WRITE THREAD: Speed (%d %s): Avr %.4f Mb/s, Cur %.4f Mb/s, Min %.4f Mb/s (%d %d %d %d %d %d)\r",
							cycle, wrfname, speed_avr, speed_cur, speed_min,
							count2000, count1900, count1800, count1700, count1600, count1500);

				if (speed_cur < g_speed_limit)
				{
					printf("\n");
					char message[256];
					sprintf(message, "WRITE Speed (%d %s): Avr %.4f Mb/s, Cur %.4f Mb/s, Min %.4f Mb/s (%d %d %d %d %d %d)",
						cycle, wrfname, speed_avr, speed_cur, speed_min,
						count2000, count1900, count1800, count1700, count1600, count1500);
					_LOG << log4cpp::Priority::INFO << message;
				}
				//	printf("\n");
				//g_read_max = (count < g_fcnt) ? count : g_fcnt;

				if (exit_app)
					break;
			}
			cycle++;
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

	SetAffinityCPU(2);

//#ifdef __linux__
//	// Set affinity mask to include CPU
//	cpu_set_t cpuset;
//	CPU_ZERO(&cpuset);
//	int cpu_num = 15;
//	CPU_SET(cpu_num, &cpuset);
//	pthread_t current_thread = pthread_self();
//	pthread_setaffinity_np(current_thread, sizeof(cpuset), &cpuset);
//	// Check the actual affinity mask assigned to the thread.
//	pthread_getaffinity_np(current_thread, sizeof(cpuset), &cpuset);
//	printf("Set returned by pthread_getaffinity_np() contained:\n");
//	for (int j = 0; j < CPU_SETSIZE; j++)
//		if (CPU_ISSET(j, &cpuset))
//			printf("    CPU %d\n", j);
//#else
//	HANDLE hCurThread = GetCurrentThread();
//	uint32_t cpu_mask = 0x200; // 8-й
//	SetThreadAffinityMask(hCurThread, cpu_mask);
//	//int cpu_num = 1;
//	//SetThreadIdealProcessor(hCurThread, cpu_num);
//#endif

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

	// создаем отсылатель сообщений в файл (с поддержкой ротации по размеру)
	std::string log_file = "rwthread_disk.log";
	log4cpp::Appender *appender2 = new log4cpp::RollingFileAppender("default", log_file, 16 * 1024 * 1024, 8);

	// настраиваем уровень логгирования
	log4cpp::Category& _LOG = log4cpp::Category::getRoot();
	//_LOG.setPriority(log4cpp::Priority::NOTICE);
	_LOG.setPriority(log4cpp::Priority::INFO);
	_LOG.addAppender(appender2);

	//_LOG << log4cpp::Priority::NOTICE << _log.message(start_message);
	_LOG << log4cpp::Priority::INFO << "Started root logging";

	ParseCommandLine(argc, argv);

	int ret = get_info_drive(drive_name);
		
	// Выделяем память для записи в файл
	printf("Allocating memory for writing to file...              \n");
	uint32_t *wrBuf = (uint32_t *)virtAlloc(g_fsize);
	if (!wrBuf)
	{
		printf("ERROR: can not alloc %d Mbytes for writing to file\n", g_fsize / SIZE_1M);
		return -1;
	}

	// Выделяем память для чтения из файла
	printf("Allocating memory for reading from file...              \n");
	uint32_t *rdBuf = (uint32_t *)virtAlloc(g_fsize);
	if (!rdBuf)
	{
		printf("ERROR: can not alloc %d Mbytes for reading from file\n", g_fsize / SIZE_1M);
		return -1;
	}

	char rdfname[512];
	double rd_time = 0.;
	//double total_time = 0.;
	//double max_time = 0.;
	size_t readsize = g_fsize;
	const int bsize = g_fsize / sizeof(int32_t);

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
		ipc_delay(5000);
		int idx = 2;
		//printf("ENTER the file number (0 - %d) for reading !\n", g_read_max-1);
		//printf("ENTER the file number for reading !\n");
		// выход из scanf по CTRL+D под Windows и по CTRL+Z под Linux 
		//int inpch_cnt = scanf("%d", &idx);
		//if (inpch_cnt != 1)
		//{
		//	printf("scanf returned NOT 1 !\n");
		//	exit_app = 1;
		//// break;
		//}
		//if (exit_app)
		//	break;

		//if (idx >= g_read_max)
		//	continue;

		// установка начального значения для функции генерации псевдослучайных чисел, которая формирует тестовые шаблоны
		//unsigned int seed = (unsigned int)time(NULL);
		//srand(seed);
		srand((unsigned int)time(NULL));
		idx = rand() % g_fcnt;
		// формула генерации случайных чисел по заданному диапазону
		//random_number = firs_value + rand() % last_value;
		// где firs_value - минимальное число из желаемого диапазона
		// last_value - ширина выборки

		{
			sprintf(rdfname, "%s_%07d", file_name, idx);

			//while (wrfile_num == idx)
			//	ipc_delay(250);
			if (wrfile_num == idx)
			{
				write_mutex.sync();
			}

			read_mutex.lock();

			rdfile_num = idx;

			ipc_time_t start_time, stop_time;
			start_time = ipc_get_time();
			int ret = 0;
			if (g_direct)
			{
				//printf("READ (MAIN) THREAD: DIRECT & SYNC reading buffer from disk!\n");
				ret = file_read_direct(rdfname, rdBuf, readsize);
				//if (ret < 0)
				//{
				//	read_mutex.unlock();
				//	return ret;
				//}
			}
			else
			{
				//printf("READ (MAIN) THREAD: Reading buffer from disk by fopen()!\n");
				ret = file_read(rdfname, rdBuf, readsize);
				//if (ret < 0)
				//{
				//	read_mutex.unlock();
				//	return ret;
				//}
			}

			stop_time = ipc_get_time();
			rd_time = ipc_get_nano_difftime(start_time, stop_time) / 1000000.;

			read_mutex.unlock();

//#ifdef __linux__
//	// Set affinity mask to include CPU
//	cpu_set_t cpuset;
//	CPU_ZERO(&cpuset);
//	pthread_t current_thread = pthread_self();
//	// Check the actual affinity mask assigned to the thread.
//	pthread_getaffinity_np(current_thread, sizeof(cpuset), &cpuset);
//	printf("Set returned by pthread_getaffinity_np() contained:\n");
//	for (int j = 0; j < CPU_SETSIZE; j++)
//		if (CPU_ISSET(j, &cpuset))
//			printf("    CPU %d\n", j);
//#endif
			if (ret == 0)
			{
				//printf("READ THREAD: Time (%s): Total %.4f s, Cur %.4f s, Max %.4f s\n",
				//	rdfname, total_time / 1000., rd_time / 1000., max_time / 1000.);
				//printf("READ THREAD: Speed (%s): Avr %.4f Mb/s, Cur %.4f Mb/s, Min %.4f Mb/s\r",
				//	rdfname, ((double)readsize * (++count) / total_time) / 1000., ((double)readsize / rd_time) / 1000., ((double)readsize / max_time) / 1000.);
				double cur_time = rd_time / 1000.;
				double cur_speed = ((double)readsize / rd_time) / 1000.;
				printf("\nREAD (%s): Cur Time %.4f s, Cur Speed %.4f Mb/s\n", rdfname, cur_time, cur_speed);

				char message[256];
				sprintf(message, "READ (%s) : Cur Time %.4f s, Cur Speed %.4f Mb/s", rdfname, cur_time, cur_speed);
				_LOG << log4cpp::Priority::INFO << message;

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

					sprintf(message, "%d ERRORS by reading !!!", err_cnt);
					_LOG << log4cpp::Priority::ERROR << message;

					ret = -1;
					break;
					//return -1;
				}
				//printf("READ (MAIN) THREAD: Verifying file %s is OK                        \n", rdfname);
			}

		}
	}

	write_thread.wait(INFINITE);
	write_thread.close();

	virtFree(rdBuf);
	virtFree(wrBuf);

	//return 0;
	return ret;
}

