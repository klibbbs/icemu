#ifndef INCLUDE_MOS6502_MEMORY_H
#define INCLUDE_MOS6502_MEMORY_H

/* TODO: Auto-generate from JSON */

#include "mos6502.h"

/* --- Constants --- */

enum { MOS6502_ADDR_SPACE = 65536 };
enum { MOS6502_ADDR_WIDTH = 16 };
enum { MOS6502_WORD_WIDTH = 8 };

/* --- Types --- */

typedef unsigned short mos6502_addr_t;
typedef unsigned char mos6502_word_t;

typedef struct {
  mos6502_word_t memory[MOS6502_ADDR_SPACE];
} mos6502_memory_t;

/* --- Functions --- */

mos6502_memory_t * mos6502_memory_init();
void mos6502_memory_destroy(mos6502_memory_t * memory);
mos6502_word_t mos6502_memory_read(const mos6502_memory_t * memory, mos6502_addr_t addr);
void mos6502_memory_write(mos6502_memory_t * memory, mos6502_addr_t addr, mos6502_word_t word);

#endif /* INCLUDE_MOS6502_MEMORY_H */