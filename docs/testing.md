# SysMon — Testing & Quality Assurance Plan

**Project:** Linux System Resource Monitor Using a Custom Character Device Driver  
**Version:** 1.0  
**Test Harness:** C++17 Automated Test Suite (`tests/test_ioctl.cpp`)  
**Target Environments:** Ubuntu 22.04 LTS / 24.04 LTS

---

## 1. Testing Philosophy & Objectives

The testing framework for SysMon ensures:
1. **Kernel Stability:** Zero kernel panics, page faults, or unhandled null dereferences under invalid user inputs.
2. **Data Integrity:** Telemetry returned via ioctls matches realistic physical host metrics.
3. **High Performance:** Minimal context switch latency and zero memory leaks.
4. **POSIX Compliance:** Correct return codes (`-ENOTTY`, `-EFAULT`, `-EBUSY`) on error conditions.

---

## 2. Test Classification

### 2.1 Automated Unit & Integration Tests

SysMon includes an automated C++ test suite (`tests/test_ioctl`) covering 13 test scenarios:

| Test ID | Test Scenario | Mechanism | Acceptance Criteria |
| :--- | :--- | :--- | :--- |
| **TC-01** | Character Device Node Existence | `stat("/dev/sysmon")` | File exists with mode `S_ISCHR`. |
| **TC-02** | File Open & Release | `open()` followed by `close()` | Returns valid descriptor (`>= 0`) and releases cleanly. |
| **TC-03** | Character Read Interface | `read()` into 2KB buffer | Formatted diagnostic text returned with status header; EOF on subsequent read. |
| **TC-04** | Driver Info ioctl | `SYSMON_GET_DRIVER_INFO` | Name is `"sysmon"`, version is `"1.0"`, major is non-zero. |
| **TC-05** | CPU Statistics ioctl | `SYSMON_GET_CPU` | `total > 0`, `busy <= total`, `idle <= total`. |
| **TC-06** | Memory Information ioctl | `SYSMON_GET_MEMORY` | `total_ram > 0`, `used_ram <= total_ram`, `available_ram <= total_ram`. |
| **TC-07** | Process Count ioctl | `SYSMON_GET_PROC_COUNT` | `process_count >= 1`, `running_count <= process_count`. |
| **TC-08** | System Uptime ioctl | `SYSMON_GET_UPTIME` | `uptime_seconds > 0`. |
| **TC-09** | Load Average ioctl | `SYSMON_GET_LOAD` | `load_1 >= 0`, `load_5 >= 0`, `load_15 >= 0`. |
| **TC-10** | Monolithic Stats ioctl | `SYSMON_GET_ALL` | Populates complete system snapshot structure in single syscall. |
| **TC-11** | Invalid ioctl Command | `ioctl(fd, BOGUS_CMD, ...)` | Fails with return `-1` and `errno == ENOTTY`. |
| **TC-12** | Invalid Buffer Pointer | `ioctl(fd, SYSMON_GET_CPU, NULL)` | Fails safely with return `-1` and `errno == EFAULT`. |
| **TC-13** | Stress & Concurrency Test | 100 consecutive `SYSMON_GET_ALL` calls | 100% success rate; logs latency per call in microseconds. |

### 2.2 Manual Verification Procedures

In addition to automated tests, the following manual procedures validate full-stack behavior:

#### Verification 1: Kernel Diagnostic Buffer
```bash
dmesg | grep -i sysmon
```
Expected output:
- `"sysmon: initializing SysMon driver v1.0"`
- `"sysmon: allocated major=... minor=..."`
- `"sysmon: driver loaded successfully (/dev/sysmon)"`

#### Verification 2: Diagnostic Stream via `cat`
```bash
cat /dev/sysmon
```
Expected output:
```text
========================================
       SysMon Driver Status
========================================
CPU Total Jiffies : 1849202
CPU Busy Jiffies  : 231404
Total RAM         : 16248924 KB
Used RAM          : 4194304 KB
Available RAM     : 12054620 KB
...
```

#### Verification 3: Ground Truth Comparison
Compare metrics against established Linux diagnostic utilities:
- **CPU & Memory:** Compare against `top` or `free -m`.
- **Uptime:** Compare against `/usr/bin/uptime`.
- **Load Averages:** Compare against `/proc/loadavg`.

---

## 3. Test Execution Instructions

### Running the Test Suite
```bash
# 1. Compile everything
make all tests

# 2. Insert kernel module
sudo make load

# 3. Execute automated test runner
sudo ./tests/test_ioctl
```

### Expected Output Format
```text
========================================================
       SysMon Kernel Driver Automated Test Suite        
========================================================

[ RUN      ] DeviceNode::ExistenceAndType
[       OK ] DeviceNode::ExistenceAndType (Found character device at /dev/sysmon)
[ RUN      ] FileOperations::OpenAndClose
[       OK ] FileOperations::OpenAndClose (Successfully opened and closed descriptor)
[ RUN      ] FileOperations::ReadInterface
[       OK ] FileOperations::ReadInterface (Read 560 bytes of formatted status text)
...
========================================================
                   TEST SUMMARY                         
========================================================
  Passed : 13
  Failed : 0
  Skipped: 0
========================================================
```
