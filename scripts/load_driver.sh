#!/usr/bin/env bash
# ==============================================================================
# SysMon - Driver Load Script
# Loads the sysmon character device driver kernel module and adjusts permissions.
# ==============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
KERNEL_DIR="${ROOT_DIR}/kernel"
MODULE_FILE="${KERNEL_DIR}/sysmon_driver.ko"
DEVICE_NODE="/dev/sysmon"

echo "=========================================="
echo "SysMon: Loading Kernel Module"
echo "=========================================="

# Check for root privileges
if [ "$(id -u)" -ne 0 ]; then
    echo "[-] Error: This script must be run as root or with sudo." >&2
    exit 1
fi

# Check if module is already loaded
if lsmod | grep -q "^sysmon_driver\b"; then
    echo "[*] Driver 'sysmon_driver' is already loaded."
    exit 0
fi

# Check if module file exists, build if missing
if [ ! -f "${MODULE_FILE}" ]; then
    echo "[!] Module file '${MODULE_FILE}' not found. Attempting to build..."
    make -C "${KERNEL_DIR}"
fi

# Insert kernel module
echo "[+] Inserting module: ${MODULE_FILE}"
insmod "${MODULE_FILE}"

# Give udev a short moment to create the device node
sleep 0.5

# Check if device node was created
if [ -c "${DEVICE_NODE}" ]; then
    echo "[+] Device node created: ${DEVICE_NODE}"
    # Set read/write permissions for all users to allow user-space testing
    chmod 666 "${DEVICE_NODE}"
    echo "[+] Permissions set: rw-rw-rw- on ${DEVICE_NODE}"
else
    echo "[-] Warning: ${DEVICE_NODE} was not automatically created by devtmpfs/udev."
    echo "[*] Inspecting kernel ring buffer (dmesg):"
    dmesg | grep sysmon | tail -n 5
    exit 1
fi

echo "[+] SysMon driver loaded successfully!"
dmesg | grep sysmon | tail -n 3
