#include "mos6502.h"

int main(int argc, char *  argv[]) {

  /* Initialize a MOS 6502 */
  mos6502_t * mos6502 = mos6502_init();

  /* Reset all devices */
  mos6502_reset(mos6502);

  /* TODO */

  /* Cleanup */
  mos6502_destroy(mos6502);

  return 0;
}
