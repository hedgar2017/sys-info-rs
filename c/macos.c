#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/sysctl.h>
#include <mach/mach_init.h>
#include <mach/mach_host.h>
#include <sys/mount.h>

#include "info.h"

#define BUFLEN 1024

/* External definitions */

int get_os_type(char *buf, size_t size) {
	int mib[] = {CTL_KERN, KERN_OSTYPE};
    if (sysctl(mib, sizeof(mib) / sizeof(mib[0]), buf, &size, NULL, 0) == -1) {
        return errno;
    }
    return SUCCESS;
}

int get_os_release(char *buf, size_t size) {
	int mib[] = {CTL_KERN, KERN_OSRELEASE};
    if (sysctl(mib, sizeof(mib) / sizeof(mib[0]), buf, &size, NULL, 0) == -1) {
        return errno;
    }
    return SUCCESS;
}

int get_uptime(int *value) {
    int mib[] = {CTL_KERN, KERN_BOOTTIME};
	struct timeval boottime;
    size_t len = sizeof(boottime);
    if (sysctl(mib, sizeof(mib) / sizeof(mib[0]), &boottime, &len, NULL, 0) == -1) {
        return errno;
    }

    time_t secs_current = time(NULL);
    time_t secs_boot = ((double) boottime.tv_sec) + ((double) boottime.tv_usec) * 1E-6;
    *value = (int) difftime(secs_current, secs_boot);
    return SUCCESS;
}

int get_hostname(char *buf, size_t size) {
    if (0 != gethostname(buf, size)) {
        return errno;
    }
    return SUCCESS;
}

int get_cpu_core_count(int *value) {
    int mib[] = {CTL_HW, HW_NCPU};
	size_t size = sizeof(*value);
    if (sysctl(mib, sizeof(mib) / sizeof(mib[0]), value, &size, NULL, 0) == -1) {
        return errno;
    }
    return SUCCESS;
}

int get_cpu_speed(int *value) {
	unsigned long speed;
	size_t len;
	
	len = sizeof(speed);
	sysctlbyname("hw.cpufrequency", &speed, &len, NULL, 0);
	speed /= 1000000;

	*value = speed;

	return SUCCESS;
}

int get_cpu_load_average(LoadAverage *data) {
	double buf[3];
	int samples = getloadavg(buf, 3);
	if (-1 == samples) {
	    return errno;
	}

	data->one = (samples > 0 ? buf[0] : 0.0);
	data->five = (samples > 1 ? buf[1] : 0.0);
	data->fifteen = (samples > 2 ? buf[2] : 0.0);

	return SUCCESS;
}

int get_process_count(int *value) {
	int mib[] = {CTL_KERN, KERN_PROC, KERN_PROC_ALL};
	size_t len = 0;
    if (sysctl(mib, sizeof(mib) / sizeof(mib[0]), NULL, &len, NULL, 0) == -1) {
        return errno;
    }
	*value = len / sizeof(struct kinfo_proc);
	return SUCCESS;
}

int get_memory_info(MemoryInfo *data) {
	int mib[] = {CTL_HW, HW_MEMSIZE};
	unsigned long long total = 0;
	size_t len = sizeof(total);
    if (sysctl(mib, sizeof(mib) / sizeof(mib[0]), &total, &len, NULL, 0) == -1) {
        return errno;
    }

	vm_statistics_data_t vm_stat;
	mach_msg_type_number_t count = HOST_VM_INFO_COUNT;
	host_statistics(mach_host_self(), HOST_VM_INFO,	(host_info_t) &vm_stat, &count);

	data->free        = vm_stat.free_count * PAGE_SIZE;
	data->total       = total;

	return SUCCESS;
}

int get_swap_info(SwapInfo *data) {
    struct xsw_usage xsu;
    size_t len = sizeof xsu;

    if (sysctlbyname("vm.swapusage", &xsu, &len, NULL, 0) == -1) {
        return errno;
    }

    data->free = xsu.xsu_avail;
    data->total = xsu.xsu_total;

    return SUCCESS;
}