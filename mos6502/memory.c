#include "memory.h"

#include <stdlib.h>

mos6502_memory_t * mos6502_memory_init() {
    return (mos6502_memory_t *)calloc(1, sizeof(mos6502_memory_t));
}

void mos6502_memory_destroy(mos6502_memory_t * memory) {
    free(memory);
}

mos6502_word_t mos6502_memory_read(const mos6502_memory_t * memory, mos6502_addr_t addr) {
    return memory->memory[addr >> 0];
}

void mos6502_memory_write(mos6502_memory_t * memory, mos6502_addr_t addr, mos6502_word_t word) {
    memory->memory[addr >> 0] = word;
}
