# SysMon — UML State Machine Diagrams

This document illustrates state transitions for both the Linux kernel character device driver and the user-space monitoring dashboard.

---

## 1. Character Device Driver Lifecycle State Machine

```mermaid
stateDiagram-v2
    [*] --> UNLOADED : System Boot

    UNLOADED --> INITIALIZING : insmod sysmon_driver.ko
    state INITIALIZING {
        [*] --> AllocChrdev
        AllocChrdev --> InitCdev : Success
        AllocChrdev --> FailAlloc : Error
        InitCdev --> CreateClass : Success
        InitCdev --> FailCdev : Error
        CreateClass --> CreateDevice : Success
        CreateClass --> FailClass : Error
        CreateDevice --> [*] : Success
    }

    INITIALIZING --> LOADED_IDLE : Init Returns 0
    INITIALIZING --> UNLOADED : Init Fails (Clean up in reverse)

    LOADED_IDLE --> OPENED : sysmon_open() (open_count++)
    OPENED --> OPENED : Additional open() (open_count++)
    OPENED --> SERVICING_IOCTL : sysmon_ioctl()
    SERVICING_IOCTL --> OPENED : copy_to_user complete

    OPENED --> SERVICING_READ : sysmon_read()
    SERVICING_READ --> OPENED : snprintf & copy_to_user complete

    OPENED --> LOADED_IDLE : sysmon_release() [open_count == 0]
    LOADED_IDLE --> TEARDOWN : rmmod sysmon_driver

    state TEARDOWN {
        [*] --> DestroyDevice
        DestroyDevice --> DestroyClass
        DestroyClass --> DelCdev
        DelCdev --> UnregisterChrdev
        UnregisterChrdev --> [*]
    }

    TEARDOWN --> UNLOADED : sysmon_exit complete
    UNLOADED --> [*]
```

---

## 2. User-Space Application State Machine

```mermaid
stateDiagram-v2
    [*] --> INIT : Process Start

    state INIT {
        [*] --> ParseArgs : Read CLI Flags
        ParseArgs --> ConnectDriver : Flags Valid
        ParseArgs --> DisplayHelp : --help / Bad Flags
        ConnectDriver --> ErrorExit : open(/dev/sysmon) Failed
        ConnectDriver --> SetSignalHandlers : open(/dev/sysmon) OK
        SetSignalHandlers --> [*]
    }

    DisplayHelp --> [*] : Exit 0
    ErrorExit --> [*] : Exit 1

    INIT --> RUN_ONCE : --once Flag Specified
    RUN_ONCE --> EXITING : Render complete

    INIT --> RUN_LOOP : Continuous Mode (Default)

    state RUN_LOOP {
        [*] --> FetchTelemetry
        FetchTelemetry --> ComputeDelta : ioctl(SYSMON_GET_ALL) OK
        FetchTelemetry --> HandleError : ioctl Failed
        ComputeDelta --> RenderUI : Update CPU, RAM, Load
        RenderUI --> SleepInterval : Rendered ANSI Frame
        SleepInterval --> FetchTelemetry : Timer Expired
    }

    RUN_LOOP --> EXITING : Catch SIGINT / SIGTERM

    state EXITING {
        [*] --> RestoreCursor
        RestoreCursor --> CloseFileDescriptor
        CloseFileDescriptor --> [*]
    }

    EXITING --> [*] : Exit 0
```
