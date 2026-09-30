# SysMon — Requirements Document

**Project:** Linux System Resource Monitor Using a Custom Character Device Driver  
**Version:** 1.0  
**Date:** 2024

---

## 1. Functional Requirements

### FR-01: Driver Registration
- The system SHALL register a Linux character device driver named `sysmon_driver`
- The driver SHALL dynamically allocate a major/minor device number using `alloc_chrdev_region()`
- The major number SHALL be printed to the kernel log on successful registration

### FR-02: Device Creation
- The kernel module SHALL create a device class named `sysmon_class`
- The module SHALL create the device node `/dev/sysmon` via `device_create()`
- The device node SHALL be automatically created by `udev` when the module is loaded

### FR-03: CPU Monitoring
- The system SHALL collect CPU time counters (user, nice, system, idle, iowait, irq, softirq, steal)
- The system SHALL compute CPU usage percentage using a dual-snapshot comparison over 500ms
- CPU usage SHALL be reported as a percentage with one decimal place

### FR-04: Memory Monitoring
- The system SHALL report total physical RAM in kilobytes
- The system SHALL report used RAM (total − available)
- The system SHALL report available RAM (free + buffers + cached)
- The system SHALL report free RAM, buffer cache, page cache, swap total, and swap free
- All memory values SHALL be convertible to KB, MB, or GB for display

### FR-05: Process Monitoring
- The system SHALL count the total number of processes (thread group leaders)
- The system SHALL count the number of currently running processes
- Process counting SHALL use RCU-protected `for_each_process()` iteration

### FR-06: Uptime Monitoring
- The system SHALL report system uptime in seconds since boot
- The user-space application SHALL format uptime as days/hours/minutes/seconds

### FR-07: Load Average Monitoring
- The system SHALL read the 1-minute, 5-minute, and 15-minute load averages
- Load averages SHALL be provided as fixed-point integers (value × 100)
- User space SHALL display them as decimal values (e.g., 1.42)

### FR-08: User-Space Communication
- The user-space application SHALL open `/dev/sysmon` using `open()`
- The application SHALL use `ioctl()` to request specific statistics
- The application SHALL use `read()` to obtain a text summary
- The application SHALL close the device when done

### FR-09: ioctl Interface
- The driver SHALL support ioctl commands: `SYSMON_GET_CPU`, `SYSMON_GET_MEMORY`, `SYSMON_GET_PROC_COUNT`, `SYSMON_GET_UPTIME`, `SYSMON_GET_LOAD`, `SYSMON_GET_ALL`, `SYSMON_GET_DRIVER_INFO`
- All ioctl commands SHALL use magic number `'S'` for validation
- The driver SHALL return `-ENOTTY` for unknown commands

### FR-10: CLI Application
- The application SHALL display an interactive numbered menu (options 1–9)
- The application SHALL implement all menu options
- Option 7 SHALL provide continuous live monitoring until Ctrl+C
- The application SHALL use ANSI color codes for visual clarity

### FR-11: Error Handling
- The application SHALL detect when `/dev/sysmon` does not exist
- The application SHALL detect permission denied errors
- The application SHALL display helpful instructions when the driver is not loaded
- The driver SHALL validate all ioctl magic numbers and return appropriate errors

---

## 2. Non-Functional Requirements

### NFR-01: Reliability
- The kernel module SHALL properly clean up all resources in `module_exit`
- The module SHALL not cause kernel panics under normal operating conditions
- All memory allocations SHALL check for failure and handle accordingly
- The module SHALL use a mutex to prevent race conditions

### NFR-02: Security
- The driver SHALL use `copy_to_user()` for all kernel-to-user data transfers
- The driver SHALL NOT trust or dereference user-space pointers directly
- The driver SHALL validate ioctl magic numbers before processing commands
- The application SHALL NOT require world-writable device permissions

### NFR-03: Performance
- Each individual ioctl call SHALL complete in under 100ms (excluding CPU sampling delay)
- CPU usage sampling (500ms dual-snapshot) is expected and acceptable
- The driver SHALL NOT introduce significant kernel scheduling delays
- Memory collection using `si_meminfo()` is O(1) and SHALL be fast

### NFR-04: Maintainability
- Source code SHALL use meaningful variable and function names
- All functions SHALL have a comment block describing their purpose
- The kernel module and user-space headers SHALL be clearly separated
- The shared interface header (`sysmon_ioctl.h`) SHALL be the single source of truth

### NFR-05: Portability
- The kernel module SHALL compile on Ubuntu 22.04 (kernel 5.15+)
- The kernel module SHALL compile on Ubuntu 24.04 (kernel 6.8+)
- The code SHALL handle API differences between kernel versions using `LINUX_VERSION_CODE`
- The user-space application SHALL use only POSIX-standard system calls

### NFR-06: Usability
- The CLI application SHALL display color-coded output for quick status assessment
- Green: normal (< 50% usage), Yellow: moderate (50–80%), Red: high (> 80%)
- Error messages SHALL include actionable instructions (e.g., `sudo insmod ...`)
- The continuous monitoring mode SHALL respond to Ctrl+C within 200ms

---

## 3. Constraints

- Programming languages: C (kernel module), C++17 (user-space application) only
- Target OS: Ubuntu Linux 22.04 / 24.04
- No external libraries (only Linux kernel headers and C++ standard library)
- No Python, JavaScript, or other scripting languages
- Kernel module must be a genuine loadable kernel module (`.ko` file)
