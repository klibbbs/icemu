#include "mos6502.h"
#include "../perfect6502/perfect6502.h"

void compare(state_t * benchmark, mos6502_t * cpu) {
}

int main(int argc, char * argv[]) {
  state_t * benchmark;
  mos6502_t * cpu;

  /* Initialize benchmark emulator */
  benchmark = initAndResetChip();

  /* Initialize MOS6502 */
  cpu = mos6502_init();

  /* Compare start-up states */
  compare(benchmark, cpu);

  /* Cleanup */
  destroyChip(benchmark);
  mos6502_destroy(cpu);

  return 0;
}
