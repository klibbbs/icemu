#ifndef INCLUDE_ICEMU_H
#define INCLUDE_ICEMU_H

#include <stddef.h>

/* --- Types --- */

typedef enum {
    false,
    true
} bool_t;

typedef enum {
    BIT_ZERO = 0,
    BIT_ONE  = 1,
    BIT_Z    = -1,
    BIT_META = -2
} bit_t;

typedef enum {
    LOGIC_NMOS = 0,
    LOGIC_PMOS = 1,
    LOGIC_CMOS = 2,
    LOGIC_TTL  = 3
} logic_t;

typedef enum {
    LEVEL_FLOAT = 0,
    LEVEL_CAP   = 1,
    LEVEL_LOAD  = 2,
    LEVEL_POWER = 3
} level_t;

typedef enum {
    PULL_DOWN  = -1,
    PULL_FLOAT = 0,
    PULL_UP    = 1
} pull_t;

char bit_char(bit_t bit);

/* --- Node --- */

typedef size_t nx_t;

typedef struct {
    level_t level;
    pull_t pull;
    bit_t state;
    bool_t dirty;
} node_t;

/* --- Load --- */

typedef size_t lx_t;

typedef struct {
    pull_t pull;
    nx_t node;
} load_t;

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
    bit_t state;
    bool_t dirty;
} transistor_t;

/* --- Buffer --- */

typedef size_t bx_t;

typedef struct {
    logic_t logic;
    bool_t inverting;
    nx_t input;
    nx_t output;
    bool_t dirty;
} buffer_t;

/* --- IC --- */

typedef struct {
    nx_t on;
    nx_t off;

    size_t nodes_count;

    const load_t * loads;
    size_t loads_count;

    const transistor_t * transistors;
    size_t transistors_count;

    const buffer_t * buffers;
    size_t buffers_count;
} icemu_layout_t;

typedef struct {
    nx_t on;
    nx_t off;

    node_t * nodes;
    size_t nodes_count;

    transistor_t * transistors;
    size_t transistors_count;

    buffer_t * buffers;
    size_t buffers_count;

    tx_t ** node_gates;
    tx_t * node_gates_lists;
    size_t * node_gates_counts;

    tx_t ** node_channels;
    tx_t * node_channels_lists;
    size_t * node_channels_counts;

    bx_t ** node_buffers;
    bx_t * node_buffers_lists;
    size_t * node_buffers_counts;

    nx_t * network_nodes;
    size_t network_nodes_count;
    level_t network_level_down;
    level_t network_level_up;
} icemu_t;

icemu_t * icemu_init(const icemu_layout_t * layout);
void icemu_destroy(icemu_t * ic);
void icemu_sync(icemu_t * ic);

bit_t icemu_read_node(const icemu_t * ic, nx_t n, pull_t pull);
void icemu_write_node(icemu_t * ic, nx_t n, bit_t state, bool_t sync);

#endif /* INCLUDE_ICEMU_H */
