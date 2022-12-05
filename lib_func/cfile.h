//=************************* CFile  - the thread class *************************

#ifndef _CFILE_H_
#define _CFILE_H_

#ifdef __linux__
#include <unistd.h> 
#include <sys/stat.h>
#include <fcntl.h>
#define MAX_PATH 260
#else
#include "windows.h"
#endif

#include	<string>
using namespace std;

//----------------------------------------------------------------------
// ����� ��� ������ � ��������� ���������
//----------------------------------------------------------------------

enum FILE_flags {

	FLG_FILE_CREATE = 0x1,     //!< ������ ������� ����
	FLG_FILE_OPEN   = 0x2,     //!< ��������� ����  (���� �� ����������, �� ���������)
	FLG_FILE_RDONLY = 0x10,    //!< ��������� ���� � ������ ������ ��� ������ (���� �� ����������, �� ������)
	FLG_FILE_WRONLY = 0x20,    //!< ��������� ���� � ������ ������ ��� ������ (���� �� ����������, �� ���������)
	FLG_FILE_RDWR   = 0x40,    //!< ��������� ���� � ������ ������/������ (���� �� ����������, �� ���������)
	FLG_FILE_DIRECT = 0x80     //!< ��������� ���� � ������ ������� ������/������ (Linux)
};

enum FILE_posMethod {

	POS_FILE_BEG = 0,    //!< ������� ����� � ������ �����
	POS_FILE_CUR = 1,    //!< ������� ����� � ������� �������
	POS_FILE_END = 2     //!< ������� ����� �� ����� �����
};

//enum FILE_Attribute {
//
//	ATT_FILE_NORMAL = 0,    //!< for WINDOWS - FILE_ATTRIBUTE_NORMAL
//	ATT_FILE_NOBUFFER = 1,    //!< for WINDOWS - FILE_FLAG_NO_BUFFERING
//	ATT_FILE_WRTHROUGH = 2    //!< for WINDOWS - FILE_FLAG_WRITE_THROUGH
//};

class CFile
{

#ifdef __linux__
	int m_hfile;
#else
	HANDLE m_hfile;
#endif

	string m_fname;

public:

	int open(const char* name, int flags);
	int read(void *buf, size_t size);
	int write(void *buf, size_t size);
	int close();

	int seek(size_t pos, int method);
	int size(size_t* size);

	CFile();
	~CFile();
};

#endif	// _CFILE_H_
