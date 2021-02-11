#ifndef INCLUDE_TRANSISTOR_H
#define INCLUDE_TRANSISTOR_H

#include "node.h"
#include "types.h"

#include <stddef.h>

typedef size_t tx_t;

typedef enum {
  TRANSISTOR_NMOS = 1
} transistor_type_t;

typedef struct {
  transistor_type_t type;
  nx_t gate;
  nx_t c1;
  nx_t c2;
  bool dirty;
} transistor_t;

#endif /* INCLUDE_TRANSISTOR_H */
