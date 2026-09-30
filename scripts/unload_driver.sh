#!/usr/bin/env bash
# ==============================================================================
# SysMon - Driver Unload Script
# Safely unloads the sysmon character device driver kernel module.
# ==============================================================================

set -euo pipefail

MODULE_NAME="sysmon_driver"
DEVICE_NODE="/dev/sysmon"

echo "=========================================="
echo "SysMon: Unloading Kernel Module"
echo "=========================================="

# Check for root privileges
if [ "$(id -u)" -ne 0 ]; then
    echo "[-] Error: This script must be run as root or with sudo." >&2
    exit 1
fi

# Check if module is loaded
if ! lsmod | grep -q "^${MODULE_NAME}\b"; then
    echo "[*] Module '${MODULE_NAME}' is not currently loaded."
    exit 0
fi

# Check if device is in active use
if [ -c "${DEVICE_NODE}" ] && command -v fuser >/dev/null 2>&1; then
    if fuser "${DEVICE_NODE}" >/dev/null 2>&1; then
        echo "[!] Warning: Processes are currently accessing ${DEVICE_NODE}."
        fuser -v "${DEVICE_NODE}" || true
        echo "[-] Aborting unload to prevent system instability."
        exit 1
    fi
fi

# Remove module
echo "[+] Removing module: ${MODULE_NAME}"
rmmod "${MODULE_NAME}"

# Verify removal
if ! lsmod | grep -q "^${MODULE_NAME}\b"; then
    echo "[+] Module '${MODULE_NAME}' unloaded successfully."
else
    echo "[-] Error: Failed to remove module '${MODULE_NAME}'." >&2
    exit 1
fi

dmesg | grep sysmon | tail -n 2 || true
