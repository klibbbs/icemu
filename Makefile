# Input variables
DEVICE ?= *

# Compile flags
CC = gcc
CFLAGS = -std=c89 -pedantic -Wall -Werror

# Build dependencies
RUNTIME_OBJS = runtime.o
RUNTIME_DEPS = runtime.h

ICEMU_OBJS = icemu.o debug.o
ICEMU_DEPS = icemu.h debug.h

MOS6502_LIB  = mos6502/mos6502.so
MOS6502_OBJS = mos6502/mos6502.o mos6502/memory.o mos6502/controller.o mos6502/adapter.o
MOS6502_DEPS = mos6502/*.h

PERFECT6502_LIB  = perfect6502/perfect6502.so
PERFECT6502_OBJS = ../perfect6502/perfect6502.o ../perfect6502/netlist_sim.o perfect6502/memory.o perfect6502/controller.o perfect6502/adapter.o
PERFECT6502_DEPS = perfect6502/*.h

DEVICE_LIBS = $(MOS6502_LIB) # $(PERFECT6502_LIB)

# Testing
TEST_CMD = bin/test

DEVICE_TESTS = $(shell find . -type f -path "./${DEVICE}/tests/*ice")

.PHONY: all clean

all: runtime

clean:
	$(RM) *.o *.so ../perfect6502/*.{o,so} mos6502/*.{o,so} runtime

.PHONY: test
test: $(TEST_CMD) $(DEVICE_TESTS) runtime
	$(TEST_CMD) $(DEVICE_TESTS)

$(MOS6502_LIB): CFLAGS += -fPIC
$(MOS6502_LIB): $(MOS6502_OBJS) $(MOS6502_DEPS) $(ICEMU_OBJS) $(ICEMU_DEPS)
	$(CC) $(CFLAGS) -o $@ --shared $(MOS6502_OBJS) $(ICEMU_OBJS)

$(PERFECT6502_LIB): CFLAGS += -fPIC -std=c99
$(PERFECT6502_LIB): $(PERFECT6502_OBJS) $(PERFECT6502_DEPS)
	$(CC) $(CFLAGS) -o $@ --shared $(PERFECT6502_OBJS)

runtime: $(RUNTIME_OBJS) $(RUNTIME_DEPS) $(DEVICE_LIBS)
	$(CC) $(CFLAGS) -o $@ $(RUNTIME_OBJS)
