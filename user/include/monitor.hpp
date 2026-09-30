/*
 * monitor.hpp - SystemMonitor class declaration
 *
 * Provides an interface to communicate with the SysMon kernel module
 * through /dev/sysmon using ioctl system calls.
 *
 * This class follows RAII principles: the device file descriptor
 * is managed by the constructor/destructor.
 */

#ifndef MONITOR_HPP
#define MONITOR_HPP

#include "sysmon_ioctl.h"
#include <string>

/*
 * SystemMonitor - Interface to the SysMon character device driver
 *
 * Usage:
 *   SystemMonitor monitor;
 *   if (!monitor.open_device()) { handle error }
 *   double cpu = monitor.calculate_cpu_usage();
 *   monitor.close_device();  // or let destructor handle it
 */
class SystemMonitor {
public:
    SystemMonitor();
    ~SystemMonitor();

    /* Prevent copying (file descriptor ownership) */
    SystemMonitor(const SystemMonitor&) = delete;
    SystemMonitor& operator=(const SystemMonitor&) = delete;

    /* Device connection management */
    bool open_device();
    void close_device();
    bool is_open() const;

    /* Data retrieval methods (via ioctl) */
    bool get_cpu_info(struct sysmon_cpu_info &info) const;
    bool get_memory_info(struct sysmon_memory_info &info) const;
    bool get_proc_info(struct sysmon_proc_info &info) const;
    bool get_uptime_info(struct sysmon_uptime_info &info) const;
    bool get_load_info(struct sysmon_load_info &info) const;
    bool get_all_stats(struct sysmon_all_stats &stats) const;
    bool get_driver_info(struct sysmon_driver_info &info) const;

    /*
     * Calculate CPU usage percentage.
     * Takes two snapshots separated by a short delay (~500ms)
     * and computes: usage = (delta_busy / delta_total) * 100
     */
    double calculate_cpu_usage() const;

    /* Get the last error message */
    std::string get_last_error() const;

private:
    int fd_;                    /* File descriptor for /dev/sysmon */
    mutable std::string last_error_;  /* Last error message      */

    /* Helper to set error message */
    void set_error(const std::string &msg) const;
};

#endif /* MONITOR_HPP */
