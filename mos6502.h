#ifndef INCLUDE_MOS6502_H
#define INCLUDE_MOS6502_H

#include "icemu.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  icemu_t * ic;
} mos6502_t;

mos6502_t * mos6502_init();
void mos6502_destroy(mos6502_t *);
void mos6502_reset(mos6502_t *);
void mos6502_step(mos6502_t *);

uint16_t mos6502_get_ab(const mos6502_t * mos6502);
bit_t mos6502_get_clk1(const mos6502_t * mos6502);
bit_t mos6502_get_clk2(const mos6502_t * mos6502);
uint8_t mos6502_get_db(const mos6502_t * mos6502);
bit_t mos6502_get_rw(const mos6502_t * mos6502);
bit_t mos6502_get_sync(const mos6502_t * mos6502);

void mos6502_set_clk(mos6502_t * mos6502, bit_t state);
void mos6502_set_db(mos6502_t * mos6502, uint8_t byte);
void mos6502_set_irq(mos6502_t * mos6502, bit_t state);
void mos6502_set_nmi(mos6502_t * mos6502, bit_t state);
void mos6502_set_res(mos6502_t * mos6502, bit_t state);
void mos6502_set_rdy(mos6502_t * mos6502, bit_t state);
void mos6502_set_so(mos6502_t * mos6502, bit_t state);

#endif /* INCLUDE_MOS6502_H */
