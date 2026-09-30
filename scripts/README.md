# SysMon Helper Scripts

This directory provides shell automation scripts for building, managing, and validating the SysMon project.

## Scripts Overview

| Script | Purpose | Execution |
| :--- | :--- | :--- |
| `build.sh` | Validates host toolchain dependencies and compiles all components (kernel module, user app, tests). | `./scripts/build.sh` |
| `load_driver.sh` | Inserts the kernel module (`insmod sysmon_driver.ko`), waits for devtmpfs, and applies appropriate permissions to `/dev/sysmon`. | `sudo ./scripts/load_driver.sh` |
| `unload_driver.sh` | Verifies no processes are holding `/dev/sysmon` open, then unloads the module (`rmmod sysmon_driver`). | `sudo ./scripts/unload_driver.sh` |
| `run_tests.sh` | Builds the test suite, ensures the driver is inserted, and runs all 13 test scenarios. | `sudo ./scripts/run_tests.sh` |

All scripts are configured with `set -euo pipefail` to ensure robust error handling and fail-safe execution.
