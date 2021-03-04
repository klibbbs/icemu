#include "memory.h"

#include <stdlib.h>

perfect6502_memory_t * perfect6502_memory_init() {
  return (perfect6502_memory_t *)calloc(1, sizeof(perfect6502_memory_t));
}

void perfect6502_memory_destroy(perfect6502_memory_t * memory) {
  free(memory);
}

perfect6502_word_t perfect6502_memory_read(const perfect6502_memory_t * memory,
                                           perfect6502_addr_t addr) {
  return memory->memory[addr];
}

void perfect6502_memory_write(perfect6502_memory_t * memory,
                              perfect6502_addr_t addr,
                              perfect6502_word_t word) {
  memory->memory[addr] = word;
}
