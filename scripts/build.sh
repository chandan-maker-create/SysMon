#!/usr/bin/env bash
# ==============================================================================
# SysMon - Project Build Script
# Validates system requirements and compiles kernel module, CLI, and test suite.
# ==============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

echo "=========================================="
echo "SysMon: Building Complete Project"
echo "=========================================="

echo "[1/4] Checking build dependencies..."
MISSING_PKGS=()

for cmd in gcc g++ make; do
    if ! command -v "${cmd}" >/dev/null 2>&1; then
        MISSING_PKGS+=("${cmd}")
    fi
done

if [ ${#MISSING_PKGS[@]} -gt 0 ]; then
    echo "[-] Missing required build utilities: ${MISSING_PKGS[*]}"
    echo "[!] Install them via: sudo apt-get update && sudo apt-get install -y build-essential"
    exit 1
fi

echo "[2/4] Checking Linux kernel headers..."
KDIR="/lib/modules/$(uname -r)/build"
FALLBACK_KDIR=$(ls -d /usr/src/linux-headers-*generic 2>/dev/null | sort | tail -n 1 || true)

if [ -d "${KDIR}" ]; then
    echo "[+] Kernel build directory found: ${KDIR}"
elif [ -n "${FALLBACK_KDIR}" ] && [ -d "${FALLBACK_KDIR}" ]; then
    echo "[+] Detected installed Linux kernel headers: ${FALLBACK_KDIR}"
else
    echo "[!] Warning: Kernel build directory ${KDIR} not found."
    echo "[!] On Ubuntu: sudo apt-get install -y linux-headers-\$(uname -r)"
fi

echo "[3/4] Compiling kernel character device driver module..."
make -C "${ROOT_DIR}/kernel"

echo "[4/4] Compiling user-space monitor application and test suite..."
make -C "${ROOT_DIR}/user"
if [ -d "${ROOT_DIR}/tests" ] && [ -f "${ROOT_DIR}/tests/Makefile" ]; then
    make -C "${ROOT_DIR}/tests"
fi

echo "=========================================="
echo "[+] Build complete!"
echo "=========================================="
