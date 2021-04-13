#include "mos6502.h"

#include "layout.h"

#include "../icemu.h"

#include <stdlib.h>

/* --- Emulator --- */

mos6502_t * mos6502_init() {

    /* Construct IC layout */
    const icemu_layout_t layout = {
        MOS6502_ON,
        MOS6502_OFF,
        MOS6502_NODE_COUNT,
        MOS6502_LOAD_DEFS,
        MOS6502_LOAD_COUNT,
        MOS6502_TRANSISTOR_DEFS,
        MOS6502_TRANSISTOR_COUNT,
        MOS6502_BUFFER_DEFS,
        MOS6502_BUFFER_COUNT,
        MOS6502_FUNCTION_DEFS,
        MOS6502_FUNCTION_COUNT
    };

    /* Initialize new IC emulator */
    icemu_t * ic = icemu_init(&layout);

    /* Initialize new device emulator */
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

/* --- Pin accessors --- */

unsigned short mos6502_get_pin_ab(const mos6502_t * mos6502) {
    return 0x0 |
        icemu_read_node(mos6502->ic, PIN_AB_0, PULL_DOWN) << 0 |
        icemu_read_node(mos6502->ic, PIN_AB_1, PULL_DOWN) << 1 |
        icemu_read_node(mos6502->ic, PIN_AB_2, PULL_DOWN) << 2 |
        icemu_read_node(mos6502->ic, PIN_AB_3, PULL_DOWN) << 3 |
        icemu_read_node(mos6502->ic, PIN_AB_4, PULL_DOWN) << 4 |
        icemu_read_node(mos6502->ic, PIN_AB_5, PULL_DOWN) << 5 |
        icemu_read_node(mos6502->ic, PIN_AB_6, PULL_DOWN) << 6 |
        icemu_read_node(mos6502->ic, PIN_AB_7, PULL_DOWN) << 7 |
        icemu_read_node(mos6502->ic, PIN_AB_8, PULL_DOWN) << 8 |
        icemu_read_node(mos6502->ic, PIN_AB_9, PULL_DOWN) << 9 |
        icemu_read_node(mos6502->ic, PIN_AB_10, PULL_DOWN) << 10 |
        icemu_read_node(mos6502->ic, PIN_AB_11, PULL_DOWN) << 11 |
        icemu_read_node(mos6502->ic, PIN_AB_12, PULL_DOWN) << 12 |
        icemu_read_node(mos6502->ic, PIN_AB_13, PULL_DOWN) << 13 |
        icemu_read_node(mos6502->ic, PIN_AB_14, PULL_DOWN) << 14 |
        icemu_read_node(mos6502->ic, PIN_AB_15, PULL_DOWN) << 15;
}

unsigned char mos6502_get_pin_db(const mos6502_t * mos6502) {
    return 0x0 |
        icemu_read_node(mos6502->ic, PIN_DB_0, PULL_DOWN) << 0 |
        icemu_read_node(mos6502->ic, PIN_DB_1, PULL_DOWN) << 1 |
        icemu_read_node(mos6502->ic, PIN_DB_2, PULL_DOWN) << 2 |
        icemu_read_node(mos6502->ic, PIN_DB_3, PULL_DOWN) << 3 |
        icemu_read_node(mos6502->ic, PIN_DB_4, PULL_DOWN) << 4 |
        icemu_read_node(mos6502->ic, PIN_DB_5, PULL_DOWN) << 5 |
        icemu_read_node(mos6502->ic, PIN_DB_6, PULL_DOWN) << 6 |
        icemu_read_node(mos6502->ic, PIN_DB_7, PULL_DOWN) << 7;
}

bit_t mos6502_get_pin_clk(const mos6502_t * mos6502) {
    return icemu_read_node(mos6502->ic, PIN_CLK, PULL_FLOAT);
}

bit_t mos6502_get_pin_clk1(const mos6502_t * mos6502) {
    return icemu_read_node(mos6502->ic, PIN_CLK1, PULL_FLOAT);
}

bit_t mos6502_get_pin_clk2(const mos6502_t * mos6502) {
    return icemu_read_node(mos6502->ic, PIN_CLK2, PULL_FLOAT);
}

bit_t mos6502_get_pin_irq(const mos6502_t * mos6502) {
    return icemu_read_node(mos6502->ic, PIN_IRQ, PULL_FLOAT);
}

bit_t mos6502_get_pin_nmi(const mos6502_t * mos6502) {
    return icemu_read_node(mos6502->ic, PIN_NMI, PULL_FLOAT);
}

bit_t mos6502_get_pin_rdy(const mos6502_t * mos6502) {
    return icemu_read_node(mos6502->ic, PIN_RDY, PULL_FLOAT);
}

bit_t mos6502_get_pin_res(const mos6502_t * mos6502) {
    return icemu_read_node(mos6502->ic, PIN_RES, PULL_FLOAT);
}

bit_t mos6502_get_pin_rw(const mos6502_t * mos6502) {
    return icemu_read_node(mos6502->ic, PIN_RW, PULL_FLOAT);
}

bit_t mos6502_get_pin_so(const mos6502_t * mos6502) {
    return icemu_read_node(mos6502->ic, PIN_SO, PULL_FLOAT);
}

bit_t mos6502_get_pin_sync(const mos6502_t * mos6502) {
    return icemu_read_node(mos6502->ic, PIN_SYNC, PULL_FLOAT);
}

/* --- Register accessors --- */

unsigned short mos6502_get_reg_pc(const mos6502_t * mos6502) {
    return 0x0 |
        icemu_read_node(mos6502->ic, REG_PC_0, PULL_DOWN) << 0 |
        icemu_read_node(mos6502->ic, REG_PC_1, PULL_DOWN) << 1 |
        icemu_read_node(mos6502->ic, REG_PC_2, PULL_DOWN) << 2 |
        icemu_read_node(mos6502->ic, REG_PC_3, PULL_DOWN) << 3 |
        icemu_read_node(mos6502->ic, REG_PC_4, PULL_DOWN) << 4 |
        icemu_read_node(mos6502->ic, REG_PC_5, PULL_DOWN) << 5 |
        icemu_read_node(mos6502->ic, REG_PC_6, PULL_DOWN) << 6 |
        icemu_read_node(mos6502->ic, REG_PC_7, PULL_DOWN) << 7 |
        icemu_read_node(mos6502->ic, REG_PC_8, PULL_DOWN) << 8 |
        icemu_read_node(mos6502->ic, REG_PC_9, PULL_DOWN) << 9 |
        icemu_read_node(mos6502->ic, REG_PC_10, PULL_DOWN) << 10 |
        icemu_read_node(mos6502->ic, REG_PC_11, PULL_DOWN) << 11 |
        icemu_read_node(mos6502->ic, REG_PC_12, PULL_DOWN) << 12 |
        icemu_read_node(mos6502->ic, REG_PC_13, PULL_DOWN) << 13 |
        icemu_read_node(mos6502->ic, REG_PC_14, PULL_DOWN) << 14 |
        icemu_read_node(mos6502->ic, REG_PC_15, PULL_DOWN) << 15;
}

unsigned char mos6502_get_reg_a(const mos6502_t * mos6502) {
    return 0x0 |
        icemu_read_node(mos6502->ic, REG_A_0, PULL_DOWN) << 0 |
        icemu_read_node(mos6502->ic, REG_A_1, PULL_DOWN) << 1 |
        icemu_read_node(mos6502->ic, REG_A_2, PULL_DOWN) << 2 |
        icemu_read_node(mos6502->ic, REG_A_3, PULL_DOWN) << 3 |
        icemu_read_node(mos6502->ic, REG_A_4, PULL_DOWN) << 4 |
        icemu_read_node(mos6502->ic, REG_A_5, PULL_DOWN) << 5 |
        icemu_read_node(mos6502->ic, REG_A_6, PULL_DOWN) << 6 |
        icemu_read_node(mos6502->ic, REG_A_7, PULL_DOWN) << 7;
}

unsigned char mos6502_get_reg_i(const mos6502_t * mos6502) {
    return 0x0 |
        icemu_read_node(mos6502->ic, REG_I_0, PULL_DOWN) << 0 |
        icemu_read_node(mos6502->ic, REG_I_1, PULL_DOWN) << 1 |
        icemu_read_node(mos6502->ic, REG_I_2, PULL_DOWN) << 2 |
        icemu_read_node(mos6502->ic, REG_I_3, PULL_DOWN) << 3 |
        icemu_read_node(mos6502->ic, REG_I_4, PULL_DOWN) << 4 |
        icemu_read_node(mos6502->ic, REG_I_5, PULL_DOWN) << 5 |
        icemu_read_node(mos6502->ic, REG_I_6, PULL_DOWN) << 6 |
        icemu_read_node(mos6502->ic, REG_I_7, PULL_DOWN) << 7;
}

unsigned char mos6502_get_reg_p(const mos6502_t * mos6502) {
    return 0x0 |
        icemu_read_node(mos6502->ic, REG_P_0, PULL_DOWN) << 0 |
        icemu_read_node(mos6502->ic, REG_P_1, PULL_DOWN) << 1 |
        icemu_read_node(mos6502->ic, REG_P_2, PULL_DOWN) << 2 |
        icemu_read_node(mos6502->ic, REG_P_3, PULL_DOWN) << 3 |
        icemu_read_node(mos6502->ic, REG_P_4, PULL_DOWN) << 4 |
        icemu_read_node(mos6502->ic, REG_P_5, PULL_DOWN) << 5 |
        icemu_read_node(mos6502->ic, REG_P_6, PULL_DOWN) << 6 |
        icemu_read_node(mos6502->ic, REG_P_7, PULL_DOWN) << 7;
}

unsigned char mos6502_get_reg_sp(const mos6502_t * mos6502) {
    return 0x0 |
        icemu_read_node(mos6502->ic, REG_SP_0, PULL_DOWN) << 0 |
        icemu_read_node(mos6502->ic, REG_SP_1, PULL_DOWN) << 1 |
        icemu_read_node(mos6502->ic, REG_SP_2, PULL_DOWN) << 2 |
        icemu_read_node(mos6502->ic, REG_SP_3, PULL_DOWN) << 3 |
        icemu_read_node(mos6502->ic, REG_SP_4, PULL_DOWN) << 4 |
        icemu_read_node(mos6502->ic, REG_SP_5, PULL_DOWN) << 5 |
        icemu_read_node(mos6502->ic, REG_SP_6, PULL_DOWN) << 6 |
        icemu_read_node(mos6502->ic, REG_SP_7, PULL_DOWN) << 7;
}

unsigned char mos6502_get_reg_x(const mos6502_t * mos6502) {
    return 0x0 |
        icemu_read_node(mos6502->ic, REG_X_0, PULL_DOWN) << 0 |
        icemu_read_node(mos6502->ic, REG_X_1, PULL_DOWN) << 1 |
        icemu_read_node(mos6502->ic, REG_X_2, PULL_DOWN) << 2 |
        icemu_read_node(mos6502->ic, REG_X_3, PULL_DOWN) << 3 |
        icemu_read_node(mos6502->ic, REG_X_4, PULL_DOWN) << 4 |
        icemu_read_node(mos6502->ic, REG_X_5, PULL_DOWN) << 5 |
        icemu_read_node(mos6502->ic, REG_X_6, PULL_DOWN) << 6 |
        icemu_read_node(mos6502->ic, REG_X_7, PULL_DOWN) << 7;
}

unsigned char mos6502_get_reg_y(const mos6502_t * mos6502) {
    return 0x0 |
        icemu_read_node(mos6502->ic, REG_Y_0, PULL_DOWN) << 0 |
        icemu_read_node(mos6502->ic, REG_Y_1, PULL_DOWN) << 1 |
        icemu_read_node(mos6502->ic, REG_Y_2, PULL_DOWN) << 2 |
        icemu_read_node(mos6502->ic, REG_Y_3, PULL_DOWN) << 3 |
        icemu_read_node(mos6502->ic, REG_Y_4, PULL_DOWN) << 4 |
        icemu_read_node(mos6502->ic, REG_Y_5, PULL_DOWN) << 5 |
        icemu_read_node(mos6502->ic, REG_Y_6, PULL_DOWN) << 6 |
        icemu_read_node(mos6502->ic, REG_Y_7, PULL_DOWN) << 7;
}

/* --- Pin modifiers --- */

void mos6502_set_pin_db(mos6502_t * mos6502, unsigned char data, bool_t sync) {
    icemu_write_node(mos6502->ic, PIN_DB_0, data >> 0 & 0x01, false);
    icemu_write_node(mos6502->ic, PIN_DB_1, data >> 1 & 0x01, false);
    icemu_write_node(mos6502->ic, PIN_DB_2, data >> 2 & 0x01, false);
    icemu_write_node(mos6502->ic, PIN_DB_3, data >> 3 & 0x01, false);
    icemu_write_node(mos6502->ic, PIN_DB_4, data >> 4 & 0x01, false);
    icemu_write_node(mos6502->ic, PIN_DB_5, data >> 5 & 0x01, false);
    icemu_write_node(mos6502->ic, PIN_DB_6, data >> 6 & 0x01, false);
    icemu_write_node(mos6502->ic, PIN_DB_7, data >> 7 & 0x01, false);

    if (sync) {
        icemu_sync(mos6502->ic);
    }
}

void mos6502_set_pin_clk(mos6502_t * mos6502, bit_t data, bool_t sync) {
    icemu_write_node(mos6502->ic, PIN_CLK, data, sync);
}

void mos6502_set_pin_irq(mos6502_t * mos6502, bit_t data, bool_t sync) {
    icemu_write_node(mos6502->ic, PIN_IRQ, data, sync);
}

void mos6502_set_pin_nmi(mos6502_t * mos6502, bit_t data, bool_t sync) {
    icemu_write_node(mos6502->ic, PIN_NMI, data, sync);
}

void mos6502_set_pin_rdy(mos6502_t * mos6502, bit_t data, bool_t sync) {
    icemu_write_node(mos6502->ic, PIN_RDY, data, sync);
}

void mos6502_set_pin_res(mos6502_t * mos6502, bit_t data, bool_t sync) {
    icemu_write_node(mos6502->ic, PIN_RES, data, sync);
}

void mos6502_set_pin_so(mos6502_t * mos6502, bit_t data, bool_t sync) {
    icemu_write_node(mos6502->ic, PIN_SO, data, sync);
}
