/*
 The core transistor emulation logic implemented below was originally derived from the perfect6502
 project, in turn derived from the Visual 6502 project, both licensed below.
*/
/*
 Copyright (c) 2010,2014 Michael Steil, Brian Silverman, Barry Silverman

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
*/

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

static void icemu_function_init(icemu_t * ic, fx_t f, const function_t * layout);
static void icemu_function_resolve(icemu_t * ic, fx_t f);
static bit_t icemu_function_output(icemu_t * ic, fx_t f);

static void icemu_cell_init(icemu_t * ic, cx_t c, const cell_t * layout);
static void icemu_cell_resolve(icemu_t * ic, cx_t c);
static bit_t icemu_cell_output(icemu_t * ic, cx_t c);

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

bit_t bit_default(bit_t bit) {
    bit_t map[4] = {BIT_ZERO, BIT_ONE, BIT_ZERO, BIT_ZERO};

    return map[(unsigned char)bit];
}

bit_t bit_invert(bit_t bit) {
    bit_t map[4] = {BIT_ONE, BIT_ZERO, BIT_Z, BIT_META};

    return map[(unsigned char)bit];
}

level_t bit_level(bit_t bit, logic_t logic) {
    switch (bit) {
        case BIT_ZERO:
            return logic == LOGIC_PMOS ? LEVEL_LOAD : LEVEL_POWER;
        case BIT_ONE:
            return logic == LOGIC_NMOS ? LEVEL_LOAD : LEVEL_POWER;
        case BIT_META:
            return LEVEL_POWER;
        case BIT_Z:
        default:
            return LEVEL_FLOAT;
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
    fx_t f, fcur;
    cx_t c, ccur;

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

    /* --- Functions --- */

    /* Initialize function list */
    ic->functions_count = layout->functions_count;
    ic->functions = malloc(sizeof(function_t) * ic->functions_count);

    for (f = 0; f < ic->functions_count; f++) {
        icemu_function_init(ic, f, &layout->functions[f]);
    }

    /* Map nodes to function inputs */
    ic->node_functions        = calloc(ic->nodes_count, sizeof(fx_t *));
    ic->node_functions_lists  = calloc(ic->functions_count * FUNCTION_INPUTS, sizeof(fx_t));
    ic->node_functions_counts = calloc(ic->nodes_count, sizeof(size_t));

    for (f = 0; f < ic->functions_count; f++) {
        for (n = 0; n < ic->functions[f].inputs_count; n++) {
            ic->node_functions_counts[ic->functions[f].inputs[n]]++;
        }
    }

    for (n = 0, fcur = 0; n < ic->nodes_count; n++) {
        if (ic->node_functions_counts[n] > 0) {
            fcur += ic->node_functions_counts[n];

            ic->node_functions[n] = ic->node_functions_lists + fcur;
        } else {
            ic->node_functions[n] = NULL;
        }
    }

    for (f = 0; f < ic->functions_count; f++) {
        for (n = 0; n < ic->functions[f].inputs_count; n++) {
            *(--ic->node_functions[ic->functions[f].inputs[n]]) = f;
        }
    }

    /* --- Cells --- */

    /* Initialize cell list */
    ic->cells_count = layout->cells_count;
    ic->cells = malloc(sizeof(cell_t) * ic->cells_count);

    for (c = 0; c < ic->cells_count; c++) {
        icemu_cell_init(ic, c, &layout->cells[c]);
    }

    /* Map nodes to cell inputs, write enables, and read enables */
    ic->node_cells        = calloc(ic->nodes_count, sizeof(cx_t *));
    ic->node_cells_lists  = calloc(ic->cells_count * CELL_TOTAL_INPUTS, sizeof(cx_t));
    ic->node_cells_counts = calloc(ic->nodes_count, sizeof(size_t));

    for (c = 0; c < ic->cells_count; c++) {
        for (n = 0; n < ic->cells[c].inputs_count; n++) {
            ic->node_cells_counts[ic->cells[c].inputs[n]]++;
        }

        for (n = 0; n < ic->cells[c].writes_count; n++) {
            ic->node_cells_counts[ic->cells[c].writes[n]]++;
        }

        for (n = 0; n < ic->cells[c].reads_count; n++) {
            ic->node_cells_counts[ic->cells[c].reads[n]]++;
        }
    }

    for (n = 0, ccur = 0; n < ic->nodes_count; n++) {
        if (ic->node_cells_counts[n] > 0) {
            ccur += ic->node_cells_counts[n];

            ic->node_cells[n] = ic->node_cells_lists + ccur;
        } else {
            ic->node_cells[n] = NULL;
        }
    }

    for (c = 0; c < ic->cells_count; c++) {
        for (n = 0; n < ic->cells[c].inputs_count; n++) {
            *(--ic->node_cells[ic->cells[c].inputs[n]]) = c;
        }

        for (n = 0; n < ic->cells[c].writes_count; n++) {
            *(--ic->node_cells[ic->cells[c].writes[n]]) = c;
        }

        for (n = 0; n < ic->cells[c].reads_count; n++) {
            *(--ic->node_cells[ic->cells[c].reads[n]]) = c;
        }
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
    free(ic->buffers);
    free(ic->functions);

    free(ic->node_gates);
    free(ic->node_gates_lists);
    free(ic->node_gates_counts);

    free(ic->node_channels);
    free(ic->node_channels_lists);
    free(ic->node_channels_counts);

    free(ic->node_buffers);
    free(ic->node_buffers_lists);
    free(ic->node_buffers_counts);

    free(ic->node_functions);
    free(ic->node_functions_lists);
    free(ic->node_functions_counts);

    free(ic->node_cells);
    free(ic->node_cells_lists);
    free(ic->node_cells_counts);

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
    fx_t f;
    cx_t c;
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

        /* Resolve dirty components and propagate changes to affected nodes */
        for (t = 0; t < ic->transistors_count; t++) {
            if (ic->transistors[t].dirty) {
                icemu_transistor_resolve(ic, t);
                resolved = false;
            }
        }

        for (b = 0; b < ic->buffers_count; b++) {
            if (ic->buffers[b].dirty) {
                icemu_buffer_resolve(ic, b);
                resolved = false;
            }
        }

        for (f = 0; f < ic->functions_count; f++) {
            if (ic->functions[f].dirty) {
                icemu_function_resolve(ic, f);
                resolved = false;
            }
        }

        for (c = 0; c < ic->cells_count; c++) {
            if (ic->cells[c].dirty) {
                icemu_cell_resolve(ic, c);
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
        fx_t f;
        cx_t c;

        /* Update dirty flags for affected components if the state changed */
        if (state != ic->nodes[n].state) {
            for (t = 0; t < ic->node_gates_counts[n]; t++) {
                ic->transistors[ic->node_gates[n][t]].dirty = true;
            }

            for (b = 0; b < ic->node_buffers_counts[n]; b++) {
                ic->buffers[ic->node_buffers[n][b]].dirty = true;
            }

            for (f = 0; f < ic->node_functions_counts[n]; f++) {
                ic->functions[ic->node_functions[n][f]].dirty = true;
            }

            for (c = 0; c < ic->node_cells_counts[n]; c++) {
                ic->cells[ic->node_cells[n][c]].dirty = true;
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

/* ================ */
/*    Transistor    */
/* ================ */

/* --- Private functions  --- */

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

/* ============ */
/*    Buffer    */
/* ============ */

/* --- Private functions  --- */

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
    bit_t input = bit_default(ic->nodes[buffer->input].state);

    if (buffer->inverting) {
        return bit_invert(input);
    } else {
        return input;
    }
}

/* ============== */
/*    Function    */
/* ============== */

void icemu_function_init(icemu_t * ic, fx_t f, const function_t * layout) {
    function_t * function = &ic->functions[f];
    bit_t output = BIT_Z;
    nx_t n;

    /* Initialize function properties */
    function->logic        = layout->logic;
    function->func         = layout->func;
    function->inputs_count = layout->inputs_count;
    function->output       = layout->output;
    function->dirty        = false;

    for (n = 0; n < function->inputs_count; n++) {
        function->inputs[n] = layout->inputs[n];
    }

    /* Calculate default output */
    output = icemu_function_output(ic, f);

    /* Apply initial load to output node */
    ic->nodes[function->output].level = bit_level(output, function->logic);
    ic->nodes[function->output].pull = bit_pull(output);
}

void icemu_function_resolve(icemu_t * ic, fx_t f) {
    function_t * function = &ic->functions[f];

    /* Calculate output value */
    bit_t output = icemu_function_output(ic, f);

    /* Apply load and set dirty flag on output node */
    ic->nodes[function->output].level = bit_level(output, function->logic);
    ic->nodes[function->output].pull = bit_pull(output);
    ic->nodes[function->output].dirty = true;

    /* Clear function dirty flag */
    function->dirty = false;
}

bit_t icemu_function_output(icemu_t * ic, fx_t f) {
    function_t * function = &ic->functions[f];
    size_t count = function->inputs_count;

    /* Fetch function arguments from input nodes */
    bit_t arg1 = count > 0 ? bit_default(ic->nodes[function->inputs[0]].state) : BIT_ZERO;
    bit_t arg2 = count > 1 ? bit_default(ic->nodes[function->inputs[1]].state) : BIT_ZERO;
    bit_t arg3 = count > 2 ? bit_default(ic->nodes[function->inputs[2]].state) : BIT_ZERO;
    bit_t arg4 = count > 3 ? bit_default(ic->nodes[function->inputs[3]].state) : BIT_ZERO;
    bit_t arg5 = count > 4 ? bit_default(ic->nodes[function->inputs[4]].state) : BIT_ZERO;
    bit_t arg6 = count > 5 ? bit_default(ic->nodes[function->inputs[5]].state) : BIT_ZERO;
    bit_t arg7 = count > 6 ? bit_default(ic->nodes[function->inputs[6]].state) : BIT_ZERO;
    bit_t arg8 = count > 7 ? bit_default(ic->nodes[function->inputs[7]].state) : BIT_ZERO;
    bit_t arg9 = count > 8 ? bit_default(ic->nodes[function->inputs[8]].state) : BIT_ZERO;

    return function->func(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);
}

/* ========== */
/*    Cell    */
/* ========== */

void icemu_cell_init(icemu_t * ic, cx_t c, const cell_t * layout) {
    cell_t * cell = &ic->cells[c];
    bit_t output = BIT_Z;
    nx_t n;

    /* Initialize cell properties */
    cell->logic         = layout->logic;
    cell->type          = layout->type;
    cell->inputs_count  = layout->inputs_count;
    cell->outputs_count = layout->outputs_count;
    cell->writes_count  = layout->writes_count;
    cell->reads_count   = layout->reads_count;
    cell->state         = BIT_Z;
    cell->dirty         = false;

    for (n = 0; n < cell->inputs_count; n++) {
        cell->inputs[n] = layout->inputs[n];
    }

    for (n = 0; n < cell->outputs_count; n++) {
        cell->outputs[n] = layout->outputs[n];
    }

    for (n = 0; n < cell->writes_count; n++) {
        cell->writes[n] = layout->writes[n];
    }

    for (n = 0; n < cell->reads_count; n++) {
        cell->reads[n] = layout->reads[n];
    }

    /* Apply initial load to non-inverting output node */
    if (cell->outputs_count > 0) {
        ic->nodes[cell->outputs[0]].level = bit_level(output, cell->logic);
        ic->nodes[cell->outputs[0]].pull = bit_pull(output);
    }

    /* Apply initial load to inverting output node */
    if (cell->outputs_count > 1) {
        ic->nodes[cell->outputs[1]].level = bit_level(bit_invert(output), cell->logic);
        ic->nodes[cell->outputs[1]].pull = bit_pull(bit_invert(output));
    }
}

void icemu_cell_resolve(icemu_t * ic, cx_t c) {
    cell_t * cell = &ic->cells[c];

    /* Calculate output value */
    bit_t output = icemu_cell_output(ic, c);

    /* Apply load and set dirty flag on non-inverting output node */
    if (cell->outputs_count > 0) {
        ic->nodes[cell->outputs[0]].level = bit_level(output, cell->logic);
        ic->nodes[cell->outputs[0]].pull = bit_pull(output);
        ic->nodes[cell->outputs[0]].dirty = true;
    }

    /* Apply load and set dirty flag on inverting output node */
    if (cell->outputs_count > 1) {
        ic->nodes[cell->outputs[1]].level = bit_level(bit_invert(output), cell->logic);
        ic->nodes[cell->outputs[1]].pull = bit_pull(bit_invert(output));
        ic->nodes[cell->outputs[1]].dirty = true;
    }

    /* Clear cell dirty flag */
    cell->dirty = false;
}

bit_t icemu_cell_output(icemu_t * ic, cx_t c) {
    cell_t * cell = &ic->cells[c];
    nx_t n;
    bool_t writable = true, readable = true;

    /* Check if writing is enabled */
    for (n = 0; n < cell->writes_count; n++) {
        if (!bit_default(ic->nodes[cell->writes[n]].state)) {
            writable = false;
            break;
        }
    }

    /* Check if reading is enabled */
    for (n = 0; n < cell->reads_count; n++) {
        if (!bit_default(ic->nodes[cell->reads[n]].state)) {
            readable = false;
            break;
        }
    }

    /* Update cell state from inputs if writable */
    if (writable) {
        switch (cell->type) {
            case CELL_D_LATCH:
                if (cell->inputs_count > 0) {
                    cell->state = bit_default(ic->nodes[cell->inputs[0]].state);
                }
                break;
        }
    }

    /* Output cell state if readable */
    if (readable) {
        return cell->state;
    }

    return BIT_Z;
}
