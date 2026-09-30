/*
 * monitor.cpp - SystemMonitor class implementation
 *
 * Implements communication with the SysMon kernel module through
 * the /dev/sysmon character device interface.
 */

#include "monitor.hpp"

#include <fcntl.h>          /* open()           */
#include <unistd.h>         /* close(), usleep() */
#include <sys/ioctl.h>      /* ioctl()          */
#include <cerrno>           /* errno            */
#include <cstring>          /* strerror()       */

/* ========== Constructor / Destructor ========== */

SystemMonitor::SystemMonitor()
    : fd_(-1)
{
}

SystemMonitor::~SystemMonitor()
{
    close_device();
}

/* ========== Device Connection ========== */

/*
 * Open the /dev/sysmon device for reading.
 * Returns true on success, false on failure (check get_last_error()).
 */
bool SystemMonitor::open_device()
{
    if (fd_ >= 0) {
        return true;  /* Already open */
    }

    fd_ = open(SYSMON_DEVICE_PATH, O_RDONLY);
    if (fd_ < 0) {
        if (errno == ENOENT) {
            set_error("Device " + std::string(SYSMON_DEVICE_PATH) +
                      " not found. Is the kernel module loaded?");
        } else if (errno == EACCES) {
            set_error("Permission denied. Try running with sudo or "
                      "adjust device permissions.");
        } else {
            set_error("Failed to open " + std::string(SYSMON_DEVICE_PATH) +
                      ": " + strerror(errno));
        }
        return false;
    }

    return true;
}

/*
 * Close the device file descriptor.
 */
void SystemMonitor::close_device()
{
    if (fd_ >= 0) {
        close(fd_);
        fd_ = -1;
    }
}

/*
 * Check if the device is currently open.
 */
bool SystemMonitor::is_open() const
{
    return fd_ >= 0;
}

/* ========== ioctl Data Retrieval ========== */

bool SystemMonitor::get_cpu_info(struct sysmon_cpu_info &info) const
{
    if (!is_open()) {
        set_error("Device not open");
        return false;
    }

    if (ioctl(fd_, SYSMON_GET_CPU, &info) < 0) {
        set_error("ioctl SYSMON_GET_CPU failed: " +
                  std::string(strerror(errno)));
        return false;
    }

    return true;
}

bool SystemMonitor::get_memory_info(struct sysmon_memory_info &info) const
{
    if (!is_open()) {
        set_error("Device not open");
        return false;
    }

    if (ioctl(fd_, SYSMON_GET_MEMORY, &info) < 0) {
        set_error("ioctl SYSMON_GET_MEMORY failed: " +
                  std::string(strerror(errno)));
        return false;
    }

    return true;
}

bool SystemMonitor::get_proc_info(struct sysmon_proc_info &info) const
{
    if (!is_open()) {
        set_error("Device not open");
        return false;
    }

    if (ioctl(fd_, SYSMON_GET_PROC_COUNT, &info) < 0) {
        set_error("ioctl SYSMON_GET_PROC_COUNT failed: " +
                  std::string(strerror(errno)));
        return false;
    }

    return true;
}

bool SystemMonitor::get_uptime_info(struct sysmon_uptime_info &info) const
{
    if (!is_open()) {
        set_error("Device not open");
        return false;
    }

    if (ioctl(fd_, SYSMON_GET_UPTIME, &info) < 0) {
        set_error("ioctl SYSMON_GET_UPTIME failed: " +
                  std::string(strerror(errno)));
        return false;
    }

    return true;
}

bool SystemMonitor::get_load_info(struct sysmon_load_info &info) const
{
    if (!is_open()) {
        set_error("Device not open");
        return false;
    }

    if (ioctl(fd_, SYSMON_GET_LOAD, &info) < 0) {
        set_error("ioctl SYSMON_GET_LOAD failed: " +
                  std::string(strerror(errno)));
        return false;
    }

    return true;
}

bool SystemMonitor::get_all_stats(struct sysmon_all_stats &stats) const
{
    if (!is_open()) {
        set_error("Device not open");
        return false;
    }

    if (ioctl(fd_, SYSMON_GET_ALL, &stats) < 0) {
        set_error("ioctl SYSMON_GET_ALL failed: " +
                  std::string(strerror(errno)));
        return false;
    }

    return true;
}

bool SystemMonitor::get_driver_info(struct sysmon_driver_info &info) const
{
    if (!is_open()) {
        set_error("Device not open");
        return false;
    }

    if (ioctl(fd_, SYSMON_GET_DRIVER_INFO, &info) < 0) {
        set_error("ioctl SYSMON_GET_DRIVER_INFO failed: " +
                  std::string(strerror(errno)));
        return false;
    }

    return true;
}

/* ========== CPU Usage Calculation ========== */

/*
 * Calculate CPU usage by taking two snapshots 500ms apart.
 *
 * CPU Usage % = (delta_busy / delta_total) * 100
 *
 * where:
 *   busy  = user + nice + system + irq + softirq + steal
 *   total = busy + idle + iowait
 *
 * Returns -1.0 on error.
 */
double SystemMonitor::calculate_cpu_usage() const
{
    struct sysmon_cpu_info snapshot1, snapshot2;

    /* Take first snapshot */
    if (!get_cpu_info(snapshot1)) {
        return -1.0;
    }

    /* Wait 500 milliseconds */
    usleep(500000);

    /* Take second snapshot */
    if (!get_cpu_info(snapshot2)) {
        return -1.0;
    }

    /* Calculate deltas */
    uint64_t delta_total = snapshot2.total - snapshot1.total;
    uint64_t delta_busy  = snapshot2.busy  - snapshot1.busy;

    if (delta_total == 0) {
        return 0.0;
    }

    return (static_cast<double>(delta_busy) / static_cast<double>(delta_total)) * 100.0;
}

/* ========== Error Handling ========== */

std::string SystemMonitor::get_last_error() const
{
    return last_error_;
}

void SystemMonitor::set_error(const std::string &msg) const
{
    last_error_ = msg;
}
