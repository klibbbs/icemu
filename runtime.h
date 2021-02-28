#ifndef INCLUDE_RUNTIME_H
#define INCLUDE_RUNTIME_H

#include <stddef.h>

typedef enum {
  RC_OK           = 0,
  RC_ERR          = 1,
  RC_FILE_ERR     = 2,
  RC_PARSE_ERR    = 3,
  RC_EMULATOR_ERR = 4,
} rc_t;

typedef struct {
  unsigned int data;
  size_t bits;
} value_t;

typedef struct {
  /* Constants */
  const size_t mem_addr_width;
  const size_t mem_word_width;

  /* Functions */
  void * (* init)();
  void (* destroy)(void * instance);
  void (* reset)(void * instance);
  void (* step)(void * instance);
  void (* run)(void * instance, size_t cycles);
  int (* can_read_pin)(const void * instance, const char * pin);
  int (* can_write_pin)(const void * instance, const char * pin);
  value_t (* read_pin)(const void * instance, const char * pin);
  void (* write_pin)(void * instance, const char * pin, unsigned int data);
  value_t (* read_mem)(const void * instance, unsigned int addr);
  void (* write_mem)(void * instance, unsigned int addr, unsigned int data);
} adapter_t;

/* --- Functions -- */

rc_t runtime_exec_file(const adapter_t * adapter, const char * file);
rc_t runtime_exec_repl(const adapter_t * adapter);

#endif /* INCLUDE_RUNTIME_H */
