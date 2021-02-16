#ifndef INCLUDE_ICEMU_H
#define INCLUDE_ICEMU_H

#include <stddef.h>

/* --- Types --- */

typedef enum {
  false,
  true
} bool;

typedef enum {
  BIT_ZERO = 0,
  BIT_ONE  = 1,
  BIT_Z    = -1,
  BIT_META = -2
} bit_t;

char bit_char(bit_t bit);

/* --- Node --- */

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

/* --- Transistor --- */

typedef size_t tx_t;

typedef enum {
  TRANSISTOR_NMOS = 1,
  TRANSISTOR_PMOS = 2
} transistor_type_t;

typedef struct {
  transistor_type_t type;
  nx_t gate;
  nx_t c1;
  nx_t c2;
  bool dirty;
} transistor_t;

/* --- IC --- */

typedef struct {
  nx_t on;
  nx_t off;
  nx_t clock;

  const pull_t * nodes;
  nx_t nodes_count;

  const transistor_t * transistors;
  tx_t transistors_count;
} icemu_def_t;

typedef struct {
  bool synced;

  nx_t on;
  nx_t off;

  node_t * nodes;
  size_t nodes_count;

  transistor_t * transistors;
  size_t transistors_count;

  tx_t ** node_gates;
  tx_t * node_gates_lists;
  size_t * node_gates_counts;

  tx_t ** node_channels;
  tx_t * node_channels_lists;
  size_t * node_channels_counts;

  nx_t * network_nodes;
  size_t network_nodes_count;
} icemu_t;

icemu_t * icemu_init(const icemu_def_t * def);
void icemu_destroy(icemu_t * ic);
void icemu_sync(icemu_t * ic);

bit_t icemu_read_node(const icemu_t * ic, nx_t n, pull_t pull);
void icemu_write_node(icemu_t * ic, nx_t n, bit_t state, bool sync);

#endif /* INCLUDE_ICEMU_H */
