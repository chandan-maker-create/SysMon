/*
 * test_ioctl.cpp - Automated Test Suite for SysMon Character Device Driver
 *
 * Validates the kernel module interface, all ioctl commands, character device
 * read functionality, boundary conditions, error handling, and stability.
 *
 * Compile: make -C tests
 * Run:     sudo ./tests/test_ioctl
 */

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <cstring>
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include "sysmon_ioctl.h"

// Terminal color codes
#define ANSI_RESET   "\033[0m"
#define ANSI_RED     "\033[1;31m"
#define ANSI_GREEN   "\033[1;32m"
#define ANSI_YELLOW  "\033[1;33m"
#define ANSI_BLUE    "\033[1;34m"
#define ANSI_CYAN    "\033[1;36m"
#define ANSI_BOLD    "\033[1m"

struct TestStats {
    int passed = 0;
    int failed = 0;
    int skipped = 0;
};

static TestStats g_stats;

void log_test_start(const std::string& name) {
    std::cout << ANSI_CYAN << "[ RUN      ] " << ANSI_RESET << name << std::endl;
}

void log_test_pass(const std::string& name, const std::string& details = "") {
    g_stats.passed++;
    std::cout << ANSI_GREEN << "[       OK ] " << ANSI_RESET << name;
    if (!details.empty()) {
        std::cout << " (" << details << ")";
    }
    std::cout << std::endl;
}

void log_test_fail(const std::string& name, const std::string& error) {
    g_stats.failed++;
    std::cout << ANSI_RED << "[  FAILED  ] " << ANSI_RESET << name
              << " - Error: " << error << std::endl;
}

void log_test_skip(const std::string& name, const std::string& reason) {
    g_stats.skipped++;
    std::cout << ANSI_YELLOW << "[  SKIPPED ] " << ANSI_RESET << name
              << " - " << reason << std::endl;
}

// ----------------------------------------------------------------------------
// Test 1: Device File Existence and Mode
// ----------------------------------------------------------------------------
bool test_device_node_exists() {
    const std::string name = "DeviceNode::ExistenceAndType";
    log_test_start(name);

    struct stat st;
    if (stat(SYSMON_DEVICE_PATH, &st) < 0) {
        log_test_fail(name, std::string("Device node ") + SYSMON_DEVICE_PATH + " not found (" + strerror(errno) + ")");
        return false;
    }

    if (!S_ISCHR(st.st_mode)) {
        log_test_fail(name, "Node exists but is not a character device");
        return false;
    }

    log_test_pass(name, "Found character device at " + std::string(SYSMON_DEVICE_PATH));
    return true;
}

// ----------------------------------------------------------------------------
// Test 2: Device Open and Close
// ----------------------------------------------------------------------------
bool test_device_open_close() {
    const std::string name = "FileOperations::OpenAndClose";
    log_test_start(name);

    int fd = open(SYSMON_DEVICE_PATH, O_RDONLY);
    if (fd < 0) {
        log_test_fail(name, std::string("Failed to open device: ") + strerror(errno));
        return false;
    }

    if (close(fd) < 0) {
        log_test_fail(name, std::string("Failed to close device: ") + strerror(errno));
        return false;
    }

    log_test_pass(name, "Successfully opened and closed descriptor");
    return true;
}

// ----------------------------------------------------------------------------
// Test 3: Read Interface (cat /dev/sysmon support)
// ----------------------------------------------------------------------------
bool test_device_read() {
    const std::string name = "FileOperations::ReadInterface";
    log_test_start(name);

    int fd = open(SYSMON_DEVICE_PATH, O_RDONLY);
    if (fd < 0) {
        log_test_fail(name, "Cannot open device for read test");
        return false;
    }

    char buffer[2048];
    std::memset(buffer, 0, sizeof(buffer));
    ssize_t bytes_read = read(fd, buffer, sizeof(buffer) - 1);

    if (bytes_read <= 0) {
        close(fd);
        log_test_fail(name, "Read returned " + std::to_string(bytes_read) + " bytes (" + strerror(errno) + ")");
        return false;
    }

    std::string text(buffer);
    if (text.find("SysMon Driver Status") == std::string::npos ||
        text.find("CPU Total Jiffies") == std::string::npos ||
        text.find("Total RAM") == std::string::npos) {
        close(fd);
        log_test_fail(name, "Read content does not contain expected header/metrics");
        return false;
    }

    // Verify subsequent read returns 0 (EOF behavior for single cat output)
    ssize_t second_read = read(fd, buffer, sizeof(buffer));
    if (second_read != 0) {
        close(fd);
        log_test_fail(name, "Expected EOF (0 bytes) on second read, got " + std::to_string(second_read));
        return false;
    }

    close(fd);
    log_test_pass(name, "Read " + std::to_string(bytes_read) + " bytes of formatted status text");
    return true;
}

// ----------------------------------------------------------------------------
// Test 4: SYSMON_GET_DRIVER_INFO ioctl
// ----------------------------------------------------------------------------
bool test_ioctl_driver_info() {
    const std::string name = "Ioctl::GetDriverInfo";
    log_test_start(name);

    int fd = open(SYSMON_DEVICE_PATH, O_RDONLY);
    if (fd < 0) {
        log_test_fail(name, "Cannot open device");
        return false;
    }

    struct sysmon_driver_info info;
    std::memset(&info, 0, sizeof(info));

    if (ioctl(fd, SYSMON_GET_DRIVER_INFO, &info) < 0) {
        close(fd);
        log_test_fail(name, std::string("ioctl failed: ") + strerror(errno));
        return false;
    }
    close(fd);

    if (std::string(info.name) != SYSMON_DEVICE_NAME) {
        log_test_fail(name, "Driver name mismatch: expected '" + std::string(SYSMON_DEVICE_NAME) + "', got '" + info.name + "'");
        return false;
    }

    if (std::string(info.version) != SYSMON_DRIVER_VERSION) {
        log_test_fail(name, "Driver version mismatch: expected '" + std::string(SYSMON_DRIVER_VERSION) + "', got '" + info.version + "'");
        return false;
    }

    if (info.major == 0) {
        log_test_fail(name, "Major number is 0");
        return false;
    }

    log_test_pass(name, "Name: " + std::string(info.name) + ", Version: " + info.version + ", Major: " + std::to_string(info.major));
    return true;
}

// ----------------------------------------------------------------------------
// Test 5: SYSMON_GET_CPU ioctl
// ----------------------------------------------------------------------------
bool test_ioctl_cpu() {
    const std::string name = "Ioctl::GetCpuInfo";
    log_test_start(name);

    int fd = open(SYSMON_DEVICE_PATH, O_RDONLY);
    if (fd < 0) {
        log_test_fail(name, "Cannot open device");
        return false;
    }

    struct sysmon_cpu_info cpu;
    std::memset(&cpu, 0, sizeof(cpu));

    if (ioctl(fd, SYSMON_GET_CPU, &cpu) < 0) {
        close(fd);
        log_test_fail(name, std::string("ioctl failed: ") + strerror(errno));
        return false;
    }
    close(fd);

    if (cpu.total == 0) {
        log_test_fail(name, "CPU total ticks == 0");
        return false;
    }

    if (cpu.busy > cpu.total) {
        log_test_fail(name, "CPU busy (" + std::to_string(cpu.busy) + ") exceeds total (" + std::to_string(cpu.total) + ")");
        return false;
    }

    log_test_pass(name, "Total Jiffies: " + std::to_string(cpu.total) + ", Busy: " + std::to_string(cpu.busy) + ", Idle: " + std::to_string(cpu.idle));
    return true;
}

// ----------------------------------------------------------------------------
// Test 6: SYSMON_GET_MEMORY ioctl
// ----------------------------------------------------------------------------
bool test_ioctl_memory() {
    const std::string name = "Ioctl::GetMemoryInfo";
    log_test_start(name);

    int fd = open(SYSMON_DEVICE_PATH, O_RDONLY);
    if (fd < 0) {
        log_test_fail(name, "Cannot open device");
        return false;
    }

    struct sysmon_memory_info mem;
    std::memset(&mem, 0, sizeof(mem));

    if (ioctl(fd, SYSMON_GET_MEMORY, &mem) < 0) {
        close(fd);
        log_test_fail(name, std::string("ioctl failed: ") + strerror(errno));
        return false;
    }
    close(fd);

    if (mem.total_ram == 0) {
        log_test_fail(name, "Total RAM reported as 0 KB");
        return false;
    }

    if (mem.available_ram > mem.total_ram) {
        log_test_fail(name, "Available RAM exceeds Total RAM");
        return false;
    }

    if (mem.used_ram > mem.total_ram) {
        log_test_fail(name, "Used RAM exceeds Total RAM");
        return false;
    }

    log_test_pass(name, "Total: " + std::to_string(mem.total_ram / 1024) + " MB, Used: " +
                  std::to_string(mem.used_ram / 1024) + " MB, Free: " + std::to_string(mem.free_ram / 1024) + " MB");
    return true;
}

// ----------------------------------------------------------------------------
// Test 7: SYSMON_GET_PROC_COUNT ioctl
// ----------------------------------------------------------------------------
bool test_ioctl_proc_count() {
    const std::string name = "Ioctl::GetProcCount";
    log_test_start(name);

    int fd = open(SYSMON_DEVICE_PATH, O_RDONLY);
    if (fd < 0) {
        log_test_fail(name, "Cannot open device");
        return false;
    }

    struct sysmon_proc_info proc;
    std::memset(&proc, 0, sizeof(proc));

    if (ioctl(fd, SYSMON_GET_PROC_COUNT, &proc) < 0) {
        close(fd);
        log_test_fail(name, std::string("ioctl failed: ") + strerror(errno));
        return false;
    }
    close(fd);

    if (proc.process_count == 0) {
        log_test_fail(name, "Process count is 0 (must be >= 1 for init/systemd)");
        return false;
    }

    if (proc.running_count > proc.process_count) {
        log_test_fail(name, "Running process count exceeds total process count");
        return false;
    }

    log_test_pass(name, "Total Processes: " + std::to_string(proc.process_count) + ", Running: " + std::to_string(proc.running_count));
    return true;
}

// ----------------------------------------------------------------------------
// Test 8: SYSMON_GET_UPTIME ioctl
// ----------------------------------------------------------------------------
bool test_ioctl_uptime() {
    const std::string name = "Ioctl::GetUptime";
    log_test_start(name);

    int fd = open(SYSMON_DEVICE_PATH, O_RDONLY);
    if (fd < 0) {
        log_test_fail(name, "Cannot open device");
        return false;
    }

    struct sysmon_uptime_info uptime;
    std::memset(&uptime, 0, sizeof(uptime));

    if (ioctl(fd, SYSMON_GET_UPTIME, &uptime) < 0) {
        close(fd);
        log_test_fail(name, std::string("ioctl failed: ") + strerror(errno));
        return false;
    }
    close(fd);

    if (uptime.uptime_seconds == 0) {
        log_test_fail(name, "Uptime is 0 seconds");
        return false;
    }

    uint64_t hours = uptime.uptime_seconds / 3600;
    uint64_t mins = (uptime.uptime_seconds % 3600) / 60;
    uint64_t secs = uptime.uptime_seconds % 60;

    log_test_pass(name, "Uptime: " + std::to_string(hours) + "h " + std::to_string(mins) + "m " + std::to_string(secs) + "s");
    return true;
}

// ----------------------------------------------------------------------------
// Test 9: SYSMON_GET_LOAD ioctl
// ----------------------------------------------------------------------------
bool test_ioctl_load() {
    const std::string name = "Ioctl::GetLoadAvg";
    log_test_start(name);

    int fd = open(SYSMON_DEVICE_PATH, O_RDONLY);
    if (fd < 0) {
        log_test_fail(name, "Cannot open device");
        return false;
    }

    struct sysmon_load_info load;
    std::memset(&load, 0, sizeof(load));

    if (ioctl(fd, SYSMON_GET_LOAD, &load) < 0) {
        close(fd);
        log_test_fail(name, std::string("ioctl failed: ") + strerror(errno));
        return false;
    }
    close(fd);

    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2)
       << "1m: " << (load.load_1 / 100.0)
       << ", 5m: " << (load.load_5 / 100.0)
       << ", 15m: " << (load.load_15 / 100.0);

    log_test_pass(name, ss.str());
    return true;
}

// ----------------------------------------------------------------------------
// Test 10: SYSMON_GET_ALL ioctl
// ----------------------------------------------------------------------------
bool test_ioctl_all() {
    const std::string name = "Ioctl::GetAllStats";
    log_test_start(name);

    int fd = open(SYSMON_DEVICE_PATH, O_RDONLY);
    if (fd < 0) {
        log_test_fail(name, "Cannot open device");
        return false;
    }

    struct sysmon_all_stats all;
    std::memset(&all, 0, sizeof(all));

    if (ioctl(fd, SYSMON_GET_ALL, &all) < 0) {
        close(fd);
        log_test_fail(name, std::string("ioctl failed: ") + strerror(errno));
        return false;
    }
    close(fd);

    if (all.cpu.total == 0 || all.memory.total_ram == 0 || all.proc.process_count == 0) {
        log_test_fail(name, "Combined stats struct returned zeroed fields");
        return false;
    }

    log_test_pass(name, "Successfully retrieved monolithic stats snapshot");
    return true;
}

// ----------------------------------------------------------------------------
// Test 11: Error Handling - Invalid ioctl command code
// ----------------------------------------------------------------------------
bool test_error_invalid_ioctl() {
    const std::string name = "ErrorHandling::InvalidCommand";
    log_test_start(name);

    int fd = open(SYSMON_DEVICE_PATH, O_RDONLY);
    if (fd < 0) {
        log_test_fail(name, "Cannot open device");
        return false;
    }

    // Command with invalid magic number
    unsigned int bogus_cmd = _IOR('Z', 99, struct sysmon_cpu_info);
    struct sysmon_cpu_info info;

    int ret = ioctl(fd, bogus_cmd, &info);
    int saved_errno = errno;
    close(fd);

    if (ret != -1 || (saved_errno != ENOTTY && saved_errno != EINVAL)) {
        log_test_fail(name, "Expected ENOTTY/EINVAL error, got ret=" + std::to_string(ret) + ", errno=" + std::to_string(saved_errno));
        return false;
    }

    log_test_pass(name, "Correctly rejected unsupported ioctl with " + std::string(strerror(saved_errno)));
    return true;
}

// ----------------------------------------------------------------------------
// Test 12: Error Handling - Bad User Pointer (EFAULT check)
// ----------------------------------------------------------------------------
bool test_error_bad_pointer() {
    const std::string name = "ErrorHandling::BadUserPointer";
    log_test_start(name);

    int fd = open(SYSMON_DEVICE_PATH, O_RDONLY);
    if (fd < 0) {
        log_test_fail(name, "Cannot open device");
        return false;
    }

    // Null pointer should trigger -EFAULT in copy_to_user
    int ret = ioctl(fd, SYSMON_GET_CPU, nullptr);
    int saved_errno = errno;
    close(fd);

    if (ret != -1 || saved_errno != EFAULT) {
        log_test_fail(name, "Expected EFAULT, got ret=" + std::to_string(ret) + ", errno=" + std::to_string(saved_errno));
        return false;
    }

    log_test_pass(name, "Driver safely handled nullptr and returned EFAULT (" + std::string(strerror(saved_errno)) + ")");
    return true;
}

// ----------------------------------------------------------------------------
// Test 13: Stress and Concurrency Test
// ----------------------------------------------------------------------------
bool test_stress_and_concurrency() {
    const std::string name = "Stress::HighFrequencyPoll";
    log_test_start(name);

    const int iterations = 100;
    int fd = open(SYSMON_DEVICE_PATH, O_RDONLY);
    if (fd < 0) {
        log_test_fail(name, "Cannot open device");
        return false;
    }

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        struct sysmon_all_stats stats;
        if (ioctl(fd, SYSMON_GET_ALL, &stats) < 0) {
            close(fd);
            log_test_fail(name, "Failed on iteration " + std::to_string(i) + ": " + strerror(errno));
            return false;
        }
    }
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now() - start
    ).count();

    close(fd);

    double avg_us = static_cast<double>(elapsed) / iterations;
    std::ostringstream ss;
    ss << iterations << " ioctls completed in " << (elapsed / 1000.0) << " ms (avg "
       << std::fixed << std::setprecision(1) << avg_us << " µs/call)";

    log_test_pass(name, ss.str());
    return true;
}

// ----------------------------------------------------------------------------
// Main Test Runner
// ----------------------------------------------------------------------------
int main() {
    std::cout << ANSI_BOLD << "========================================================\n"
              << "       SysMon Kernel Driver Automated Test Suite        \n"
              << "========================================================" << ANSI_RESET << "\n\n";

    if (!test_device_node_exists()) {
        std::cerr << ANSI_RED << "\n[FATAL] Device node check failed. Is the kernel module loaded?\n"
                  << "Run 'make load' or 'sudo insmod kernel/sysmon_driver.ko' first.\n"
                  << ANSI_RESET << std::endl;
        return 1;
    }

    test_device_open_close();
    test_device_read();
    test_ioctl_driver_info();
    test_ioctl_cpu();
    test_ioctl_memory();
    test_ioctl_proc_count();
    test_ioctl_uptime();
    test_ioctl_load();
    test_ioctl_all();
    test_error_invalid_ioctl();
    test_error_bad_pointer();
    test_stress_and_concurrency();

    std::cout << "\n" << ANSI_BOLD << "========================================================\n"
              << "                   TEST SUMMARY                         \n"
              << "========================================================" << ANSI_RESET << "\n";
    std::cout << "  Passed : " << ANSI_GREEN << g_stats.passed << ANSI_RESET << "\n";
    std::cout << "  Failed : " << (g_stats.failed > 0 ? ANSI_RED : ANSI_RESET) << g_stats.failed << ANSI_RESET << "\n";
    std::cout << "  Skipped: " << g_stats.skipped << "\n";
    std::cout << "========================================================\n";

    return (g_stats.failed == 0) ? 0 : 1;
}
