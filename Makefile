# ==============================================================================
# SysMon - Linux System Resource Monitor
# Root Makefile for building, testing, installing, and managing the project
# ==============================================================================

KERNEL_DIR = kernel
USER_DIR   = user
TESTS_DIR  = tests
SCRIPTS_DIR = scripts

.PHONY: all kernel user tests clean clean-all load unload reload test help install-deps info

# Default target: build both kernel module and user application
all: kernel user

# Build the Linux character device driver module
kernel:
	@echo "=========================================="
	@echo "Building SysMon Kernel Module..."
	@echo "=========================================="
	$(MAKE) -C $(KERNEL_DIR)

# Build the C++ user-space monitoring application
user:
	@echo "=========================================="
	@echo "Building SysMon User-Space Application..."
	@echo "=========================================="
	$(MAKE) -C $(USER_DIR)

# Build the automated test suite
tests:
	@echo "=========================================="
	@echo "Building SysMon Test Suite..."
	@echo "=========================================="
	$(MAKE) -C $(TESTS_DIR)

# Clean all build artifacts
clean:
	@echo "Cleaning kernel module artifacts..."
	$(MAKE) -C $(KERNEL_DIR) clean
	@echo "Cleaning user application artifacts..."
	$(MAKE) -C $(USER_DIR) clean
	@if [ -d $(TESTS_DIR) ] && [ -f $(TESTS_DIR)/Makefile ]; then \
		echo "Cleaning test suite artifacts..."; \
		$(MAKE) -C $(TESTS_DIR) clean; \
	fi
	@echo "Clean completed."

clean-all: clean

# Load the kernel driver into the kernel
load:
	@bash $(SCRIPTS_DIR)/load_driver.sh

# Unload the kernel driver from the kernel
unload:
	@bash $(SCRIPTS_DIR)/unload_driver.sh

# Reload the kernel driver
reload: unload load

# Run automated tests (requires driver to be loaded)
test: tests
	@bash $(SCRIPTS_DIR)/run_tests.sh

# Display driver and system status
info:
	@echo "=== SysMon Device Status ==="
	@ls -l /dev/sysmon 2>/dev/null || echo "Device /dev/sysmon not found (driver not loaded)"
	@echo ""
	@echo "=== Loaded Module Status ==="
	@lsmod | grep sysmon || echo "sysmon driver not currently loaded in kernel"
	@echo ""
	@echo "=== Recent Kernel Logs ==="
	@dmesg | grep -i sysmon | tail -n 10 || true

# Display available Makefile commands
help:
	@echo "SysMon Build System"
	@echo "Available targets:"
	@echo "  make              - Build both kernel module and user application"
	@echo "  make kernel       - Build only the kernel character driver module"
	@echo "  make user         - Build only the user-space C++ CLI application"
	@echo "  make tests        - Build the automated test suite"
	@echo "  make clean        - Remove all build artifacts"
	@echo "  make load         - Insert kernel module (requires sudo/root)"
	@echo "  make unload       - Remove kernel module (requires sudo/root)"
	@echo "  make reload       - Unload and reload kernel module"
	@echo "  make test         - Run test suite"
	@echo "  make info         - Show module status and dmesg log"
	@echo "  make help         - Display this help message"
