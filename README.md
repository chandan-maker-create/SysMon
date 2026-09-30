# SysMon — Linux System Resource Monitor

> A custom Linux character device driver (`/dev/sysmon`) paired with a C++17 CLI application to monitor CPU, memory, processes, uptime, and load average in real time.

---

## Table of Contents

1. [Project Overview](#1-project-overview)
2. [Problem Statement](#2-problem-statement)
3. [Objectives](#3-objectives)
4. [Features](#4-features)
5. [Technologies Used](#5-technologies-used)
6. [System Requirements](#6-system-requirements)
7. [Architecture](#7-architecture)
8. [Project Structure](#8-project-structure)
9. [Kernel Module Explanation](#9-kernel-module-explanation)
10. [Character Device Explanation](#10-character-device-explanation)
11. [ioctl Interface Explanation](#11-ioctl-interface-explanation)
12. [User-Space Application Explanation](#12-user-space-application-explanation)
13. [Installation](#13-installation)
14. [Compilation](#14-compilation)
15. [Execution](#15-execution)
16. [Testing](#16-testing)
17. [Troubleshooting](#17-troubleshooting)
18. [Sample Output](#18-sample-output)
19. [Future Enhancements](#19-future-enhancements)
20. [Contributors](#20-contributors)
21. [License](#21-license)

---

## 1. Project Overview

**SysMon** is a Linux system monitoring project that demonstrates Linux kernel module development, character device drivers, and C++ system programming. It consists of two main components:

- **Kernel Module** (`sysmon_driver.ko`): A loadable kernel module that registers the character device `/dev/sysmon` and collects real system resource data from inside the kernel.
- **User-Space CLI Application** (`sysmon`): A C++17 command-line application that communicates with the kernel module through `/dev/sysmon` using `ioctl()` and `read()` system calls.

---

## 2. Problem Statement

System administrators and developers often need to monitor Linux system resources. While tools like `top`, `htop`, and `vmstat` exist, they read data from `/proc` in user space. SysMon demonstrates a lower-level approach: exposing kernel data directly through a custom character device driver, illustrating core Linux kernel programming concepts.

---

## 3. Objectives

- Implement a functional Linux kernel character device driver
- Create a proper kernel/user-space communication interface using `ioctl`
- Collect real CPU, memory, process, uptime, and load data from within the kernel
- Build a clean C++17 CLI application demonstrating RAII and modular design
- Demonstrate proper software architecture and Linux system programming

---

## 4. Features

| Feature | Description |
|---|---|
| CPU Usage | Real-time CPU usage % via dual-snapshot jiffies comparison |
| Memory Stats | Total, used, available, free RAM + swap in KB/MB/GB |
| Process Count | Total and running process count via kernel task list |
| System Uptime | Boot time in days/hours/minutes/seconds |
| Load Average | 1, 5, 15 minute load averages from kernel `avenrun[]` |
| Complete Stats | All statistics in one combined view |
| Continuous Monitor | Live refresh mode, stops on Ctrl+C |
| Driver Info | Major/minor numbers, driver version, device path |
| Color Output | ANSI-colored terminal with threshold-based color coding |
| Error Handling | Graceful messages when driver is not loaded |

---

## 5. Technologies Used

| Component | Technology |
|---|---|
| Kernel Module | C (GNU C11, Linux Kernel APIs) |
| User Application | C++17 |
| Build System | GNU Make + Linux Kbuild |
| Device Interface | Character Device (`cdev`), `ioctl` |
| Kernel APIs | `kcpustat`, `si_meminfo`, `for_each_process`, `get_avenrun` |
| OS Target | Ubuntu Linux 22.04 / 24.04 |

---

## 6. System Requirements

- Ubuntu Linux 22.04 LTS or 24.04 LTS (or any modern Ubuntu-based distro)
- Linux kernel 5.15+ (for Ubuntu 22.04) or 6.8+ (for Ubuntu 24.04)
- `linux-headers-$(uname -r)` installed
- `build-essential` (gcc, g++, make) installed
- Root/sudo access to load the kernel module

---

## 7. Architecture

```
+-------------------------+
|   User / Administrator  |
+------------+------------+
             |
             v  (menu selection)
+-------------------------+
| C++ CLI Monitoring App  |   user/src/main.cpp
| (sysmon binary)         |   user/src/monitor.cpp
+------------+------------+   user/src/display.cpp
             |                user/src/utils.cpp
      open() / ioctl()
             |
             v
+-------------------------+
| /dev/sysmon             |   Character Device Node
| (udev-created)          |
+------------+------------+
             |
             v
+-------------------------+
| Linux Kernel Module     |   kernel/sysmon_driver.c
| sysmon_driver.ko        |
+------------+------------+
             |
    +--------+--------+--------+
    |         |                |
    v         v                v
CPU Stats  Memory Stats   Process Stats
(kcpustat) (si_meminfo)   (for_each_process)
    |         |                |
    +--------+--------+--------+
                 |
                 v
      Kernel Internal Data
```

---

## 8. Project Structure

```
SysMon/
├── Makefile                   ← Root orchestrating Makefile (all, clean, load, unload, test)
├── README.md                  ← Comprehensive project documentation & usage guide
├── LICENSE                    ← MIT License
├── .gitignore                 ← Git ignore rules
│
├── kernel/
│   ├── sysmon_driver.c        ← Kernel character device driver source
│   ├── sysmon_ioctl.h         ← Shared kernel/user interface header & ioctl ABI
│   └── Makefile               ← Kbuild Makefile for building sysmon_driver.ko
│
├── user/
│   ├── include/
│   │   ├── monitor.hpp        ← SystemMonitor class declaration
│   │   ├── display.hpp        ← DisplayManager class declaration
│   │   └── utils.hpp          ← String and terminal helper utilities
│   ├── src/
│   │   ├── main.cpp           ← CLI application entry point and argument parsing
│   │   ├── monitor.cpp        ← ioctl calls and CPU delta calculations
│   │   ├── display.cpp        ← ANSI visual gauges, tables, and views
│   │   └── utils.cpp          ← Byte/time formatters and terminal controls
│   └── Makefile               ← C++17 Makefile with strict warnings
│
├── tests/
│   ├── test_ioctl.cpp         ← Automated 13-case unit, integration & stress test suite
│   ├── Makefile               ← Makefile for compiling test_ioctl
│   └── README.md              ← Test documentation and coverage matrix
│
├── scripts/
│   ├── build.sh               ← Dependency validation and full-project build
│   ├── load_driver.sh         ← Kernel module loader and permission setup
│   ├── unload_driver.sh       ← Safe module unloader with fuser verification
│   ├── run_tests.sh           ← Test runner orchestrator
│   └── README.md              ← Helper script reference guide
│
└── docs/
    ├── requirements.md        ← Functional & non-functional requirements specification
    ├── architecture.md        ← High-level multi-tier system architecture
    ├── design.md              ← Detailed low-level design & synchronization model
    ├── testing.md             ← Quality assurance plan & test execution matrix
    ├── presentation_notes.md  ← Capstone defense slide outline & examiner Q&A
    └── uml/
        ├── use-case.md        ← Mermaid UML use case diagrams
        ├── class-diagram.md   ← Mermaid UML C++ class diagrams
        ├── sequence-diagram.md← Mermaid UML sequence diagrams
        └── state-machine.md   ← Mermaid UML state machine diagrams
```

---

## 9. Kernel Module Explanation

The kernel module (`kernel/sysmon_driver.c`) is the core of SysMon. It runs in **kernel space** and has direct access to kernel data structures.

**What it does:**
1. Calls `alloc_chrdev_region()` to dynamically allocate a major/minor device number
2. Initializes a `cdev` structure with file operations (`open`, `release`, `read`, `ioctl`)
3. Creates a device class with `class_create()` so `udev` automatically creates `/dev/sysmon`
4. Uses `device_create()` to register the device node
5. Implements data collection using kernel-internal APIs:
   - `kcpustat_cpu()` — per-CPU time counters
   - `si_meminfo()` — RAM and swap information
   - `for_each_process()` — iterates the task list
   - `ktime_get_boottime_seconds()` — system uptime
   - `get_avenrun()` — load averages

**Safety:**
- Uses `copy_to_user()` for all kernel → user data transfers
- Uses a `mutex` to serialize access
- Validates `ioctl` magic numbers
- Handles all error paths and cleans up in `module_exit`

---

## 10. Character Device Explanation

A **character device** is a Linux device that transfers data as a stream of bytes (character by character). It appears as a special file in `/dev/`.

- Device file: `/dev/sysmon`
- Type: Character device (`c` in `ls -l /dev/sysmon`)
- Created by: `device_create()` in the kernel module (via `udev`)
- Accessed by: Standard POSIX calls — `open()`, `read()`, `ioctl()`, `close()`

```bash
# Inspect the device
ls -l /dev/sysmon
# Output: crw------- 1 root root 240, 0 ... /dev/sysmon

# Quick read (human-readable summary)
sudo cat /dev/sysmon
```

---

## 11. ioctl Interface Explanation

`ioctl` (Input/Output Control) is a system call used to send device-specific commands that don't fit the standard read/write model.

**Interface defined in `kernel/sysmon_ioctl.h`:**

| ioctl Command | Number | Returns |
|---|---|---|
| `SYSMON_GET_CPU` | 1 | `struct sysmon_cpu_info` |
| `SYSMON_GET_MEMORY` | 2 | `struct sysmon_memory_info` |
| `SYSMON_GET_PROC_COUNT` | 3 | `struct sysmon_proc_info` |
| `SYSMON_GET_UPTIME` | 4 | `struct sysmon_uptime_info` |
| `SYSMON_GET_LOAD` | 5 | `struct sysmon_load_info` |
| `SYSMON_GET_ALL` | 6 | `struct sysmon_all_stats` |
| `SYSMON_GET_DRIVER_INFO` | 7 | `struct sysmon_driver_info` |

**Magic number:** `'S'` (0x53) — prevents ioctl commands from one driver accidentally being sent to another.

**Example usage:**
```c
struct sysmon_memory_info mem;
int fd = open("/dev/sysmon", O_RDONLY);
ioctl(fd, SYSMON_GET_MEMORY, &mem);
printf("Total RAM: %llu KB\n", mem.total_ram);
close(fd);
```

---

## 12. User-Space Application Explanation

The C++17 CLI application consists of four files:

| File | Responsibility |
|---|---|
| `main.cpp` | Menu loop, signal handling, Ctrl+C support |
| `monitor.cpp` | Opens `/dev/sysmon`, calls `ioctl()` to get data |
| `display.cpp` | ANSI-colored terminal output, formatted tables |
| `utils.cpp` | Formats KB→GB, seconds→uptime, checks module status |

**Design pattern:** The `SystemMonitor` class follows RAII — it opens the device in `open_device()` and automatically closes it in the destructor.

---

## 13. Installation

```bash
# Step 1: Update package list
sudo apt update

# Step 2: Install kernel headers and build tools
sudo apt install -y linux-headers-$(uname -r) build-essential g++

# Step 3: Clone the repository
git clone https://github.com/yourusername/SysMon.git
cd SysMon
```

---

## 14. Compilation

### Compile Kernel Module
```bash
cd kernel
make
# Expected output: sysmon_driver.ko
ls -lh sysmon_driver.ko
```

### Compile User-Space Application
```bash
cd ../user
make
# Expected output: sysmon binary
ls -lh sysmon
```

---

## 15. Execution

### Step 1: Load the Kernel Module
```bash
cd kernel
sudo insmod sysmon_driver.ko
```

### Step 2: Verify Device Created
```bash
ls -l /dev/sysmon
# Expected: crw------- 1 root root 240, 0 ... /dev/sysmon

dmesg | tail -5
# Expected: sysmon: driver loaded successfully (/dev/sysmon)
```

### Step 3: Quick Test (optional)
```bash
sudo cat /dev/sysmon
```

### Step 4: Run the Application
```bash
cd ../user
sudo ./sysmon
```

### Step 5: When Done — Unload the Module
```bash
sudo rmmod sysmon_driver
dmesg | tail -3
# Expected: sysmon: driver unloaded successfully
```

---

## 16. Testing

See [`docs/testing.md`](docs/testing.md) for full test cases.

**Quick smoke test:**
```bash
# TC01: Module loads
sudo insmod kernel/sysmon_driver.ko
echo $?   # Must be 0

# TC02: Device exists
ls /dev/sysmon
echo $?   # Must be 0

# TC03: cat works
sudo cat /dev/sysmon | head -5

# TC10: Module unloads
sudo rmmod sysmon_driver
echo $?   # Must be 0
```

---

## 17. Troubleshooting

### Problem: `insmod: ERROR: could not insert module`
```bash
dmesg | tail -20   # Read the kernel error
# Common cause: mismatched kernel headers
sudo apt install --reinstall linux-headers-$(uname -r)
cd kernel && make clean && make
```

### Problem: `/dev/sysmon not found`
```bash
# Module may not be loaded
lsmod | grep sysmon
sudo insmod kernel/sysmon_driver.ko
```

### Problem: `Permission denied`
```bash
# Run with sudo
sudo ./user/sysmon
# OR adjust permissions (less secure)
sudo chmod 666 /dev/sysmon
```

### Problem: `invalid module format`
```bash
# Module compiled for different kernel version
uname -r                          # Note current kernel
ls /lib/modules/$(uname -r)/build # Confirm headers exist
cd kernel && make clean && make
```

### Problem: Compilation fails with unknown symbol
```bash
# Ensure kernel headers match running kernel exactly
sudo apt install linux-headers-$(uname -r)
make -C /lib/modules/$(uname -r)/build M=$(pwd) modules
```

---

## 18. Sample Output

```
========================================
        LINUX SYSTEM MONITOR
========================================

  1. Show CPU Usage
  2. Show Memory Usage
  ...
  Enter choice: 6

========================================
  LIVE SYSTEM MONITOR
========================================

  CPU Usage      : 12.3 %
  Memory Usage   : 58.4 %
  Total RAM      : 15.53 GB
  Used RAM       : 9.07 GB
  Available RAM  : 6.46 GB
  Processes      : 248 (3 running)
  System Uptime  : 2h 41m 5s
  Load Average   : 0.92  0.87  0.79
```

---

## 19. Future Enhancements

- Per-CPU usage breakdown (using `ncurses`)
- Process list with PID, name, CPU%, memory%
- Network interface statistics
- Disk I/O statistics
- `procfs` entry at `/proc/sysmon` for quick scripting
- Configurable refresh interval for continuous monitoring
- Log statistics to a file

---

## 20. Contributors

| Name | Role |
|---|---|
| Your Name | Developer |

---

## 21. License

This project is licensed under the **MIT License** — see [LICENSE](LICENSE) for details.

The kernel module (`kernel/sysmon_driver.c`) is licensed under **GPL-2.0** as required for Linux kernel modules.
