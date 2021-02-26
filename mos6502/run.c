#include "adapter.h"

#include "../runtime.h"

#include <stdlib.h>

int main(int argc, char * argv[]) {
  rc_t rc = RC_OK;

  /* Construct a MOS 6502 adapter */
  adapter_t * adapter = mos6502_adapter_init();

  /* Execute files if specified, otherwise open a REPL environment */
  if (argc > 1) {
    int i;

    for (i = 1; i < argc; i++) {
      rc = runtime_exec_file(adapter, argv[i]);

      if (rc != RC_OK) {
        break;
      }
    }
  } else {
    rc = runtime_exec_repl(adapter);
  }

  /* Cleanup */
  mos6502_adapter_destroy(adapter);

  return rc;
}
