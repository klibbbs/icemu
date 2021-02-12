#include "mos6502.h"

#include "debug.h"

int main(int argc, char *  argv[]) {
  mos6502_t * mos6502;

  /* Initialize a MOS 6502 */
  debug(("INIT...\n"));
  mos6502 = mos6502_init();

  debug(("RESET...\n"));
  /* Reset all devices */
  mos6502_reset(mos6502);

  /* TODO */

  debug(("DESTROY...\n"));
  /* Cleanup */
  mos6502_destroy(mos6502);

  return 0;
}
