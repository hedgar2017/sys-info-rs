#include <Windows.h>
#include <stdio.h>
#include <psapi.h>
#include <intrin.h>

#include "info.h"

#pragma intrinsic(__rdtsc)

/* Internal declarations */

static double calculate_cpu_load(unsigned long long, unsigned long long);
static unsigned long long file_time_to_ull(const FILETIME);

/* External definitions */

int get_os_type(char *buf, size_t size) {
    snprintf(buf, size, "Windows");
	return SUCCESS;
}

int get_os_release(char *buf, size_t size) {
	OSVERSIONINFO version_info;
	ZeroMemory(&version_info, sizeof(version_info));
	version_info.dwOSVersionInfoSize = sizeof(version_info);

	if (!GetVersionEx(&version_info)) {
        return GetLastError();
    }
    snprintf(buf, size, "%d.%d", version_info.dwMajorVersion, version_info.dwMinorVersion);
	return SUCCESS;
}

int get_uptime(int *value) {
    *value = (int) (GetTickCount64() * 1E-3);
    return SUCCESS;
}

int get_hostname(char *buf, size_t size) {
    if (!GetComputerNameEx(ComputerNameDnsHostname, buf, &size)) {
        return GetLastError();
    }
    return SUCCESS;
}

int get_cpu_core_count(int *value) {
	SYSTEM_INFO sys_info;
	GetSystemInfo(&sys_info);
	*value = sys_info.dwNumberOfProcessors;
	return SUCCESS;
}

int get_cpu_speed(int *value) {
	LARGE_INTEGER qw_wait, qw_start, qw_current;
	QueryPerformanceCounter(&qw_start);
	QueryPerformanceFrequency(&qw_wait);
	qw_wait.QuadPart >>= 5;

	unsigned __int64 start = __rdtsc();
	do {
		QueryPerformanceCounter(&qw_current);
	} while (qw_current.QuadPart - qw_start.QuadPart < qw_wait.QuadPart);
	*value = ((__rdtsc() - start) << 5) / 1E6;
	return SUCCESS;
}

int get_cpu_load_average(LoadAverage *data) {
	FILETIME idle_time, kernel_time, user_time;
	double load =  GetSystemTimes(&idle_time, &kernel_time, &user_time) ?
		calculate_cpu_load(file_time_to_ull(idle_time), file_time_to_ull(kernel_time) + file_time_to_ull(user_time)) :
		-1.0;
	
	data->one = load;
	data->five = load;
	data->fifteen = load;

	return SUCCESS;
}

int get_process_count(int *value) {
	DWORD buf[1024];
	DWORD size = 0;
	if (!EnumProcesses(buf, sizeof(buf), &size)) {
	    return GetLastError();
	}
	*value = size / sizeof(int);
	return SUCCESS;
}

int get_memory_info(MemoryInfo *data) {
	MEMORYSTATUSEX stat;
	stat.dwLength = sizeof(stat);
	if (!GlobalMemoryStatusEx(&stat)) {
	    return GetLastError();
	}

    data->free  = stat.ullAvailPhys;
    data->total = stat.ullTotalPhys;

	return SUCCESS;
}

int get_swap_info(SwapInfo *data) {
	MEMORYSTATUSEX stat;
	stat.dwLength = sizeof(stat);
	if (!GlobalMemoryStatusEx(&stat)) {
	    return GetLastError();
	}

    data->free = stat.ullAvailPageFile;
    data->total = stat.ullTotalPageFile;

	return SUCCESS;
}

/* Internal definitions */

double calculate_cpu_load(unsigned long long idle_ticks, unsigned long long total_ticks) {
	static unsigned long long prev_total_ticks = 0;
	static unsigned long long prev_idle_ticks = 0;

	unsigned long long total_ticks_since_last_time = total_ticks - prev_total_ticks;
	unsigned long long idle_ticks_since_last_time = idle_ticks - prev_idle_ticks;

	prev_total_ticks = total_ticks;
	prev_idle_ticks = idle_ticks;
	return 1.0 - ((total_ticks_since_last_time > 0) ? ((double) idle_ticks_since_last_time) / total_ticks_since_last_time : 0);
}

unsigned long long file_time_to_ull(const FILETIME ft) {
	return (((unsigned long long) ft.dwHighDateTime) << 32) | ((unsigned long long) ft.dwLowDateTime);
}