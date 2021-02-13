CC = gcc
CFLAGS = --std=c89 --pedantic -Wall -DDEBUG

.PHONY: all clean

all: mos6502 compare

clean:
	$(RM) *.o mos6502

mos6502: icemu.o mos6502.o mos6502_main.o
	$(CC) $(CFLAGS) -o $@ $^

compare: icemu.o mos6502.o compare.o ../perfect6502/perfect6502.o ../perfect6502/netlist_sim.o
	$(CC) $(CFLAGS) -o $@ $^
