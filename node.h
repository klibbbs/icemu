#ifndef INCLUDE_NODE_H
#define INCLUDE_NODE_H

#include "types.h"

#include <stddef.h>

typedef size_t nx_t;

typedef enum {
  BIT_ZERO = 0,
  BIT_ONE  = 1,
  BIT_Z    = -1,
  BIT_META = -2
} bit_t;

typedef enum {
  PULL_DOWN  = -1,
  PULL_FLOAT = 0,
  PULL_UP    = 1
} pull_t;

typedef struct {
  pull_t pull;
  bit_t state;
  bool dirty;
} node_t;

#endif /* INCLUDE_NODE_H */
