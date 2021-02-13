mos6502: Makefile *.c *.h
	gcc --ansi --pedantic -Wall -DDEBUG -o mos6502 *.c
