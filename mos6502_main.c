#include "mos6502.h"

#include "debug.h"

static unsigned char memory[65536]; /* 16-bit address space */

void step(mos6502_t * cpu) {
  static unsigned int cycle = 1;
  static bit_t clk = BIT_ONE;

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

  /* Print outputs */
  printf("%5u: %c [ab] $%04X [db] $%02X [rw] %x [res] %x [sync] %x\n",
         cycle,
         (mos6502_get_clk(cpu) == BIT_ONE) ? '+' : '-',
         mos6502_get_ab(cpu),
         mos6502_get_db(cpu),
         mos6502_get_rw(cpu),
         mos6502_get_res(cpu),
         mos6502_get_sync(cpu));

  /* Count total cycles */
  cycle += (clk == BIT_ONE) ? 1 : 0;
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
  unsigned int i;
  mos6502_t * cpu;

  /* Initialize devices */
  cpu = mos6502_init();

  /* Reset all devices */
  reset(cpu);

  /* Main loop */
  for (i = 0; i < 16; i++) {
    step(cpu);
  }

  /* Cleanup */
  mos6502_destroy(cpu);

  return 0;
}
