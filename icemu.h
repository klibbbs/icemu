#ifndef INCLUDE_ICEMU_H
#define INCLUDE_ICEMU_H

#include "node.h"
#include "transistor.h"

#include <stddef.h>

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

bit_t icemu_read_node(const icemu_t * ic, nx_t n, pull_t pull);
void icemu_write_node(icemu_t * ic, nx_t n, bit_t state);

#endif /* INCLUDE_ICEMU_H */
