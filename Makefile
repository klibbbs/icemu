#CC = gcc
#CFLAGS = --std=c89 --pedantic -Wall -DDEBUG

ICEMU_OBJS = icemu.o debug.o runtime.o
ICEMU_DEPS = icemu.h debug.h runtime.h

MOS6502_OBJS = mos6502/mos6502.o mos6502/memory.o mos6502/adapter.o mos6502/controller.o
MOS6502_DEPS = mos6502/*.h

.PHONY: all clean

all: compare mos6502

clean:
	$(RM) *.o ../perfect6502/*.o mos6502/*.o compare mos6502/run

compare: icemu.o debug.o mos6502/mos6502.o compare.o ../perfect6502/perfect6502.o ../perfect6502/netlist_sim.o
	gcc -o $@ $^

.PHONY: mos6502
mos6502: mos6502/run

mos6502/run: mos6502/run.o $(MOS6502_OBJS) $(MOS6502_DEPS) $(ICEMU_OBJS) $(ICEMU_DEPS)
	gcc --std=c89 --pedantic -Wall -o $@ mos6502/run.o $(MOS6502_OBJS) $(ICEMU_OBJS)
