#include <stdio.h>

#ifdef _WIN32
#include "windows.h"
#include <conio.h>
//#include <stdint.h>
//#include <process.h> 
#else
#include <unistd.h> 
//#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
//#include <errno.h>
#include <linux/hdreg.h>
#endif

#include "time_func.h"

int file_write_direct(char* fname, void *buf, size_t size)
{
#ifdef __linux__
	int sysflag = O_CREAT | O_TRUNC | O_WRONLY | O_DIRECT | O_SYNC;
	int hfile = open(fname, sysflag, 0666);
	if (hfile < 0)
	{
		printf("ERROR: can not open %s\n", fname);
		return -1;
	}
	printf("Writing file %s ...                   \r", fname);
	//size_t writesize = SIZE_1G;
	int res = write(hfile, buf, size);
	if (res < 0) {
		printf("ERROR: can not write %s\n", fname);
		return -2;
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
		printf("ERROR: can not open %s\n", fname);
		return -1;
	}
	printf("Writing file %s ...                   \r", fname);
	unsigned long writesize;
	int ret = WriteFile(hfile, buf, (DWORD)size, &writesize, NULL);
	if (!ret)
	{
		printf("ERROR: can not write %s\n", fname);
		return -2;
	}
	CloseHandle(hfile);
#endif // __linux__
	return 0;
}

int file_write(char* fname, void *buf, size_t size)
{
	FILE *fp = fopen(fname, "wb");
	if (!fp) {
		printf("ERROR: can not open %s\n", fname);
		return -1;
	}

	//printf("THREAD: Writing file %s ...                   \r", wrfname);
	size_t ret = fwrite(buf, 1, size, fp);
	if (ret != size)
	{
		printf("ERROR: can not write %s\n", fname);
		return -2;
	}

	fclose(fp);

	return 0;
}

int file_read_direct(char* fname, void *buf, size_t size)
{
#ifdef __linux__
	int sysflag = O_RDONLY | O_DIRECT | O_SYNC;
	int hfile = open(fname, sysflag, 0666);
	if (hfile < 0)
	{
		printf("ERROR: can not open %s\n", fname);
		return -1;
	}
	printf("Reading file %s ...                   \r", fname);
	//size_t readsize = SIZE_1G;
	int res = read(hfile, buf, size);
	if (res < 0) {
		printf("ERROR: can not read %s\n", fname);
		return -2;
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
		printf("ERROR: can not open %s\n", fname);
		return -1;
	}
	printf("Reading file %s ...                   \r", fname);
	unsigned long readsize;
	int ret = ReadFile(hfile, buf, (DWORD)size, &readsize, NULL);
	if (!ret)
	{
		printf("ERROR: can not read %s\n", fname);
		return -2;
	}
	CloseHandle(hfile);
#endif // __linux__
	return 0;
}

int file_read(char* fname, void *buf, size_t size)
{
	FILE *fp = fopen(fname, "rb");
	if (!fp) {
		printf("ERROR: can not open %s\n", fname);
		return -1;
	}

	//printf("MAIN: Reading file %s ...                   \n", rdfname);
	size_t ret = fread(buf, 1, size, fp);
	if (ret != size)
	{
		printf("ERROR: can not read %s\n", fname);
		return -2;
	}

	//printf("MAIN: Closing file %s ...                   \n", rdfname);
	fclose(fp);
	return 0;
}

#ifdef _WIN32
ULONG GetSerial(HANDLE hFile)
{
	static STORAGE_PROPERTY_QUERY spq = { StorageDeviceProperty, PropertyStandardQuery };

	union {
		PVOID buf;
		PSTR psz;
		PSTORAGE_DEVICE_DESCRIPTOR psdd;
	};

	ULONG size = sizeof(STORAGE_DEVICE_DESCRIPTOR) + 0x100;

	ULONG dwError;

	do
	{
		dwError = ERROR_NO_SYSTEM_RESOURCES;

		if (buf = LocalAlloc(0, size))
		{
			ULONG BytesReturned;

			if (DeviceIoControl(hFile, IOCTL_STORAGE_QUERY_PROPERTY, &spq, sizeof(spq), buf, size, &BytesReturned, 0))
			{
				if (psdd->Version >= sizeof(STORAGE_DEVICE_DESCRIPTOR))
				{
					if (psdd->Size > size)
					{
						size = psdd->Size;
						dwError = ERROR_MORE_DATA;
					}
					else
					{
						if (psdd->SerialNumberOffset)
						{
							printf("Serial number - %s\n", psz + psdd->SerialNumberOffset);
							dwError = NOERROR;
						}
						else
						{
							dwError = ERROR_NO_DATA;
						}
					}
				}
				else
				{
					dwError = ERROR_GEN_FAILURE;
				}
			}
			else
			{
				dwError = GetLastError();
			}

			LocalFree(buf);
		}
	} while (dwError == ERROR_MORE_DATA);

	return dwError;
}
#endif

int get_info_drive(char* drvname)
{
#ifdef __linux__
	//O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH | S_IWOTH
	//int sysflag = O_WRONLY;
	int sysflag = O_RDWR | O_DIRECT | O_SYNC;
	int hfile = open(drvname, sysflag, 0666);
	if (hfile < 0)
	{
		printf("ERROR: can not open %s\n", drvname);
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
	close(hfile);
#else

	//char volume_name[MAX_PATH]; // lpVolumeNameBuffer
	//char file_system[MAX_PATH]; // lpFileSystemNameBuffer
	//DWORD serial_no; // VolumeSerialNumber
	//DWORD max_path_length; // MaximumComponentLength
	//DWORD file_system_flags; // FileSystemFlags

	//if (GetVolumeInformation(drvname, volume_name, sizeof(volume_name),
	//	&serial_no, &max_path_length, &file_system_flags, file_system, sizeof(file_system)))
	//{
	//	printf("Volume name - %s\n", volume_name);
	//	printf("Serial number - %u\n", serial_no);
	//	printf("File system - %s\n", file_system);
	//	printf("Max path length - %d\n", max_path_length);
	//	printf("File system flags - %08X\n", file_system_flags);
	//}

	unsigned long amode = GENERIC_WRITE;
	unsigned long cmode = OPEN_EXISTING;
	unsigned long fattr = 0;
	HANDLE  hfile = CreateFile(drvname,
		amode,
		//						FILE_SHARE_WRITE | FILE_SHARE_READ,
		0,
		NULL,
		cmode,
		fattr,
		NULL);
	if (hfile == INVALID_HANDLE_VALUE)
	{
		printf("ERROR: can not open %s\n", drvname);
		_getch();
		return -1;
	}

	GetSerial(hfile);

	DISK_GEOMETRY pdg = { 0 }; // disk drive geometry 
	DWORD junk = 0;                     // discard results
	BOOL bResult = DeviceIoControl(hfile,      // device to be queried
					IOCTL_DISK_GET_DRIVE_GEOMETRY, // operation to perform
					NULL, 0,                       // no input buffer
					&pdg, sizeof(pdg),            // output buffer
					&junk,                         // # bytes returned
					(LPOVERLAPPED)NULL);          // synchronous I/O

	//printf("Drive path      = %s\n", wszDrive);
	printf("Cylinders       = %I64d\n", pdg.Cylinders);
	printf("Tracks/cylinder = %ld\n", (ULONG)pdg.TracksPerCylinder);
	printf("Sectors/track   = %ld\n", (ULONG)pdg.SectorsPerTrack);
	printf("Bytes/sector    = %ld\n", (ULONG)pdg.BytesPerSector);

	ULONGLONG DiskSize = 0;    // size of the drive, in bytes
	DiskSize = pdg.Cylinders.QuadPart * (ULONG)pdg.TracksPerCylinder *
		(ULONG)pdg.SectorsPerTrack * (ULONG)pdg.BytesPerSector;
	printf("Disk size  = %I64d (Bytes) or  %.2f (Gb)\n", DiskSize, (double)DiskSize / (1024 * 1024 * 1024));

	CloseHandle(hfile);
#endif // __linux__
	return 0;
}

#define SIZE_1G 1024*1024*1024

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
		printf("ERROR: can not open %s\n", fname);
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
			printf("ERROR: can not write %s\n", fname);
			close(hfile);
			return -1;
		}
		if (SIZE_1G != res)
		{
			printf("ERROR: disk %s is full\n", fname);
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
			printf("ERROR: disk %s is full\n", fname);
			CloseHandle(hfile);
			_getch();
			return -1;
		}
		if (!ret)
		{
			printf("ERROR: can not write %s\n", fname);
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
		printf("ERROR: can not open %s\n", fname);
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
		printf("ERROR: can not open %s\n", fname);
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
			printf("ERROR: can not read %s\n", fname);
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
