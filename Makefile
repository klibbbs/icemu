CC = gcc
CFLAGS = --std=c99 --pedantic -Wall -DDEBUG

RUNTIME_OBJS = runtime.o
RUNTIME_DEPS = runtime.h

ICEMU_OBJS = icemu.o debug.o
ICEMU_DEPS = icemu.h debug.h

MOS6502_LIB  = mos6502/mos6502.so
MOS6502_OBJS = mos6502/mos6502.o mos6502/memory.o mos6502/adapter.o mos6502/controller.o
MOS6502_DEPS = mos6502/*.h

.PHONY: all clean

all: compare runtime

clean:
	$(RM) *.o *.so ../perfect6502/*.o mos6502/*.{o,so} compare runtime

$(MOS6502_LIB): CFLAGS += -fPIC
$(MOS6502_LIB): $(MOS6502_OBJS) $(MOS6502_DEPS) $(ICEMU_OBJS) $(ICEMU_DEPS)
	$(CC) $(CFLAGS) -o $@ --shared $(MOS6502_OBJS) $(ICEMU_OBJS)

compare: icemu.o debug.o mos6502/mos6502.o compare.o ../perfect6502/perfect6502.o ../perfect6502/netlist_sim.o
	$(CC) $(CFLAGS) -o $@ $^

runtime: $(RUNTIME_OBJS) $(RUNTIME_DEPS) $(MOS6502_LIB)
	$(CC) $(CFLAGS) -o $@ $(RUNTIME_OBJS) $(MOS6502_LIB)
