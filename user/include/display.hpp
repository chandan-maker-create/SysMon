/*
 * display.hpp - Display class declaration
 *
 * Handles all output formatting and presentation for the SysMon
 * CLI application, including the menu, statistics display, and
 * error messages.
 *
 * Uses ANSI escape codes for colored terminal output.
 */

#ifndef DISPLAY_HPP
#define DISPLAY_HPP

#include "sysmon_ioctl.h"
#include <string>

/*
 * Display - Static utility class for formatted terminal output
 *
 * All methods are static since no state is maintained.
 * Uses ANSI color codes for visually appealing output.
 */
class Display {
public:
    /* Menu and headers */
    static void show_menu();
    static void show_header(const std::string &title);
    static void show_separator();

    /* Individual statistics display */
    static void show_cpu_usage(double usage_percent);
    static void show_memory_info(const struct sysmon_memory_info &info);
    static void show_process_info(const struct sysmon_proc_info &info);
    static void show_uptime(const struct sysmon_uptime_info &info);
    static void show_load_average(const struct sysmon_load_info &info);

    /* Combined statistics display */
    static void show_all_stats(const struct sysmon_all_stats &stats,
                               double cpu_usage);

    /* Driver information display */
    static void show_driver_info(const struct sysmon_driver_info &info);

    /* Error and informational messages */
    static void show_error(const std::string &message);
    static void show_info(const std::string &message);
    static void show_success(const std::string &message);

private:
    /* ANSI color code constants */
    static const char* RESET;
    static const char* BOLD;
    static const char* RED;
    static const char* GREEN;
    static const char* YELLOW;
    static const char* BLUE;
    static const char* CYAN;
    static const char* WHITE;

    /* Prevent instantiation */
    Display() = delete;
};

#endif /* DISPLAY_HPP */
