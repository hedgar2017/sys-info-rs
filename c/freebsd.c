#include "info.h"

static const char *os_type = "Linux";

/* External definitions */

// /proc/sys/kernel/ostype
const char *get_os_type(void) {
	return os_type;
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
    LoadAvg avg = {0};
    return avg;
}

unsigned long get_proc_total(void) {
    return 0;
}

// /proc/meminfo
MemInfo get_mem_info(void) {
    MemInfo info = {0};
    return info;
}

DiskInfo get_disk_info(void) {
	DiskInfo info = {0};
	return info;
}

double get_uptime(void) {
    return 0;
}