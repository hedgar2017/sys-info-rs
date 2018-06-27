#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <kvm.h>
#include <fcntl.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <sys/user.h>
#include <sys/mount.h>

#include "info.h"

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
    size_t size = sizeof(value);
    if (sysctl(mib, sizeof(mib) / sizeof(mib[0]), value, &size, NULL, 0) == -1) {
        return errno;
    }
    return SUCCESS;
}

int get_cpu_speed(int *value) {
	size_t size = sizeof(*value);
	if (sysctlbyname("hw.clockrate", value, &size, NULL, 0) == -1) {
	    return errno;
	}
	return SUCCESS;
}

int get_cpu_load_average(LoadAverage *data) {
	double buf[3];
	int samples = getloadavg(buf, 3);
	if (-1 == samples) {
	    return samples;
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
	int mib[] = {CTL_HW, HW_PHYSMEM};
    unsigned long long total = 0;
    size_t size = sizeof(total);
    if (sysctl(mib, sizeof(mib) / sizeof(mib[0]), &total, &size, NULL, 0) == -1) {
        return errno;
    }
    data->total = total;

    int free = 0;
    size = sizeof(free);
    if (sysctlbyname("vm.stats.vm.v_free_count", &free, &size, NULL, 0) != -1) {
        data->free = ((unsigned long long) free) * getpagesize();
    }

    return SUCCESS;
}

int get_swap_info(SwapInfo *data) {
    struct kvm_swap swap[1];
    kvm_t *kd = kvm_open(NULL, NULL, NULL, O_RDONLY, NULL);
    if (NULL == kd) {
        return -2;
    }
    if (0 != kvm_getswapinfo(kd, swap, 1, 0)) {
        kvm_close(kd);
        return -1;
    }

    data->free = ((unsigned long long) (swap[0].ksw_total - swap[0].ksw_used)) * getpagesize();
    data->total = ((unsigned long long) swap[0].ksw_total) * getpagesize();

    kvm_close(kd);
    return SUCCESS;
}
