# SysMon Automated Test Suite

This directory contains the automated test harness for the SysMon Linux Character Device Driver.

## Overview

The test runner (`test_ioctl`) directly interacts with `/dev/sysmon` using both standard UNIX file operations (`open`, `read`, `close`) and the specialized `ioctl(2)` system call.

### Test Coverage Matrix

| Test Case | Method / Command | Description | Expected Result |
| :--- | :--- | :--- | :--- |
| **DeviceNode::ExistenceAndType** | `stat("/dev/sysmon")` | Checks node presence and character device mode (`S_ISCHR`) | Mode is `c`, exists |
| **FileOperations::OpenAndClose** | `open()`, `close()` | Verifies file descriptor lifecycle & kernel mutex lock | `fd >= 0`, rc == 0 |
| **FileOperations::ReadInterface** | `read()` | Tests human-readable diagnostic report generation | Formatted text returned; EOF on next read |
| **Ioctl::GetDriverInfo** | `SYSMON_GET_DRIVER_INFO` | Fetches driver metadata, name, version, and major number | Name matches "sysmon", v1.0, major > 0 |
| **Ioctl::GetCpuInfo** | `SYSMON_GET_CPU` | Queries CPU tick counters (user, nice, sys, idle, etc.) | Total > 0, busy <= total |
| **Ioctl::GetMemoryInfo** | `SYSMON_GET_MEMORY` | Reads RAM and Swap usage figures | Total > 0, used <= total |
| **Ioctl::GetProcCount** | `SYSMON_GET_PROC_COUNT` | Walks task tree under RCU to count processes | Count >= 1, running <= count |
| **Ioctl::GetUptime** | `SYSMON_GET_UPTIME` | Reads uptime seconds from kernel boot timer | Uptime > 0 |
| **Ioctl::GetLoadAvg** | `SYSMON_GET_LOAD` | Fetches 1, 5, 15-minute load averages | Values >= 0 |
| **Ioctl::GetAllStats** | `SYSMON_GET_ALL` | Retrieves complete system snapshot in a single context switch | All fields populated |
| **ErrorHandling::InvalidCommand** | Bogus `_IOR('Z', ...)` | Tests kernel validation of unknown ioctl command numbers | Returns `-1` with `errno == ENOTTY` |
| **ErrorHandling::BadUserPointer** | `ioctl(fd, cmd, NULL)` | Verifies `copy_to_user` safety with a null pointer | Returns `-1` with `errno == EFAULT` |
| **Stress::HighFrequencyPoll** | 100x `SYSMON_GET_ALL` | Rapid back-to-back ioctl invocations | 0 drops, computes avg µs/call |

## How to Run

1. Ensure the driver is compiled and loaded:
   ```bash
   sudo make load
   ```

2. Compile and run the tests:
   ```bash
   make tests
   sudo ./tests/test_ioctl
   ```
   Or via the root Makefile:
   ```bash
   sudo make test
   ```
