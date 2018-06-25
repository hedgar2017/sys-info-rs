#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <sys/syscall.h>
#include <sys/sysinfo.h>
#include <sys/statvfs.h>

#include "info.h"

#define BUFLEN 1024

/* External definitions */

int get_os_type(char *buf, size_t size) {
    struct utsname info;
    if (-1 == uname(&info)) {
        return errno;
    }
    strncpy(buf, info.sysname, size);
    return SUCCESS;
}

int get_os_release(char *buf, size_t size) {
    struct utsname info;
    if (-1 == uname(&info)) {
        return errno;
    }
    strncpy(buf, info.release, size);
    return SUCCESS;
}

int get_uptime(int *value) {
    struct sysinfo info;
    if (sysinfo(&info) == -1) {
        return errno;
    }

    *value = info.uptime;

    return SUCCESS;
}

int get_hostname(char *buf, size_t size) {
    if (-1 == gethostname(buf, size)) {
        return errno;
    }
    return SUCCESS;
}

int get_cpu_core_count(int *value) {
#ifdef __ANDROID__
    *value = sysconf(_SC_NPROCESSORS_ONLN);
#else
    *value = get_nprocs();
#endif
	return SUCCESS;
}

int get_cpu_speed(int *value) {
	FILE *file = fopen("/proc/cpuinfo", "r");
	if (NULL == file) {
		return errno;
	}

	char buf[BUFLEN];
	const char *pattern = "cpu MHz";
	while (NULL != fgets(buf, BUFLEN, file)) {
	    if (0 != strncmp(buf, pattern, sizeof(pattern)-1)) {
	        continue;
	    }
        char *ptr = buf;
        while (':' != *ptr) {
            ++ptr;
            if ('\0' == *ptr) return -1;
        }
        ++ptr;
        *value = atoi(ptr);
        return (*value > 0 ? SUCCESS : -1);
	}

    return -1;
}

int get_cpu_load_average(LoadAverage *data) {
    struct sysinfo info;
    if (sysinfo(&info) == -1) {
        return errno;
    }

    data->one = ((double) info.loads[0]) / ((double) (1 << SI_LOAD_SHIFT));
    data->five = ((double) info.loads[1]) / ((double) (1 << SI_LOAD_SHIFT));
    data->fifteen = ((double) info.loads[2]) / ((double) (1 << SI_LOAD_SHIFT));

    return SUCCESS;
}

int get_process_count(int *value) {
    struct sysinfo info;
    if (sysinfo(&info) == -1) {
        return errno;
    }

    *value = info.procs;

    return SUCCESS;
}

int get_memory_info(MemoryInfo *data) {
    struct sysinfo info;
    if (sysinfo(&info) == -1) {
        return errno;
    }

    data->free = info.freeram;
    data->total = info.totalram;

    return SUCCESS;
}

int get_swap_info(SwapInfo *data) {
    struct sysinfo info;
    if (sysinfo(&info) == -1) {
        return errno;
    }

    data->free = info.freeswap;
    data->total = info.totalswap;

    return SUCCESS;
}