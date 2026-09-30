/* SPDX-License-Identifier: GPL-2.0 */
/*
 * sysmon_ioctl.h - Shared interface between SysMon kernel module and user-space
 *
 * This header defines the ioctl commands and data structures used for
 * communication between the SysMon character device driver and the
 * user-space monitoring application.
 *
 * It is included by both the kernel module (sysmon_driver.c) and the
 * C++ user-space application.
 */

#ifndef SYSMON_IOCTL_H
#define SYSMON_IOCTL_H

#ifdef __KERNEL__
#include <linux/types.h>
#include <linux/ioctl.h>
#else
#include <sys/ioctl.h>
#include <linux/types.h>
#include <stdint.h>
#endif

/* ========== Constants ========== */

#define SYSMON_MAGIC            'S'         /* ioctl magic number          */
#define SYSMON_DEVICE_NAME      "sysmon"    /* Device name in /dev         */
#define SYSMON_DEVICE_PATH      "/dev/sysmon"
#define SYSMON_CLASS_NAME       "sysmon_class"
#define SYSMON_DRIVER_VERSION   "1.0"

/* ========== Data Structures ========== */

/*
 * CPU time counters (in jiffies).
 * User space computes CPU usage % by taking two snapshots and
 * calculating: usage = (delta_busy / delta_total) * 100
 */
struct sysmon_cpu_info {
    __u64 user;         /* Time spent in user mode                */
    __u64 nice;         /* Time spent in user mode (low priority) */
    __u64 system;       /* Time spent in kernel mode              */
    __u64 idle;         /* Time spent idle                        */
    __u64 iowait;       /* Time waiting for I/O                   */
    __u64 irq;          /* Time servicing hardware interrupts     */
    __u64 softirq;      /* Time servicing software interrupts     */
    __u64 steal;        /* Time stolen by hypervisor              */
    __u64 total;        /* Sum of all CPU times                   */
    __u64 busy;         /* total - idle - iowait                  */
};

/*
 * Memory information (all values in kilobytes).
 */
struct sysmon_memory_info {
    __u64 total_ram;        /* Total physical RAM              */
    __u64 free_ram;         /* Free (unused) RAM               */
    __u64 available_ram;    /* Estimated available RAM          */
    __u64 used_ram;         /* Used RAM (total - available)     */
    __u64 buffers;          /* Buffer cache memory             */
    __u64 cached;           /* Page cache memory               */
    __u64 total_swap;       /* Total swap space                */
    __u64 free_swap;        /* Free swap space                 */
};

/*
 * Load average (fixed-point: value * 100).
 * Example: load of 1.42 is stored as 142.
 */
struct sysmon_load_info {
    __u64 load_1;       /* 1-minute load average * 100  */
    __u64 load_5;       /* 5-minute load average * 100  */
    __u64 load_15;      /* 15-minute load average * 100 */
};

/*
 * System uptime in seconds since boot.
 */
struct sysmon_uptime_info {
    __u64 uptime_seconds;
};

/*
 * Process count information.
 */
struct sysmon_proc_info {
    __u64 process_count;    /* Total number of processes  */
    __u64 running_count;    /* Number of running processes */
};

/*
 * Combined statistics (all of the above in one structure).
 */
struct sysmon_all_stats {
    struct sysmon_cpu_info      cpu;
    struct sysmon_memory_info   memory;
    struct sysmon_load_info     load;
    struct sysmon_uptime_info   uptime;
    struct sysmon_proc_info     proc;
};

/*
 * Driver metadata information.
 */
struct sysmon_driver_info {
    char  name[32];         /* Driver name                     */
    char  version[16];      /* Interface version               */
    __u32 major;            /* Major device number             */
    __u32 minor;            /* Minor device number             */
};

/* ========== ioctl Command Definitions ========== */

/* _IOR: data flows from kernel to user (read) */
#define SYSMON_GET_CPU          _IOR(SYSMON_MAGIC, 1, struct sysmon_cpu_info)
#define SYSMON_GET_MEMORY       _IOR(SYSMON_MAGIC, 2, struct sysmon_memory_info)
#define SYSMON_GET_PROC_COUNT   _IOR(SYSMON_MAGIC, 3, struct sysmon_proc_info)
#define SYSMON_GET_UPTIME       _IOR(SYSMON_MAGIC, 4, struct sysmon_uptime_info)
#define SYSMON_GET_LOAD         _IOR(SYSMON_MAGIC, 5, struct sysmon_load_info)
#define SYSMON_GET_ALL          _IOR(SYSMON_MAGIC, 6, struct sysmon_all_stats)
#define SYSMON_GET_DRIVER_INFO  _IOR(SYSMON_MAGIC, 7, struct sysmon_driver_info)

#endif /* SYSMON_IOCTL_H */
