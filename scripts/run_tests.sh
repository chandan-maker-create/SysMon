#!/usr/bin/env bash
# ==============================================================================
# SysMon - Automated Test Execution Script
# Ensures driver is loaded, compiles test harness, and executes the suite.
# ==============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
TEST_BIN="${ROOT_DIR}/tests/test_ioctl"

echo "=========================================="
echo "SysMon: Running Automated Verification"
echo "=========================================="

# Build test runner if needed
if [ ! -f "${TEST_BIN}" ]; then
    echo "[+] Compiling test suite..."
    make -C "${ROOT_DIR}/tests"
fi

# Ensure driver is loaded
if ! lsmod | grep -q "^sysmon_driver\b"; then
    echo "[*] SysMon driver is not currently loaded. Attempting to load..."
    if [ "$(id -u)" -eq 0 ]; then
        bash "${SCRIPT_DIR}/load_driver.sh"
    elif command -v sudo >/dev/null 2>&1; then
        sudo bash "${SCRIPT_DIR}/load_driver.sh"
    else
        echo "[-] Cannot load driver without root privileges." >&2
        exit 1
    fi
fi

# Execute test suite
echo "[+] Executing test runner..."
"${TEST_BIN}"
