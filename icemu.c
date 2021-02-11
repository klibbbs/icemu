#include "icemu.h"

#include "debug.h"
#include "types.h"

#include <stdlib.h>

/* --- Private declarations --- */

static void icemu_resolve(icemu_t * ic);

static void icemu_network_reset(icemu_t * ic);
static void icemu_network_add(icemu_t * ic, nx_t n);
static void icemu_network_resolve(icemu_t * ic);

static void icemu_transistor_resolve(icemu_t * ic, tx_t t);

enum {ICEMU_RESOLVE_LIMIT = 10};

typedef enum {
  LEVEL_FLOAT  = 0,
  LEVEL_CAP    = 1,
  LEVEL_PULL   = 2,
  LEVEL_SOURCE = 3
} level_t;

/*
======================
   Public functions
======================
*/

icemu_t * icemu_init(const icemu_def_t * def) {
  nx_t n;
  tx_t t, cur;

  icemu_t * ic = malloc(sizeof(icemu_t));

  ic->on = def->on;
  ic->off = def->off;

  /* Allocate and initialize nodes */
  ic->nodes_count = def->nodes_count;
  ic->nodes = malloc(sizeof(node_t) * ic->nodes_count);

  for (n = 0; n < ic->nodes_count; n++) {
    ic->nodes[n].pull  = def->nodes[n];
    ic->nodes[n].state = BIT_Z;
    ic->nodes[n].dirty = true;
  }

  /* Allocate and initialize transistors */
  ic->transistors_count = def->transistors_count;
  ic->transistors = malloc(sizeof(transistor_t) * ic->transistors_count);

  for (t = 0; t < ic->transistors_count; t++) {
    ic->transistors[t].type  = def->transistors[t].type;
    ic->transistors[t].gate  = def->transistors[t].gate;
    ic->transistors[t].c1    = def->transistors[t].c1;
    ic->transistors[t].c2    = def->transistors[t].c2;
    ic->transistors[t].dirty = true;
  }

  /* Allocate and initialize map of nodes to transistor gates */
  ic->node_gates = malloc(sizeof(*ic->node_gates) * ic->nodes_count);
  ic->node_gates_lists = malloc(sizeof(*ic->node_gates_lists) * ic->transistors_count);
  ic->node_gates_counts = calloc(ic->nodes_count, sizeof(*ic->node_gates_counts));

  for (t = 0; t < ic->transistors_count; t++) {
    ic->node_gates_counts[ic->transistors[t].gate]++;
  }

  for (n = 0, cur = 0; n < ic->nodes_count; n++) {
    if (ic->node_gates_counts[n] > 0) {
      cur += ic->node_gates_counts[n];

      ic->node_gates[n] = ic->node_gates_lists + cur;
    } else {
      ic->node_gates[n] = NULL;
    }
  }

  for (t = 0; t < ic->transistors_count; t++) {
    *(--ic->node_gates[ic->transistors[t].gate]) = t;
  }

  /* Allocate and initialize map of nodes to transistors channels */
  ic->node_channels = malloc(sizeof(*ic->node_channels) * ic->nodes_count);
  ic->node_channels_lists = malloc(sizeof(*ic->node_channels_lists) * ic->transistors_count * 2);
  ic->node_channels_counts = calloc(ic->nodes_count, sizeof(*ic->node_channels_counts));

  for (t = 0; t < ic->transistors_count; t++) {
    if (ic->transistors[t].c1 == ic->transistors[t].c2) {
      continue;
    }

    ic->node_channels_counts[ic->transistors[t].c1]++;
    ic->node_channels_counts[ic->transistors[t].c2]++;
  }

  for (n = 0, cur = 0; n < ic->nodes_count; n++) {
    if (ic->node_channels_counts[n] > 0) {
      cur += ic->node_channels_counts[n];

      ic->node_channels[n] = ic->node_channels_lists + cur;
    } else {
      ic->node_channels[n] = NULL;
    }
  }

  for (t = 0; t < ic->transistors_count; t++) {
    if (ic->transistors[t].c1 == ic->transistors[t].c2) {
      continue;
    }

    *(--ic->node_channels[ic->transistors[t].c1]) = t;
    *(--ic->node_channels[ic->transistors[t].c2]) = t;
  }

  /* Allocate network node list */
  ic->network_nodes = malloc(sizeof(*ic->network_nodes) * ic->nodes_count);
  ic->network_nodes_count = 0;

  debug(("Init: %zd nodes\t%zd transistors\n", ic->nodes_count, ic->transistors_count));

  /* Initialize circuit */
  icemu_resolve(ic);

  return ic;
}

void icemu_destroy(icemu_t * ic) {
  free(ic->nodes);
  free(ic->transistors);

  free(ic->node_gates);
  free(ic->node_gates_lists);
  free(ic->node_gates_counts);

  free(ic->node_channels);
  free(ic->node_channels_lists);
  free(ic->node_channels_counts);

  free(ic->network_nodes);

  free(ic);
}

bit_t icemu_read_node(const icemu_t * ic, nx_t n, pull_t pull) {
  bit_t state = ic->nodes[n].state;

  if (state == BIT_Z) {
    switch (pull) {
      case PULL_DOWN:
        return BIT_ZERO;
      case PULL_FLOAT:
        return BIT_Z;
      case PULL_UP:
        return BIT_ONE;
    }
  }

  return state;
}

void icemu_write_node(icemu_t * ic, nx_t n, bit_t state) {
  if (state == BIT_ZERO) {
    ic->nodes[n].pull = PULL_DOWN;
  } else if (state == BIT_ONE) {
    ic->nodes[n].pull = PULL_UP;
  }

  /* TODO */
}

/*
=======================
   Private functions
=======================
*/

void icemu_resolve(icemu_t * ic) {
  int i;
  nx_t n, dirty_nodes;
  tx_t t, dirty_transistors;

  for (i = 0; i < ICEMU_RESOLVE_LIMIT; i++) {

    /* Reset counters */
    dirty_nodes = 0;
    dirty_transistors = 0;

    /* Iterate through all dirty nodes */
    for (n = 0; n < ic->nodes_count; n++) {
      if (ic->nodes[n].dirty) {
        dirty_nodes++;

        if (i > 3) {
          debug(("N %4zd: pull %d state %d\n", n, ic->nodes[n].pull, ic->nodes[n].state));
        }

        /* Find the network of all connected nodes */
        icemu_network_add(ic, n);

        /* Resolve nodes in the network and propagate changes to affected transistors */
        icemu_network_resolve(ic);

        /* Clean up the network */
        icemu_network_reset(ic);
      }
    }

    /* Iterate through all dirty transistors */
    for (t = 0; t < ic->transistors_count; t++) {
      if (ic->transistors[t].dirty) {
        dirty_transistors++;

        if (i > 3) {
          debug(("T %4zd: %4zd | %4zd  %4zd\n", t, ic->transistors[t].gate, ic->transistors[t].c1, ic->transistors[t].c2));
        }

        /* Resolve this transistor and propagate changes to affected nodes */
        icemu_transistor_resolve(ic, t);
      }
    }

    debug(("Iteration %d: %zd nodes\t%zd transistors\n", i, dirty_nodes, dirty_transistors));
  }
}

void icemu_network_reset(icemu_t * ic) {
  ic->network_nodes_count = 0;
}

void icemu_network_add(icemu_t * ic, nx_t n) {
  nx_t nn;
  tx_t t;

  /* Check if this node has already been added to the network */
  for (nn = 0; nn < ic->network_nodes_count; nn++) {
    if (ic->network_nodes[nn] == n) {
      return;
    }
  }

  /* Append this node to the network */
  ic->network_nodes[ic->network_nodes_count++] = n;

  /* Stop here if this node is a voltage source */
  if (n == ic->on || n == ic->off) {
    return;
  }

  /* Search for transistor channels connected to this node */
  for (t = 0; t < ic->node_channels_counts[n]; t++) {
    const transistor_t * transistor = &ic->transistors[ic->node_channels[n][t]];
    const node_t * gate = &ic->nodes[transistor->gate];

    /* If the transistor is enabled, recursively expand the network to the other terminal */
    if (transistor_is_open(transistor, gate)) {
      if (transistor->c1 == n) {
        icemu_network_add(ic, transistor->c2);
      } else if (transistor->c2 == n) {
        icemu_network_add(ic, transistor->c1);
      }
    }
  }
}

void icemu_network_resolve(icemu_t * ic) {
  nx_t nn;
  tx_t t;
  level_t level_down = LEVEL_FLOAT;
  level_t level_up = LEVEL_FLOAT;
  bit_t state = BIT_Z;

  /* Find the strongest signal pulling the network up or down */
  for (nn = 0; nn < ic->network_nodes_count; nn++) {
    nx_t n = ic->network_nodes[nn];
    node_t * node = &ic->nodes[n];

    if (n == ic->off) {
      level_down = LEVEL_SOURCE;
    } else if (n == ic->on) {
      level_up = LEVEL_SOURCE;
    } else if (node->pull == PULL_DOWN) {
      level_down = LEVEL_PULL > level_down ? LEVEL_PULL : level_down;
    } else if (node->pull == PULL_UP) {
      level_up = LEVEL_PULL > level_up ? LEVEL_PULL : level_up;
    } else if (node->state == BIT_ZERO) {
      level_down = LEVEL_CAP > level_down ? LEVEL_CAP : level_down;
    } else if (node->state == BIT_ONE) {
      level_up = LEVEL_CAP > level_up ? LEVEL_CAP : level_down;
    }
  }

  if (level_up > level_down) {
    state = BIT_ONE;
  } else if (level_down > level_up) {
    state = BIT_ZERO;
  } else {
    if (level_up == LEVEL_SOURCE) {
      debug(("VSS <-> VCC: "));
      for (nn = 0; nn < ic->network_nodes_count; nn++) {
        debug(("%4zd ", ic->network_nodes[nn]));
      }
      debug(("\n"));
    }
    state = BIT_Z;
  }

  /* Propagate the strongest signal to all nodes in the network */
  for (nn = 0; nn < ic->network_nodes_count; nn++) {
    nx_t n = ic->network_nodes[nn];
    node_t * node = &ic->nodes[n];

    /* Update dirty flags for affected transistors */
    if (node->state != state) {
      debug(("FLIP %4zd from %d to %d\n", n, node->state, state));
      for (t = 0; t < ic->node_gates_counts[n]; t++) {
        ic->transistors[ic->node_gates[n][t]].dirty = true;
      }
    }

    /* Update node states and clear dirty flags */
    node->state = state;
    node->dirty = false;
  }
}

void icemu_transistor_resolve(icemu_t * ic, tx_t t) {
  transistor_t * transistor = &ic->transistors[t];

  /* Update dirty flags for affected nodes */
  if (transistor->dirty) {
    ic->nodes[transistor->c1].dirty = true;
    ic->nodes[transistor->c2].dirty = true;
  }

  /* Clear transistor dirty flag */
  transistor->dirty = false;
}
