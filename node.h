#ifndef INCLUDE_NODE_H
#define INCLUDE_NODE_H

#include "types.h"

#include <stddef.h>

typedef size_t nx_t;

typedef enum {
  PULL_DOWN  = -1,
  PULL_FLOAT = 0,
  PULL_UP    = 1
} pull_t;

typedef struct {
  pull_t load;
  bit_t state;
  bool dirty;
} node_t;

#endif /* INCLUDE_NODE_H */
