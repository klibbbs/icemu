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

/* --- Register accessors --- */

unsigned short mos6502_get_reg_pc(const mos6502_t * mos6502) {
  return 0x0 |
    icemu_read_node(mos6502->ic, REG_PCL0, PULL_DOWN) << 0 |
    icemu_read_node(mos6502->ic, REG_PCL1, PULL_DOWN) << 1 |
    icemu_read_node(mos6502->ic, REG_PCL2, PULL_DOWN) << 2 |
    icemu_read_node(mos6502->ic, REG_PCL3, PULL_DOWN) << 3 |
    icemu_read_node(mos6502->ic, REG_PCL4, PULL_DOWN) << 4 |
    icemu_read_node(mos6502->ic, REG_PCL5, PULL_DOWN) << 5 |
    icemu_read_node(mos6502->ic, REG_PCL6, PULL_DOWN) << 6 |
    icemu_read_node(mos6502->ic, REG_PCL7, PULL_DOWN) << 7 |
    icemu_read_node(mos6502->ic, REG_PCH0, PULL_DOWN) << 8 |
    icemu_read_node(mos6502->ic, REG_PCH1, PULL_DOWN) << 9 |
    icemu_read_node(mos6502->ic, REG_PCH2, PULL_DOWN) << 10 |
    icemu_read_node(mos6502->ic, REG_PCH3, PULL_DOWN) << 11 |
    icemu_read_node(mos6502->ic, REG_PCH4, PULL_DOWN) << 12 |
    icemu_read_node(mos6502->ic, REG_PCH5, PULL_DOWN) << 13 |
    icemu_read_node(mos6502->ic, REG_PCH6, PULL_DOWN) << 14 |
    icemu_read_node(mos6502->ic, REG_PCH7, PULL_DOWN) << 15;
}

unsigned char mos6502_get_reg_p(const mos6502_t * mos6502) {
  return 0x0 |
    icemu_read_node(mos6502->ic, REG_P0, PULL_DOWN) << 0 |
    icemu_read_node(mos6502->ic, REG_P1, PULL_DOWN) << 1 |
    icemu_read_node(mos6502->ic, REG_P2, PULL_DOWN) << 2 |
    icemu_read_node(mos6502->ic, REG_P3, PULL_DOWN) << 3 |
    icemu_read_node(mos6502->ic, REG_P4, PULL_DOWN) << 4 |
    icemu_read_node(mos6502->ic, REG_P5, PULL_DOWN) << 5 |
    icemu_read_node(mos6502->ic, REG_P6, PULL_DOWN) << 6 |
    icemu_read_node(mos6502->ic, REG_P7, PULL_DOWN) << 7;
}

unsigned char mos6502_get_reg_a(const mos6502_t * mos6502) {
  return 0x0 |
    icemu_read_node(mos6502->ic, REG_A0, PULL_DOWN) << 0 |
    icemu_read_node(mos6502->ic, REG_A1, PULL_DOWN) << 1 |
    icemu_read_node(mos6502->ic, REG_A2, PULL_DOWN) << 2 |
    icemu_read_node(mos6502->ic, REG_A3, PULL_DOWN) << 3 |
    icemu_read_node(mos6502->ic, REG_A4, PULL_DOWN) << 4 |
    icemu_read_node(mos6502->ic, REG_A5, PULL_DOWN) << 5 |
    icemu_read_node(mos6502->ic, REG_A6, PULL_DOWN) << 6 |
    icemu_read_node(mos6502->ic, REG_A7, PULL_DOWN) << 7;
}

unsigned char mos6502_get_reg_x(const mos6502_t * mos6502) {
  return 0x0 |
    icemu_read_node(mos6502->ic, REG_X0, PULL_DOWN) << 0 |
    icemu_read_node(mos6502->ic, REG_X1, PULL_DOWN) << 1 |
    icemu_read_node(mos6502->ic, REG_X2, PULL_DOWN) << 2 |
    icemu_read_node(mos6502->ic, REG_X3, PULL_DOWN) << 3 |
    icemu_read_node(mos6502->ic, REG_X4, PULL_DOWN) << 4 |
    icemu_read_node(mos6502->ic, REG_X5, PULL_DOWN) << 5 |
    icemu_read_node(mos6502->ic, REG_X6, PULL_DOWN) << 6 |
    icemu_read_node(mos6502->ic, REG_X7, PULL_DOWN) << 7;
}

unsigned char mos6502_get_reg_y(const mos6502_t * mos6502) {
  return 0x0 |
    icemu_read_node(mos6502->ic, REG_Y0, PULL_DOWN) << 0 |
    icemu_read_node(mos6502->ic, REG_Y1, PULL_DOWN) << 1 |
    icemu_read_node(mos6502->ic, REG_Y2, PULL_DOWN) << 2 |
    icemu_read_node(mos6502->ic, REG_Y3, PULL_DOWN) << 3 |
    icemu_read_node(mos6502->ic, REG_Y4, PULL_DOWN) << 4 |
    icemu_read_node(mos6502->ic, REG_Y5, PULL_DOWN) << 5 |
    icemu_read_node(mos6502->ic, REG_Y6, PULL_DOWN) << 6 |
    icemu_read_node(mos6502->ic, REG_Y7, PULL_DOWN) << 7;
}

unsigned char mos6502_get_reg_sp(const mos6502_t * mos6502) {
  return 0x0 |
    icemu_read_node(mos6502->ic, REG_SP0, PULL_DOWN) << 0 |
    icemu_read_node(mos6502->ic, REG_SP1, PULL_DOWN) << 1 |
    icemu_read_node(mos6502->ic, REG_SP2, PULL_DOWN) << 2 |
    icemu_read_node(mos6502->ic, REG_SP3, PULL_DOWN) << 3 |
    icemu_read_node(mos6502->ic, REG_SP4, PULL_DOWN) << 4 |
    icemu_read_node(mos6502->ic, REG_SP5, PULL_DOWN) << 5 |
    icemu_read_node(mos6502->ic, REG_SP6, PULL_DOWN) << 6 |
    icemu_read_node(mos6502->ic, REG_SP7, PULL_DOWN) << 7;
}

unsigned char mos6502_get_reg_i(const mos6502_t * mos6502) {
  return 0x0 |
    icemu_read_node(mos6502->ic, REG_I0, PULL_DOWN) << 0 |
    icemu_read_node(mos6502->ic, REG_I1, PULL_DOWN) << 1 |
    icemu_read_node(mos6502->ic, REG_I2, PULL_DOWN) << 2 |
    icemu_read_node(mos6502->ic, REG_I3, PULL_DOWN) << 3 |
    icemu_read_node(mos6502->ic, REG_I4, PULL_DOWN) << 4 |
    icemu_read_node(mos6502->ic, REG_I5, PULL_DOWN) << 5 |
    icemu_read_node(mos6502->ic, REG_I6, PULL_DOWN) << 6 |
    icemu_read_node(mos6502->ic, REG_I7, PULL_DOWN) << 7;
}

/* --- Pin accessors --- */

unsigned short mos6502_get_ab(const mos6502_t * mos6502) {
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

bit_t mos6502_get_clk(const mos6502_t * mos6502) {
  return icemu_read_node(mos6502->ic, PIN_CLK, PULL_FLOAT);
}

bit_t mos6502_get_clk1(const mos6502_t * mos6502) {
  return icemu_read_node(mos6502->ic, PIN_CLK1, PULL_FLOAT);
}

bit_t mos6502_get_clk2(const mos6502_t * mos6502) {
  return icemu_read_node(mos6502->ic, PIN_CLK2, PULL_FLOAT);
}

bit_t mos6502_get_irq(const mos6502_t * mos6502) {
  return icemu_read_node(mos6502->ic, PIN_IRQ, PULL_FLOAT);
}

bit_t mos6502_get_nmi(const mos6502_t * mos6502) {
  return icemu_read_node(mos6502->ic, PIN_NMI, PULL_FLOAT);
}

bit_t mos6502_get_res(const mos6502_t * mos6502) {
  return icemu_read_node(mos6502->ic, PIN_RES, PULL_FLOAT);
}

bit_t mos6502_get_rdy(const mos6502_t * mos6502) {
  return icemu_read_node(mos6502->ic, PIN_RDY, PULL_FLOAT);
}

bit_t mos6502_get_rw(const mos6502_t * mos6502) {
  return icemu_read_node(mos6502->ic, PIN_RW, PULL_FLOAT);
}

bit_t mos6502_get_so(const mos6502_t * mos6502) {
  return icemu_read_node(mos6502->ic, PIN_SO, PULL_FLOAT);
}

bit_t mos6502_get_sync(const mos6502_t * mos6502) {
  return icemu_read_node(mos6502->ic, PIN_SYNC, PULL_FLOAT);
}

/* --- Pin modifiers --- */

void mos6502_set_clk(mos6502_t * mos6502, bit_t state, bool sync) {
  icemu_write_node(mos6502->ic, PIN_CLK, state, sync);
}

void mos6502_set_db(mos6502_t * mos6502, unsigned char data, bool sync) {
  icemu_write_node(mos6502->ic, PIN_DB0, data >> 0 & 0x01, false);
  icemu_write_node(mos6502->ic, PIN_DB1, data >> 1 & 0x01, false);
  icemu_write_node(mos6502->ic, PIN_DB2, data >> 2 & 0x01, false);
  icemu_write_node(mos6502->ic, PIN_DB3, data >> 3 & 0x01, false);
  icemu_write_node(mos6502->ic, PIN_DB4, data >> 4 & 0x01, false);
  icemu_write_node(mos6502->ic, PIN_DB5, data >> 5 & 0x01, false);
  icemu_write_node(mos6502->ic, PIN_DB6, data >> 6 & 0x01, false);
  icemu_write_node(mos6502->ic, PIN_DB7, data >> 7 & 0x01, false);

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
