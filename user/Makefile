# Makefile for SysMon User-Space Application
#
# Usage:
#   make          - Build the sysmon application
#   make clean    - Remove build artifacts
#   make run      - Run the application (requires sudo)
#
# Prerequisites:
#   sudo apt install g++ build-essential

CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -O2

# Include paths: local headers + shared kernel ioctl header
INCLUDES = -Iinclude -I../kernel

# Source and output
TARGET   = sysmon
SRCDIR   = src
SRCS     = $(SRCDIR)/main.cpp \
           $(SRCDIR)/monitor.cpp \
           $(SRCDIR)/display.cpp \
           $(SRCDIR)/utils.cpp
OBJS     = $(SRCS:.cpp=.o)

# Default target
all: $(TARGET)

# Link object files into final executable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^
	@echo ""
	@echo "Build successful: ./$(TARGET)"
	@echo "Run with: sudo ./$(TARGET)"

# Compile source files to object files
$(SRCDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<

# Clean build artifacts
clean:
	rm -f $(OBJS) $(TARGET)

# Run the application (convenience target)
run: $(TARGET)
	sudo ./$(TARGET)

.PHONY: all clean run
