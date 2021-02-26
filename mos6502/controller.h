#ifndef INCLUDE_MOS6502_CONTROLLER_H
#define INCLUDE_MOS6502_CONTROLLER_H

#include "memory.h"
#include "mos6502.h"

void mos6502_controller_reset(mos6502_t * mos6502);
void mos6502_controller_step(mos6502_t * mos6502, mos6502_memory_t * memory);
void mos6502_controller_run(mos6502_t * mos6502, mos6502_memory_t * memory, size_t cycles);

#endif /* INCLUDE_MOS6502_CONTROLLER_H */
