#include "controller.h"

#include "memory.h"
#include "mos6502.h"

void mos6502_controller_reset(mos6502_t * mos6502) {
  size_t i;

  /* Pull RES low */
  mos6502_set_res(mos6502, BIT_ZERO, false);

  /* Neutralize other inputs */
  mos6502_set_clk(mos6502, BIT_ONE, false);
  mos6502_set_rdy(mos6502, BIT_ONE, false);
  mos6502_set_so(mos6502, BIT_ZERO, false);
  mos6502_set_irq(mos6502, BIT_ONE, false);
  mos6502_set_nmi(mos6502, BIT_ONE, false);

  /* Sync all inputs */
  mos6502_sync(mos6502);

  /* Toggle CLK input for 8 full cycles */
  for (i = 0; i < 16; i++) {
    mos6502_set_clk(mos6502, !mos6502_get_clk(mos6502), true);
  }

  /* Pull RES high and sync */
  mos6502_set_res(mos6502, BIT_ONE, true);
}

void mos6502_controller_step(mos6502_t * mos6502, mos6502_memory_t * memory) {

  /* Toggle CLK input and sync */
  mos6502_set_clk(mos6502, !mos6502_get_clk(mos6502), true);

  /* Access memory only after rising clock edge */
  if (mos6502_get_clk(mos6502)) {
    if (mos6502_get_rw(mos6502) == BIT_ZERO) {

      /* Write memory from data bus */
      mos6502_memory_write(memory, mos6502_get_ab(mos6502), mos6502_get_db(mos6502));
    } else {

      /* Read memory into data bus and sync */
      mos6502_set_db(mos6502, mos6502_memory_read(memory, mos6502_get_ab(mos6502)), true);
    }
  }
}

void mos6502_controller_run(mos6502_t * mos6502, mos6502_memory_t * memory, size_t cycles) {

  /* Each step is a half-cycle */
  for (size_t i = 0; i < cycles; i++) {
    mos6502_controller_step(mos6502, memory);
    mos6502_controller_step(mos6502, memory);
  }
}
