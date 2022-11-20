
#ifndef _MEM_FUNC_H_
#define _MEM_FUNC_H_

#ifdef __linux__

#include <unistd.h> 
#include <stdio.h>

//! Функция выделяет страницы виртуальной памяти процесса
void* virtAlloc(size_t nSize)
{
	void *ptr = NULL;
	long pageSize = sysconf(_SC_PAGESIZE);
	int  res = posix_memalign(&ptr, pageSize, nSize);

	if ((res != 0) || !ptr)
		return NULL;

	return ptr;
}

//! Функция освобождает страницы виртуальной памяти процесса
void virtFree(void *ptr)
{
	free(ptr);
}

#else

#include "windows.h"

//! Функция выделяет страницы виртуальной памяти процесса
void* virtAlloc(size_t nSize)
{
	void* ptr = VirtualAlloc(NULL, nSize, MEM_COMMIT, PAGE_READWRITE);

	return ptr;
}

//! Функция освобождает страницы виртуальной памяти процесса
void virtFree(void *ptr)
{
	VirtualFree(ptr, 0, MEM_RELEASE);

}

#endif

#endif	// _MEM_FUNC_H_
