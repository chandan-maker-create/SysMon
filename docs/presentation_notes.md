# SysMon — Capstone Defense & Presentation Notes

**Project:** Linux System Resource Monitor Using a Custom Character Device Driver  
**Presentation Time:** 15–20 minutes  
**Target Audience:** CSE Academic Faculty & Industry Evaluation Committee  

---

## 1. Slide Deck Structure & Speaking Script

### Slide 1: Title & Overview
- **Title:** Linux System Resource Monitor Using a Custom Character Device Driver (SysMon)
- **Presenter:** Computer Science & Engineering Capstone Candidate
- **Talking Points:**
  - Welcome committee members.
  - Introduce SysMon: an end-to-end Linux systems programming capstone that bridges kernel-space character device driver design with modern C++17 user-space visualization.

### Slide 2: Problem Statement & Motivation
- **Talking Points:**
  - Most user-space monitors (`top`, `htop`) depend heavily on parsing ASCII text streams from `/proc` and `/sysfs` filesystems.
  - Text parsing creates unnecessary CPU overhead, memory allocations, string parsing bottlenecks, and repetitive kernel-user context switches.
  - SysMon demonstrates an alternative architectural paradigm: a direct binary `ioctl` protocol between a dedicated Linux character device driver and an optimized C++ client.

### Slide 3: System Architecture
- **Talking Points:**
  - 5-layer architecture:
    1. Linux Kernel Subsystems (`kcpustat`, `si_meminfo`, `avenrun`, `sched`)
    2. Character Device Driver (`sysmon_driver.c`) with dynamic major allocation and devtmpfs registration
    3. Shared Binary ABI (`sysmon_ioctl.h`) using `_IOR` commands and fixed-width types
    4. C++17 System Monitor abstraction (`SystemMonitor`)
    5. Terminal Visualization Engine (`DisplayManager`) with ANSI color coding

### Slide 4: Kernel Module Engineering Deep-Dive
- **Talking Points:**
  - Dynamic device creation via `alloc_chrdev_region`, `cdev_init`, `class_create`, and `device_create`.
  - Concurrency management using `mutex` for device operations and RCU (`rcu_read_lock`) for non-blocking task tree iteration.
  - Safe memory transfer using `copy_to_user()` with verified error handling (`-EFAULT`).

### Slide 5: User-Space C++ Application
- **Talking Points:**
  - RAII-based device lifecycle management.
  - Instantaneous CPU percentage calculations using consecutive tick differences:
    $$\Delta \text{busy} / \Delta \text{total} \times 100$$
  - Real-time animated terminal gauges with configurable refresh intervals and signal-safe cleanup (`SIGINT`/`SIGTERM`).

### Slide 6: Verification, Testing & Performance
- **Talking Points:**
  - 13 automated test suites covering positive paths, boundary conditions, invalid ioctls (`-ENOTTY`), and null pointer dereferences (`-EFAULT`).
  - Micro-benchmark results: average `ioctl` turnaround latency is sub-10 microseconds.

---

## 2. Live Demo Script (Step-by-Step)

1. **Clean & Build:**
   ```bash
   make clean
   make all tests
   ```
   *Show that both kernel module and C++ application compile with zero warnings using `-Wall -Wextra -Wpedantic`.*

2. **Load Driver:**
   ```bash
   sudo make load
   ```
   *Point out kernel ring buffer log (`dmesg`) showing allocated major/minor number and `/dev/sysmon` node.*

3. **Verify via Character Device `read()`:**
   ```bash
   cat /dev/sysmon
   ```
   *Demonstrates fallback text interface directly from the kernel.*

4. **Run Automated Test Suite:**
   ```bash
   sudo ./tests/test_ioctl
   ```
   *Highlight that all 13 test cases pass, including error handling (`ENOTTY`, `EFAULT`).*

5. **Launch Interactive Dashboard:**
   ```bash
   sudo ./user/sysmon
   ```
   *Demonstrate real-time CPU gauges, memory meters, uptime, load averages, and responsive resizing.*

6. **Clean Shutdown & Unload:**
   ```bash
   # Press Ctrl+C in dashboard (terminal resets cleanly)
   sudo make unload
   ```

---

## 3. Anticipated Viva / Examiner Questions & Answers

### Q1: Why develop a character device driver instead of reading from `/proc`?
> **Answer:** `/proc` is a virtual filesystem that relies on the `seq_file` interface to format kernel binary statistics into ASCII strings. User space must then re-parse those strings into binary integers. SysMon avoids string serialization, string parsing, and multiple read syscalls by returning strongly typed binary C structs via a single `ioctl` call, minimizing latency and CPU overhead.

### Q2: How does SysMon prevent kernel panics when an invalid user pointer is passed?
> **Answer:** All kernel-to-user memory transfers are executed strictly through `copy_to_user()`. If an unmapped or invalid virtual memory address (like `NULL`) is supplied, `copy_to_user()` catches the fault and returns the number of uncopied bytes. The driver detects this condition and returns `-EFAULT` to user space without crashing the kernel.

### Q3: Why is RCU (`rcu_read_lock`) used in process counting?
> **Answer:** The Linux task list is continuously modified by the scheduler as processes fork and exit. Using traditional spinlocks or write locks on `tasklist_lock` would degrade system-wide scheduling performance. RCU allows our driver to read and iterate over `for_each_process()` concurrently without blocking task creation or termination.

### Q4: How are CPU percentages calculated from jiffies?
> **Answer:** The kernel provides monotonic tick counters for states such as `user`, `system`, `idle`, and `iowait`. A single snapshot only reflects cumulative time since boot. The user-space client takes two consecutive samples separated by $\Delta t$, computes $\Delta \text{busy} = \Delta \text{total} - \Delta \text{idle} - \Delta \text{iowait}$, and calculates:
> $$\text{Usage \%} = \frac{\Delta \text{busy}}{\Delta \text{total}} \times 100$$
