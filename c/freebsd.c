#include "info.h"

/* External definitions */

// /proc/sys/kernel/ostype
const char *get_os_type(void) {
	char *oldval = (char*) malloc(BUFLEN * sizeof(char));
    size_t oldval_size = BUFLEN;
    int name[] = {CTL_KERN, KERN_OSTYPE};

    struct __sysctl_args args;
    memset(&args, 0, sizeof(struct __sysctl_args));
    args.name = name;
    args.nlen = sizeof(name) / sizeof(name[0]);
    args.oldval = oldval;
    args.oldlenp = &oldval_size;

    if (syscall(SYS__sysctl, &args) == -1) {
        printf("%d\n", errno);
        return NULL;
    }
    return oldval;
}

// /proc/sys/kernel/osrelease
const char *get_os_release(void) {
	return "";
}

unsigned int get_cpu_num(void) {
	return 0;
}

// /sys/devices/system/cpu/cpu0
unsigned long get_cpu_speed(void) {
    return 0;
}

// /proc/loadavg
LoadAvg get_loadavg(void) {
    LoadAvg avg = {0.0, 0.0, 0.0};
    return avg;
}

unsigned long get_proc_total(void) {
    return 0;
}

// /proc/meminfo
MemInfo get_mem_info(void) {
    MemInfo info = {0, 0, 0, 0, 0, 0, 0};
    return info;
}

DiskInfo get_disk_info(void) {
	DiskInfo info = {0, 0};
	return info;
}

double get_uptime(void) {
    return 0.0;
}