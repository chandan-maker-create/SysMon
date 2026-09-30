/*
 * utils.cpp - Utility functions implementation
 *
 * Provides formatting helpers and system checks used throughout
 * the SysMon user-space application.
 */

#include "utils.hpp"

#include <sstream>
#include <iomanip>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <sys/stat.h>

namespace utils {

/*
 * Convert kilobytes to a human-readable string with appropriate unit.
 *
 * Automatically selects KB, MB, or GB based on magnitude.
 * Examples:
 *   512       -> "512 KB"
 *   1024      -> "1.00 MB"
 *   1572864   -> "1.50 GB"
 */
std::string format_kb(uint64_t kilobytes)
{
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);

    if (kilobytes >= 1048576) {
        /* >= 1 GB: display in GB */
        double gb = static_cast<double>(kilobytes) / 1048576.0;
        oss << gb << " GB";
    } else if (kilobytes >= 1024) {
        /* >= 1 MB: display in MB */
        double mb = static_cast<double>(kilobytes) / 1024.0;
        oss << mb << " MB";
    } else {
        oss << kilobytes << " KB";
    }

    return oss.str();
}

/*
 * Convert seconds to a human-readable uptime string.
 *
 * Format: "Xd Yh Zm Ws" (days shown only if > 0)
 * Examples:
 *   60    -> "1m 0s"
 *   3661  -> "1h 1m 1s"
 *   90061 -> "1d 1h 1m 1s"
 */
std::string format_uptime(uint64_t seconds)
{
    uint64_t days    = seconds / 86400;
    uint64_t hours   = (seconds % 86400) / 3600;
    uint64_t minutes = (seconds % 3600) / 60;
    uint64_t secs    = seconds % 60;

    std::ostringstream oss;

    if (days > 0) {
        oss << days << "d ";
    }
    if (days > 0 || hours > 0) {
        oss << hours << "h ";
    }
    oss << minutes << "m " << secs << "s";

    return oss.str();
}

/*
 * Format a double value as a percentage string with one decimal place.
 */
std::string format_percentage(double value)
{
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << value << " %";
    return oss.str();
}

/*
 * Format a fixed-point load average (value * 100) as a decimal string.
 * Example: 142 -> "1.42"
 */
std::string format_load(uint64_t load_x100)
{
    std::ostringstream oss;
    oss << (load_x100 / 100) << "."
        << std::setfill('0') << std::setw(2) << (load_x100 % 100);
    return oss.str();
}

/*
 * Clear the terminal screen using ANSI escape sequence.
 */
void clear_screen()
{
    /* Move cursor to top-left and clear the screen */
    std::cout << "\033[2J\033[H" << std::flush;
}

/*
 * Check if the sysmon_driver kernel module is currently loaded.
 *
 * Reads /proc/modules and searches for "sysmon_driver" entry.
 */
bool is_module_loaded()
{
    std::ifstream proc_modules("/proc/modules");
    if (!proc_modules.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(proc_modules, line)) {
        if (line.find("sysmon_driver") != std::string::npos) {
            return true;
        }
    }

    return false;
}

/*
 * Check if the /dev/sysmon device file exists.
 */
bool device_exists()
{
    struct stat st;
    return (stat("/dev/sysmon", &st) == 0);
}

} // namespace utils
