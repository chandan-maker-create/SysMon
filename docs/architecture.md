# SysMon — Architecture Document

**Project:** Linux System Resource Monitor Using a Custom Character Device Driver  
**Version:** 1.0

---

## 1. Overview

SysMon is a two-component system:

1. **Kernel-Space Component** — A Linux loadable kernel module (LKM) that collects system resource data and exposes it through a character device
2. **User-Space Component** — A C++17 CLI application that communicates with the kernel module and presents data to the user

The two components communicate through the `/dev/sysmon` character device using the `ioctl()` and `read()` system calls.

---

## 2. Layer Architecture

```
┌─────────────────────────────────────────────────────────┐
│                  PRESENTATION LAYER                      │
│           display.cpp — ANSI CLI Output                  │
│    (color coding, formatting, menu, continuous mode)     │
└────────────────────────┬────────────────────────────────┘
                         │
┌────────────────────────▼────────────────────────────────┐
│               DATA PROCESSING LAYER                      │
│       utils.cpp — Formatting & Conversion                │
│  (KB→GB, seconds→uptime, load fixed-point→decimal)       │
└────────────────────────┬────────────────────────────────┘
                         │
┌────────────────────────▼────────────────────────────────┐
│           USER-SPACE COMMUNICATION LAYER                 │
│       monitor.cpp — SystemMonitor Class                  │
│  open("/dev/sysmon") → ioctl() → close()                 │
└────────────────────────┬────────────────────────────────┘
                         │  System Call Boundary
                    ioctl() / read()
                         │
┌────────────────────────▼────────────────────────────────┐
│            DEVICE DRIVER INTERFACE LAYER                 │
│           /dev/sysmon Character Device                   │
│    sysmon_fops: open, release, read, unlocked_ioctl      │
└────────────────────────┬────────────────────────────────┘
                         │
┌────────────────────────▼────────────────────────────────┐
│              KERNEL MODULE LAYER                         │
│           sysmon_driver.c                                │
│                                                          │
│  ┌───────────┐  ┌──────────────┐  ┌──────────────────┐  │
│  │ CPU Stats │  │ Memory Stats │  │  Process Stats   │  │
│  │kcpustat() │  │ si_meminfo() │  │ for_each_process │  │
│  └───────────┘  └──────────────┘  └──────────────────┘  │
│                                                          │
│  ┌───────────┐  ┌──────────────┐                         │
│  │  Uptime   │  │ Load Average │                         │
│  │ktime_get_ │  │ get_avenrun()│                         │
│  │boottime() │  │              │                         │
│  └───────────┘  └──────────────┘                         │
└─────────────────────────────────────────────────────────┘
```

---

## 3. Layer Descriptions

### 3.1 Kernel Module Layer (`kernel/sysmon_driver.c`)

This is the deepest layer, running in kernel space with ring-0 privileges.

**Responsibilities:**
- Register a character device with `cdev_init()` and `cdev_add()`
- Allocate device numbers with `alloc_chrdev_region()`
- Create `/dev/sysmon` via `class_create()` + `device_create()`
- Collect real system data from kernel structures
- Provide file operations: `open`, `release`, `read`, `unlocked_ioctl`
- Ensure thread safety with `DEFINE_MUTEX`
- Clean up all resources in `module_exit`

**Key Kernel APIs Used:**

| API | Purpose |
|---|---|
| `kcpustat_cpu(cpu)` | Per-CPU time counters |
| `si_meminfo(&si)` | RAM and swap statistics |
| `for_each_process(task)` | Iterate all processes |
| `ktime_get_boottime_seconds()` | System uptime |
| `get_avenrun(avnrun, ...)` | Load average values |
| `global_node_page_state(NR_FILE_PAGES)` | Page cache size |
| `copy_to_user(dst, src, size)` | Safe kernel→user copy |
| `alloc_chrdev_region()` | Dynamic major/minor allocation |
| `class_create()` | Device class for udev |
| `device_create()` | Create /dev/sysmon node |

### 3.2 Device Driver Interface Layer

This layer is the boundary between kernel and user space. It consists of:

- The device file `/dev/sysmon` (type: character device)
- The shared header `kernel/sysmon_ioctl.h` defining structures and commands
- The `file_operations` struct registering handler functions

The ioctl magic number `'S'` (0x53) is used in `_IOR()` macros to generate unique command codes that prevent accidental cross-driver usage.

### 3.3 User-Space Communication Layer (`user/src/monitor.cpp`)

The `SystemMonitor` class encapsulates all device communication:

- Opens `/dev/sysmon` once in `open_device()`
- Issues specific `ioctl()` calls for each data type
- Closes the file descriptor in the destructor (RAII)
- CPU usage requires two snapshots: calls `ioctl(SYSMON_GET_CPU)` twice with 500ms sleep between them

### 3.4 Data Processing Layer (`user/src/utils.cpp`)

Pure formatting functions with no state:
- `format_kb(kb)` — Converts kilobytes to human-readable KB/MB/GB
- `format_uptime(secs)` — Formats seconds as `Xd Yh Zm Ws`
- `format_load(x100)` — Converts fixed-point load to decimal string
- `is_module_loaded()` — Checks `/proc/modules` for `sysmon_driver`
- `device_exists()` — Uses `stat()` to check `/dev/sysmon`

### 3.5 Presentation Layer (`user/src/display.cpp`)

The `Display` class (all static methods) handles all terminal output:
- ANSI color codes for visual emphasis
- Color thresholds: green (<50%), yellow (50–80%), red (>80%)
- Formatted headers, separators, and data rows
- Error/info/success message formatting

---

## 4. Data Flow

### 4.1 Single Statistic Query (e.g., Memory)

```
User selects option 2
       │
       ▼
main.cpp: calls monitor.get_memory_info(mem)
       │
       ▼
monitor.cpp: ioctl(fd_, SYSMON_GET_MEMORY, &mem)
       │
       │  [kernel boundary]
       ▼
sysmon_driver.c: sysmon_ioctl() dispatches SYSMON_GET_MEMORY
       │
       ▼
collect_memory_info(): calls si_meminfo(), computes cached pages
       │
       ▼
copy_to_user(user_ptr, &info, sizeof(info))
       │
       │  [kernel boundary]
       ▼
monitor.cpp: ioctl() returns 0 (success)
       │
       ▼
main.cpp: calls Display::show_memory_info(mem)
       │
       ▼
display.cpp: formats and prints to stdout with ANSI colors
       │
       ▼
User sees formatted memory output
```

### 4.2 CPU Usage Calculation

```
User selects option 1
       │
       ▼
monitor.calculate_cpu_usage()
       │
       ├── ioctl(SYSMON_GET_CPU) → snapshot1
       │
       ├── usleep(500000) — wait 500ms
       │
       └── ioctl(SYSMON_GET_CPU) → snapshot2
               │
               ▼
       delta_total = snapshot2.total - snapshot1.total
       delta_busy  = snapshot2.busy  - snapshot1.busy
       usage%      = (delta_busy / delta_total) * 100
               │
               ▼
       Display::show_cpu_usage(usage%)
```

---

## 5. Shared Interface Design (`sysmon_ioctl.h`)

The shared header is compiled into both the kernel module and the user-space application. This is the single source of truth for the kernel/user ABI.

**Design decisions:**
- Uses `__u64` (fixed 64-bit) instead of `unsigned long` to avoid 32/64-bit ABI issues
- All memory values are in kilobytes to keep the numbers manageable
- Load averages are stored as `value × 100` (integer) to avoid floating point in kernel space
- CPU times are converted from nanoseconds to jiffies for consistency

---

## 6. Module Lifecycle

```
insmod sysmon_driver.ko
       │
       ▼
sysmon_init():
  alloc_chrdev_region()    ← get major/minor
  cdev_init() + cdev_add() ← register char device
  class_create()           ← create sysmon_class
  device_create()          ← create /dev/sysmon
       │
       ▼
[Driver is active, /dev/sysmon exists]
       │
       ▼
rmmod sysmon_driver
       │
       ▼
sysmon_exit():
  device_destroy()         ← remove /dev/sysmon
  class_destroy()          ← remove sysmon_class
  cdev_del()               ← unregister char device
  unregister_chrdev_region() ← release major/minor
```
