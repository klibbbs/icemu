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
static void icemu_transistor_resolve(icemu_t * ic, tx_t t);
static bool_t icemu_transistor_open(const icemu_t * ic, tx_t t);

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

/* ======== */
/*    IC    */
/* ======== */

/* --- Public functions  --- */

icemu_t * icemu_init(const icemu_layout_t * layout) {
    nx_t n, g;
    lx_t l;
    tx_t t, cur;

    icemu_t * ic = malloc(sizeof(icemu_t));

    ic->synced = false;
    ic->on = layout->on;
    ic->off = layout->off;

    /* Allocate and initialize nodes */
    ic->nodes_count = layout->nodes_count;
    ic->nodes = malloc(sizeof(node_t) * ic->nodes_count);

    for (n = 0; n < ic->nodes_count; n++) {
        ic->nodes[n].load  = PULL_FLOAT;
        ic->nodes[n].state = BIT_Z;
        ic->nodes[n].dirty = false;
    }

    for (l = 0; l < layout->loads_count; l++) {
        ic->nodes[layout->loads[l].node].load = layout->loads[l].load;
    }

    /* Allocate and initialize transistors */
    ic->transistors_count = layout->transistors_count;
    ic->transistors = malloc(sizeof(transistor_t) * ic->transistors_count);

    for (t = 0; t < ic->transistors_count; t++) {
        ic->transistors[t].type        = layout->transistors[t].type;
        ic->transistors[t].topology    = layout->transistors[t].topology;
        ic->transistors[t].gates       = layout->transistors[t].gates;
        ic->transistors[t].gates_count = layout->transistors[t].gates_count;
        ic->transistors[t].c1          = layout->transistors[t].c1;
        ic->transistors[t].c2          = layout->transistors[t].c2;
        ic->transistors[t].dirty       = false;
    }

    /* Allocate and initialize map of nodes to transistor gates */
    ic->node_gates = malloc(sizeof(*ic->node_gates) * ic->nodes_count);
    ic->node_gates_lists = malloc(sizeof(*ic->node_gates_lists) * layout->gates_count);
    ic->node_gates_counts = calloc(ic->nodes_count, sizeof(*ic->node_gates_counts));

    for (t = 0; t < ic->transistors_count; t++) {
        for (g = 0; g < ic->transistors[t].gates_count; g++) {
            ic->node_gates_counts[ic->transistors[t].gates[g]]++;
        }
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
        for (g = 0; g < ic->transistors[t].gates_count; g++) {
            *(--ic->node_gates[ic->transistors[t].gates[g]]) = t;
        }
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

    free(ic->network_nodes);

    free(ic);
}

void icemu_sync(icemu_t * ic) {
    icemu_resolve(ic);
}

bit_t icemu_read_node(const icemu_t * ic, nx_t n, pull_t load) {
    bit_t state = ic->nodes[n].state;

    if (state == BIT_Z || state == BIT_META) {
        switch (load) {
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
    if (state == BIT_ZERO) {
        ic->nodes[n].load = PULL_DOWN;
    } else if (state == BIT_ONE) {
        ic->nodes[n].load = PULL_UP;
    }

    /* Flag the node as dirty so it will be reevaluated */
    ic->nodes[n].dirty = true;

    /* Synchronize the chip if requested */
    if (sync) {
        icemu_sync(ic);
    }
}

/* --- Private functions --- */

void icemu_resolve(icemu_t * ic) {
    unsigned int i;
    nx_t n;
    tx_t t;
    size_t dirty_nodes, dirty_transistors;

    for (i = 0; i < ICEMU_RESOLVE_LIMIT; i++) {

        /* Reset counters */
        dirty_nodes = 0;
        dirty_transistors = 0;

        /* Iterate through all dirty nodes */
        for (n = 0; n < ic->nodes_count; n++) {
            if (ic->nodes[n].dirty) {
                dirty_nodes++;

                /* Find the network of all connected nodes */
                icemu_network_add(ic, n);

                /* Resolve nodes in the network and propagate changes to affected transistors */
                icemu_network_resolve(ic, i);

                /* Clean up the network */
                icemu_network_reset(ic);
            }
        }

        /* Iterate through all dirty transistors */
        for (t = 0; t < ic->transistors_count; t++) {
            if (ic->transistors[t].dirty) {
                dirty_transistors++;

                /* Resolve this transistor and propagate changes to affected nodes */
                icemu_transistor_resolve(ic, t);
            }
        }

        /* If no transistors were marked dirty, resolution is complete */
        if (dirty_transistors == 0) {
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

    if (node->load == PULL_DOWN && LEVEL_LOAD > ic->network_level_down) {
        ic->network_level_down = LEVEL_LOAD;
    } else if (node->load == PULL_UP && LEVEL_LOAD > ic->network_level_up) {
        ic->network_level_up = LEVEL_LOAD;
    } else if (node->state == BIT_ZERO && LEVEL_CAP > ic->network_level_down) {
        ic->network_level_down = LEVEL_CAP;
    } else if (node->state == BIT_ONE && LEVEL_CAP > ic->network_level_up) {
        ic->network_level_up = LEVEL_CAP;
    }

    /* Search for transistor channels connected to this node */
    for (c = 0; c < ic->node_channels_counts[n]; c++) {
        tx_t t = ic->node_channels[n][c];

        /* If the transistor is enabled, recursively expand the network to the other terminal */
        if (icemu_transistor_open(ic, t)) {
            const transistor_t * transistor = &ic->transistors[t];

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
        tx_t g;

        /* Update dirty flags for affected transistors if the state changed */
        if (state != ic->nodes[n].state) {
            for (g = 0; g < ic->node_gates_counts[n]; g++) {
                ic->transistors[ic->node_gates[n][g]].dirty = true;
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
    }
#endif
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

bool_t icemu_transistor_open(const icemu_t * ic, tx_t t) {
    const transistor_t * transistor = &ic->transistors[t];
    const node_t * gate = &ic->nodes[transistor->gates[0]];

    switch (transistor->type) {
        case TRANSISTOR_NMOS:
            return gate->state == BIT_ONE;
        case TRANSISTOR_PMOS:
            return gate->state == BIT_ZERO;
    }

    return false;
}
