
#ifndef _CPU_FUNC_H_
#define _CPU_FUNC_H_

#ifdef __linux__
#include <pthread.h>
#else
#include <stdint.h>
#endif // __linux__

#ifdef __linux__
//! Привязка потока к указанным процессорам (CPU)
void SetAffinityCPU(uint32_t cpu_num)
{
	// Set affinity mask to include CPU
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(cpu_num, &cpuset);
	pthread_t current_thread = pthread_self();
	pthread_setaffinity_np(current_thread, sizeof(cpuset), &cpuset);
}

int32_t GetAffinityCPU()
{
	cpu_set_t cpuset;
	pthread_t current_thread = pthread_self();
	// Check the actual affinity mask assigned to the thread.
	pthread_getaffinity_np(current_thread, sizeof(cpuset), &cpuset);
	//printf("Set returned by pthread_getaffinity_np() contained:\n");
	for (uint32_t icpu = 0; icpu < CPU_SETSIZE; icpu++)
		if (CPU_ISSET(icpu, &cpuset))
			return icpu;
		//	printf("    CPU %d\n", icpu);
	return -1;
}

#else
void SetAffinityCPU(uint32_t cpu_num)
{
	HANDLE hCurThread = GetCurrentThread();
	//uint32_t cpu_mask = 0x200; // 8-й
	uint32_t cpu_mask = 1 << cpu_num;
	SetThreadAffinityMask(hCurThread, cpu_mask);
}

void SetSoftAffinityCPU(uint32_t cpu_num)
{
	HANDLE hCurThread = GetCurrentThread();
	SetThreadIdealProcessor(hCurThread, cpu_num);
}
#endif

#endif	// _CPU_FUNC_H_
