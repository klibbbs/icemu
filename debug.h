#ifndef INCLUDE_DEBUG_H
#define INCLUDE_DEBUG_H

#include "icemu.h"

static const char DEBUG_NODES_ENV[] = "DEBUG_NODES";

typedef struct {
    nx_t * debug_nodes;
    size_t debug_nodes_count;
} icemu_debug_t;

icemu_debug_t * debug_instance(void);

bool_t debug_test_node(nx_t n);
bool_t debug_test_network(const icemu_t * ic);

void debug_print_network(const icemu_t * ic, const char * delim);

#endif /* INCLUDE_DEBUG_H */
