#include "mos6502.h"
#include "icemu.h"

#include "../perfect6502/perfect6502.h"
#include "../perfect6502/types.h"
#include "../perfect6502/netlist_sim.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern BOOL netlist_6502_node_is_pullup[];
extern netlist_transdefs netlist_6502_transdefs[];

enum { BUF_LEN = 256 };
enum { NUM_BENCH_NODES = 1725 };
enum { NUM_BENCH_TRANS = 3510 };

const char FMT_STATE[] =
  "(%c) Ab[%04X] Db[%02X] Res[%c] RW[%c] Sy[%c] PC[%04X] P[%02X] A[%02X] X[%02X] Y[%02X] SP[%02X]";

static unsigned char bench_memory[65536];
static unsigned char icemu_memory[65536];

static unsigned int debug_nodes[NUM_BENCH_NODES];
static size_t debug_nodes_len = 0;

char bool_char(BOOL b) {
  return b ? '1' : '0';
}

char clk_char(bit_t clk1, bit_t clk2) {
  if (clk1 == BIT_ONE && clk2 == BIT_ZERO) {
    return '1';
  } else if (clk1 == BIT_ZERO && clk2 == BIT_ONE) {
    return '2';
  } else {
    return '?';
  }
}

BOOL get_bench_node(state_t * bench, unsigned int node) {
  return isNodeHigh(bench, node);
}

bit_t get_icemu_node(mos6502_t * icemu, unsigned int node) {
  return icemu_read_node(icemu->ic, node, PULL_FLOAT);
}

void get_bench_state(state_t * bench, char * buf) {
  unsigned short pin_ab = readAddressBus(bench);
  unsigned char pin_db = readDataBus(bench);

  BOOL pin_clk = isNodeHigh(bench, 1171);
  BOOL pin_clk1 = isNodeHigh(bench, 1163);
  BOOL pin_clk2 = isNodeHigh(bench, 421);
  BOOL pin_irq = isNodeHigh(bench, 103);
  BOOL pin_nmi = isNodeHigh(bench, 1297);
  BOOL pin_res = isNodeHigh(bench, 159);
  BOOL pin_rdy = isNodeHigh(bench, 89);
  BOOL pin_rw = isNodeHigh(bench, 1156);
  BOOL pin_so = isNodeHigh(bench, 1672);
  BOOL pin_sync = isNodeHigh(bench, 539);

  unsigned short reg_pc = readPC(bench);
  unsigned char reg_p = readP(bench);
  unsigned char reg_a = readA(bench);
  unsigned char reg_x = readX(bench);
  unsigned char reg_y = readY(bench);
  unsigned char reg_sp = readSP(bench);

  sprintf(buf,
          FMT_STATE,
          clk_char(pin_clk1, pin_clk2),
          pin_ab,
          pin_db,
          bool_char(pin_res),
          bool_char(pin_rw),
          bool_char(pin_sync),
          reg_pc,
          reg_p,
          reg_a,
          reg_x,
          reg_y,
          reg_sp);
}

void get_icemu_state(mos6502_t * icemu, char * buf) {
  unsigned short pin_ab = mos6502_get_ab(icemu);
  unsigned char pin_db = mos6502_get_db(icemu);

  bit_t pin_clk = mos6502_get_clk(icemu);
  bit_t pin_clk1 = mos6502_get_clk1(icemu);
  bit_t pin_clk2 = mos6502_get_clk2(icemu);
  bit_t pin_irq = mos6502_get_irq(icemu);
  bit_t pin_nmi = mos6502_get_nmi(icemu);
  bit_t pin_res = mos6502_get_res(icemu);
  bit_t pin_rdy = mos6502_get_rdy(icemu);
  bit_t pin_rw = mos6502_get_rw(icemu);
  bit_t pin_so = mos6502_get_so(icemu);
  bit_t pin_sync = mos6502_get_sync(icemu);

  unsigned short reg_pc = mos6502_get_reg_pc(icemu);
  unsigned char reg_p = mos6502_get_reg_p(icemu);
  unsigned char reg_a = mos6502_get_reg_a(icemu);
  unsigned char reg_x = mos6502_get_reg_x(icemu);
  unsigned char reg_y = mos6502_get_reg_y(icemu);
  unsigned char reg_sp = mos6502_get_reg_sp(icemu);

  sprintf(buf,
          FMT_STATE,
          clk_char(pin_clk1, pin_clk2),
          pin_ab,
          pin_db,
          bit_char(pin_res),
          bit_char(pin_rw),
          bit_char(pin_sync),
          reg_pc,
          reg_p,
          reg_a,
          reg_x,
          reg_y,
          reg_sp);
}

state_t * init_bench() {
  state_t * bench;

  bench = setupNodesAndTransistors(netlist_6502_transdefs,
                                   netlist_6502_node_is_pullup,
                                   NUM_BENCH_NODES,
                                   NUM_BENCH_TRANS,
                                   558 /* VSS */,
                                   657 /* VCC */);

  return bench;
}

mos6502_t * init_icemu() {
  mos6502_t * icemu;

  icemu = mos6502_init();


  return icemu;
}

void destroy_bench(state_t * bench) {
  destroyChip(bench);

}

void destroy_icemu(mos6502_t * icemu) {
  mos6502_destroy(icemu);
}

void step_bench(state_t * bench) {
  static BOOL clk;

  clk = isNodeHigh(bench, 1171);

  setNode(bench, 1171, !clk);
  recalcNodeList(bench);

  if (!clk) {
    if (isNodeHigh(bench, 1156)) {
      writeDataBus(bench, bench_memory[readAddressBus(bench)]);
    } else {
      memory[readAddressBus(bench)] = readDataBus(bench);
    }
  }
}

void step_icemu(mos6502_t * icemu) {
  static bit_t clk;

  clk = mos6502_get_clk(icemu);

  mos6502_set_clk(icemu, !clk, true);

  if (mos6502_get_rw(icemu) == BIT_ZERO) {
    icemu_memory[mos6502_get_ab(icemu)] = mos6502_get_db(icemu);
  } else {
    mos6502_set_db(icemu, icemu_memory[mos6502_get_ab(icemu)], true);
  }
}

void compare(const char * msg, state_t * bench, mos6502_t * icemu) {
  char bench_buf[BUF_LEN] = {0};
  char icemu_buf[BUF_LEN] = {0};
  int i;

  get_bench_state(bench, bench_buf);
  get_icemu_state(icemu, icemu_buf);

  printf("%s\n", msg);

  printf("  BENCH: %s", bench_buf);

  for (i = 0; i < debug_nodes_len; i++) {
    int node = debug_nodes[i];

    printf(" %d[%c]", node, bool_char(get_bench_node(bench, node)));
  }

  printf("\n");
  printf("  ICEMU: %s", icemu_buf);

  for (i = 0; i < debug_nodes_len; i++) {
    int node = debug_nodes[i];

    printf(" %d[%c]", node, bit_char(get_icemu_node(icemu, node)));
  }

  printf("\n");
}

int main(int argc, char * argv[]) {
  state_t * bench;
  mos6502_t * icemu;
  int i;
  char buf[BUF_LEN] = {0};

  /* Read debug nodes */
  if (argc > 1) {
    FILE * f = fopen(argv[1], "r");

    if (f) {
      while (fgets(buf, sizeof(buf), f)) {
        char * ptr = buf;
        char * tok;

        while ((tok = strtok(ptr, " \n\t\r\v"))) {
          if (strlen(tok) > 0) {
            int node = atoi(tok);

            if (node > 0 || tok[0] == '0') {
              debug_nodes[debug_nodes_len++] = atoi(tok);
            }
          }
          ptr = NULL;
        }
      }
    } else {
      fprintf(stderr, "Error opening file '%s'\n", argv[1]);
    }
  }

  if (debug_nodes_len > 0) {
    printf("DEBUG");

    for (i = 0; i < debug_nodes_len; i++) {
      printf(" %u", debug_nodes[i]);
    }

    printf("\n");
  }

  /* Initialize emulators */
  bench = init_bench();
  icemu = init_icemu();

  /* Compare start-up states */
  compare("INIT", bench, icemu);

  /* Reset sequences */
  mos6502_set_res(icemu, BIT_ZERO, false);
  mos6502_set_clk(icemu, BIT_ONE, false);
  mos6502_set_rdy(icemu, BIT_ONE, false);
  mos6502_set_so(icemu, BIT_ZERO, false);
  mos6502_set_irq(icemu, BIT_ONE, false);
  mos6502_set_nmi(icemu, BIT_ONE, false);
  mos6502_sync(icemu);

  setNode(bench, 159 /* RES */, 0);
  setNode(bench, 1171 /* CLK */, 1);
  setNode(bench, 89 /* RDY */, 1);
  setNode(bench, 1672 /* SO */, 0);
  setNode(bench, 103 /* IRQ */, 1);
  setNode(bench, 1297 /* NMI */, 1);
  stabilizeChip(bench);

  compare("RES LOW", bench, icemu);
  /*
  for (i = 0; i < 16; i++) {
    sprintf(buf, "WAIT %d", i / 2 + 1);

    step_bench(bench);
    step_icemu(icemu);

    compare(buf, bench, icemu);
  }

  mos6502_set_res(icemu, BIT_ONE, true);

  setNode(bench, 159, 1);
  recalcNodeList(bench);

  compare("RES HI", bench, icemu);
  */
  /* TODO: Loop and compare */

  /* Cleanup */
  destroy_bench(bench);
  destroy_icemu(icemu);

  return 0;
}