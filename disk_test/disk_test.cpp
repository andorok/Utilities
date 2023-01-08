
#include <stdio.h>

//#include <numa.h>
//#include <sched.h>
//#include <math.h>

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

#include "../lib_func/time_func.h"
#include "../lib_func/mem_func.h"
#include "../lib_func/cpu_func.h"
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
char drive_name[MAX_PATH] = "/dev/sda";
//char drive_name[MAX_PATH] = "/dev/sdc1";
//char drive_name[MAX_PATH] = "/dev/nvme0n1";
char file_name[MAX_PATH] = "../data";
//char file_name[MAX_PATH] = "/mnt/diskG/data";
//char file_name[MAX_PATH] = "/mnt/f/32G/data";
#else
//char drive_name[MAX_PATH] = "\\\\.\\C:";
//char file_name[MAX_PATH] = "C:/_WORKS/data";
//char drive_name[MAX_PATH] = "\\\\.\\G:";
char drive_name[MAX_PATH] = "\\\\.\\H:";
char file_name[MAX_PATH] = "H:/data";
//char file_name[MAX_PATH] = "F:/32G/data";
#endif // __linux__

//#pragma pack(push,1)
//
//// информация о параметрах работы приложения
//typedef struct {
//	int g_fcnt = 1;
//	int g_direct = 0;
//	int g_drive = 0;
//	int g_read = 0;
//} APP_OPTIONS, *PAPP_OPTIONS;
//
//#pragma pack(pop)

int g_fcnt = 1;
int g_direct = 0;
int g_drive = 0;
int g_read = 0;
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
	printf(" -drv  - writing to drive without file system\n");
	printf(" -rd  - reading after writing\n");
	//	printf(" -thr<N>  - use N threads\n");
	printf(" -n<N>  - writing N files\n");
	printf("\n");
}

// разобрать командную строку
void ParseCommandLine(int argc, char *argv[])
{
	int			ii;
	//char		*pLin, *endptr;

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

		// Указать режим записи на диск без файловой системы
		g_drive = get_cmdl_arg(ii, argv, "-drv", ARG_TYPE_NOT_NUM, g_drive);

		// Выполнить чтение и проверку после записи
		g_read = get_cmdl_arg(ii, argv, "-rd", ARG_TYPE_NOT_NUM, g_read);

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
int write2drive(char* fname, void *buf, int num);
int read_drive(char* fname, void *buf, int num);
int get_info_drive(char* drvname);

int main(int argc, char *argv[])
{
	// выход по Ctrl+C
	signal(SIGINT, signal_handler);

	SetAffinityCPU(3);

	// создаем отсылатель сообщений в файл (с поддержкой ротации по размеру)
	std::string log_file = "disk_test.log";
	log4cpp::Appender *appender2 = new log4cpp::RollingFileAppender("default", log_file, 16 * 1024 * 1024, 8);

	// настраиваем уровень логгирования
	log4cpp::Category& _LOG = log4cpp::Category::getRoot();
	//_LOG.setPriority(log4cpp::Priority::NOTICE);
	_LOG.setPriority(log4cpp::Priority::INFO);
	_LOG.addAppender(appender2);

	//_LOG << log4cpp::Priority::NOTICE << _log.message(start_message);
	_LOG << log4cpp::Priority::INFO << "Started root logging";

	//int node = 0;
	ParseCommandLine(argc, argv);

	/*int ret = */get_info_drive(drive_name);

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

	char wrfname[512];
	double wr_time = 0.;
	double rd_time = 0.;
	double total_time = 0.;
	double max_time = 0.;
	double quad_time[4] = { 0., 0., 0., 0.};
	int idxq = 0;

	size_t writesize = g_fsize;
	const int bsize = g_fsize / sizeof(int32_t);

	if (g_drive)
	{
		int ret = write2drive(drive_name, wrBuf, g_fcnt);

		if (g_read)
		{
			ret = read_drive(drive_name, rdBuf, g_fcnt);
			//ret = read_drive_block(drive_name, rdBuf, g_fcnt / 2);
		}
		return ret;
	}

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

			int ret = 0;
			sprintf(wrfname, "%s_%07d", file_name, idx);

			ipc_time_t start_time, stop_time;
			start_time = ipc_get_time();

			if (g_direct)
			{
				printf("DIRECT & SYNC writing buffer on disk!\r");
				ret = file_write_direct(wrfname, wrBuf, writesize);
			}
			else
			{
				ret = file_write(wrfname, wrBuf, writesize);
			}

			stop_time = ipc_get_time();
			wr_time = ipc_get_nano_difftime(start_time, stop_time) / 1000000.;

			if (ret == 0)
			{
				total_time += wr_time;
				if (wr_time > max_time)
					max_time = wr_time;

				quad_time[idxq++] = wr_time;
				if (idxq == 4) idxq = 0;
				double quad_avr = ((double)writesize * 4 / (quad_time[0] + quad_time[1] + quad_time[2] + quad_time[3])) / 1000.;

				//double speed_avr = ((double)writesize * (idx + 1) / total_time) / 1000.;
				double speed_avr = ((double)writesize * (++count) / total_time) / 1000.;
				double speed_cur = ((double)writesize / wr_time) / 1000.;
				double speed_min = ((double)writesize / max_time) / 1000.;

				if (speed_cur < 1500.) count1500++;
				else if (speed_cur < 1600.) count1600++;
					else if (speed_cur < 1700.) count1700++;
						else if (speed_cur < 1800.) count1800++;
							else if (speed_cur < 1900.) count1900++;
								else if (speed_cur < 2000.) count2000++;

				printf("WRITE Speed (%d %s): Avr %.4f Mb/s (%.4f), Cur %.4f Mb/s, Min %.4f Mb/s (%d %d %d %d %d %d)\r", 
					cycle, wrfname, speed_avr, quad_avr, speed_cur, speed_min, 
					count2000, count1900, count1800, count1700, count1600, count1500);
				if (speed_cur < g_speed_limit)
				{
					printf("\n");
					char message[256];
					sprintf(message, "WRITE Speed (%d %s): Avr %.4f Mb/s (%.4f), Cur %.4f Mb/s, Min %.4f Mb/s (%d %d %d %d %d %d)",
						cycle, wrfname, speed_avr, quad_avr, speed_cur, speed_min,
						count2000, count1900, count1800, count1700, count1600, count1500);
					_LOG << log4cpp::Priority::INFO << message;
				}

			}
			if (exit_app)
				break;
		}
		cycle++;
	}

	if (g_read)
	{
		total_time = 0.;
		max_time = 0.;

		char rdfname[512];
		size_t readsize = g_fsize;

		for (int idx = 0; idx < g_fcnt; idx++)
		{
			int ret = 0;
			sprintf(rdfname, "%s_%07d", file_name, idx);

			ipc_time_t start_time, stop_time;
			start_time = ipc_get_time();

			if (g_direct)
			{
				printf("DIRECT & SYNC reading buffer from disk!\r");
				ret = file_read_direct(rdfname, rdBuf, readsize);
				//ret = read_drive_block(drive_name, rdBuf, g_fcnt / 2);		
			}
			else
			{
				printf("Reading buffer from disk by fopen()!\r");
				ret = file_read(rdfname, rdBuf, readsize);
			}

			stop_time = ipc_get_time();
			rd_time = ipc_get_nano_difftime(start_time, stop_time) / 1000000.;

			if (ret == 0)
			{
				total_time += rd_time;
				if (rd_time > max_time)
					max_time = rd_time;

				printf("READ Speed (%s): Avr %.4f Mb/s, Cur %.4f Mb/s, Min %.4f Mb/s\n", rdfname, ((double)readsize * (idx + 1) / total_time) / 1000., ((double)readsize / rd_time) / 1000., ((double)readsize / max_time) / 1000.);

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
				printf("Verifying is OK                        \n");
			}
		}
	}

#ifdef _WIN32
	//if (g_key)
	{
		printf("Press any key to quit of program\n");
		_getch();
	}
#endif
	return 0;
}

