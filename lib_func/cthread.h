//=************************* CThread  - the thread class *************************

#ifndef _THREAD_H_
#define _THREAD_H_

#ifdef __linux__
#include <pthread.h>
#define INFINITE -1
#else
#include "windows.h"
#endif

class CThread
{

#ifdef __linux__
	pthread_t m_thread_id;
#else
	HANDLE m_hThread;
#endif

protected:

public:

#ifdef __linux__
	int create(void* (*function)(void *), void* param);
#else
	int create(unsigned int(*function)(void *), void* param);
#endif

	int wait(int timeout);
	int close();

	CThread();
	~CThread();

};

class CMutex
{

#ifdef __linux__
	pthread_mutex_t m_mutex;
#else
	HANDLE m_hMutex;
#endif

protected:

public:

	inline void lock();
	inline void unlock();
	inline void sync();

	CMutex()
	{
#ifdef __linux__
		pthread_mutex_init(&m_mutex, NULL);//Инициализация мьютекса
#else
		m_hMutex = CreateMutex(NULL, FALSE, NULL);
#endif
	}

	~CMutex()
	{
#ifdef __linux__
		pthread_mutex_destroy(&m_mutex); //Уничтожение мьютекса
#else
		CloseHandle(m_hMutex);
#endif
	}

};

//CMutex::CMutex()
//{
//#ifdef __linux__
//	pthread_mutex_init(&m_mutex, NULL);//Инициализация мьютекса
//#else
//	m_hMutex = CreateMutex(NULL, FALSE, NULL);
//#endif
//}
//
//CMutex::~CMutex()
//{
//#ifdef __linux__
//	pthread_mutex_destroy(&m_mutex); //Уничтожение мьютекса
//#else
//	CloseHandle(m_hMutex);
//#endif
//}

void CMutex::lock()
{
#ifdef __linux__
	pthread_mutex_lock(&m_mutex);
#else
	WaitForSingleObject(m_hMutex, INFINITE);
#endif
}

void CMutex::unlock()
{
#ifdef __linux__
	pthread_mutex_unlock(&m_mutex);
#else
	ReleaseMutex(m_hMutex);
#endif
}

void CMutex::sync()
{
#ifdef __linux__
	pthread_mutex_lock(&m_mutex);
	pthread_mutex_unlock(&m_mutex);
#else
	WaitForSingleObject(m_hMutex, INFINITE);
	ReleaseMutex(m_hMutex);
#endif
}

#endif	// _THREAD_H_
