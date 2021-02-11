mos6502: Makefile *.c *.h
	gcc --ansi --pedantic -Wall -Wno-unused-function -DDEBUG -o mos6502 *.c
