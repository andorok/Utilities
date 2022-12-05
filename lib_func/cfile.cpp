
#include <stdio.h>

#ifdef __linux__
#include <unistd.h> 
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#else
#include "windows.h"
#include <conio.h>
#endif

#include "cfile.h"

CFile::CFile()
{
}

CFile::~CFile()
{
}

#ifdef __linux__
static int convert_ipc_flags(int ipc_flags)
{
	int flags = 0;

	if (ipc_flags & FLG_FILE_CREATE) {
		flags |= (O_CREAT | O_TRUNC);
	}

	if (ipc_flags & FLG_FILE_RDONLY) {
		flags |= (O_RDONLY);
	}

	if (ipc_flags & FLG_FILE_WRONLY) {
		flags |= (O_WRONLY);
	}

	if (ipc_flags & FLG_FILE_RDWR) {
		flags |= (O_RDWR);
	}

	if (ipc_flags & FLG_FILE_DIRECT) {
		flags |= (O_DIRECT | O_SYNC);
	}

	return flags;
}
#endif

//-----------------------------------------------------------------------------
int CFile::open(const char* fname, int flags)
{
	m_fname = fname;
#ifdef __linux__
	//int sysflag = O_CREAT | O_TRUNC | O_WRONLY | O_DIRECT | O_SYNC;
	int sysflag = convert_ipc_flags(flags);
	m_hfile = ::open(fname, sysflag, 0666);
	if (m_hfile < 0)
	{
		printf("ERROR: can not open %s\n", fname);
		return -1;
	}
#else
	//unsigned long amode = GENERIC_WRITE;
	//unsigned long cmode = CREATE_ALWAYS;
	//unsigned long fattr = FILE_FLAG_NO_BUFFERING;

	unsigned long amode = 0;
	unsigned long cmode = 0;
	unsigned long fattr = FILE_ATTRIBUTE_NORMAL;
	if (FLG_FILE_CREATE & (flags & 0xf))
		cmode = CREATE_ALWAYS;
	if (FLG_FILE_OPEN & (flags & 0xf))
		cmode = OPEN_EXISTING;

	if (FLG_FILE_RDONLY & (flags & 0xf0))
		amode = GENERIC_READ;
	if (FLG_FILE_WRONLY & (flags & 0xf0))
		amode |= GENERIC_WRITE;
	if (FLG_FILE_RDWR & (flags & 0xf0))
		amode = GENERIC_READ | GENERIC_WRITE;

	if (FLG_FILE_DIRECT & (flags & 0xf0))
		fattr = FILE_FLAG_NO_BUFFERING;
	//if (attr == FLG_FILE_WRTHROUGH)
	//	fattr = FILE_FLAG_WRITE_THROUGH;

	m_hfile = CreateFile(fname,
		amode,
		//						FILE_SHARE_WRITE | FILE_SHARE_READ,
		0,
		NULL,
		cmode,
		fattr,
		NULL);
	if (m_hfile == INVALID_HANDLE_VALUE)
	{
		printf("ERROR: can not open %s\n", fname);
		return -1;
	}
#endif // __linux__
	return 0;
}

//-----------------------------------------------------------------------------
int CFile::close()
{
#ifdef __linux__
	int res = ::close(m_hfile);
	if (res < 0) {
		//DEBUG_PRINT("%s(): %s\n", __FUNCTION__, strerror(errno));
		return -1;
	}
#else
	int ret = CloseHandle(m_hfile);
	if (!ret)
		return -1;
#endif // __linux__
	return 0;
}

//-----------------------------------------------------------------------------
int CFile::read(void *buf, size_t size)
{
#ifdef __linux__
	int res = ::read(m_hfile, buf, size);
	if (res < 0) {
		printf("ERROR: can not read %s\n", m_fname.c_str());
		return -1;
	}
#else
	unsigned long readsize;
	int ret = ReadFile(m_hfile, buf, (DWORD)size, &readsize, NULL);
	if (!ret)
	{
		printf("ERROR: can not read %s\n", m_fname.c_str());
		return -1;
	}
#endif // __linux__
	return 0;
}

int CFile::write(void *buf, size_t size)
{
#ifdef __linux__
	int res = ::write(m_hfile, buf, size);
	if (res < 0) {
		printf("ERROR: can not write %s\n", m_fname.c_str());
		return -1;
	}
#else
	unsigned long writesize;
	int ret = WriteFile(m_hfile, buf, (DWORD)size, &writesize, NULL);
	if (!ret)
	{
		printf("ERROR: can not write %s\n", m_fname.c_str());
		return -1;
	}
#endif // __linux__
	return 0;
}

int CFile::seek(size_t pos, int method)
{
	//fseek();
#ifdef __linux__
	//off_t offset = (off_t)pos;
	//off_t new_offset = lseek(m_hfile, offset, method);
	off_t new_offset = lseek(m_hfile, pos, method);
	if (new_offset < 0) {
		printf("ERROR: can not seek  %s\n", m_fname.c_str());
		return -1;
	}
#else
	// match (совпадают)
	//if (method == SEEK_SET)
	//	method = FILE_BEGIN;
	//else
	//	if (method == SEEK_CUR)
	//		method = FILE_CURRENT;
	//	else
	//		if (method == SEEK_END)
	//			method = FILE_END;

	long lo_offset = (long)pos;
	long hi_offset = (long)(pos >> 32);
	uint32_t lo_new_offset = SetFilePointer(m_hfile, lo_offset, &hi_offset, method);
	if (lo_new_offset == 0xFFFFFFFF) {
		printf("ERROR: can not seek  %s\n", m_fname.c_str());
		return -1;
	};
#endif // __linux__
	return 0;
}

int CFile::size(size_t* size)
{
#ifdef __linux__
	struct stat finfo;
	int res = fstat(m_hfile, &finfo);
	if (res < 0) {
		printf("ERROR: can not get size of file %s\n", m_fname.c_str());
		return -1;
	}
	*size = (size_t)finfo.st_size;
#else
	LARGE_INTEGER file_size;
	int ret = GetFileSizeEx(m_hfile, &file_size);
	if (!ret) {
		printf("ERROR: can not get size of file %s\n", m_fname.c_str());
		return -1;
	};
	*size = (size_t)file_size.QuadPart;
#endif // __linux__
	return 0;
}
