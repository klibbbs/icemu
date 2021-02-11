#include "mos6502.h"
#include "mos6502_defs.h"
#include "icemu.h"

#include <stdlib.h>

static bit_t mos6502_get_pin(const mos6502_t * mos6502, mos6502_pin_t pin, pull_t pull);
static void  mos6502_set_pin(mos6502_t * mos6502, mos6502_pin_t pin, bit_t state);

/*
======================
   Public functions
======================
*/

/* --- Emulator control --- */

mos6502_t * mos6502_init() {

  /* Create IC definition */
  icemu_def_t def = {
    MOS6502_ON,
    MOS6502_OFF,
    MOS6502_CLOCK,
    MOS6502_NODE_DEFS,
    sizeof(MOS6502_NODE_DEFS) / sizeof(MOS6502_NODE_DEFS[0]),
    MOS6502_TRANSISTOR_DEFS,
    sizeof(MOS6502_TRANSISTOR_DEFS) / sizeof(MOS6502_TRANSISTOR_DEFS[0]),
  };

  /* Initialize new IC emulator */
  icemu_t * ic = icemu_init(&def);

  /* Initialize new MOS6502 emulator */
  mos6502_t * mos6502 = malloc(sizeof(mos6502_t));

  mos6502->ic = ic;

  return mos6502;
}

void mos6502_destroy(mos6502_t * mos6502) {
  icemu_destroy(mos6502->ic);

  free(mos6502);
}

void mos6502_reset(mos6502_t * mos6502) {
  int i;

  /* Set RES low */
  mos6502_set_res(mos6502, BIT_ZERO);

  /* Set neutral inputs */
  mos6502_set_clk(mos6502, BIT_ONE);
  mos6502_set_rdy(mos6502, BIT_ONE);
  mos6502_set_so(mos6502, BIT_ZERO);
  mos6502_set_irq(mos6502, BIT_ONE);
  mos6502_set_nmi(mos6502, BIT_ONE);

  /* Wait 8 cycles */
  for (i = 0; i < 16; i++) {
    mos6502_step(mos6502);
  }

  /* Set RES high */
  mos6502_set_res(mos6502, BIT_ONE);
}

void mos6502_step(mos6502_t * mos6502) {
  /* TODO */
}

/* --- Pin getters --- */

uint16_t mos6502_get_ab(const mos6502_t * mos6502) {
  return 0x0 |
    mos6502_get_pin(mos6502, PIN_AB0, PULL_DOWN) << 0 |
    mos6502_get_pin(mos6502, PIN_AB1, PULL_DOWN) << 1 |
    mos6502_get_pin(mos6502, PIN_AB2, PULL_DOWN) << 2 |
    mos6502_get_pin(mos6502, PIN_AB3, PULL_DOWN) << 3 |
    mos6502_get_pin(mos6502, PIN_AB4, PULL_DOWN) << 4 |
    mos6502_get_pin(mos6502, PIN_AB5, PULL_DOWN) << 5 |
    mos6502_get_pin(mos6502, PIN_AB6, PULL_DOWN) << 6 |
    mos6502_get_pin(mos6502, PIN_AB7, PULL_DOWN) << 7 |
    mos6502_get_pin(mos6502, PIN_AB8, PULL_DOWN) << 8 |
    mos6502_get_pin(mos6502, PIN_AB9, PULL_DOWN) << 9 |
    mos6502_get_pin(mos6502, PIN_AB10, PULL_DOWN) << 10 |
    mos6502_get_pin(mos6502, PIN_AB11, PULL_DOWN) << 11 |
    mos6502_get_pin(mos6502, PIN_AB12, PULL_DOWN) << 12 |
    mos6502_get_pin(mos6502, PIN_AB13, PULL_DOWN) << 13 |
    mos6502_get_pin(mos6502, PIN_AB14, PULL_DOWN) << 14 |
    mos6502_get_pin(mos6502, PIN_AB15, PULL_DOWN) << 15;
}

bit_t mos6502_get_clk1(const mos6502_t * mos6502) {
  return mos6502_get_pin(mos6502, PIN_CLK1, PULL_FLOAT);
}

bit_t mos6502_get_clk2(const mos6502_t * mos6502) {
  return mos6502_get_pin(mos6502, PIN_CLK2, PULL_FLOAT);
}

uint8_t mos6502_get_db(const mos6502_t * mos6502) {
  return 0x0 |
    mos6502_get_pin(mos6502, PIN_DB0, PULL_DOWN) << 0 |
    mos6502_get_pin(mos6502, PIN_DB1, PULL_DOWN) << 1 |
    mos6502_get_pin(mos6502, PIN_DB2, PULL_DOWN) << 2 |
    mos6502_get_pin(mos6502, PIN_DB3, PULL_DOWN) << 3 |
    mos6502_get_pin(mos6502, PIN_DB4, PULL_DOWN) << 4 |
    mos6502_get_pin(mos6502, PIN_DB5, PULL_DOWN) << 5 |
    mos6502_get_pin(mos6502, PIN_DB6, PULL_DOWN) << 6 |
    mos6502_get_pin(mos6502, PIN_DB7, PULL_DOWN) << 7;
}

bit_t mos6502_get_rw(const mos6502_t * mos6502) {
  return mos6502_get_pin(mos6502, PIN_RW, PULL_FLOAT);
}

bit_t mos6502_get_sync(const mos6502_t * mos6502) {
  return mos6502_get_pin(mos6502, PIN_SYNC, PULL_FLOAT);
}

/* --- Pin setters --- */

void mos6502_set_clk(mos6502_t * mos6502, bit_t state) {
  mos6502_set_pin(mos6502, PIN_CLK, state);
}

void mos6502_set_db(mos6502_t * mos6502, uint8_t data) {
  mos6502_set_pin(mos6502, PIN_DB0, data & 0x01);
  mos6502_set_pin(mos6502, PIN_DB1, data & 0x02);
  mos6502_set_pin(mos6502, PIN_DB2, data & 0x04);
  mos6502_set_pin(mos6502, PIN_DB3, data & 0x08);
  mos6502_set_pin(mos6502, PIN_DB4, data & 0x10);
  mos6502_set_pin(mos6502, PIN_DB5, data & 0x20);
  mos6502_set_pin(mos6502, PIN_DB6, data & 0x40);
  mos6502_set_pin(mos6502, PIN_DB7, data & 0x80);
}

void mos6502_set_irq(mos6502_t * mos6502, bit_t state) {
  mos6502_set_pin(mos6502, PIN_IRQ, state);
}

void mos6502_set_nmi(mos6502_t * mos6502, bit_t state) {
  mos6502_set_pin(mos6502, PIN_NMI, state);
}

void mos6502_set_res(mos6502_t * mos6502, bit_t state) {
  mos6502_set_pin(mos6502, PIN_RES, state);
}

void mos6502_set_rdy(mos6502_t * mos6502, bit_t state) {
  mos6502_set_pin(mos6502, PIN_RDY, state);
}

void mos6502_set_so(mos6502_t * mos6502, bit_t state) {
  mos6502_set_pin(mos6502, PIN_SO, state);
}

/*
=======================
   Private functions
=======================
*/

static bit_t mos6502_get_pin(const mos6502_t * mos6502, mos6502_pin_t pin, pull_t pull) {
  return icemu_read_node(mos6502->ic, pin, pull);
}

static void mos6502_set_pin(mos6502_t * mos6502, mos6502_pin_t pin, bit_t state) {
  icemu_write_node(mos6502->ic, pin, state);
}
