# Makefile for SysMon Kernel Module
#
# Usage:
#   make          - Build the kernel module
#   make clean    - Remove build artifacts
#
# Prerequisites:
#   sudo apt install linux-headers-$(uname -r) build-essential

obj-m += sysmon_driver.o

# Path to the kernel build directory (auto-detects running kernel or installed headers)
DEFAULT_KDIR := /lib/modules/$(shell uname -r)/build
FALLBACK_KDIR := $(lastword $(sort $(wildcard /usr/src/linux-headers-*generic)))

ifeq ($(wildcard $(DEFAULT_KDIR)),)
    KDIR ?= $(FALLBACK_KDIR)
else
    KDIR ?= $(DEFAULT_KDIR)
endif

# Current directory (module source)
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

# Convenience targets
load:
	sudo insmod sysmon_driver.ko
	@echo "Module loaded. Check: ls -l /dev/sysmon"

unload:
	sudo rmmod sysmon_driver
	@echo "Module unloaded."

reload: unload load

info:
	@echo "=== Module Info ==="
	modinfo sysmon_driver.ko 2>/dev/null || echo "Module not built yet."
	@echo ""
	@echo "=== Device Status ==="
	@ls -l /dev/sysmon 2>/dev/null || echo "/dev/sysmon not found (module not loaded)."
	@echo ""
	@echo "=== Kernel Log (last 5 lines) ==="
	@dmesg | grep sysmon | tail -5

.PHONY: all clean load unload reload info
