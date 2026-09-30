# SysMon — UML Sequence Diagrams

This document models the interaction timelines for device initialization, telemetry sampling, and shutdown.

---

## 1. Monitor Polling Loop Sequence

```mermaid
sequenceDiagram
    autonumber
    actor User
    participant Main as main.cpp
    participant SM as SystemMonitor
    participant VFS as Linux VFS (/dev/sysmon)
    participant Driver as sysmon_driver (Kernel)
    participant Subsys as Kernel Subsystems (kcpustat/mm)
    participant DM as DisplayManager

    User->>Main: Run ./sysmon --interval 1
    Main->>SM: SystemMonitor()
    SM->>VFS: open("/dev/sysmon", O_RDONLY)
    VFS->>Driver: sysmon_open(inode, file)
    Driver-->>VFS: return 0
    VFS-->>SM: return fd

    loop Every 1 second until SIGINT
        Main->>SM: get_all_stats(stats)
        SM->>VFS: ioctl(fd, SYSMON_GET_ALL, &stats)
        VFS->>Driver: sysmon_ioctl(cmd=SYSMON_GET_ALL, arg)
        Driver->>Driver: mutex_lock(&sysmon_mutex)
        Driver->>Subsys: collect_cpu_info()
        Driver->>Subsys: collect_memory_info()
        Driver->>Subsys: collect_load_info()
        Driver->>Subsys: collect_proc_info()
        Driver->>Driver: copy_to_user(arg, &stats, sizeof)
        Driver->>Driver: mutex_unlock(&sysmon_mutex)
        Driver-->>VFS: return 0
        VFS-->>SM: return 0
        SM-->>Main: return true

        Main->>SM: calculate_cpu_usage(stats.cpu)
        SM-->>Main: return cpu_percent

        Main->>DM: render_dashboard(stats, cpu_percent)
        DM-->>User: Refresh ANSI gauges on terminal
    end

    User->>Main: Send SIGINT (Ctrl+C)
    Main->>DM: show_cursor()
    Main->>SM: ~SystemMonitor()
    SM->>VFS: close(fd)
    VFS->>Driver: sysmon_release(inode, file)
    Driver-->>VFS: return 0
    Main-->>User: Process exit 0
```

---

## 2. Kernel Module Lifecycle Sequence

```mermaid
sequenceDiagram
    autonumber
    actor Admin
    participant Shell as Bash Shell / Insmod
    participant Core as Kernel Core / Syscall
    participant Driver as sysmon_driver
    participant Udev as Devtmpfs / Udev

    Admin->>Shell: sudo insmod sysmon_driver.ko
    Shell->>Core: sys_finit_module()
    Core->>Driver: sysmon_init()
    Driver->>Core: alloc_chrdev_region()
    Core-->>Driver: Major=236, Minor=0
    Driver->>Core: cdev_init() & cdev_add()
    Driver->>Core: class_create("sysmon_class")
    Driver->>Core: device_create(..., "sysmon")
    Core->>Udev: KOBJECT_ADD event
    Udev-->>Udev: Create node /dev/sysmon (c 236 0)
    Driver-->>Core: return 0 (Success)
    Core-->>Shell: Loaded module

    Note over Admin,Driver: Module is Active and Serving Requests

    Admin->>Shell: sudo rmmod sysmon_driver
    Shell->>Core: sys_delete_module("sysmon_driver")
    Core->>Driver: sysmon_exit()
    Driver->>Core: device_destroy()
    Driver->>Core: class_destroy()
    Driver->>Core: cdev_del()
    Driver->>Core: unregister_chrdev_region()
    Core->>Udev: Remove /dev/sysmon
    Driver-->>Core: Completed cleanup
    Core-->>Shell: Module unloaded
```
