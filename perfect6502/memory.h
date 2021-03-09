#ifndef INCLUDE_PERFECT6502_MEMORY_H
#define INCLUDE_PERFECT6502_MEMORY_H

/* --- Constants --- */

enum { PERFECT6502_MEMORY_ADDR_SPACE = 65536 };
enum { PERFECT6502_MEMORY_ADDR_WIDTH = 16 };
enum { PERFECT6502_MEMORY_WORD_WIDTH = 8 };

/* --- Types --- */

typedef unsigned short perfect6502_addr_t;
typedef unsigned char perfect6502_word_t;

typedef struct {
    perfect6502_word_t memory[PERFECT6502_MEMORY_ADDR_SPACE];
} perfect6502_memory_t;

/* --- Functions --- */

perfect6502_memory_t * perfect6502_memory_init();
void perfect6502_memory_destroy(perfect6502_memory_t * memory);
perfect6502_word_t perfect6502_memory_read(const perfect6502_memory_t * memory,
                                           perfect6502_addr_t addr);
void perfect6502_memory_write(perfect6502_memory_t * memory,
                              perfect6502_addr_t addr,
                              perfect6502_word_t word);

#endif /* INCLUDE_PERFECT6502_MEMORY_H */
