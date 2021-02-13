#include "mos6502.h"

#include "debug.h"
#include "types.h"

static unsigned char memory[65536]; /* 16-bit address space */

void step(mos6502_t * cpu) {
  static bit_t clk = BIT_ZERO;

  /* Invert CLK signal */
  clk = (clk == BIT_ONE) ? BIT_ZERO : BIT_ONE;

  /* Write pin synchronously */
  mos6502_set_clk(cpu, clk, true);

  /* Handle memory access */
  if (mos6502_get_rw(cpu) == BIT_ZERO) {
    memory[mos6502_get_ab(cpu)] = mos6502_get_db(cpu);
  } else {
    mos6502_set_db(cpu, memory[mos6502_get_ab(cpu)], true);
  }
}

void reset(mos6502_t * cpu) {
  unsigned int i;

  /* Set RES low */
  mos6502_set_res(cpu, BIT_ZERO, false);

  /* Set neutral inputs */
  mos6502_set_clk(cpu, BIT_ONE, false);
  mos6502_set_rdy(cpu, BIT_ONE, false);
  mos6502_set_so(cpu, BIT_ZERO, false);
  mos6502_set_irq(cpu, BIT_ONE, false);
  mos6502_set_nmi(cpu, BIT_ONE, false);

  /* Sync chip */
  mos6502_sync(cpu);

  /* Wait 8 cycles */
  for (i = 0; i < 16; i++) {
    step(cpu);
  }

  /* Set RES high and sync */
  mos6502_set_res(cpu, BIT_ONE, true);
}

int main(int argc, char *  argv[]) {
  mos6502_t * cpu;

  /* Initialize devices */
  cpu = mos6502_init();

  /* Reset all devices */
  reset(cpu);

  /* TODO */

  /* Cleanup */
  mos6502_destroy(cpu);

  return 0;
}
