#ifndef INCLUDE_PERFECT6502_CONTROLLER_H
#define INCLUDE_PERFECT6502_CONTROLLER_H

#include "memory.h"

#include "../../perfect6502/perfect6502.h"

#include <stddef.h>

void perfect6502_controller_reset(state_t * perfect6502);
void perfect6502_controller_step(state_t * perfect6502, perfect6502_memory_t * memory);
void perfect6502_controller_run(state_t * perfect6502,
                                perfect6502_memory_t * memory,
                                size_t cycles);

#endif /* INCLUDE_PERFECT6502_CONTROLLER_H */
