
#include <stdio.h>
#include <stdint.h>

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

#include "../lib_func/cpu_func.h"
#include "../lib_func/cmdline.h"

#include <cstring>
#include <tuple>

#ifdef __linux__
#define MAX_PATH          260
char drive_name[MAX_PATH] = "/dev/sda";
//char drive_name[MAX_PATH] = "/dev/sdc1";
//char drive_name[MAX_PATH] = "/dev/nvme0n1";
#else
//char drive_name[MAX_PATH] = "\\\\.\\G:";
char drive_name[MAX_PATH] = "\\\\.\\D:";
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
		//if (!strcmp(argv[ii], "-dir"))
		//{
		//	g_direct = 1;
		//	printf("Command line: -dir\n");
		//}

		// Указать режим записи на диск без файловой системы
		g_drive = get_cmdl_arg(ii, argv, "-drv", ARG_TYPE_NOT_NUM, g_drive);
		//if (!strcmp(argv[ii], "-drv"))
		//{
		//	g_drive = 1;
		//	printf("Command line: -drv\n");
		//}

		// Выполнить чтение и проверку после записи
		g_read = get_cmdl_arg(ii, argv, "-rd", ARG_TYPE_NOT_NUM, g_read);
		//if (!strcmp(argv[ii], "-rd"))
		//{
		//	g_read = 1;
		//	printf("Command line: -rd\n");
		//}

		// Указать число записываемых файлов
		g_fcnt = get_cmdl_arg(ii, argv, "-n", ARG_TYPE_CHR_NUM, g_fcnt);
		//if (tolower(argv[ii][1]) == 'n')
		//{
		//	pLin = &argv[ii][2];
		//	if (argv[ii][2] == '\0')
		//	{
		//		ii++;
		//		pLin = argv[ii];
		//	}
		//	g_fcnt = strtoul(pLin, &endptr, 0);
		//	printf("Command line: -n%d\n", g_fcnt);
		//}

	}
}

int get_info_drive(char* drvname);

int main(int argc, char *argv[])
{
	//SetAffinityCPU(3);

	ParseCommandLine(argc, argv);

	get_info_drive(drive_name);

#ifdef _WIN32
	//if (g_key)
	{
		printf("Press any key to quit of program\n");
		_getch();
	}
#endif
	return 0;
}
