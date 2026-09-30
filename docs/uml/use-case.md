# SysMon — UML Use Case Diagram

This document models the functional interactions between primary actors and the SysMon monitoring ecosystem.

```mermaid
flowchart TD
    subgraph Actors
        User["Standard User"]
        Admin["System Administrator (Root)"]
        Kernel["Linux Kernel Core"]
    end

    subgraph "SysMon Boundary"
        UC1(["Load Character Driver"])
        UC2(["Unload Character Driver"])
        UC3(["Inspect Diagnostic Text (cat /dev/sysmon)"])
        UC4(["Run Interactive Dashboard (sysmon)"])
        UC5(["Query Specific Subsystems (--cpu, --mem, --proc)"])
        UC6(["Execute Automated Test Suite"])
        UC7(["Serve Binary ioctl Requests"])
        UC8(["Traverse Kernel Telemetry (CPU, Mem, Sched)"])
    end

    Admin --> UC1
    Admin --> UC2
    Admin --> UC4
    Admin --> UC6

    User --> UC3
    User --> UC4
    User --> UC5

    UC4 --> UC7
    UC5 --> UC7
    UC6 --> UC7
    UC3 -.->|Invokes read fop| UC8
    UC7 -->|Invokes unlocked_ioctl fop| UC8
    UC8 --> Kernel
```

## Use Case Descriptions

| Use Case | Primary Actor | Description |
| :--- | :--- | :--- |
| **Load Character Driver** | System Administrator | Inserts `sysmon_driver.ko` into the running Linux kernel using `insmod`. Registers character major/minor numbers and creates `/dev/sysmon`. |
| **Unload Character Driver** | System Administrator | Removes `sysmon_driver` module using `rmmod`, ensuring no open file handles remain, and cleans up device nodes and classes. |
| **Inspect Diagnostic Text** | Standard User | Reads from `/dev/sysmon` using `cat` to inspect human-readable system diagnostic summaries. |
| **Run Interactive Dashboard** | User / Admin | Launches the C++ terminal UI displaying live gauges for CPU, memory, uptime, and load averages. |
| **Execute Automated Test Suite** | Developer / QA | Runs `test_ioctl` to validate all kernel entry points, error handling, and performance metrics. |
| **Serve Binary ioctl Requests** | Device Driver | Receives `ioctl` requests, collects kernel stats, and copies binary structures to user space via `copy_to_user()`. |
