#ifndef INFO_H_
#define INFO_H_

#include <stdlib.h>

typedef struct LoadAverage {
    double one;
    double five;
    double fifteen;
} LoadAverage;

typedef struct MemoryInfo {
    unsigned long long free;
    unsigned long long total;
} MemoryInfo;

typedef struct SwapInfo {
    unsigned long long free;
    unsigned long long total;
} SwapInfo;

typedef struct DiskInfo {
    unsigned long long free;
    unsigned long long total;
} DiskInfo;

#define SUCCESS 0

int                 get_os_type(char *buf, size_t size);
int                 get_os_release(char *buf, size_t size);

int                 get_uptime(int *value);
int                 get_hostname(char *buf, size_t size);

int                 get_cpu_core_count(int *value);
int                 get_cpu_speed(int *value);
int                 get_cpu_load_average(LoadAverage *data);
int                 get_process_count(int *value);

int                 get_memory_info(MemoryInfo *data);
int                 get_swap_info(SwapInfo *data);
int                 get_disk_info(DiskInfo *data);

#endif