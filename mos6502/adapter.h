#ifndef INCLUDE_MOS6502_ADAPTER_H
#define INCLUDE_MOS6502_ADAPTER_H

#include "../runtime.h"

adapter_t * mos6502_adapter_init();
void mos6502_adapter_destroy(adapter_t * adapter);

#endif /* INCLUDE_MOS6502_ADAPTER_H */
