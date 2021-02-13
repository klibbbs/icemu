CC = gcc
CFLAGS = --std=c89 --pedantic -Wall -DDEBUG

.PHONY: all clean

all: mos6502

clean:
	$(RM) *.o mos6502

mos6502: icemu.o mos6502.o mos6502_main.o
	$(CC) $(CFLAGS) -o $@ $^
