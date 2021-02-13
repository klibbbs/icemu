#include "mos6502.h"
#include "mos6502_defs.h"

#include "debug.h"
#include "icemu.h"

#include <stdlib.h>

/* --- Emulator ---*/

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

void mos6502_sync(mos6502_t * mos6502) {
  icemu_sync(mos6502->ic);
}

/* --- Pin getters --- */

unsigned short int mos6502_get_ab(const mos6502_t * mos6502) {
  return 0x0 |
    icemu_read_node(mos6502->ic, PIN_AB0, PULL_DOWN) << 0 |
    icemu_read_node(mos6502->ic, PIN_AB1, PULL_DOWN) << 1 |
    icemu_read_node(mos6502->ic, PIN_AB2, PULL_DOWN) << 2 |
    icemu_read_node(mos6502->ic, PIN_AB3, PULL_DOWN) << 3 |
    icemu_read_node(mos6502->ic, PIN_AB4, PULL_DOWN) << 4 |
    icemu_read_node(mos6502->ic, PIN_AB5, PULL_DOWN) << 5 |
    icemu_read_node(mos6502->ic, PIN_AB6, PULL_DOWN) << 6 |
    icemu_read_node(mos6502->ic, PIN_AB7, PULL_DOWN) << 7 |
    icemu_read_node(mos6502->ic, PIN_AB8, PULL_DOWN) << 8 |
    icemu_read_node(mos6502->ic, PIN_AB9, PULL_DOWN) << 9 |
    icemu_read_node(mos6502->ic, PIN_AB10, PULL_DOWN) << 10 |
    icemu_read_node(mos6502->ic, PIN_AB11, PULL_DOWN) << 11 |
    icemu_read_node(mos6502->ic, PIN_AB12, PULL_DOWN) << 12 |
    icemu_read_node(mos6502->ic, PIN_AB13, PULL_DOWN) << 13 |
    icemu_read_node(mos6502->ic, PIN_AB14, PULL_DOWN) << 14 |
    icemu_read_node(mos6502->ic, PIN_AB15, PULL_DOWN) << 15;
}

bit_t mos6502_get_clk1(const mos6502_t * mos6502) {
  return icemu_read_node(mos6502->ic, PIN_CLK1, PULL_FLOAT);
}

bit_t mos6502_get_clk2(const mos6502_t * mos6502) {
  return icemu_read_node(mos6502->ic, PIN_CLK2, PULL_FLOAT);
}

unsigned char mos6502_get_db(const mos6502_t * mos6502) {
  return 0x0 |
    icemu_read_node(mos6502->ic, PIN_DB0, PULL_DOWN) << 0 |
    icemu_read_node(mos6502->ic, PIN_DB1, PULL_DOWN) << 1 |
    icemu_read_node(mos6502->ic, PIN_DB2, PULL_DOWN) << 2 |
    icemu_read_node(mos6502->ic, PIN_DB3, PULL_DOWN) << 3 |
    icemu_read_node(mos6502->ic, PIN_DB4, PULL_DOWN) << 4 |
    icemu_read_node(mos6502->ic, PIN_DB5, PULL_DOWN) << 5 |
    icemu_read_node(mos6502->ic, PIN_DB6, PULL_DOWN) << 6 |
    icemu_read_node(mos6502->ic, PIN_DB7, PULL_DOWN) << 7;
}

bit_t mos6502_get_rw(const mos6502_t * mos6502) {
  return icemu_read_node(mos6502->ic, PIN_RW, PULL_FLOAT);
}

bit_t mos6502_get_sync(const mos6502_t * mos6502) {
  return icemu_read_node(mos6502->ic, PIN_SYNC, PULL_FLOAT);
}

/* --- Pin setters --- */

void mos6502_set_clk(mos6502_t * mos6502, bit_t state, bool sync) {
  icemu_write_node(mos6502->ic, PIN_CLK, state, sync);
}

void mos6502_set_db(mos6502_t * mos6502, unsigned char data, bool sync) {
  icemu_write_node(mos6502->ic, PIN_DB0, data & 0x01, false);
  icemu_write_node(mos6502->ic, PIN_DB1, data & 0x02, false);
  icemu_write_node(mos6502->ic, PIN_DB2, data & 0x04, false);
  icemu_write_node(mos6502->ic, PIN_DB3, data & 0x08, false);
  icemu_write_node(mos6502->ic, PIN_DB4, data & 0x10, false);
  icemu_write_node(mos6502->ic, PIN_DB5, data & 0x20, false);
  icemu_write_node(mos6502->ic, PIN_DB6, data & 0x40, false);
  icemu_write_node(mos6502->ic, PIN_DB7, data & 0x80, false);

  if (sync) {
    icemu_sync(mos6502->ic);
  }
}

void mos6502_set_irq(mos6502_t * mos6502, bit_t state, bool sync) {
  icemu_write_node(mos6502->ic, PIN_IRQ, state, sync);
}

void mos6502_set_nmi(mos6502_t * mos6502, bit_t state, bool sync) {
  icemu_write_node(mos6502->ic, PIN_NMI, state, sync);
}

void mos6502_set_res(mos6502_t * mos6502, bit_t state, bool sync) {
  icemu_write_node(mos6502->ic, PIN_RES, state, sync);
}

void mos6502_set_rdy(mos6502_t * mos6502, bit_t state, bool sync) {
  icemu_write_node(mos6502->ic, PIN_RDY, state, sync);
}

void mos6502_set_so(mos6502_t * mos6502, bit_t state, bool sync) {
  icemu_write_node(mos6502->ic, PIN_SO, state, sync);
}
