#include "icemu.h"

#include "debug.h"

#include <stdio.h>
#include <stdlib.h>

/* --- Private declarations --- */

enum { ICEMU_RESOLVE_LIMIT = 50 };

static void icemu_resolve(icemu_t * ic);
static void icemu_network_reset(icemu_t * ic);
static void icemu_network_add(icemu_t * ic, nx_t n);
static void icemu_network_resolve(icemu_t * ic, unsigned int iter);

static void icemu_transistor_init(icemu_t * ic, tx_t t, const transistor_t * layout);
static void icemu_transistor_resolve(icemu_t * ic, tx_t t);
static bit_t icemu_transistor_state(icemu_t * ic, tx_t t);

static void icemu_buffer_init(icemu_t * ic, bx_t b, const buffer_t * layout);
static void icemu_buffer_resolve(icemu_t * ic, bx_t b);
static bit_t icemu_buffer_output(icemu_t * ic, bx_t b);

/* =========== */
/*    Types    */
/* =========== */

/* --- Public functions --- */

char bit_char(bit_t bit) {
    switch (bit) {
        case BIT_ZERO:
            return '0';
        case BIT_ONE:
            return '1';
        case BIT_Z:
            return 'Z';
        case BIT_META:
            return 'M';
    }

    return '?';
}

/* --- Private functions --- */

level_t bit_level(bit_t bit, logic_t logic) {
    switch (bit) {
        case BIT_ZERO:
            return logic == LOGIC_PMOS ? LEVEL_LOAD : LEVEL_POWER;
        case BIT_ONE:
            return logic == LOGIC_NMOS ? LEVEL_LOAD : LEVEL_POWER;
        default:
            return (logic == LOGIC_NMOS || logic == LOGIC_PMOS) ? LEVEL_LOAD : LEVEL_POWER;
    }
}

pull_t bit_pull(bit_t bit) {
    pull_t map[4] = {PULL_DOWN, PULL_UP, PULL_FLOAT, PULL_FLOAT};

    return map[(unsigned char)bit];
}

/* ============ */
/*    Device    */
/* ============ */

/* --- Public functions  --- */

icemu_t * icemu_init(const icemu_layout_t * layout) {
    nx_t n;
    lx_t l;
    tx_t t, tcur;
    bx_t b, bcur;

    icemu_t * ic = malloc(sizeof(icemu_t));

    /* --- Nodes --- */

    /* Initialize node list */
    ic->nodes_count = layout->nodes_count;
    ic->nodes = malloc(sizeof(node_t) * ic->nodes_count);

    for (n = 0; n < ic->nodes_count; n++) {
        ic->nodes[n].level = LEVEL_FLOAT;
        ic->nodes[n].pull  = PULL_FLOAT;
        ic->nodes[n].state = BIT_Z;
        ic->nodes[n].dirty = false;
    }

    /* Initialize power sources */
    ic->on = layout->on;
    ic->off = layout->off;

    ic->nodes[ic->on].level = LEVEL_POWER;
    ic->nodes[ic->on].pull = PULL_UP;
    ic->nodes[ic->on].state = BIT_ONE;

    ic->nodes[ic->off].level = LEVEL_POWER;
    ic->nodes[ic->off].pull = PULL_DOWN;
    ic->nodes[ic->off].state = BIT_ZERO;

    /* Apply loads */
    for (l = 0; l < layout->loads_count; l++) {
        ic->nodes[layout->loads[l].node].level = LEVEL_LOAD;
        ic->nodes[layout->loads[l].node].pull = layout->loads[l].pull;
    }

    /* --- Transistors --- */

    /* Initialize transistor list */
    ic->transistors_count = layout->transistors_count;
    ic->transistors = malloc(sizeof(transistor_t) * ic->transistors_count);

    for (t = 0; t < ic->transistors_count; t++) {
        icemu_transistor_init(ic, t, &layout->transistors[t]);
    }

    /* Map nodes to transistor gates */
    ic->node_gates        = calloc(ic->nodes_count, sizeof(tx_t *));
    ic->node_gates_lists  = calloc(ic->transistors_count, sizeof(tx_t));
    ic->node_gates_counts = calloc(ic->nodes_count, sizeof(size_t));

    for (t = 0; t < ic->transistors_count; t++) {
        ic->node_gates_counts[ic->transistors[t].gate]++;
    }

    for (n = 0, tcur = 0; n < ic->nodes_count; n++) {
        if (ic->node_gates_counts[n] > 0) {
            tcur += ic->node_gates_counts[n];

            ic->node_gates[n] = ic->node_gates_lists + tcur;
        } else {
            ic->node_gates[n] = NULL;
        }
    }

    for (t = 0; t < ic->transistors_count; t++) {
        *(--ic->node_gates[ic->transistors[t].gate]) = t;
    }

    /* Map nodes to transistor channels */
    ic->node_channels        = calloc(ic->nodes_count, sizeof(tx_t *));
    ic->node_channels_lists  = calloc(ic->transistors_count * 2, sizeof(tx_t));
    ic->node_channels_counts = calloc(ic->nodes_count, sizeof(size_t));

    for (t = 0; t < ic->transistors_count; t++) {
        if (ic->transistors[t].c1 == ic->transistors[t].c2) {
            continue;
        }

        ic->node_channels_counts[ic->transistors[t].c1]++;
        ic->node_channels_counts[ic->transistors[t].c2]++;
    }

    for (n = 0, tcur = 0; n < ic->nodes_count; n++) {
        if (ic->node_channels_counts[n] > 0) {
            tcur += ic->node_channels_counts[n];

            ic->node_channels[n] = ic->node_channels_lists + tcur;
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

    /* --- Buffers --- */

    /* Initialize buffer list */
    ic->buffers_count = layout->buffers_count;
    ic->buffers = malloc(sizeof(buffer_t) * ic->buffers_count);

    for (b = 0; b < ic->buffers_count; b++) {
        icemu_buffer_init(ic, b, &layout->buffers[b]);
    }

    /* Map nodes to buffer inputs */
    ic->node_buffers        = calloc(ic->nodes_count, sizeof(bx_t *));
    ic->node_buffers_lists  = calloc(ic->buffers_count, sizeof(bx_t));
    ic->node_buffers_counts = calloc(ic->nodes_count, sizeof(size_t));

    for (b = 0; b < ic->buffers_count; b++) {
        ic->node_buffers_counts[ic->buffers[b].input]++;
    }

    for (n = 0, bcur = 0; n < ic->nodes_count; n++) {
        if (ic->node_buffers_counts[n] > 0) {
            bcur += ic->node_buffers_counts[n];

            ic->node_buffers[n] = ic->node_buffers_lists + bcur;
        } else {
            ic->node_buffers[n] = NULL;
        }
    }

    for (b = 0; b < ic->buffers_count; b++) {
        *(--ic->node_buffers[ic->buffers[b].input]) = b;
    }

    /* --- Network --- */

    /* Initialize network state */
    ic->network_nodes = malloc(sizeof(nx_t) * ic->nodes_count);
    ic->network_nodes_count = 0;
    ic->network_level_down = LEVEL_FLOAT;
    ic->network_level_up = LEVEL_FLOAT;

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

    free(ic->node_buffers);
    free(ic->node_buffers_lists);
    free(ic->node_buffers_counts);

    free(ic->network_nodes);

    free(ic);
}

void icemu_sync(icemu_t * ic) {
    icemu_resolve(ic);
}

bit_t icemu_read_node(const icemu_t * ic, nx_t n, pull_t pull) {
    bit_t state = ic->nodes[n].state;

    if (state == BIT_Z || state == BIT_META) {
        switch (pull) {
            case PULL_DOWN:
                return BIT_ZERO;
            case PULL_UP:
                return BIT_ONE;
            case PULL_FLOAT:
                return state;
        }
    }

    return state;
}

void icemu_write_node(icemu_t * ic, nx_t n, bit_t state, bool_t sync) {

    /* Apply a load to the node in the desired direction */
    ic->nodes[n].level = LEVEL_LOAD;

    if (state == BIT_ZERO) {
        ic->nodes[n].pull = PULL_DOWN;
    } else if (state == BIT_ONE) {
        ic->nodes[n].pull = PULL_UP;
    }

    /* Flag the node as dirty so it will be re-evaluated */
    ic->nodes[n].dirty = true;

    /* Synchronize the device if requested */
    if (sync) {
        icemu_sync(ic);
    }
}

/* --- Private functions --- */

void icemu_resolve(icemu_t * ic) {
    unsigned int i;
    nx_t n;
    tx_t t;
    bx_t b;
    bool_t resolved;

    for (i = 0; i < ICEMU_RESOLVE_LIMIT; i++) {

        /* Iterate through all dirty nodes */
        for (n = 0; n < ic->nodes_count; n++) {
            if (ic->nodes[n].dirty) {

                /* Find the network of all connected nodes */
                icemu_network_add(ic, n);

                /* Resolve nodes in the network and propagate changes to affected transistors */
                icemu_network_resolve(ic, i);

                /* Clean up the network */
                icemu_network_reset(ic);
            }
        }

        /* Reset resolution flag */
        resolved = true;

        /* Resolve dirty transistors and propagate changes to affected nodes */
        for (t = 0; t < ic->transistors_count; t++) {
            if (ic->transistors[t].dirty) {
                icemu_transistor_resolve(ic, t);
                resolved = false;
            }
        }

        /* Resolve dirty buffers and propagate changes to affected nodes */
        for (b = 0; b < ic->buffers_count; b++) {
            if (ic->buffers[b].dirty) {
                icemu_buffer_resolve(ic, b);
                resolved = false;
            }
        }

        /* If no components were marked dirty, resolution is complete */
        if (resolved) {
            return;
        }
    }

    /* Resolution is incomplete */
    fprintf(stderr, "[WARNING] Resolution incomplete after %d iterations\n", i);
}

void icemu_network_reset(icemu_t * ic) {
    ic->network_nodes_count = 0;
    ic->network_level_down = LEVEL_FLOAT;
    ic->network_level_up = LEVEL_FLOAT;
}

void icemu_network_add(icemu_t * ic, nx_t n) {
    node_t * node;
    nx_t nn;
    tx_t c;

    /* Stop here if this node is a power rail */
    if (n == ic->off) {
        ic->network_level_down = LEVEL_POWER;
        return;
    }

    if (n == ic->on) {
        ic->network_level_up = LEVEL_POWER;
        return;
    }

    /* Check if this node has already been added to the network */
    for (nn = 0; nn < ic->network_nodes_count; nn++) {
        if (ic->network_nodes[nn] == n) {
            return;
        }
    }

    /* Append this node to the network */
    ic->network_nodes[ic->network_nodes_count++] = n;

    /* Update network signal level */
    node = &ic->nodes[n];

    if (node->pull == PULL_DOWN && node->level > ic->network_level_down) {
        ic->network_level_down = node->level;
    } else if (node->pull == PULL_UP && node->level > ic->network_level_up) {
        ic->network_level_up = node->level;
    } else if (node->state == BIT_ZERO && LEVEL_CAP > ic->network_level_down) {
        ic->network_level_down = LEVEL_CAP;
    } else if (node->state == BIT_ONE && LEVEL_CAP > ic->network_level_up) {
        ic->network_level_up = LEVEL_CAP;
    }

    /* Search for transistor channels connected to this node */
    for (c = 0; c < ic->node_channels_counts[n]; c++) {
        tx_t t = ic->node_channels[n][c];
        transistor_t * transistor = &ic->transistors[t];

        /* If the transistor is enabled, recursively expand the network to the other terminal */
        if (transistor->state == BIT_ONE) {
            if (transistor->c1 == n) {
                icemu_network_add(ic, transistor->c2);
            } else if (transistor->c2 == n) {
                icemu_network_add(ic, transistor->c1);
            }
        }
    }
}

void icemu_network_resolve(icemu_t * ic, unsigned int iter) {
    bit_t state = BIT_Z;
    nx_t nn;

    /* Find the strongest signal pulling the network up or down */
    if (ic->network_level_up > ic->network_level_down) {
        state = BIT_ONE;
    } else if (ic->network_level_down > ic->network_level_up) {
        state = BIT_ZERO;
    } else if (ic->network_level_up < LEVEL_LOAD) {
        /* Ambiguous node levels with no connection to power are high-impedance */
        state = BIT_Z;
    } else {
        /* Ambiguous node levels with connection to power are metastable */
        state = BIT_META;
    }

    /* Propagate the strongest signal to all nodes in the network */
    for (nn = 0; nn < ic->network_nodes_count; nn++) {
        nx_t n = ic->network_nodes[nn];
        tx_t t;
        bx_t b;

        /* Update dirty flags for affected components if the state changed */
        if (state != ic->nodes[n].state) {
            for (t = 0; t < ic->node_gates_counts[n]; t++) {
                ic->transistors[ic->node_gates[n][t]].dirty = true;
            }

            for (b = 0; b < ic->node_buffers_counts[n]; b++) {
                ic->buffers[ic->node_buffers[n][b]].dirty = true;
            }
        }

        /* Update node states and clear dirty flags */
        ic->nodes[n].state = state;
        ic->nodes[n].dirty = false;
    }

#ifdef DEBUG
    if (debug_test_network(ic)) {
        bool_t dirty = false;

        printf("#%-2u [ ", iter);

        /* Print sorted network list */
        debug_print_network(ic, " ");

        /* Determine whether the network state is dirty */
        for (nn = 0; nn < ic->network_nodes_count; nn++) {
            nx_t n = ic->network_nodes[nn];
            tx_t g;

            for (g = 0; g < ic->node_gates_counts[n]; g++) {
                if (ic->transistors[ic->node_gates[n][g]].dirty) {
                    dirty = true;
                    break;
                }
            }
        }

        /*  Print resulting network state */
        if (dirty) {
            printf(" ] <= %c", bit_char(state));
        } else {
            printf(" ]    %c", bit_char(state));
        }

        /* Print network signal levels */
        printf("    (+%d -%d)", ic->network_level_up, ic->network_level_down);
        printf("\n");

        /* Print affected downstream transistors */
        /*
        if (dirty) {
            for (nn = 0; nn < ic->network_nodes_count; nn++) {
                nx_t n = ic->network_nodes[nn];
                tx_t g;

                if (ic->node_gates_counts[n] > 10) {
                    printf("(+%zd transistors)\n", ic->node_gates_counts[n]);
                } else {
                    for (g = 0; g < ic->node_gates_counts[n]; g++) {
                        const transistor_t * transistor = &ic->transistors[ic->node_gates[n][g]];

                        if (transistor->dirty) {
                            char gate = transistor_is_open(transistor, &ic->nodes[n]) ? '=' : '/';

                            printf("%4zd <%c> %zd\n", transistor->c1, gate, transistor->c2);
                        }
                    }
                }
            }
        }
        */
    }
#endif
}

void icemu_transistor_init(icemu_t * ic, tx_t t, const transistor_t * layout) {
    transistor_t * transistor = &ic->transistors[t];

    /* Initialize transistor properties */
    transistor->type  = layout->type;
    transistor->gate  = layout->gate;
    transistor->c1    = layout->c1;
    transistor->c2    = layout->c2;
    transistor->state = BIT_Z;
    transistor->dirty = false;
}

void icemu_transistor_resolve(icemu_t * ic, tx_t t) {
    transistor_t * transistor = &ic->transistors[t];

    /* Calculate transistor state */
    bit_t state = icemu_transistor_state(ic, t);

    /* Update dirty flags for affected nodes if the state changed */
    if (state != transistor->state) {
        ic->nodes[transistor->c1].dirty = true;
        ic->nodes[transistor->c2].dirty = true;
    }

    /* Update transistor state and clear dirty flag */
    transistor->state = state;
    transistor->dirty = false;
}

bit_t icemu_transistor_state(icemu_t * ic, tx_t t) {
    transistor_t * transistor = &ic->transistors[t];
    bit_t gate = ic->nodes[transistor->gate].state;

    switch (transistor->type) {
        case TRANSISTOR_NMOS:
            return gate == BIT_ONE;
        case TRANSISTOR_PMOS:
            return gate == BIT_ZERO;
    }

    return BIT_Z;
}

void icemu_buffer_init(icemu_t * ic, bx_t b, const buffer_t * layout) {
    buffer_t * buffer = &ic->buffers[b];
    bit_t output;

    /* Initialize buffer properties */
    buffer->logic     = layout->logic;
    buffer->inverting = layout->inverting;
    buffer->input     = layout->input;
    buffer->output    = layout->output;
    buffer->dirty     = false;

    /* Calculate default output */
    output = icemu_buffer_output(ic, b);

    /* Apply initial load to output node */
    ic->nodes[buffer->output].level = bit_level(output, buffer->logic);
    ic->nodes[buffer->output].pull = bit_pull(output);
}

void icemu_buffer_resolve(icemu_t * ic, bx_t b) {
    buffer_t * buffer = &ic->buffers[b];

    /* Calculate output value */
    bit_t output = icemu_buffer_output(ic, b);

    /* Apply load and set dirty flag on output node */
    ic->nodes[buffer->output].level = bit_level(output, buffer->logic);
    ic->nodes[buffer->output].pull = bit_pull(output);
    ic->nodes[buffer->output].dirty = true;

    /* Clear buffer dirty flag */
    buffer->dirty = false;
}

bit_t icemu_buffer_output(icemu_t * ic, bx_t b) {
    buffer_t * buffer = &ic->buffers[b];
    bit_t input = ic->nodes[buffer->input].state;

    switch (buffer->logic) {
        case LOGIC_NMOS:
            if (buffer->inverting) {
                return (input < 0) ? BIT_ONE : !input;
            } else {
                return (input < 0) ? BIT_ZERO : input;
            }
        case LOGIC_TTL:
            if (buffer->inverting) {
                return (input < 0) ? BIT_Z : !input;
            } else {
                return (input < 0) ? BIT_Z : input;
            }
        default:
            fprintf(stderr, "[WARNING] Unsupported buffer logic type %d\n", buffer->logic);
            return BIT_META;
    }
}
