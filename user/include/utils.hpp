/*
 * utils.hpp - Utility functions declaration
 *
 * Provides helper functions for formatting values (bytes, time,
 * percentages) and terminal operations used across the application.
 */

#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <cstdint>

namespace utils {

/*
 * Format kilobytes into a human-readable string.
 * Examples: 1024 KB -> "1.00 MB", 1048576 KB -> "1.00 GB"
 */
std::string format_kb(uint64_t kilobytes);

/*
 * Format seconds into a human-readable uptime string.
 * Example: 9060 -> "2h 31m 0s"
 */
std::string format_uptime(uint64_t seconds);

/*
 * Format a double value as a percentage string.
 * Example: 34.567 -> "34.6 %"
 */
std::string format_percentage(double value);

/*
 * Format a fixed-point load average value (value * 100).
 * Example: 142 -> "1.42"
 */
std::string format_load(uint64_t load_x100);

/*
 * Clear the terminal screen using ANSI escape code.
 */
void clear_screen();

/*
 * Check if the sysmon kernel module is loaded.
 * Reads /proc/modules to determine module status.
 */
bool is_module_loaded();

/*
 * Check if /dev/sysmon device file exists.
 */
bool device_exists();

} // namespace utils

#endif /* UTILS_HPP */
