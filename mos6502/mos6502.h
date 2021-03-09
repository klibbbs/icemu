#ifndef INCLUDE_MOS6502_MOS6502_H
#define INCLUDE_MOS6502_MOS6502_H

#include "../icemu.h"

/* ------------------------- */
/*    MOS Technology 6502    */
/* ------------------------- */

typedef struct {
    icemu_t * ic;
} mos6502_t;

/* --- Emulator --- */

mos6502_t * mos6502_init();
void mos6502_destroy(mos6502_t * mos6502);
void mos6502_sync(mos6502_t * mos6502);

/* --- Pin accessors --- */

unsigned short mos6502_get_pin_ab(const mos6502_t * mos6502);

unsigned char mos6502_get_pin_db(const mos6502_t * mos6502);

bit_t mos6502_get_pin_clk(const mos6502_t * mos6502);
bit_t mos6502_get_pin_clk1(const mos6502_t * mos6502);
bit_t mos6502_get_pin_clk2(const mos6502_t * mos6502);
bit_t mos6502_get_pin_irq(const mos6502_t * mos6502);
bit_t mos6502_get_pin_nmi(const mos6502_t * mos6502);
bit_t mos6502_get_pin_rdy(const mos6502_t * mos6502);
bit_t mos6502_get_pin_res(const mos6502_t * mos6502);
bit_t mos6502_get_pin_rw(const mos6502_t * mos6502);
bit_t mos6502_get_pin_so(const mos6502_t * mos6502);
bit_t mos6502_get_pin_sync(const mos6502_t * mos6502);

/* --- Register accessors --- */

unsigned short mos6502_get_reg_pc(const mos6502_t * mos6502);

unsigned char mos6502_get_reg_a(const mos6502_t * mos6502);
unsigned char mos6502_get_reg_i(const mos6502_t * mos6502);
unsigned char mos6502_get_reg_p(const mos6502_t * mos6502);
unsigned char mos6502_get_reg_sp(const mos6502_t * mos6502);
unsigned char mos6502_get_reg_x(const mos6502_t * mos6502);
unsigned char mos6502_get_reg_y(const mos6502_t * mos6502);

/* --- Pin modifiers --- */

void mos6502_set_pin_db(mos6502_t * mos6502, unsigned char data, bool_t sync);

void mos6502_set_pin_clk(mos6502_t * mos6502, bit_t data, bool_t sync);
void mos6502_set_pin_irq(mos6502_t * mos6502, bit_t data, bool_t sync);
void mos6502_set_pin_nmi(mos6502_t * mos6502, bit_t data, bool_t sync);
void mos6502_set_pin_rdy(mos6502_t * mos6502, bit_t data, bool_t sync);
void mos6502_set_pin_res(mos6502_t * mos6502, bit_t data, bool_t sync);
void mos6502_set_pin_so(mos6502_t * mos6502, bit_t data, bool_t sync);

#endif /* INCLUDE_MOS6502_MOS6502_H */
