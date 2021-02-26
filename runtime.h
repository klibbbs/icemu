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
} output_t;

typedef struct {
  void * (* init)();
  void (* destroy)(void * instance);
  void (* reset)(void * instance);
  int (* can_read_pin)(const void * instance, const char * pin);
  int (* can_write_pin)(const void * instance, const char * pin);
  output_t (* read_pin)(const void * instance, const char * pin);
  void (* write_pin)(void * instance, const char * pin, unsigned int data);
  output_t (* read_mem)(const void * instance, unsigned int addr);
  void (* write_mem)(void * instance, unsigned int addr, unsigned int data);
} adapter_t;

/* --- Functions -- */

rc_t runtime_exec_file(const adapter_t * adapter, const char * file);
rc_t runtime_exec_repl(const adapter_t * adapter);

#endif /* INCLUDE_RUNTIME_H */
