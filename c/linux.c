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

/* Internal declarations */
typedef struct nlist {
	struct nlist *next;
	char *name;
} nlist;

#define BUFLEN 1024
#define DFHASHSIZE 101
#define UNUSED(x) (void)(x)

static struct nlist *DFhashvector[DFHASHSIZE];
unsigned int DFhash(const char*);
struct nlist *seen_before(const char*);
void DFcleanup(void);
int remote_mount(const char*, const char*);
float device_space(char*, char*, double*, double*);

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
	*value = get_nprocs();
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

int get_disk_info(DiskInfo *data) {
	char procline[1024];
	char *mount, *device, *type, *mode, *other;
	float thispct, max=0.0;
	double dtotal = 0.0, dfree = 0.0;

	FILE *mounts = fopen("/proc/mounts", "r");
	if (NULL == mounts) {
		return -1;
	}
	while ( fgets(procline, sizeof(procline), mounts) ) {
		device = procline;
		mount = index(procline, ' ');
		if (mount == NULL) continue;
		*mount++ = '\0';
		type = index(mount, ' ');
		if (type == NULL) continue;
		*type++ = '\0';
		mode = index(type, ' ');
		if (mode == NULL) continue;
		*mode++ = '\0';
		other = index(mode, ' ');
		if (other != NULL) *other = '\0';
		if (!strncmp(mode, "ro", 2)) continue;
		if (remote_mount(device, type)) continue;
		if (strncmp(device, "/dev/", 5) != 0 &&
		    strncmp(device, "/dev2/", 6) != 0) continue;
		thispct = device_space(mount, device, &dtotal, &dfree);
		if (!max || max<thispct)
			max = thispct;
	}
	fclose(mounts);
	DFcleanup();

	data->free = dfree;
	data->total = dtotal;

	return SUCCESS;
}

/* Internal definitions */

unsigned int DFhash(const char *s) {
	unsigned int hashval;
	for (hashval=0; *s != '\0'; s++) {
		hashval = *s + 31 * hashval;
	}
	return hashval % DFHASHSIZE;
}

/* From K&R C book, pp. 144-145 */
nlist *seen_before(const char *name) {
	struct nlist *found=0, *np;
	unsigned int hashval;

	/* lookup */
	hashval=DFhash(name);
	for (np=DFhashvector[hashval]; np; np=np->next) {
		if (!strcmp(name,np->name)) {
			found=np;
			break;
		}
	}
	if (!found) {
		np = (struct nlist *) malloc(sizeof(*np));
		if (!np || !(np->name = (char *) strdup(name))) {
		    return NULL;
		}
		np->next = DFhashvector[hashval];
		DFhashvector[hashval] = np;
		return NULL;
	}
    return found;
}

void DFcleanup() {
	nlist *np, *next;
	for (int i = 0; i < DFHASHSIZE; i++) {
		/* Non-standard for loop. Note the last clause happens at the end of the loop. */
		for (np = DFhashvector[i]; np; np=next) {
			next=np->next;
			free(np->name);
			free(np);
		}
		DFhashvector[i] = 0;
	}
}

int remote_mount(const char *device, const char *type) {
	/* From ME_REMOTE macro in mountlist.h:
	      A file system is `remote' if its Fs_name contains a `:'
	      or if (it is of type smbfs and its Fs_name starts with `//'). */
	return ((strchr(device,':') != 0)
		|| (!strcmp(type, "smbfs") && device[0]=='/' && device[1]=='/')
		|| (!strncmp(type, "nfs", 3)) || (!strcmp(type, "autofs"))
		|| (!strcmp(type,"gfs")) || (!strcmp(type,"none")) );
}

float device_space(char *mount, char *device, double *total_size, double *total_free) {
	struct statvfs svfs;
	double blocksize;
	double free;
	double size;
	/* The percent used: used/total * 100 */
	float pct = 0.0;

	/* Avoid multiply-mounted disks - not done in df. */
	if (seen_before(device)) return pct;

	if (statvfs(mount, &svfs)) {
		/* Ignore funky devices... */
		return pct;
	}

	free = svfs.f_bavail;
	size = svfs.f_blocks;
	blocksize = svfs.f_bsize;
	/* Keep running sum of total used, free local disk space. */
	*total_size += size * blocksize;
	*total_free += free * blocksize;
	/* The percentage of space used on this partition. */
	pct = size ? ((size - free) / (float) size) * 100 : 0.0;
	return pct;
}