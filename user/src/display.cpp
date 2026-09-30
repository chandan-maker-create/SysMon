/*
 * display.cpp - Display class implementation
 *
 * Handles all formatted terminal output for the SysMon CLI application.
 * Uses ANSI escape codes for colored, visually structured output.
 */

#include "display.hpp"
#include "utils.hpp"

#include <iostream>
#include <iomanip>
#include <sstream>

/* ========== ANSI Color Constants ========== */

const char* Display::RESET  = "\033[0m";
const char* Display::BOLD   = "\033[1m";
const char* Display::RED    = "\033[31m";
const char* Display::GREEN  = "\033[32m";
const char* Display::YELLOW = "\033[33m";
const char* Display::BLUE   = "\033[34m";
const char* Display::CYAN   = "\033[36m";
const char* Display::WHITE  = "\033[37m";

/* ========== Menu and Headers ========== */

void Display::show_menu()
{
    std::cout << "\n"
              << BOLD << CYAN
              << "========================================\n"
              << "        LINUX SYSTEM MONITOR\n"
              << "========================================\n"
              << RESET
              << "\n"
              << "  1. Show CPU Usage\n"
              << "  2. Show Memory Usage\n"
              << "  3. Show Process Count\n"
              << "  4. Show System Uptime\n"
              << "  5. Show Load Average\n"
              << "  6. Show Complete System Statistics\n"
              << "  7. Continuous Monitoring\n"
              << "  8. Driver Information\n"
              << "  9. Exit\n"
              << "\n";
}

void Display::show_header(const std::string &title)
{
    std::cout << "\n"
              << BOLD << CYAN
              << "========================================\n"
              << "  " << title << "\n"
              << "========================================\n"
              << RESET;
}

void Display::show_separator()
{
    std::cout << CYAN << "----------------------------------------" << RESET << "\n";
}

/* ========== Individual Statistics ========== */

void Display::show_cpu_usage(double usage_percent)
{
    show_header("CPU USAGE");

    /* Color based on usage level */
    const char* color = GREEN;
    if (usage_percent > 80.0) color = RED;
    else if (usage_percent > 50.0) color = YELLOW;

    std::cout << "\n"
              << "  CPU Usage      : " << BOLD << color
              << std::fixed << std::setprecision(1)
              << usage_percent << " %" << RESET << "\n\n";
}

void Display::show_memory_info(const struct sysmon_memory_info &info)
{
    show_header("MEMORY USAGE");

    double usage_pct = 0.0;
    if (info.total_ram > 0) {
        usage_pct = (static_cast<double>(info.used_ram) /
                     static_cast<double>(info.total_ram)) * 100.0;
    }

    const char* color = GREEN;
    if (usage_pct > 80.0) color = RED;
    else if (usage_pct > 60.0) color = YELLOW;

    std::cout << "\n"
              << "  Memory Usage   : " << BOLD << color
              << std::fixed << std::setprecision(1)
              << usage_pct << " %" << RESET << "\n"
              << "  Total RAM      : " << utils::format_kb(info.total_ram) << "\n"
              << "  Used RAM       : " << utils::format_kb(info.used_ram) << "\n"
              << "  Available RAM  : " << utils::format_kb(info.available_ram) << "\n"
              << "  Free RAM       : " << utils::format_kb(info.free_ram) << "\n"
              << "  Buffers        : " << utils::format_kb(info.buffers) << "\n"
              << "  Cached         : " << utils::format_kb(info.cached) << "\n"
              << "  Total Swap     : " << utils::format_kb(info.total_swap) << "\n"
              << "  Free Swap      : " << utils::format_kb(info.free_swap) << "\n"
              << "\n";
}

void Display::show_process_info(const struct sysmon_proc_info &info)
{
    show_header("PROCESS INFORMATION");

    std::cout << "\n"
              << "  Total Processes   : " << BOLD << WHITE
              << info.process_count << RESET << "\n"
              << "  Running Processes : " << BOLD << GREEN
              << info.running_count << RESET << "\n"
              << "\n";
}

void Display::show_uptime(const struct sysmon_uptime_info &info)
{
    show_header("SYSTEM UPTIME");

    std::cout << "\n"
              << "  System Uptime  : " << BOLD << WHITE
              << utils::format_uptime(info.uptime_seconds) << RESET << "\n"
              << "  Total Seconds  : " << info.uptime_seconds << "\n"
              << "\n";
}

void Display::show_load_average(const struct sysmon_load_info &info)
{
    show_header("LOAD AVERAGE");

    std::cout << "\n"
              << "  1  minute      : " << BOLD << WHITE
              << utils::format_load(info.load_1) << RESET << "\n"
              << "  5  minutes     : " << BOLD << WHITE
              << utils::format_load(info.load_5) << RESET << "\n"
              << "  15 minutes     : " << BOLD << WHITE
              << utils::format_load(info.load_15) << RESET << "\n"
              << "\n";
}

/* ========== Combined Statistics ========== */

void Display::show_all_stats(const struct sysmon_all_stats &stats,
                             double cpu_usage)
{
    show_header("LIVE SYSTEM MONITOR");

    /* CPU usage color */
    const char* cpu_color = GREEN;
    if (cpu_usage > 80.0) cpu_color = RED;
    else if (cpu_usage > 50.0) cpu_color = YELLOW;

    /* Memory usage */
    double mem_pct = 0.0;
    if (stats.memory.total_ram > 0) {
        mem_pct = (static_cast<double>(stats.memory.used_ram) /
                   static_cast<double>(stats.memory.total_ram)) * 100.0;
    }
    const char* mem_color = GREEN;
    if (mem_pct > 80.0) mem_color = RED;
    else if (mem_pct > 60.0) mem_color = YELLOW;

    std::cout << "\n"
              << "  CPU Usage      : " << BOLD << cpu_color
              << std::fixed << std::setprecision(1) << cpu_usage << " %"
              << RESET << "\n"
              << "  Memory Usage   : " << BOLD << mem_color
              << std::fixed << std::setprecision(1) << mem_pct << " %"
              << RESET << "\n"
              << "  Total RAM      : " << utils::format_kb(stats.memory.total_ram) << "\n"
              << "  Used RAM       : " << utils::format_kb(stats.memory.used_ram) << "\n"
              << "  Available RAM  : " << utils::format_kb(stats.memory.available_ram) << "\n"
              << "  Processes      : " << BOLD << WHITE
              << stats.proc.process_count << RESET
              << " (" << stats.proc.running_count << " running)\n"
              << "  System Uptime  : " << utils::format_uptime(stats.uptime.uptime_seconds) << "\n"
              << "  Load Average   : "
              << utils::format_load(stats.load.load_1) << "  "
              << utils::format_load(stats.load.load_5) << "  "
              << utils::format_load(stats.load.load_15) << "\n"
              << "\n";
}

/* ========== Driver Information ========== */

void Display::show_driver_info(const struct sysmon_driver_info &info)
{
    show_header("DRIVER INFORMATION");

    std::cout << "\n"
              << "  Driver Name    : " << BOLD << WHITE
              << info.name << RESET << "\n"
              << "  Device         : " << SYSMON_DEVICE_PATH << "\n"
              << "  Interface      : Character Device\n"
              << "  Status         : " << BOLD << GREEN << "Loaded" << RESET << "\n"
              << "  Interface Ver. : " << info.version << "\n"
              << "  Major Number   : " << info.major << "\n"
              << "  Minor Number   : " << info.minor << "\n"
              << "\n";
}

/* ========== Messages ========== */

void Display::show_error(const std::string &message)
{
    std::cerr << "\n" << BOLD << RED << "  ERROR: " << RESET
              << RED << message << RESET << "\n\n";
}

void Display::show_info(const std::string &message)
{
    std::cout << "\n" << BOLD << CYAN << "  INFO: " << RESET
              << message << "\n\n";
}

void Display::show_success(const std::string &message)
{
    std::cout << "\n" << BOLD << GREEN << "  OK: " << RESET
              << message << "\n\n";
}
