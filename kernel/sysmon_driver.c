// SPDX-License-Identifier: GPL-2.0
/*
 * sysmon_driver.c - SysMon Linux Character Device Driver
 *
 * This kernel module creates a character device /dev/sysmon that provides
 * real-time system resource information to user-space applications.
 *
 * Supported operations:
 *   - read()  : Returns a text summary of all system stats
 *   - ioctl() : Returns structured data for specific resource categories
 *
 * The module collects data from:
 *   - Kernel CPU statistics (kcpustat)
 *   - Memory information (si_meminfo)
 *   - Process list (for_each_process)
 *   - Boot time (ktime_get_boottime_seconds)
 *   - Load averages (avenrun)
 *
 * Compatible with Ubuntu 22.04 (kernel 5.15+) and Ubuntu 24.04 (kernel 6.8+).
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/swap.h>
#include <linux/kernel_stat.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/sched/loadavg.h>
#include <linux/cpumask.h>
#include <linux/timekeeping.h>
#include <linux/version.h>
#include <linux/vmstat.h>

#include "sysmon_ioctl.h"

/* ========== Module Information ========== */

#define DRIVER_NAME    "sysmon"
#define DRIVER_VERSION "1.0"
#define READ_BUF_SIZE  2048

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SysMon Team");
MODULE_DESCRIPTION("Linux System Resource Monitor - Custom Character Device Driver");
MODULE_VERSION(DRIVER_VERSION);

/* ========== Device Variables ========== */

static dev_t          sysmon_dev_number;   /* Dynamically allocated dev_t      */
static struct cdev    sysmon_cdev;         /* Character device structure        */
static struct class  *sysmon_class;        /* Device class for udev/devfs      */
static struct device *sysmon_device;       /* Device instance                   */

/* Mutex to serialize access to device operations */
static DEFINE_MUTEX(sysmon_mutex);

/* Track number of open file descriptors (informational) */
static int open_count = 0;

/* ========== Forward Declarations ========== */

static int     sysmon_open(struct inode *inode, struct file *file);
static int     sysmon_release(struct inode *inode, struct file *file);
static ssize_t sysmon_read(struct file *file, char __user *buf,
                           size_t count, loff_t *offset);
static long    sysmon_ioctl(struct file *file, unsigned int cmd,
                            unsigned long arg);

/* ========== File Operations ========== */

static const struct file_operations sysmon_fops = {
    .owner          = THIS_MODULE,
    .open           = sysmon_open,
    .release        = sysmon_release,
    .read           = sysmon_read,
    .unlocked_ioctl = sysmon_ioctl,
};

/* ================================================================
 *                  DATA COLLECTION HELPERS
 * ================================================================ */

/*
 * collect_cpu_info() - Gather aggregate CPU time counters
 *
 * Iterates over all online CPUs and sums their time spent in each
 * state (user, system, idle, etc.). Values are in nanoseconds,
 * which are converted to jiffies for consistency.
 *
 * User-space computes CPU usage percentage by taking two snapshots
 * and computing: usage% = (delta_busy / delta_total) * 100
 */
static void collect_cpu_info(struct sysmon_cpu_info *info)
{
    int cpu;
    u64 user = 0, nice = 0, system = 0, idle = 0;
    u64 iowait = 0, irq = 0, softirq = 0, steal = 0;
    u64 total, busy;

    for_each_online_cpu(cpu) {
        struct kernel_cpustat ks = kcpustat_cpu(cpu);

        user    += ks.cpustat[CPUTIME_USER];
        nice    += ks.cpustat[CPUTIME_NICE];
        system  += ks.cpustat[CPUTIME_SYSTEM];
        idle    += ks.cpustat[CPUTIME_IDLE];
        iowait  += ks.cpustat[CPUTIME_IOWAIT];
        irq     += ks.cpustat[CPUTIME_IRQ];
        softirq += ks.cpustat[CPUTIME_SOFTIRQ];
        steal   += ks.cpustat[CPUTIME_STEAL];
    }

    /* Store values (kernel stores cputime in nanoseconds internally) */
    info->user    = nsecs_to_jiffies64(user);
    info->nice    = nsecs_to_jiffies64(nice);
    info->system  = nsecs_to_jiffies64(system);
    info->idle    = nsecs_to_jiffies64(idle);
    info->iowait  = nsecs_to_jiffies64(iowait);
    info->irq     = nsecs_to_jiffies64(irq);
    info->softirq = nsecs_to_jiffies64(softirq);
    info->steal   = nsecs_to_jiffies64(steal);

    /* Compute totals */
    total = info->user + info->nice + info->system + info->idle +
            info->iowait + info->irq + info->softirq + info->steal;
    busy  = total - info->idle - info->iowait;

    info->total = total;
    info->busy  = busy;
}

/*
 * collect_memory_info() - Gather system memory statistics
 *
 * Uses si_meminfo() to read physical RAM and swap information.
 * Computes cached memory from the page cache statistics.
 * All returned values are in kilobytes.
 */
static void collect_memory_info(struct sysmon_memory_info *info)
{
    struct sysinfo si;
    unsigned long cached_pages;

    si_meminfo(&si);

    /* si_meminfo returns values in pages; mem_unit = PAGE_SIZE */
    info->total_ram  = ((u64)si.totalram  * si.mem_unit) / 1024;
    info->free_ram   = ((u64)si.freeram   * si.mem_unit) / 1024;
    info->buffers    = ((u64)si.bufferram * si.mem_unit) / 1024;
    info->total_swap = ((u64)si.totalswap * si.mem_unit) / 1024;
    info->free_swap  = ((u64)si.freeswap  * si.mem_unit) / 1024;

    /* Cached = file-backed pages minus buffer pages */
    cached_pages = global_node_page_state(NR_FILE_PAGES);
    if (cached_pages > si.bufferram)
        info->cached = ((u64)(cached_pages - si.bufferram) * PAGE_SIZE) / 1024;
    else
        info->cached = 0;

    /* Available RAM ≈ free + buffers + cached (simplified estimate) */
    info->available_ram = info->free_ram + info->buffers + info->cached;
    if (info->available_ram > info->total_ram)
        info->available_ram = info->total_ram;

    /* Used = total - available */
    info->used_ram = info->total_ram - info->available_ram;
}

/*
 * collect_load_info() - Read system load averages
 *
 * The kernel maintains load averages in the avenrun[] array,
 * scaled by FIXED_1 (= 1 << FSHIFT = 2048).
 * We convert to fixed-point with 2 decimal places (* 100).
 */
static void collect_load_info(struct sysmon_load_info *info)
{
    /*
     * The kernel exports avenrun[] (load averages scaled by FIXED_1).
     * FIXED_1 = 1 << FSHIFT = 2048.
     * We convert to fixed-point with 2 decimal places (* 100).
     */
    info->load_1  = (avenrun[0] * 100) >> FSHIFT;
    info->load_5  = (avenrun[1] * 100) >> FSHIFT;
    info->load_15 = (avenrun[2] * 100) >> FSHIFT;
}

/*
 * collect_uptime_info() - Get system uptime
 *
 * Uses ktime_get_boottime_seconds() which returns the number of
 * seconds elapsed since the system was booted.
 */
static void collect_uptime_info(struct sysmon_uptime_info *info)
{
    info->uptime_seconds = (u64)ktime_get_boottime_seconds();
}

/*
 * collect_proc_info() - Count processes in the system
 *
 * Iterates the task list under RCU protection and counts:
 *   - Total number of processes (thread group leaders)
 *   - Number of processes in TASK_RUNNING state
 */
static void collect_proc_info(struct sysmon_proc_info *info)
{
    struct task_struct *task;
    u64 count   = 0;
    u64 running = 0;

    rcu_read_lock();
    for_each_process(task) {
        count++;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 14, 0)
        if (task_is_running(task))
#else
        if (task->state == TASK_RUNNING)
#endif
            running++;
    }
    rcu_read_unlock();

    info->process_count = count;
    info->running_count = running;
}

/* ================================================================
 *                   FILE OPERATION HANDLERS
 * ================================================================ */

/*
 * sysmon_open() - Called when user-space opens /dev/sysmon
 *
 * Logs the open event and increments the open counter.
 * Multiple simultaneous opens are allowed.
 */
static int sysmon_open(struct inode *inode, struct file *file)
{
    mutex_lock(&sysmon_mutex);
    open_count++;
    pr_info("sysmon: device opened (count: %d)\n", open_count);
    mutex_unlock(&sysmon_mutex);
    return 0;
}

/*
 * sysmon_release() - Called when user-space closes /dev/sysmon
 *
 * Logs the close event and decrements the open counter.
 */
static int sysmon_release(struct inode *inode, struct file *file)
{
    mutex_lock(&sysmon_mutex);
    if (open_count > 0)
        open_count--;
    pr_info("sysmon: device closed (count: %d)\n", open_count);
    mutex_unlock(&sysmon_mutex);
    return 0;
}

/*
 * sysmon_read() - Called when user-space reads from /dev/sysmon
 *
 * Generates a human-readable text summary of all system statistics.
 * Supports 'cat /dev/sysmon' for quick status checks.
 *
 * Returns data only once per open (subsequent reads return 0/EOF).
 */
static ssize_t sysmon_read(struct file *file, char __user *buf,
                           size_t count, loff_t *offset)
{
    char *kbuf;
    int len;
    struct sysmon_all_stats stats;

    /* Only produce output on the first read (offset == 0) */
    if (*offset > 0)
        return 0;

    /* Allocate kernel buffer for formatted output */
    kbuf = kmalloc(READ_BUF_SIZE, GFP_KERNEL);
    if (!kbuf) {
        pr_err("sysmon: failed to allocate read buffer\n");
        return -ENOMEM;
    }

    /* Collect all system statistics */
    mutex_lock(&sysmon_mutex);
    collect_cpu_info(&stats.cpu);
    collect_memory_info(&stats.memory);
    collect_load_info(&stats.load);
    collect_uptime_info(&stats.uptime);
    collect_proc_info(&stats.proc);
    mutex_unlock(&sysmon_mutex);

    /* Format the output as a readable text summary */
    len = snprintf(kbuf, READ_BUF_SIZE,
        "========================================\n"
        "       SysMon Driver Status\n"
        "========================================\n"
        "CPU Total Jiffies : %llu\n"
        "CPU Busy Jiffies  : %llu\n"
        "Total RAM         : %llu KB\n"
        "Used RAM          : %llu KB\n"
        "Available RAM     : %llu KB\n"
        "Free RAM          : %llu KB\n"
        "Buffers           : %llu KB\n"
        "Cached            : %llu KB\n"
        "Swap Total        : %llu KB\n"
        "Swap Free         : %llu KB\n"
        "Processes         : %llu\n"
        "Running           : %llu\n"
        "Uptime            : %llu seconds\n"
        "Load Average      : %llu.%02llu  %llu.%02llu  %llu.%02llu\n"
        "========================================\n",
        (unsigned long long)stats.cpu.total,
        (unsigned long long)stats.cpu.busy,
        (unsigned long long)stats.memory.total_ram,
        (unsigned long long)stats.memory.used_ram,
        (unsigned long long)stats.memory.available_ram,
        (unsigned long long)stats.memory.free_ram,
        (unsigned long long)stats.memory.buffers,
        (unsigned long long)stats.memory.cached,
        (unsigned long long)stats.memory.total_swap,
        (unsigned long long)stats.memory.free_swap,
        (unsigned long long)stats.proc.process_count,
        (unsigned long long)stats.proc.running_count,
        (unsigned long long)stats.uptime.uptime_seconds,
        (unsigned long long)(stats.load.load_1 / 100),
        (unsigned long long)(stats.load.load_1 % 100),
        (unsigned long long)(stats.load.load_5 / 100),
        (unsigned long long)(stats.load.load_5 % 100),
        (unsigned long long)(stats.load.load_15 / 100),
        (unsigned long long)(stats.load.load_15 % 100));

    /* Clamp output to requested size */
    if (len > (int)count)
        len = (int)count;

    /* Copy data from kernel buffer to user-space buffer */
    if (copy_to_user(buf, kbuf, len)) {
        kfree(kbuf);
        pr_err("sysmon: copy_to_user failed in read\n");
        return -EFAULT;
    }

    *offset += len;
    kfree(kbuf);
    return len;
}

/*
 * sysmon_ioctl() - Handle ioctl commands from user space
 *
 * Dispatches commands to the appropriate data collection function
 * and copies the result to the user-space buffer.
 *
 * Returns 0 on success, negative errno on failure.
 */
static long sysmon_ioctl(struct file *file, unsigned int cmd,
                         unsigned long arg)
{
    int ret = 0;
    void __user *user_ptr = (void __user *)arg;

    /* Validate the ioctl magic number */
    if (_IOC_TYPE(cmd) != SYSMON_MAGIC) {
        pr_warn("sysmon: invalid ioctl magic 0x%x\n", _IOC_TYPE(cmd));
        return -ENOTTY;
    }

    mutex_lock(&sysmon_mutex);

    switch (cmd) {

    case SYSMON_GET_CPU: {
        struct sysmon_cpu_info info;
        collect_cpu_info(&info);
        if (copy_to_user(user_ptr, &info, sizeof(info)))
            ret = -EFAULT;
        break;
    }

    case SYSMON_GET_MEMORY: {
        struct sysmon_memory_info info;
        collect_memory_info(&info);
        if (copy_to_user(user_ptr, &info, sizeof(info)))
            ret = -EFAULT;
        break;
    }

    case SYSMON_GET_PROC_COUNT: {
        struct sysmon_proc_info info;
        collect_proc_info(&info);
        if (copy_to_user(user_ptr, &info, sizeof(info)))
            ret = -EFAULT;
        break;
    }

    case SYSMON_GET_UPTIME: {
        struct sysmon_uptime_info info;
        collect_uptime_info(&info);
        if (copy_to_user(user_ptr, &info, sizeof(info)))
            ret = -EFAULT;
        break;
    }

    case SYSMON_GET_LOAD: {
        struct sysmon_load_info info;
        collect_load_info(&info);
        if (copy_to_user(user_ptr, &info, sizeof(info)))
            ret = -EFAULT;
        break;
    }

    case SYSMON_GET_ALL: {
        struct sysmon_all_stats stats;
        collect_cpu_info(&stats.cpu);
        collect_memory_info(&stats.memory);
        collect_load_info(&stats.load);
        collect_uptime_info(&stats.uptime);
        collect_proc_info(&stats.proc);
        if (copy_to_user(user_ptr, &stats, sizeof(stats)))
            ret = -EFAULT;
        break;
    }

    case SYSMON_GET_DRIVER_INFO: {
        struct sysmon_driver_info info;
        memset(&info, 0, sizeof(info));
        strscpy(info.name, DRIVER_NAME, sizeof(info.name));
        strscpy(info.version, DRIVER_VERSION, sizeof(info.version));
        info.major = MAJOR(sysmon_dev_number);
        info.minor = MINOR(sysmon_dev_number);
        if (copy_to_user(user_ptr, &info, sizeof(info)))
            ret = -EFAULT;
        break;
    }

    default:
        pr_warn("sysmon: unknown ioctl command 0x%x\n", cmd);
        ret = -ENOTTY;
        break;
    }

    mutex_unlock(&sysmon_mutex);

    if (ret == -EFAULT)
        pr_err("sysmon: copy_to_user failed for ioctl 0x%x\n", cmd);

    return ret;
}

/* ================================================================
 *                    MODULE INIT / EXIT
 * ================================================================ */

/*
 * sysmon_init() - Module initialization
 *
 * Registers the character device with dynamically allocated numbers,
 * creates a device class, and creates the /dev/sysmon device node.
 */
static int __init sysmon_init(void)
{
    int ret;

    pr_info("sysmon: initializing SysMon driver v%s\n", DRIVER_VERSION);

    /* Step 1: Allocate a major/minor number dynamically */
    ret = alloc_chrdev_region(&sysmon_dev_number, 0, 1, DRIVER_NAME);
    if (ret < 0) {
        pr_err("sysmon: failed to allocate device number (error %d)\n", ret);
        return ret;
    }
    pr_info("sysmon: allocated major=%d minor=%d\n",
            MAJOR(sysmon_dev_number), MINOR(sysmon_dev_number));

    /* Step 2: Initialize and add the character device */
    cdev_init(&sysmon_cdev, &sysmon_fops);
    sysmon_cdev.owner = THIS_MODULE;

    ret = cdev_add(&sysmon_cdev, sysmon_dev_number, 1);
    if (ret < 0) {
        pr_err("sysmon: failed to add cdev (error %d)\n", ret);
        goto err_cdev;
    }

    /* Step 3: Create device class (for automatic /dev node creation) */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
    sysmon_class = class_create(SYSMON_CLASS_NAME);
#else
    sysmon_class = class_create(THIS_MODULE, SYSMON_CLASS_NAME);
#endif
    if (IS_ERR(sysmon_class)) {
        ret = PTR_ERR(sysmon_class);
        pr_err("sysmon: failed to create device class (error %d)\n", ret);
        goto err_class;
    }

    /* Step 4: Create the device node /dev/sysmon */
    sysmon_device = device_create(sysmon_class, NULL, sysmon_dev_number,
                                  NULL, SYSMON_DEVICE_NAME);
    if (IS_ERR(sysmon_device)) {
        ret = PTR_ERR(sysmon_device);
        pr_err("sysmon: failed to create device (error %d)\n", ret);
        goto err_device;
    }

    pr_info("sysmon: driver loaded successfully (%s)\n", SYSMON_DEVICE_PATH);
    return 0;

/* Cleanup on error (reverse order) */
err_device:
    class_destroy(sysmon_class);
err_class:
    cdev_del(&sysmon_cdev);
err_cdev:
    unregister_chrdev_region(sysmon_dev_number, 1);
    return ret;
}

/*
 * sysmon_exit() - Module cleanup
 *
 * Removes the device node, destroys the class, removes the cdev,
 * and releases the allocated device numbers.
 */
static void __exit sysmon_exit(void)
{
    device_destroy(sysmon_class, sysmon_dev_number);
    class_destroy(sysmon_class);
    cdev_del(&sysmon_cdev);
    unregister_chrdev_region(sysmon_dev_number, 1);

    pr_info("sysmon: driver unloaded successfully\n");
}

module_init(sysmon_init);
module_exit(sysmon_exit);
