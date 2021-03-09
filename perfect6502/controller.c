#include "controller.h"

#include "memory.h"

#include "../../perfect6502/perfect6502.h"
#include "../../perfect6502/types.h"
#include "../../perfect6502/netlist_sim.h"

void perfect6502_controller_reset(state_t * perfect6502) {
    size_t i;

    /* Pull REST low and initialize other inputs */
    setNode(perfect6502, 159  /* RES */, 0);
    setNode(perfect6502, 1171 /* CLK */, 1);
    setNode(perfect6502, 89   /* RDY */, 1);
    setNode(perfect6502, 1672 /* SO  */, 0);
    setNode(perfect6502, 103  /* IRQ */, 1);
    setNode(perfect6502, 1297 /* NMI */, 1);

    /* Recalculate all nodes */
    stabilizeChip(perfect6502);

    /* Toggle CLK input for 8 full cycles */
    for (i = 0; i < 16; i++) {
        setNode(perfect6502, 1171, !isNodeHigh(perfect6502, 1171));
    }

    /* Pull RES high */
    setNode(perfect6502, 159 /* RES */, 1);
    recalcNodeList(perfect6502);
}

void perfect6502_controller_step(state_t * perfect6502, perfect6502_memory_t * memory) {

    /* Toggle CLK input */
    setNode(perfect6502, 1171, !isNodeHigh(perfect6502, 1171));

    /* Access memory only after rising clock edge */
    if (isNodeHigh(perfect6502, 1171 /* CLK */)) {
        if (isNodeHigh(perfect6502, 1156 /* RW */)) {

            /* Read memory into data bus and sync */
            writeDataBus(perfect6502, perfect6502_memory_read(memory, readAddressBus(perfect6502)));
        } else {

            /* Write memory from data bus */
            perfect6502_memory_write(memory, readAddressBus(perfect6502), readDataBus(perfect6502));
        }
    }
}

void perfect6502_controller_run(state_t * perfect6502,
                                perfect6502_memory_t * memory,
                                size_t cycles) {

    /* Each step is a half-cycle */
    for (size_t i = 0; i < cycles; i++) {
        perfect6502_controller_step(perfect6502, memory);
        perfect6502_controller_step(perfect6502, memory);
    }
}
