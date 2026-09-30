/*
 * main.cpp - SysMon User-Space Application Entry Point
 *
 * This is the main file for the Linux System Monitor CLI application.
 * It communicates with the SysMon kernel module through /dev/sysmon
 * to display real-time system resource information.
 *
 * Usage:
 *   sudo ./sysmon
 *
 * The kernel module (sysmon_driver.ko) must be loaded before running.
 */

#include "monitor.hpp"
#include "display.hpp"
#include "utils.hpp"

#include <iostream>
#include <csignal>
#include <cstdlib>
#include <unistd.h>

/* Global flag for continuous monitoring - set by signal handler */
static volatile sig_atomic_t g_running = 1;

/*
 * Signal handler for SIGINT (Ctrl+C).
 * Used to gracefully stop continuous monitoring mode.
 */
static void sigint_handler(int /*signum*/)
{
    g_running = 0;
}

/*
 * run_continuous_monitoring() - Live system stats display
 *
 * Refreshes system statistics every ~2.5 seconds until the user
 * presses Ctrl+C. CPU usage sampling takes ~500ms per update.
 */
static void run_continuous_monitoring(SystemMonitor &monitor)
{
    /* Install signal handler for graceful Ctrl+C exit */
    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, nullptr);

    g_running = 1;

    Display::show_info("Starting continuous monitoring. Press Ctrl+C to stop.");
    sleep(1);

    while (g_running) {
        utils::clear_screen();

        /* Calculate CPU usage (takes ~500ms due to dual-snapshot) */
        double cpu_usage = monitor.calculate_cpu_usage();
        if (cpu_usage < 0.0) {
            Display::show_error("Failed to read CPU data: " +
                                monitor.get_last_error());
            break;
        }

        /* Get all other statistics */
        struct sysmon_all_stats stats;
        if (!monitor.get_all_stats(stats)) {
            Display::show_error("Failed to read system data: " +
                                monitor.get_last_error());
            break;
        }

        /* Display combined statistics */
        Display::show_all_stats(stats, cpu_usage);
        std::cout << "  Press Ctrl+C to stop.\n" << std::flush;

        /* Sleep for 2 seconds between updates */
        for (int i = 0; i < 20 && g_running; i++) {
            usleep(100000);  /* 100ms intervals for responsive Ctrl+C */
        }
    }

    /* Restore default signal handler */
    signal(SIGINT, SIG_DFL);
    std::cout << "\n  Monitoring stopped.\n\n";
}

/*
 * handle_choice() - Process a single menu selection
 *
 * Returns false when the user selects Exit (choice 9).
 */
static bool handle_choice(int choice, SystemMonitor &monitor)
{
    switch (choice) {

    case 1: {
        /* Show CPU Usage */
        Display::show_info("Measuring CPU usage (please wait ~0.5s)...");
        double usage = monitor.calculate_cpu_usage();
        if (usage >= 0.0) {
            Display::show_cpu_usage(usage);
        } else {
            Display::show_error(monitor.get_last_error());
        }
        break;
    }

    case 2: {
        /* Show Memory Usage */
        struct sysmon_memory_info mem;
        if (monitor.get_memory_info(mem)) {
            Display::show_memory_info(mem);
        } else {
            Display::show_error(monitor.get_last_error());
        }
        break;
    }

    case 3: {
        /* Show Process Count */
        struct sysmon_proc_info proc;
        if (monitor.get_proc_info(proc)) {
            Display::show_process_info(proc);
        } else {
            Display::show_error(monitor.get_last_error());
        }
        break;
    }

    case 4: {
        /* Show System Uptime */
        struct sysmon_uptime_info uptime;
        if (monitor.get_uptime_info(uptime)) {
            Display::show_uptime(uptime);
        } else {
            Display::show_error(monitor.get_last_error());
        }
        break;
    }

    case 5: {
        /* Show Load Average */
        struct sysmon_load_info load;
        if (monitor.get_load_info(load)) {
            Display::show_load_average(load);
        } else {
            Display::show_error(monitor.get_last_error());
        }
        break;
    }

    case 6: {
        /* Show Complete System Statistics */
        Display::show_info("Collecting all statistics (please wait ~0.5s)...");
        double cpu_usage = monitor.calculate_cpu_usage();
        struct sysmon_all_stats stats;

        if (cpu_usage >= 0.0 && monitor.get_all_stats(stats)) {
            Display::show_all_stats(stats, cpu_usage);
        } else {
            Display::show_error(monitor.get_last_error());
        }
        break;
    }

    case 7:
        /* Continuous Monitoring */
        run_continuous_monitoring(monitor);
        break;

    case 8: {
        /* Driver Information */
        struct sysmon_driver_info dinfo;
        if (monitor.get_driver_info(dinfo)) {
            Display::show_driver_info(dinfo);
        } else {
            Display::show_error(monitor.get_last_error());
        }
        break;
    }

    case 9:
        /* Exit */
        std::cout << "\n  Exiting SysMon. Goodbye!\n\n";
        return false;

    default:
        Display::show_error("Invalid choice. Please enter a number (1-9).");
        break;
    }

    return true;
}

static void print_usage(const char* prog_name)
{
    std::cout << "\nUsage: " << prog_name << " [OPTIONS]\n\n"
              << "Options:\n"
              << "  -h, --help        Show this help message and exit\n"
              << "  -v, --version     Show version information and exit\n"
              << "  -o, --once        Read and display all statistics once, then exit\n"
              << "  -c, --continuous  Start continuous real-time monitoring mode\n\n"
              << "If no options are provided, SysMon launches in interactive menu mode.\n"
              << "Requires kernel module loaded: sudo insmod kernel/sysmon_driver.ko\n\n";
}

/* ========== Main Function ========== */

int main(int argc, char *argv[])
{
    /* Parse command-line options before checking device node */
    bool once_mode = false;
    bool continuous_mode = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            return 0;
        } else if (arg == "-v" || arg == "--version") {
            std::cout << "SysMon v" << SYSMON_DRIVER_VERSION
                      << " (Linux System Resource Monitor)\n";
            return 0;
        } else if (arg == "-o" || arg == "--once") {
            once_mode = true;
        } else if (arg == "-c" || arg == "--continuous") {
            continuous_mode = true;
        } else {
            std::cerr << "Unknown option: " << arg << "\n";
            print_usage(argv[0]);
            return 1;
        }
    }

    /* Pre-flight checks */
    if (!utils::device_exists()) {
        Display::show_error(std::string(SYSMON_DEVICE_PATH) + " not found.");
        std::cerr << "  Please load the kernel module first:\n\n"
                  << "    cd kernel\n"
                  << "    make\n"
                  << "    sudo insmod sysmon_driver.ko\n\n"
                  << "  Then verify:\n"
                  << "    ls -l /dev/sysmon\n"
                  << "    dmesg | tail\n\n";
        return 1;
    }

    /* Open the SysMon device */
    SystemMonitor monitor;
    if (!monitor.open_device()) {
        Display::show_error(monitor.get_last_error());

        if (!utils::is_module_loaded()) {
            std::cerr << "  The sysmon_driver kernel module is not loaded.\n"
                      << "  Load it with: sudo insmod kernel/sysmon_driver.ko\n\n";
        } else {
            std::cerr << "  The device exists but cannot be opened.\n"
                      << "  Try running with sudo or adjust permissions:\n"
                      << "    sudo chmod 666 /dev/sysmon\n\n";
        }
        return 1;
    }

    if (once_mode) {
        struct sysmon_all_stats stats;
        if (!monitor.get_all_stats(stats)) {
            Display::show_error(monitor.get_last_error());
            return 1;
        }
        double cpu_usage = monitor.calculate_cpu_usage();
        Display::show_all_stats(stats, cpu_usage);
        return 0;
    }

    if (continuous_mode) {
        run_continuous_monitoring(monitor);
        return 0;
    }

    Display::show_success("Connected to " +
                          std::string(SYSMON_DEVICE_PATH) + " successfully.");

    /* Main menu loop */
    int choice;
    bool running = true;

    while (running) {
        Display::show_menu();
        std::cout << "  Enter choice: ";

        if (!(std::cin >> choice)) {
            /* Handle non-numeric input */
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            Display::show_error("Invalid input. Please enter a number (1-9).");
            continue;
        }

        running = handle_choice(choice, monitor);
    }

    /* Device is closed by SystemMonitor destructor (RAII) */
    return 0;
}
