#include "debug.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* --- Private declarations --- */

static int debug_comp_int(const void * a, const void * b);

/* --- Public functions --- */

icemu_debug_t * debug_instance() {
  static icemu_debug_t * debug = NULL;

  /* Initialize singleton debug info */
  if (debug == NULL) {
    char * debug_nodes_env;
    char * debug_nodes_str;

    debug = malloc(sizeof(icemu_debug_t));

    /* Initialize debug node list */
    debug_nodes_env = getenv(DEBUG_NODES_ENV);

    if (debug_nodes_env != NULL) {
      char * ptr;
      char * tok;
      size_t debug_nodes_max = 1;

      debug_nodes_str = calloc(strlen(debug_nodes_env) + 1, sizeof(char));
      strcpy(debug_nodes_str, debug_nodes_env);

      /* Parse debug node list from environment variable */
      debug->debug_nodes = malloc(sizeof(nx_t) * debug_nodes_max);
      debug->debug_nodes_count = 0;

      ptr = debug_nodes_str;

      while ((tok = strtok(ptr, " \n\t\r\v")) != NULL) {
        if (strlen(tok) > 0) {
          nx_t node = atoi(tok);

          if (node > 0 || tok[0] == '0') {

            /* Reallocate space for more nodes if necessary */
            if (debug->debug_nodes_count == debug_nodes_max) {
              debug_nodes_max *= 2;

              debug->debug_nodes = realloc(debug->debug_nodes, sizeof(nx_t) * debug_nodes_max);
            }

            debug->debug_nodes[debug->debug_nodes_count++] = atoi(tok);
          }
        }

        ptr = NULL;
      }

      free(debug_nodes_str);
    } else {
      debug->debug_nodes = NULL;
      debug->debug_nodes_count = 0;
    }
  }

  /* Return singleton instance */
  return debug;
}

bool debug_test_node(nx_t n) {
  nx_t dn;
  icemu_debug_t * debug = debug_instance();

  for (dn = 0; dn < debug->debug_nodes_count; dn++) {
    if (n == debug->debug_nodes[dn]) {
      return true;
    }
  }

  return false;
}

bool debug_test_network(const icemu_t * ic) {
  nx_t nn;

  for (nn = 0; nn < ic->network_nodes_count; nn++) {
    if (debug_test_node(ic->network_nodes[nn])) {
      return true;
    }
  }

  return false;
}

void debug_print_network(const icemu_t * ic, const char * delim) {
  nx_t dnn;
  nx_t * debug_network_nodes = malloc(sizeof(nx_t) * ic->network_nodes_count);

  /* Copy the current network and sort the copied version */
  memcpy(debug_network_nodes, ic->network_nodes, sizeof(nx_t) * ic->network_nodes_count);
  qsort(debug_network_nodes, ic->network_nodes_count, sizeof(nx_t), debug_comp_int);

  for (dnn = 0; dnn < ic->network_nodes_count; dnn++) {
    if (dnn != 0) {
      printf("%s", delim);
    }

    printf("%4zd", debug_network_nodes[dnn]);
  }

  free(debug_network_nodes);
}

/* --- Private functions --- */

int debug_comp_int(const void * a, const void * b) {
  int aa = *(const int *)a;
  int bb = *(const int *)b;

  return (aa > bb) - (aa < bb);
}
