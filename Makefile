#CC = gcc
#CFLAGS = --std=c89 --pedantic -Wall -DDEBUG

.PHONY: all clean

all: compare

clean:
	$(RM) *.o ../perfect6502/*.o mos6502/*.o compare

#mos6502: icemu.o debug.o mos6502.o mos6502_main.o
#	gcc --std=c89 --pedantic -Wall -o $@ $^

compare: icemu.o debug.o mos6502/mos6502.o compare.o ../perfect6502/perfect6502.o ../perfect6502/netlist_sim.o
	gcc -o $@ $^
