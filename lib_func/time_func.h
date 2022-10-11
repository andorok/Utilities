
#ifndef _TIME_FUNC_H
#define _TIME_FUNC_H

#include <thread>
#include <chrono>

using ipc_time_t = std::chrono::time_point<std::chrono::high_resolution_clock>;

inline ipc_time_t ipc_get_time()
{
    return std::chrono::high_resolution_clock::now();
}

inline double ipc_get_difftime(ipc_time_t start, ipc_time_t end)
{
    return (double)std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
}

inline double ipc_get_nano_difftime(ipc_time_t start, ipc_time_t end)
{
	return (double)std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
}

inline void ipc_delay(int ms)
{
	//std::this_thread::sleep_for(std::chrono_literals::operator""ms(ms));
	std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

#endif // _TIME_FUNC_H
