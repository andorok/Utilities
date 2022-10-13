
#include <stdio.h>

//#include <numa.h>
//#include <sched.h>
#include <math.h>

#ifdef _WIN32
#include "windows.h"
#include <conio.h>
#include <process.h> 
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

#define SIZE_1G 1024*1024*1024
#define SIZE_32M 32*1024*1024
#define SIZE_16M 16*1024*1024
#define SIZE_1M 1024*1024

#include <cstring>
#include <tuple>

#ifdef __linux__
#define MAX_PATH          260
char drive_name[MAX_PATH] = "/dev/sdc1";
//char drive_name[MAX_PATH] = "/dev/nvme0n1";
char file_name[MAX_PATH] = "../data";
//char file_name[MAX_PATH] = "/mnt/diskG/data";
//char file_name[MAX_PATH] = "/mnt/f/32G/data";
#else
//char drive_name[MAX_PATH] = "\\\\.\\G:";
char drive_name[MAX_PATH] = "\\\\.\\C:";
char file_name[MAX_PATH] = "C:/data";
//char file_name[MAX_PATH] = "F:/32G/data";
#endif // __linux__

int g_fcnt = 1;
int g_direct = 0;
int g_drive = 0;
int g_read = 0;

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
		if (!strcmp(argv[ii], "-drv"))
		{
			g_drive = 1;
			printf("Command line: -drv\n");
		}

		// Выполнить чтение и проверку после записи
		if (!strcmp(argv[ii], "-rd"))
		{
			g_read = 1;
			printf("Command line: -rd\n");
		}

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
int write2drive(char* fname, void *buf, int num);
int read_drive(char* fname, void *buf, int num);
int get_info_drive(char* drvname);

int main(int argc, char *argv[])
{
	//int node = 0;
	ParseCommandLine(argc, argv);

	int ret = get_info_drive(drive_name);

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

	char wrfname[512];
	double wr_time = 0.;
	double rd_time = 0.;
	double total_time = 0.;
	double max_time = 0.;
	size_t writesize = SIZE_1G;
	const int bsize = SIZE_1G / sizeof(int32_t);

	if (g_drive)
	{
		int ret = write2drive(drive_name, wrBuf, g_fcnt);

		if (g_read)
		{
			ret = read_drive(drive_name, rdBuf, g_fcnt);
		}
		return ret;
	}

    for (int idx = 0; idx < g_fcnt; idx++)
	{
		for (int i = 0; i < bsize; i++)
			wrBuf[i] = (idx << 16) + i;

		int ret = 0;
		sprintf(wrfname, "%s_%03d", file_name, idx);

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

			printf("WRITE Speed (%s): Avr %.4f Mb/s, Cur %.4f Mb/s, Min %.4f Mb/s\r", wrfname, ((double)writesize * (idx + 1) / total_time) / 1000., ((double)writesize / wr_time) / 1000., ((double)writesize / max_time) / 1000.);

			printf("\n");

		}
	}

	if (g_read)
	{
		total_time = 0.;
		max_time = 0.;

		char rdfname[512];
		size_t readsize = SIZE_1G;

		for (int idx = 0; idx < g_fcnt; idx++)
		{
			int ret = 0;
			sprintf(rdfname, "%s_%03d", file_name, idx);

			ipc_time_t start_time, stop_time;
			start_time = ipc_get_time();

			if (g_direct)
			{
				printf("DIRECT & SYNC reading buffer from disk!\r");
				ret = file_read_direct(rdfname, rdBuf, readsize);
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

	return 0;
}

