# SysMon — Low-Level Design (LLD) Document

**Project:** Linux System Resource Monitor Using a Custom Character Device Driver  
**Version:** 1.0  
**Authors:** SysMon Engineering Team  
**Target Platform:** Linux 5.15+ / 6.8+ (x86_64, Ubuntu 22.04 / 24.04 LTS)

---

## 1. Introduction

This document details the low-level architectural and design specifications for **SysMon**. SysMon is an enterprise-grade academic capstone project providing real-time kernel-space instrumentation and system resource telemetry via a dedicated character device (`/dev/sysmon`) to a modern user-space C++17 dashboard.

---

## 2. Kernel Module Design (`kernel/`)

### 2.1 Character Device Architecture

The character device driver interacts with the VFS (Virtual Filesystem Switch) using the `file_operations` structure.

```c
static const struct file_operations sysmon_fops = {
    .owner          = THIS_MODULE,
    .open           = sysmon_open,
    .release        = sysmon_release,
    .read           = sysmon_read,
    .unlocked_ioctl = sysmon_ioctl,
};
```

#### Major and Minor Number Allocation
- **Dynamic Allocation:** Utilizes `alloc_chrdev_region(&sysmon_dev_number, 0, 1, "sysmon")` to avoid static device number conflicts.
- **cdev Binding:** `cdev_init(&sysmon_cdev, &sysmon_fops)` links operations, followed by `cdev_add()`.
- **Automatic Device Node Creation:** `class_create()` followed by `device_create()` signals devtmpfs/udev to dynamically create `/dev/sysmon`.

### 2.2 Kernel Telemetry Subsystems

The kernel driver accesses internal kernel subsystems without procfs/sysfs overhead:

| Metric Category | Kernel Source | Implementation Mechanism |
| :--- | :--- | :--- |
| **CPU Time** | `kcpustat_cpu(cpu)` | Iterates online CPUs using `for_each_online_cpu(cpu)` and aggregates nanoseconds converted via `nsecs_to_jiffies64()`. |
| **RAM & Swap** | `si_meminfo(&si)` | Multiplies page units by `mem_unit` to yield exact KB. Uses `global_node_page_state(NR_FILE_PAGES)` for accurate buffer/cache accounting. |
| **Process Count** | `for_each_process(task)` | Iterates task tree under `rcu_read_lock()` / `rcu_read_unlock()`. Inspects `task_is_running(task)` to count active threads. |
| **System Uptime** | `ktime_get_boottime_seconds()` | Queries monotonic boot timer including system suspension periods. |
| **Load Averages** | `avenrun[]` | Reads kernel exported fixed-point load averages directly, shifting right by `FSHIFT` (11 bits) and scaling to percentage integers. |

### 2.3 Synchronization & Concurrency

- **Device Mutex (`sysmon_mutex`):** A kernel `mutex` protects shared state, serializes ioctl requests, and prevents race conditions during module read/ioctl operations.
- **RCU (Read-Copy Update):** Process iteration relies on RCU primitives (`rcu_read_lock()` and `rcu_read_unlock()`) to safely traverse task structs without stalling the kernel scheduler or locking all task lists.
- **User Memory Copy Safety:** All data transfers out of kernel space are executed through `copy_to_user()`. Returns `-EFAULT` if user buffer pointers are invalid.

---

## 3. Shared ioctl Protocol (`kernel/sysmon_ioctl.h`)

### 3.1 Command Encoding

Commands are encoded with the kernel standard `_IOR` macro using magic byte `'S'`:

```c
#define SYSMON_MAGIC 'S'

#define SYSMON_GET_CPU          _IOR(SYSMON_MAGIC, 1, struct sysmon_cpu_info)
#define SYSMON_GET_MEMORY       _IOR(SYSMON_MAGIC, 2, struct sysmon_memory_info)
#define SYSMON_GET_PROC_COUNT   _IOR(SYSMON_MAGIC, 3, struct sysmon_proc_info)
#define SYSMON_GET_UPTIME       _IOR(SYSMON_MAGIC, 4, struct sysmon_uptime_info)
#define SYSMON_GET_LOAD         _IOR(SYSMON_MAGIC, 5, struct sysmon_load_info)
#define SYSMON_GET_ALL          _IOR(SYSMON_MAGIC, 6, struct sysmon_all_stats)
#define SYSMON_GET_DRIVER_INFO  _IOR(SYSMON_MAGIC, 7, struct sysmon_driver_info)
```

### 3.2 Fixed-Width Layout

All struct fields use explicit 64-bit (`__u64`) and 32-bit (`__u32`) fixed-width integers to ensure identical byte alignment across 32-bit and 64-bit ABIs without compiler packing padding discrepancies.

---

## 4. User-Space C++ Application Design (`user/`)

### 4.1 Class Hierarchy and Responsibilities

```
                   +------------------------+
                   |       main.cpp         |
                   | (CLI & Signal Manager) |
                   +-----------+------------+
                               |
              +----------------+----------------+
              |                                 |
              v                                 v
   +--------------------+             +--------------------+
   |   SystemMonitor    |             |   DisplayManager   |
   | (IOCTL Controller) |             | (ANSI Visualization|
   +----------+---------+             +---------+----------+
              |                                 |
              +----------------+----------------+
                               |
                               v
                      +-----------------+
                      | utils namespace |
                      | (Formatters)    |
                      +-----------------+
```

### 4.2 Module Descriptions

#### 1. `SystemMonitor` (`include/monitor.hpp`, `src/monitor.cpp`)
- Encapsulates file descriptor management for `/dev/sysmon`.
- RAII-compliant constructor opens device; destructor automatically closes file descriptor.
- Computes CPU delta usage percentages across consecutive polling intervals:
  $$\text{Usage} = \left(\frac{\Delta \text{busy}}{\Delta \text{total}}\right) \times 100$$
- Implements both granular methods (`get_cpu_info()`, `get_memory_info()`, etc.) and batch method (`get_all_stats()`).

#### 2. `DisplayManager` (`include/display.hpp`, `src/display.cpp`)
- Renders rich ASCII/ANSI dashboard.
- Uses color-coded threshold progress bars:
  - Green: `< 60%`
  - Yellow: `60% - 85%`
  - Red: `> 85%`
- Supports single-shot dump (`--once`), continuous live update, and categorized subviews (`--cpu`, `--mem`, `--proc`).

#### 3. `utils` (`include/utils.hpp`, `src/utils.cpp`)
- Formats kilobytes into human-readable units (KB, MB, GB).
- Formats seconds into human-readable uptime string (`Xd Xh Xm Xs`).
- Formats load averages into floating point representations.
- Handles terminal cursor repositioning and screen clearing.

---

## 5. Error Handling & Fault Tolerance

1. **Missing Driver:** If `/dev/sysmon` is absent, the C++ client gracefully informs the user with instructions to execute `sudo make load`.
2. **Permission Denied:** Returns a clear error if user does not have read/write access to `/dev/sysmon`.
3. **Invalid Command:** Kernel driver logs warning via `pr_warn` and returns standard UNIX `-ENOTTY`.
4. **Invalid Memory Pointer:** Kernel driver detects failed `copy_to_user` and aborts with `-EFAULT` without crashing the kernel.
5. **Signal Handling:** User-space client catches `SIGINT` (Ctrl+C) and `SIGTERM`, resetting cursor visibility and terminating cleanly without terminal state corruption.
