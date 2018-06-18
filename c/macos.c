#include <stdlib.h>
#include <string.h>
#include <sys/sysctl.h>
#include <mach/mach_init.h>
#include <mach/mach_host.h>
#include <sys/mount.h>

#include "info.h"

#define LEN 20
#define MNT_IGNORE 0

/* Internal declarations */
static size_t regetmntinfo(struct statfs **mntbufp, long mntsize, const char **vfslist);
static const char **makevfslist(char *fslist);
static int checkvfsname(const char *vfsname, const char **vfslist);
static char *makenetvfslist(void);

static int skipvfs;

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
    return SUCCESS;
}

int get_cpu_core_count(int *value) {
    int mib[] = {CTL_HW, HW_NCPU};
	size_t len = sizeof(*value);
    if (sysctl(mib, sizeof(mib) / sizeof(mib[0]), value, &size, NULL, 0) == -1) {
        return errno;
    }
    return SUCCESS;
}

int get_cpu_speed(int *value) {
	size_t len = sizeof(*value);
	sysctlbyname("hw.cpufrequency", value, &len, NULL, 0);
	*value /= 1E6;
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
	static unsigned long long size = 0;
	size_t len;
	int mib[2];
	vm_statistics_data_t vm_stat;
	mach_msg_type_number_t count = HOST_VM_INFO_COUNT;

	if (size == 0) {
		mib[0] = CTL_HW;
		mib[1] = HW_MEMSIZE;
		len = sizeof(size);
		sysctl(mib, 2, &size, &len, NULL, 0);
		size /= 1024;
	}
	
	host_statistics(mach_host_self(), HOST_VM_INFO,
				(host_info_t)&vm_stat, &count);

	data->free        = vm_stat.free_count * PAGE_SIZE / 1024;
	data->total       = size;

	return SUCCESS;
}

int get_swap_info(SwapInfo *data) {
    return SUCCESS;
}

int get_disk_info(DiskInfo *data) {
	struct statfs *mntbuf;
	const char **vfslist;
	char *str;
	size_t i, mntsize;
	size_t used, availblks;
	const double reported_units = 1e9;
	float pct;
	float most_full = 0.0;
	double toru, dtotal = 0.0, dfree = 0.0;

	str = makenetvfslist();
	vfslist = makevfslist(str);
	free(str);

	mntsize = getmntinfo(&mntbuf, MNT_NOWAIT);
	mntsize = regetmntinfo(&mntbuf, mntsize, vfslist);

	for (i = 0; i < mntsize; i++) {
		if ((mntbuf[i].f_flags & MNT_IGNORE) == 0) {
			used = mntbuf[i].f_blocks - mntbuf[i].f_bfree;
			availblks = mntbuf[i].f_bavail + used;
			pct = (availblks == 0 ? 100.0 :
			       (double)used / (double)availblks * 100.0);
			if (pct > most_full)
				most_full = pct;
			toru = reported_units / mntbuf[i].f_bsize;
			dtotal += mntbuf[i].f_blocks / toru;
			dfree += mntbuf[i].f_bavail / toru;
		}
	}

	free(vfslist);
	data->total = dtotal * 1000000;
	data->free = dfree * 1000000;
	return SUCCESS;
}

/* Internal definitions */

const char **makevfslist(char *fslist) {
	const char **av;
	int i;
	char *nextcp;

	if (fslist == NULL)
		return (NULL);
	if (fslist[0] == 'n' && fslist[1] == 'o') {
		fslist += 2;
		skipvfs = 1;
	}
	for (i = 0, nextcp = fslist; *nextcp; nextcp++)
		if (*nextcp == ',')
			i++;
	if ((av = (const char**)malloc((size_t)(i + 2) * sizeof(char *))) == NULL) {
		return (NULL);
	}
	nextcp = fslist;
	i = 0;
	av[i++] = nextcp;
	while ((nextcp = strchr(nextcp, ',')) != NULL) {
		*nextcp++ = '\0';
		av[i++] = nextcp;
	}
	av[i++] = NULL;

	return (av);

}

size_t regetmntinfo(struct statfs **mntbufp, long mntsize,
		    const char **vfslist) {
	int i, j;
	struct statfs *mntbuf;

	if (vfslist == NULL)
		return (getmntinfo(mntbufp, MNT_WAIT));

	mntbuf = *mntbufp;
	for (j = 0, i = 0; i < mntsize; i++) {
		if (checkvfsname(mntbuf[i].f_fstypename, vfslist))
			continue;
		(void)statfs(mntbuf[i].f_mntonname,&mntbuf[j]);
		j++;
	}
	return (j);
}

int checkvfsname(const char *vfsname, const char **vfslist) {

	if (vfslist == NULL)
		return (0);
	while (*vfslist != NULL) {
		if (strcmp(vfsname, *vfslist) == 0)
			return (skipvfs);
		++vfslist;
	}
	return (!skipvfs);
}

char *makenetvfslist(void){
	char *str, *strptr, **listptr;
	int mib[4], maxvfsconf, cnt=0, i;
	size_t miblen;
	struct vfsconf vfc;

	mib[0] = CTL_VFS;
	mib[1] = VFS_GENERIC;
	mib[2] = VFS_MAXTYPENUM;
	miblen=sizeof(maxvfsconf);
	if (sysctl(mib, 3, &maxvfsconf, &miblen, NULL, 0)) {
		return (NULL);
	}

	if ((listptr = (char**)malloc(sizeof(char*)*maxvfsconf)) == NULL) {
		return (NULL);
	}

	miblen = sizeof (struct vfsconf);
	mib[2] = VFS_CONF;
	for (i = 0; i < maxvfsconf; i++) {
		mib[3] = i;
		if (sysctl(mib, 4, &vfc, &miblen, NULL, 0) == 0) {
			if (!(vfc.vfc_flags & MNT_LOCAL)) {
				listptr[cnt++] = strdup(vfc.vfc_name);
				if (listptr[cnt-1] == NULL) {
					return (NULL);
				}
			}
		}
	}

	if (cnt == 0 ||
	    (str = (char*)malloc(sizeof(char) * (32 * cnt + cnt + 2))) == NULL) {
		free(listptr);
		return (NULL);
	}

	*str = 'n';
	*(str + 1) = 'o';
	for (i = 0, strptr = str + 2; i < cnt; i++, strptr++) {
		strncpy(strptr, listptr[i], 32);
		strptr += strlen(listptr[i]);
		*strptr = ',';
		free(listptr[i]);
	}
	*(--strptr) = NULL;
	free(listptr);
	return (str);
}