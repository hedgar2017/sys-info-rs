#include "info.h"

#include <stdio.h>

#define BUFSIZE 256

int main(void) {
    char buf[BUFSIZE];
    int i_value = 0;
    LoadAverage load = {.one = 0.0, .five = 0.0, .fifteen = 0.0};
    MemoryInfo memory = {.free = 0, .total = 0};
    SwapInfo swap = {.free = 0, .total = 0};
    DiskInfo disk = {.free = 0, .total = 0};

    get_os_type(buf, BUFSIZE);
	printf("OS type           : %s\n", buf);
    get_os_release(buf, BUFSIZE);
	printf("OS release        : %s\n\n", buf);

    i_value = 0; get_uptime(&i_value);
	printf("System uptime     : %d seconds\n", i_value);
	get_hostname(buf, BUFSIZE);
	printf("Hostname          : %s\n\n", buf);

    i_value = 0; get_cpu_core_count(&i_value);
	printf("CPU cores count   : %d\n", i_value);
    i_value = 0; get_cpu_speed(&i_value);
    printf("CPU frequency     : %d MHz\n", i_value);
    get_cpu_load_average(&load);
	printf("CPU load average  : %f %f %f\n", load.one, load.five, load.fifteen);
    i_value = 0; get_process_count(&i_value);
    printf("Process count     : %d\n\n", i_value);

    get_memory_info(&memory);
    printf("Memory info       : free %llu bytes, total %llu bytes\n", memory.free, memory.total);
    get_swap_info(&swap);
    printf("Swap info         : free %llu bytes, total %llu bytes\n", swap.free, swap.total);
    get_disk_info(&disk);
    printf("Disk info         : free %llu bytes, total %llu bytes\n", disk.free, disk.total);

	return EXIT_SUCCESS;
}