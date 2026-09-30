# Makefile for SysMon Test Suite
#
# Usage:
#   make          - Build the test runner
#   make clean    - Remove build artifacts
#   make run      - Run tests (requires driver loaded)

CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -O2
INCLUDES = -I../kernel -I../user/include

TARGET   = test_ioctl
SRCS     = test_ioctl.cpp
OBJS     = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^
	@echo "Test suite built: $(TARGET)"

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
