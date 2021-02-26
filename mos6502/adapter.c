#include "adapter.h"

#include "controller.h"
#include "memory.h"
#include "mos6502.h"

#include "../runtime.h"

#include <stdlib.h>
#include <string.h>

/* --- Pin mapping --- */

typedef struct {
  const char * pin;
  unsigned short (* read_func)(const mos6502_t * mos6502);
  void (* write_func)(mos6502_t * mos6502, unsigned short data, bool sync);
} pin_16_func_t;

typedef struct {
  const char * pin;
  unsigned char (* read_func)(const mos6502_t * mos6502);
  void (* write_func)(mos6502_t * mos6502, unsigned char data, bool sync);
} pin_8_func_t;

typedef struct {
  const char * pin;
  bit_t (* read_func)(const mos6502_t * mos6502);
  void (* write_func)(mos6502_t * mos6502, bit_t state, bool sync);
} pin_1_func_t;

static const pin_16_func_t MOS6502_PIN_16_MAP[] = {
  { "ab.pin", mos6502_get_ab, NULL },
  { "pc.reg", mos6502_get_reg_pc, NULL },
};

static const size_t MOS6502_PIN_16_COUNT = sizeof(MOS6502_PIN_16_MAP) / sizeof(pin_16_func_t);

static const pin_8_func_t MOS6502_PIN_8_MAP[] = {
  { "db.pin", mos6502_get_db, mos6502_set_db },
  { "p.reg", mos6502_get_reg_p, NULL },
  { "a.reg", mos6502_get_reg_a, NULL },
  { "x.reg", mos6502_get_reg_x, NULL },
  { "y.reg", mos6502_get_reg_y, NULL },
  { "sp.reg", mos6502_get_reg_sp, NULL },
  { "i.reg", mos6502_get_reg_i, NULL },
};

static const size_t MOS6502_PIN_8_COUNT = sizeof(MOS6502_PIN_8_MAP) / sizeof(pin_8_func_t);

static const pin_1_func_t MOS6502_PIN_1_MAP[] = {
  { "clk.pin", mos6502_get_clk, mos6502_set_clk },
  { "clk1.pin", mos6502_get_clk1, NULL },
  { "clk2.pin", mos6502_get_clk2, NULL },
  { "irq.pin", mos6502_get_irq, mos6502_set_irq },
  { "nmi.pin", mos6502_get_nmi, mos6502_set_nmi },
  { "res.pin", mos6502_get_res, mos6502_set_res },
  { "rdy.pin", mos6502_get_rdy, mos6502_set_rdy },
  { "rw.pin", mos6502_get_rw, NULL },
  { "so.pin", mos6502_get_so, mos6502_set_so },
  { "sync.pin", mos6502_get_sync, NULL },
};

static const size_t MOS6502_PIN_1_COUNT = sizeof(MOS6502_PIN_1_MAP) / sizeof(pin_1_func_t);

/* --- Adapter declarations --- */

typedef struct {
  mos6502_t * mos6502;
  mos6502_memory_t * memory;
} mos6502_instance_t;

static void * adapter_instance_init();
static void adapter_instance_destroy(void * instance);
static void adapter_instance_reset(void * instance);
static void adapter_instance_step(void * instance);
static void adapter_instance_run(void * instance, size_t cycles);
static int adapter_instance_can_read_pin(const void * instance, const char * pin);
static int adapter_instance_can_write_pin(const void * instance, const char * pin);
static output_t adapter_instance_read_pin(const void * instance, const char * pin);
static void adapter_instance_write_pin(void * instance, const char * pin, unsigned int data);
static output_t adapter_instance_read_mem(const void * instance, unsigned int addr);
static void adapter_instance_write_mem(void * instance, unsigned int addr, unsigned int data);

/* --- Public functions --- */

adapter_t * mos6502_adapter_init() {
  adapter_t * adapter = malloc(sizeof(adapter_t));

  adapter->init = adapter_instance_init;
  adapter->destroy = adapter_instance_destroy;
  adapter->reset = adapter_instance_reset;
  adapter->step = adapter_instance_step;
  adapter->run = adapter_instance_run;
  adapter->can_read_pin = adapter_instance_can_read_pin;
  adapter->can_write_pin = adapter_instance_can_write_pin;
  adapter->read_pin = adapter_instance_read_pin;
  adapter->write_pin = adapter_instance_write_pin;
  adapter->read_mem = adapter_instance_read_mem;
  adapter->write_mem = adapter_instance_write_mem;

  return adapter;
}

void mos6502_adapter_destroy(adapter_t * adapter) {
  free(adapter);
}

/* --- Adapter functions --- */

void * adapter_instance_init() {
  mos6502_instance_t * mos6502_instance = malloc(sizeof(mos6502_instance_t));

  mos6502_instance->mos6502 = mos6502_init();
  mos6502_instance->memory = mos6502_memory_init();

  return mos6502_instance;
}

void adapter_instance_destroy(void * instance) {
  mos6502_instance_t * mos6502_instance = (mos6502_instance_t *)instance;

  mos6502_destroy(mos6502_instance->mos6502);
  mos6502_memory_destroy(mos6502_instance->memory);

  free(mos6502_instance);
}

void adapter_instance_reset(void * instance) {
  mos6502_instance_t * mos6502_instance = (mos6502_instance_t *)instance;

  mos6502_controller_reset(mos6502_instance->mos6502);
}

void adapter_instance_step(void * instance) {
  mos6502_instance_t * mos6502_instance = (mos6502_instance_t *)instance;

  mos6502_controller_step(mos6502_instance->mos6502, mos6502_instance->memory);
}

void adapter_instance_run(void * instance, size_t cycles) {
  mos6502_instance_t * mos6502_instance = (mos6502_instance_t *)instance;

  mos6502_controller_run(mos6502_instance->mos6502, mos6502_instance->memory, cycles);
}

int adapter_instance_can_read_pin(const void * instance, const char * pin) {
  size_t i;

  for (i = 0; i < MOS6502_PIN_16_COUNT; i++) {
    if (strcmp(pin, MOS6502_PIN_16_MAP[i].pin) == 0) {
      if (MOS6502_PIN_16_MAP[i].read_func == NULL) {
        return 0;
      } else {
        return 1;
      }
    }
  }

  for (i = 0; i < MOS6502_PIN_8_COUNT; i++) {
    if (strcmp(pin, MOS6502_PIN_8_MAP[i].pin) == 0) {
      if (MOS6502_PIN_8_MAP[i].read_func == NULL) {
        return 0;
      } else {
        return 1;
      }
    }
  }

  for (i = 0; i < MOS6502_PIN_1_COUNT; i++) {
    if (strcmp(pin, MOS6502_PIN_1_MAP[i].pin) == 0) {
      if (MOS6502_PIN_1_MAP[i].read_func == NULL) {
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

  for (i = 0; i < MOS6502_PIN_16_COUNT; i++) {
    if (strcmp(pin, MOS6502_PIN_16_MAP[i].pin) == 0) {
      if (MOS6502_PIN_16_MAP[i].write_func == NULL) {
        return 0;
      } else {
        return 1;
      }
    }
  }

  for (i = 0; i < MOS6502_PIN_8_COUNT; i++) {
    if (strcmp(pin, MOS6502_PIN_8_MAP[i].pin) == 0) {
      if (MOS6502_PIN_8_MAP[i].write_func == NULL) {
        return 0;
      } else {
        return 1;
      }
    }
  }

  for (i = 0; i < MOS6502_PIN_1_COUNT; i++) {
    if (strcmp(pin, MOS6502_PIN_1_MAP[i].pin) == 0) {
      if (MOS6502_PIN_1_MAP[i].write_func == NULL) {
        return 0;
      } else {
        return 1;
      }
    }
  }

  return 0;
}

output_t adapter_instance_read_pin(const void * instance, const char * pin) {
  /* TODO */
  return (output_t){ 0, 0 };
}

void adapter_instance_write_pin(void * instance, const char * pin, unsigned int data) {
  /* TODO */
}

output_t adapter_instance_read_mem(const void * instance, unsigned int addr) {
  mos6502_instance_t * mos6502_instance = (mos6502_instance_t *)instance;

  output_t output = {
    mos6502_memory_read(mos6502_instance->memory, addr),
    MOS6502_WORD_WIDTH,
  };

  return output;
}

void adapter_instance_write_mem(void * instance, unsigned int addr, unsigned int data) {
  mos6502_instance_t * mos6502_instance = (mos6502_instance_t *)instance;

  mos6502_memory_write(mos6502_instance->memory, addr, data);
}
