#include "adapter.h"

#include "controller.h"
#include "memory.h"

#include "../runtime.h"

#include "../../perfect6502/perfect6502.h"
#include "../../perfect6502/types.h"
#include "../../perfect6502/netlist_sim.h"

#include <stdlib.h>
#include <string.h>

extern BOOL netlist_6502_node_is_pullup[];
extern netlist_transdefs netlist_6502_transdefs[];

/* --- Wrapper declarations --- */

BOOL perfect6502_get_clk(state_t * perfect6502);
BOOL perfect6502_get_clk1(state_t * perfect6502);
BOOL perfect6502_get_clk2(state_t * perfect6502);
BOOL perfect6502_get_irq(state_t * perfect6502);
BOOL perfect6502_get_nmi(state_t * perfect6502);
BOOL perfect6502_get_res(state_t * perfect6502);
BOOL perfect6502_get_rdy(state_t * perfect6502);
BOOL perfect6502_get_rw(state_t * perfect6502);
BOOL perfect6502_get_so(state_t * perfect6502);
BOOL perfect6502_get_sync(state_t * perfect6502);

void  perfect6502_set_clk(state_t * perfect6502, BOOL s);
void  perfect6502_set_irq(state_t * perfect6502, BOOL s);
void  perfect6502_set_nmi(state_t * perfect6502, BOOL s);
void  perfect6502_set_res(state_t * perfect6502, BOOL s);
void  perfect6502_set_rdy(state_t * perfect6502, BOOL s);
void  perfect6502_set_so(state_t * perfect6502, BOOL s);

/* --- Pin mapping --- */

typedef struct {
    const char * pin;
    unsigned short (* read_func)(state_t * perfect6502);
    void (* write_func)(state_t * perfect6502, unsigned short data);
} pin_16_func_t;

typedef struct {
    const char * pin;
    unsigned char (* read_func)(state_t * perfect6502);
    void (* write_func)(state_t * perfect6502, unsigned char data);
} pin_8_func_t;

typedef struct {
    const char * pin;
    BOOL (* read_func)(state_t * perfect6502);
    void (* write_func)(state_t * perfect6502, BOOL state);
} pin_1_func_t;

static const pin_16_func_t PERFECT6502_PIN_16_HEX_MAP[] = {
    { "ab.pin", readAddressBus, NULL },
    { "pc.reg", readPC, NULL },
};

static const size_t PERFECT6502_PIN_16_HEX_COUNT =
    sizeof(PERFECT6502_PIN_16_HEX_MAP) / sizeof(pin_16_func_t);

static const pin_8_func_t PERFECT6502_PIN_8_HEX_MAP[] = {
    { "db.pin", readDataBus, writeDataBus },
    { "a.reg", readA, NULL },
    { "x.reg", readX, NULL },
    { "y.reg", readY, NULL },
    { "sp.reg", readSP, NULL },
    { "i.reg", readIR, NULL },
};

static const size_t PERFECT6502_PIN_8_HEX_COUNT =
    sizeof(PERFECT6502_PIN_8_HEX_MAP) / sizeof(pin_8_func_t);

static const pin_8_func_t PERFECT6502_PIN_8_BIN_MAP[] = {
    { "p.reg", readP, NULL },
};

static const size_t PERFECT6502_PIN_8_BIN_COUNT =
    sizeof(PERFECT6502_PIN_8_BIN_MAP) / sizeof(pin_8_func_t);

static const pin_1_func_t PERFECT6502_PIN_1_MAP[] = {
    { "clk.pin", perfect6502_get_clk, perfect6502_set_clk },
    { "clk1.pin", perfect6502_get_clk1, NULL },
    { "clk2.pin", perfect6502_get_clk2, NULL },
    { "irq.pin", perfect6502_get_irq, perfect6502_set_irq },
    { "nmi.pin", perfect6502_get_nmi, perfect6502_set_nmi },
    { "res.pin", perfect6502_get_res, perfect6502_set_res },
    { "rdy.pin", perfect6502_get_rdy, perfect6502_set_rdy },
    { "rw.pin", perfect6502_get_rw, NULL },
    { "so.pin", perfect6502_get_so, perfect6502_set_so },
    { "sync.pin", perfect6502_get_sync, NULL },
};

static const size_t PERFECT6502_PIN_1_COUNT = sizeof(PERFECT6502_PIN_1_MAP) / sizeof(pin_1_func_t);

/* --- Adapter declarations --- */

typedef struct {
    state_t * perfect6502;
    perfect6502_memory_t * memory;
} perfect6502_instance_t;

static void * adapter_instance_init();
static void adapter_instance_destroy(void * instance);
static void adapter_instance_reset(void * instance);
static void adapter_instance_step(void * instance);
static void adapter_instance_run(void * instance, size_t cycles);
static int adapter_instance_can_read_pin(const void * instance, const char * pin);
static int adapter_instance_can_write_pin(const void * instance, const char * pin);
static value_t adapter_instance_read_pin(const void * instance, const char * pin);
static void adapter_instance_write_pin(void * instance, const char * pin, unsigned int data);
static value_t adapter_instance_read_mem(const void * instance, unsigned int addr);
static void adapter_instance_write_mem(void * instance, unsigned int addr, unsigned int data);

/* --- Adapter singleton --- */

static const adapter_t PERFECT6502_ADAPTER = {
    "perfect6502",
    "MOS Technology 6502",
    PERFECT6502_MEMORY_ADDR_WIDTH,
    PERFECT6502_MEMORY_WORD_WIDTH,
    adapter_instance_init,
    adapter_instance_destroy,
    adapter_instance_reset,
    adapter_instance_step,
    adapter_instance_run,
    adapter_instance_can_read_pin,
    adapter_instance_can_write_pin,
    adapter_instance_read_pin,
    adapter_instance_write_pin,
    adapter_instance_read_mem,
    adapter_instance_write_mem,
};

/* --- Public functions --- */

const adapter_t * perfect6502_adapter() {
    return &PERFECT6502_ADAPTER;
}

/* --- Adapter functions --- */

void * adapter_instance_init() {
    perfect6502_instance_t * perfect6502_instance = malloc(sizeof(perfect6502_instance_t));

    perfect6502_instance->perfect6502 = setupNodesAndTransistors(netlist_6502_transdefs,
                                                                 netlist_6502_node_is_pullup,
                                                                 1725 /* Num nodes */,
                                                                 3510 /* Num transistors */,
                                                                 558 /* VSS */,
                                                                 657 /* VCC */);
    perfect6502_instance->memory = perfect6502_memory_init();

    return perfect6502_instance;
}

void adapter_instance_destroy(void * instance) {
    perfect6502_instance_t * perfect6502_instance = (perfect6502_instance_t *)instance;

    destroyChip(perfect6502_instance->perfect6502);
    perfect6502_memory_destroy(perfect6502_instance->memory);

    free(perfect6502_instance);
}

void adapter_instance_reset(void * instance) {
    perfect6502_instance_t * perfect6502_instance = (perfect6502_instance_t *)instance;

    perfect6502_controller_reset(perfect6502_instance->perfect6502);
}

void adapter_instance_step(void * instance) {
    perfect6502_instance_t * perfect6502_instance = (perfect6502_instance_t *)instance;

    perfect6502_controller_step(perfect6502_instance->perfect6502, perfect6502_instance->memory);
}

void adapter_instance_run(void * instance, size_t cycles) {
    perfect6502_instance_t * perfect6502_instance = (perfect6502_instance_t *)instance;

    perfect6502_controller_run(perfect6502_instance->perfect6502, perfect6502_instance->memory, cycles);
}

int adapter_instance_can_read_pin(const void * instance, const char * pin) {
    size_t i;

    for (i = 0; i < PERFECT6502_PIN_16_HEX_COUNT; i++) {
        if (strcmp(pin, PERFECT6502_PIN_16_HEX_MAP[i].pin) == 0) {
            if (PERFECT6502_PIN_16_HEX_MAP[i].read_func == NULL) {
                return 0;
            } else {
                return 1;
            }
        }
    }

    for (i = 0; i < PERFECT6502_PIN_8_HEX_COUNT; i++) {
        if (strcmp(pin, PERFECT6502_PIN_8_HEX_MAP[i].pin) == 0) {
            if (PERFECT6502_PIN_8_HEX_MAP[i].read_func == NULL) {
                return 0;
            } else {
                return 1;
            }
        }
    }

    for (i = 0; i < PERFECT6502_PIN_8_BIN_COUNT; i++) {
        if (strcmp(pin, PERFECT6502_PIN_8_BIN_MAP[i].pin) == 0) {
            if (PERFECT6502_PIN_8_BIN_MAP[i].read_func == NULL) {
                return 0;
            } else {
                return 1;
            }
        }
    }

    for (i = 0; i < PERFECT6502_PIN_1_COUNT; i++) {
        if (strcmp(pin, PERFECT6502_PIN_1_MAP[i].pin) == 0) {
            if (PERFECT6502_PIN_1_MAP[i].read_func == NULL) {
                return 0;
            } else {
                return 1;
            }
        }
    }

    return 0;
}

int adapter_instance_can_write_pin(const void * instance, const char * pin) {
    size_t i;

    for (i = 0; i < PERFECT6502_PIN_16_HEX_COUNT; i++) {
        if (strcmp(pin, PERFECT6502_PIN_16_HEX_MAP[i].pin) == 0) {
            if (PERFECT6502_PIN_16_HEX_MAP[i].write_func == NULL) {
                return 0;
            } else {
                return 1;
            }
        }
    }

    for (i = 0; i < PERFECT6502_PIN_8_HEX_COUNT; i++) {
        if (strcmp(pin, PERFECT6502_PIN_8_HEX_MAP[i].pin) == 0) {
            if (PERFECT6502_PIN_8_HEX_MAP[i].write_func == NULL) {
                return 0;
            } else {
                return 1;
            }
        }
    }

    for (i = 0; i < PERFECT6502_PIN_8_BIN_COUNT; i++) {
        if (strcmp(pin, PERFECT6502_PIN_8_BIN_MAP[i].pin) == 0) {
            if (PERFECT6502_PIN_8_BIN_MAP[i].write_func == NULL) {
                return 0;
            } else {
                return 1;
            }
        }
    }

    for (i = 0; i < PERFECT6502_PIN_1_COUNT; i++) {
        if (strcmp(pin, PERFECT6502_PIN_1_MAP[i].pin) == 0) {
            if (PERFECT6502_PIN_1_MAP[i].write_func == NULL) {
                return 0;
            } else {
                return 1;
            }
        }
    }

    return 0;
}

value_t adapter_instance_read_pin(const void * instance, const char * pin) {
    const perfect6502_instance_t * perfect6502_instance = (perfect6502_instance_t *)instance;
    size_t i;

    for (i = 0; i < PERFECT6502_PIN_16_HEX_COUNT; i++) {
        if (strcmp(pin, PERFECT6502_PIN_16_HEX_MAP[i].pin) == 0) {
            if (PERFECT6502_PIN_16_HEX_MAP[i].read_func == NULL) {
                break;
            }

            return (value_t){ PERFECT6502_PIN_16_HEX_MAP[i].read_func(perfect6502_instance->perfect6502), 16, 16 };
        }
    }

    for (i = 0; i < PERFECT6502_PIN_8_HEX_COUNT; i++) {
        if (strcmp(pin, PERFECT6502_PIN_8_HEX_MAP[i].pin) == 0) {
            if (PERFECT6502_PIN_8_HEX_MAP[i].read_func == NULL) {
                break;
            }

            return (value_t){ PERFECT6502_PIN_8_HEX_MAP[i].read_func(perfect6502_instance->perfect6502), 8, 16 };
        }
    }

    for (i = 0; i < PERFECT6502_PIN_8_BIN_COUNT; i++) {
        if (strcmp(pin, PERFECT6502_PIN_8_BIN_MAP[i].pin) == 0) {
            if (PERFECT6502_PIN_8_BIN_MAP[i].read_func == NULL) {
                break;
            }

            return (value_t){ PERFECT6502_PIN_8_BIN_MAP[i].read_func(perfect6502_instance->perfect6502), 8, 2 };
        }
    }

    for (i = 0; i < PERFECT6502_PIN_1_COUNT; i++) {
        if (strcmp(pin, PERFECT6502_PIN_1_MAP[i].pin) == 0) {
            if (PERFECT6502_PIN_1_MAP[i].read_func == NULL) {
                break;
            }

            return (value_t){ PERFECT6502_PIN_1_MAP[i].read_func(perfect6502_instance->perfect6502), 1, 10 };
        }
    }

    return (value_t){ 0, 0, 0 };
}

void adapter_instance_write_pin(void * instance, const char * pin, unsigned int data) {
    perfect6502_instance_t * perfect6502_instance = (perfect6502_instance_t *)instance;
    size_t i;

    for (i = 0; i < PERFECT6502_PIN_16_HEX_COUNT; i++) {
        if (strcmp(pin, PERFECT6502_PIN_16_HEX_MAP[i].pin) == 0) {
            if (PERFECT6502_PIN_16_HEX_MAP[i].write_func == NULL) {
                break;
            }

            PERFECT6502_PIN_16_HEX_MAP[i].write_func(perfect6502_instance->perfect6502, data);
            return;
        }
    }

    for (i = 0; i < PERFECT6502_PIN_8_HEX_COUNT; i++) {
        if (strcmp(pin, PERFECT6502_PIN_8_HEX_MAP[i].pin) == 0) {
            if (PERFECT6502_PIN_8_HEX_MAP[i].write_func == NULL) {
                break;
            }

            PERFECT6502_PIN_8_HEX_MAP[i].write_func(perfect6502_instance->perfect6502, data);
            return;
        }
    }

    for (i = 0; i < PERFECT6502_PIN_8_BIN_COUNT; i++) {
        if (strcmp(pin, PERFECT6502_PIN_8_BIN_MAP[i].pin) == 0) {
            if (PERFECT6502_PIN_8_BIN_MAP[i].write_func == NULL) {
                break;
            }

            PERFECT6502_PIN_8_BIN_MAP[i].write_func(perfect6502_instance->perfect6502, data);
            return;
        }
    }

    for (i = 0; i < PERFECT6502_PIN_1_COUNT; i++) {
        if (strcmp(pin, PERFECT6502_PIN_1_MAP[i].pin) == 0) {
            if (PERFECT6502_PIN_1_MAP[i].write_func == NULL) {
                break;
            }

            PERFECT6502_PIN_1_MAP[i].write_func(perfect6502_instance->perfect6502, data);
            return;
        }
    }
}

value_t adapter_instance_read_mem(const void * instance, unsigned int addr) {
    const perfect6502_instance_t * perfect6502_instance = (perfect6502_instance_t *)instance;

    value_t val = {
        perfect6502_memory_read(perfect6502_instance->memory, addr),
        PERFECT6502_MEMORY_WORD_WIDTH,
        16,
    };

    return val;
}

void adapter_instance_write_mem(void * instance, unsigned int addr, unsigned int data) {
    perfect6502_instance_t * perfect6502_instance = (perfect6502_instance_t *)instance;

    perfect6502_memory_write(perfect6502_instance->memory, addr, data);
}

/* --- Wrapper functions --- */

BOOL perfect6502_get_clk(state_t * perfect6502) {
    return isNodeHigh(perfect6502, 1171);
}

BOOL perfect6502_get_clk1(state_t * perfect6502) {
    return isNodeHigh(perfect6502, 1163);
}

BOOL perfect6502_get_clk2(state_t * perfect6502) {
    return isNodeHigh(perfect6502, 421);
}

BOOL perfect6502_get_irq(state_t * perfect6502) {
    return isNodeHigh(perfect6502, 103);
}

BOOL perfect6502_get_nmi(state_t * perfect6502) {
    return isNodeHigh(perfect6502, 1297);
}

BOOL perfect6502_get_res(state_t * perfect6502) {
    return isNodeHigh(perfect6502, 159);
}

BOOL perfect6502_get_rdy(state_t * perfect6502) {
    return isNodeHigh(perfect6502, 89);
}

BOOL perfect6502_get_rw(state_t * perfect6502) {
    return isNodeHigh(perfect6502, 1156);
}

BOOL perfect6502_get_so(state_t * perfect6502) {
    return isNodeHigh(perfect6502, 1672);
}

BOOL perfect6502_get_sync(state_t * perfect6502) {
    return isNodeHigh(perfect6502, 539);
}

void  perfect6502_set_clk(state_t * perfect6502, BOOL s) {
    setNode(perfect6502, 1171, s);
}

void  perfect6502_set_irq(state_t * perfect6502, BOOL s) {
    setNode(perfect6502, 103, s);
}

void  perfect6502_set_nmi(state_t * perfect6502, BOOL s) {
    setNode(perfect6502, 1297, s);
}

void  perfect6502_set_res(state_t * perfect6502, BOOL s) {
    setNode(perfect6502, 159, s);
}

void  perfect6502_set_rdy(state_t * perfect6502, BOOL s) {
    setNode(perfect6502, 89, s);
}

void  perfect6502_set_so(state_t * perfect6502, BOOL s) {
    setNode(perfect6502, 1672, s);
}
