#include "runtime.h"

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* --- Private declarations --- */

enum {
  BUF_LEN  = 4096,
  PIN_LEN  = 16,
  MAX_PINS = 64,
};

typedef enum {
  STYLE_NONE = 0,
  STYLE_TEXT = 1,
  STYLE_OK   = 32,
  STYLE_ERR  = 31,
  STYLE_INFO = 35,
  STYLE_CMD  = 34,
} style_t;

typedef enum {
  STATE_ERR,
  STATE_START,
  STATE_NEXT,
  STATE_EXIT,
  STATE_CMD,
  STATE_EXEC_FILE,
  STATE_PINDEF_PIN,
  STATE_PINTEST_DATA,
  STATE_MEMSET_ADDR,
  STATE_MEMSET_DATA,
  STATE_MEMTEST_ADDR,
  STATE_MEMTEST_DATA,
  STATE_RUN_CYCLES,
} state_t;

typedef struct {
  unsigned int data;
  unsigned int mask;
  size_t bits;
  size_t base;
} test_t;

typedef struct {
  const adapter_t * adapter;
  void * instance;

  const char * file;
  size_t line;
  bool success;

  char ** pins;
  char * pins_buf;
  size_t pins_count;

  test_t * pin_tests;
  size_t pin_tests_count;

  unsigned int mem_addr;
  size_t mem_offset;
} env_t;

static state_t runtime_exec_stream(env_t * env, FILE * stream);
static state_t runtime_exec_line(env_t * env, char * buf, state_t state);
static state_t runtime_exec_eof(env_t * env, state_t state);
static state_t runtime_exec_token(env_t * env, const char * tok, const char * buf, state_t state);

static state_t runtime_handle_start(env_t * env, const char * tok, const char * buf);
static state_t runtime_handle_cmd(env_t * env, const char * tok, const char * buf);
static state_t runtime_handle_nop(env_t * env, const char * tok, const char * buf);
static state_t runtime_handle_exit(env_t * env, const char * tok, const char * buf);
static state_t runtime_handle_exec(env_t * env, const char * tok, const char * buf);
static state_t runtime_handle_exec_file(env_t * env, const char * tok, const char * buf);
static state_t runtime_handle_info(env_t * env, const char * tok, const char * buf);
static state_t runtime_handle_pins(env_t * env, const char * tok, const char * buf);
static state_t runtime_handle_pindef(env_t * env, const char * tok, const char * buf);
static state_t runtime_handle_pindef_pin(env_t * env, const char * tok, const char * buf);
static state_t runtime_handle_pindef_end(env_t * env, const char * tok, const char * buf);
static state_t runtime_handle_pintest(env_t * env, const char * tok, const char * buf);
static state_t runtime_handle_pintest_data(env_t * env, const char * tok, const char * buf);
static state_t runtime_handle_pintest_end(env_t * env, const char * tok, const char * buf);
static state_t runtime_handle_memset(env_t * env, const char * tok, const char * buf);
static state_t runtime_handle_memset_addr(env_t * env, const char * tok, const char * buf);
static state_t runtime_handle_memset_data(env_t * env, const char * tok, const char * buf);
static state_t runtime_handle_memset_end(env_t * env, const char * tok, const char * buf);
static state_t runtime_handle_memtest(env_t * env, const char * tok, const char * buf);
static state_t runtime_handle_memtest_addr(env_t * env, const char * tok, const char * buf);
static state_t runtime_handle_memtest_data(env_t * env, const char * tok, const char * buf);
static state_t runtime_handle_memtest_end(env_t * env, const char * tok, const char * buf);
static state_t runtime_handle_reset(env_t * env, const char * tok, const char * buf);
static state_t runtime_handle_step(env_t * env, const char * tok, const char * buf);
static state_t runtime_handle_run(env_t * env, const char * tok, const char * buf);
static state_t runtime_handle_run_cycles(env_t * env, const char * tok, const char * buf);

static rc_t runtime_parse_value(const env_t * env, const char * tok, value_t * val);
static rc_t runtime_parse_test(const env_t * env, const char * tok, test_t * test);
static rc_t runtime_parse_file(const env_t * env, const char * tok, char * file);

static void runtime_error(const env_t * env, const char * format, ...);

static void runtime_print(const env_t * env, style_t style, const char * format, ...);
static void runtime_print_addr(const env_t * env, style_t style, unsigned int addr);
static void runtime_print_word(const env_t * env, style_t style, unsigned int word);
static void runtime_print_value(const env_t * env, style_t style, value_t val);
static void runtime_print_test(const env_t * env, style_t style, test_t test);

static env_t * runtime_env_init();
static void runtime_env_destroy(env_t * env);

/* --- Public functions --- */

rc_t runtime_exec_file(const adapter_t * adapter, const char * file) {

  /* Open file for reading */
  FILE * f;
  f = fopen(file, "r");

  if (f == NULL) {
    fprintf(stderr, "Error opening '%s': %s\n", file, strerror(errno));

    return RC_FILE_ERR;
  }

  /* Initialize runtime environment */
  env_t * env = runtime_env_init(adapter, file);
  state_t state = STATE_START;

  /* Execute contents of file */
  state = runtime_exec_stream(env, f);

  /* Clean up environment */
  runtime_env_destroy(env);

  /* Close stream */
  fclose(f);

  return state == STATE_ERR ? RC_ERR : RC_OK;
}

rc_t runtime_exec_repl(const adapter_t * adapter) {
  rc_t rc = RC_OK;
  char buf[BUF_LEN];

  /* Initialize environment and state */
  env_t * env = runtime_env_init(adapter, "shell");
  state_t state = STATE_START;

  /* Print initial prompt */
  runtime_print(env, STYLE_INFO, "icescript REPL\n");
  runtime_print(env, STYLE_INFO, "> ");

  /* Parse and execute line by line */
  while (fgets(buf, BUF_LEN, stdin) != NULL) {
    state = runtime_exec_line(env, buf, state);

    /* Stop execution only on exit */
    if (state == STATE_EXIT) {
      break;
    }

    /* On error, read the next command */
    if (state == STATE_ERR) {
      state = STATE_CMD;
    }

    /* Finish unterminated commands on EOL */
    state = runtime_exec_eof(env, state);

    /* Print a command prompt */
    runtime_print(env, STYLE_INFO, "> ");
  }

  /* Clean up environment */
  runtime_env_destroy(env);

  if (ferror(stdin)) {
    runtime_error(env, "%s", strerror(errno));
    rc = RC_FILE_ERR;
  }

  return rc;
}

/* --- Private functions --- */

state_t runtime_exec_stream(env_t * env, FILE * stream) {
  char buf[BUF_LEN];
  state_t state = STATE_START;

  /* Parse and execute line by line */
  while (fgets(buf, BUF_LEN, stream) != NULL) {
    state = runtime_exec_line(env, buf, state);

    /* Stop execution on exit or on error */
    if (state == STATE_EXIT || state == STATE_ERR) {
      break;
    }

    /* Increment line counter */
    env->line++;
  }

  /* Finish execution on EOF */
  if (feof(stream)) {
    state = runtime_exec_eof(env, state);
  }

  if (ferror(stream)) {
    runtime_error(env, "%s", strerror(errno));
    return STATE_ERR;
  }

  return state;
}

state_t runtime_exec_line(env_t * env, char * buf, state_t state) {
  char * ptr;
  char * tok;
  char * end;

  /* Strip comments */
  if ((ptr = strchr(buf, '!')) != NULL) {
    *(ptr + 0) = '\n';
    *(ptr + 1) = '\0';
  }

  /* Store a pointer to null terminator */
  end = buf + strlen(buf);

  /* Tokenize on whitespace */
  ptr = buf;

  while ((tok = strtok(ptr, " \n\t\r\v")) != NULL) {

    /* Re-point the buffer to the remainder of the line */
    if (tok + strlen(tok) >= end) {
      buf = end;
    } else {
      buf = tok + strlen(tok) + 1;
    }

    /* Handle state */
    state = runtime_exec_token(env, tok, buf, state);

    /* Resume parsing on the next line on NEXT */
    if (state == STATE_NEXT) {
      return STATE_CMD;
    }

    /* Bubble up EXIT and ERR */
    if (state == STATE_EXIT || state == STATE_ERR) {
      return state;
    }

    /* Advance to the next token */
    ptr = NULL;
  }

  return state;
}

state_t runtime_exec_eof(env_t * env, state_t state) {

  /* Execute a .nop to finish any unterminated commands */
  return runtime_exec_token(env, ".nop", "", state);
}

state_t runtime_exec_token(env_t * env, const char * tok, const char * buf, state_t state) {

  /* Handle token based on state */
  switch (state) {
    case STATE_START:
      return runtime_handle_start(env, tok, buf);
    case STATE_CMD:
      return runtime_handle_cmd(env, tok, buf);
    case STATE_EXEC_FILE:
      return runtime_handle_exec_file(env, tok, buf);
    case STATE_PINDEF_PIN:
      return runtime_handle_pindef_pin(env, tok, buf);
    case STATE_PINTEST_DATA:
      return runtime_handle_pintest_data(env, tok, buf);
    case STATE_MEMSET_ADDR:
      return runtime_handle_memset_addr(env, tok, buf);
    case STATE_MEMSET_DATA:
      return runtime_handle_memset_data(env, tok, buf);
    case STATE_MEMTEST_ADDR:
      return runtime_handle_memtest_addr(env, tok, buf);
    case STATE_MEMTEST_DATA:
      return runtime_handle_memtest_data(env, tok, buf);
    case STATE_RUN_CYCLES:
      return runtime_handle_run_cycles(env, tok, buf);
    case STATE_NEXT:
    case STATE_EXIT:
    case STATE_ERR:
      return state;
    default:
      runtime_error(env, "Unexpected state %d", state);
      return STATE_ERR;
  }
}

state_t runtime_handle_start(env_t * env, const char * tok, const char * buf) {
  return runtime_handle_cmd(env, tok, buf);
}

state_t runtime_handle_cmd(env_t * env, const char * tok, const char * buf) {

  /* Validate token */
  if (tok[0] != '.') {
    runtime_error(env, "Expected command, found '%s'", tok);
    return STATE_ERR;
  }

  /* Parse command */
  if (strcmp(tok, ".nop") == 0) {
    return runtime_handle_nop(env, tok, buf);
  } else if (strcmp(tok, ".exit") == 0) {
    return runtime_handle_exit(env, tok, buf);
  } else if (strcmp(tok, ".exec") == 0) {
    return runtime_handle_exec(env, tok, buf);
  } else if (strcmp(tok, ".info") == 0) {
    return runtime_handle_info(env, tok, buf);
  } else if (strcmp(tok, ".test") == 0) {
    /* TODO: Define test program */
    runtime_error(env, "Unimplemented command '%s'", tok);
    return STATE_ERR;
  } else if (strcmp(tok, ".pins") == 0) {
    return runtime_handle_pins(env, tok, buf);
  } else if (strcmp(tok, ".pindef") == 0) {
    return runtime_handle_pindef(env, tok, buf);
  } else if (strcmp(tok, ".pinget") == 0) {
    /* TODO: Read from specified pin(s) */
    runtime_error(env, "Unimplemented command '%s'", tok);
    return STATE_ERR;
  } else if (strcmp(tok, ".pinset") == 0) {
    /* TODO: Write to specified pin */
    runtime_error(env, "Unimplemented command '%s'", tok);
    return STATE_ERR;
  } else if (strcmp(tok, ".pintest") == 0) {
    return runtime_handle_pintest(env, tok, buf);
  } else if (strcmp(tok, ".memget") == 0) {
    /* TODO: Read from specified memory address */
    runtime_error(env, "Unimplemented command '%s'", tok);
    return STATE_ERR;
  } else if (strcmp(tok, ".memset") == 0) {
    return runtime_handle_memset(env, tok, buf);
  } else if (strcmp(tok, ".memtest") == 0) {
    return runtime_handle_memtest(env, tok, buf);
  } else if (strcmp(tok, ".reset") == 0) {
    return runtime_handle_reset(env, tok, buf);
  } else if (strcmp(tok, ".step") == 0) {
    return runtime_handle_step(env, tok, buf);
  } else if (strcmp(tok, ".run") == 0) {
    return runtime_handle_run(env, tok, buf);
  }

  runtime_error(env, "Unrecognized command '%s'", tok);
  return STATE_ERR;
}

state_t runtime_handle_nop(env_t * env, const char * tok, const char * buf) {
  return STATE_CMD;
}

state_t runtime_handle_exit(env_t * env, const char * tok, const char * buf) {
  runtime_print(env, STYLE_CMD, "EXIT\n");

  return STATE_EXIT;
}

state_t runtime_handle_exec(env_t * env, const char * tok, const char * buf) {
  return STATE_EXEC_FILE;
}

state_t runtime_handle_exec_file(env_t * env, const char * tok, const char * buf) {

  /* Validate filename */
  char file[BUF_LEN];

  if (runtime_parse_file(env, tok, file) != RC_OK) {
    runtime_error(env, "Expected filename, found '%s'\n", tok);
    return STATE_ERR;
  }

  /* Open file for reading */
  FILE * f;
  f = fopen(file, "r");

  if (f == NULL) {
    runtime_error(env, "Error opening file '%s': %s", tok, strerror(errno));
    return STATE_ERR;
  }

  /* Store filename and line number */
  const char * old_file = env->file;
  size_t old_line = env->line;

  /* Initialize new filename and line number */
  env->file = file;
  env->line = 1;

  /* Execute contents of file */
  runtime_print(env, STYLE_CMD, "EXEC\t");
  runtime_print(env, STYLE_NONE, "%s\n", file);
  state_t state = runtime_exec_stream(env, f);

  /* Restore filename and line number */
  env->file = old_file;
  env->line = old_line;

  /* Close file */
  fclose(f);

  if (state == STATE_ERR) {
    runtime_error(env, "Error executing file '%s'", file);
    return STATE_ERR;
  }

  runtime_print(env, STYLE_CMD, "DONE\t");
  runtime_print(env, STYLE_NONE, "%s\n", file);

  return STATE_CMD;
}

state_t runtime_handle_info(env_t * env, const char * tok, const char * buf) {

  /* Print the remainder of the line */
  runtime_print(env, STYLE_CMD, "INFO\t");
  runtime_print(env, STYLE_TEXT, "%s", buf);

  return STATE_NEXT;
}

state_t runtime_handle_pins(env_t * env, const char * tok, const char * buf) {

  /* Print pin values */
  runtime_print(env, STYLE_CMD, "PINS\t");

  for (size_t p = 0; p < env->pins_count; p++) {
    if (p > 0) {
      runtime_print(env, STYLE_NONE, " ");
    }

    /* Read from pin */
    value_t val = env->adapter->read_pin(env->instance, env->pins[p]);

    runtime_print(env, STYLE_NONE, "%s[", env->pins[p]);
    runtime_print_value(env, STYLE_NONE, val);
    runtime_print(env, STYLE_NONE, "]");
  }

  runtime_print(env, STYLE_NONE, "\n");

  return STATE_CMD;
}

state_t runtime_handle_pindef(env_t * env, const char * tok, const char * buf) {

  /* Reset pin list */
  env->pins_count = 0;

  return STATE_PINDEF_PIN;
}

state_t runtime_handle_pindef_pin(env_t * env, const char * tok, const char * buf) {

  /* Check for end condition */
  if (tok[0] == '.') {
    return runtime_handle_pindef_end(env, tok, buf);
  }

  /* Validate pin */
  if (strlen(tok) >= PIN_LEN) {
    runtime_error(env, "Invalid pin name '%s'", tok);
    return STATE_ERR;
  }

  if (!env->adapter->can_read_pin(env->instance, tok)) {
    runtime_error(env, "Unreadable pin '%s'", tok);
    return STATE_ERR;
  }

  /* Register pin */
  env->pins[env->pins_count++] = strcpy(&env->pins_buf[env->pins_count * PIN_LEN], tok);

  return STATE_PINDEF_PIN;
}

state_t runtime_handle_pindef_end(env_t * env, const char * tok, const char * buf) {

  /* Print pin list */
  runtime_print(env, STYLE_CMD, "PINDEF\t");

  for (size_t p = 0; p < env->pins_count; p++) {
    if (p > 0) {
      runtime_print(env, STYLE_NONE, " ");
    }

    runtime_print(env, STYLE_NONE, "%s", env->pins[p]);
  }

  runtime_print(env, STYLE_NONE, "\n");

  return runtime_handle_cmd(env, tok, buf);
}

state_t runtime_handle_pintest(env_t * env, const char * tok, const char * buf) {

  /* Reset pin tests */
  env->pin_tests_count = 0;

  return STATE_PINTEST_DATA;
}

state_t runtime_handle_pintest_data(env_t * env, const char * tok, const char * buf) {

  /* Check for end condition */
  if (tok[0] == '.') {
    return runtime_handle_pintest_end(env, tok, buf);
  }

  /* Check for an available pin */
  if (env->pin_tests_count >= env->pins_count) {
    runtime_error(env, "Only %zu pins specified\n", env->pins_count);
    return STATE_ERR;
  }

  /* Validate test value */
  test_t test;

  if (runtime_parse_test(env, tok, &test) != RC_OK) {
    runtime_error(env, "Expected pin test value, found '%s'\n", tok);
    return STATE_ERR;
  }

  /* Register test value */
  env->pin_tests[env->pin_tests_count++] = test;

  return STATE_PINTEST_DATA;
}

state_t runtime_handle_pintest_end(env_t * env, const char * tok, const char * buf) {

  /* Print test values */
  runtime_print(env, STYLE_CMD, "PINTEST\t");

  for (size_t p = 0; p < env->pin_tests_count; p++) {
    if (p > 0) {
      runtime_print(env, STYLE_NONE, " ");
    }

    runtime_print(env, STYLE_NONE, "%s[", env->pins[p]);
    runtime_print_test(env, STYLE_NONE, env->pin_tests[p]);
    runtime_print(env, STYLE_NONE, "]");
  }

  runtime_print(env, STYLE_NONE, "\n");

  /* Compare pin list */
  runtime_print(env, STYLE_NONE, "       \t");

  for (size_t p = 0; p < env->pin_tests_count; p++) {
    if (p > 0) {
      runtime_print(env, STYLE_NONE, " ");
    }

    /* Read from pin */
    value_t val = env->adapter->read_pin(env->instance, env->pins[p]);

    /* Compare */
    bool match = (env->pin_tests[p].data == (val.data & env->pin_tests[p].mask));

    env->success = env->success && match;

    runtime_print(env, match ? STYLE_OK : STYLE_ERR, "%s[", env->pins[p]);
    runtime_print_value(env, match ? STYLE_OK : STYLE_ERR, val);
    runtime_print(env, match ? STYLE_OK : STYLE_ERR, "]");
  }

  runtime_print(env, STYLE_NONE, "\n");

  return runtime_handle_cmd(env, tok, buf);
}

state_t runtime_handle_memset(env_t * env, const char * tok, const char * buf) {

  /* Reset memory reference */
  env->mem_addr = 0;
  env->mem_offset = 0;

  return STATE_MEMSET_ADDR;
}

state_t runtime_handle_memset_addr(env_t * env, const char * tok, const char * buf) {

  /* Validate address */
  value_t val;
  unsigned int addr;

  if (runtime_parse_value(env, tok, &val) == RC_OK) {
    addr = val.data;
  } else {
    runtime_error(env, "Expected address, found '%s'\n", tok);
    return STATE_ERR;
  }

  /* Register reference address */
  env->mem_addr = addr;

  runtime_print(env, STYLE_CMD, "MEMSET\t");
  runtime_print_addr(env, STYLE_NONE, addr);
  runtime_print(env, STYLE_NONE, "\n");

  return STATE_MEMSET_DATA;
}

state_t runtime_handle_memset_data(env_t * env, const char * tok, const char * buf) {

  /* Check for end condition */
  if (tok[0] == '.') {
    return runtime_handle_memset_end(env, tok, buf);
  }

  /* Validate word */
  value_t val;
  unsigned int word;

  if (runtime_parse_value(env, tok, &val) == RC_OK) {
    word = val.data;
  } else {
    runtime_error(env, "Expected word, found '%s'\n", tok);
    return STATE_ERR;
  }

  /* Write to memory */
  unsigned int addr = env->mem_addr + env->mem_offset++;

  env->adapter->write_mem(env->instance, addr, word);

  runtime_print(env, STYLE_NONE, "\t");
  runtime_print_addr(env, STYLE_NONE, addr);
  runtime_print(env, STYLE_NONE, "\t");
  runtime_print_word(env, STYLE_NONE, word);
  runtime_print(env, STYLE_NONE, "\n");

  return STATE_MEMSET_DATA;
}

state_t runtime_handle_memset_end(env_t * env, const char * tok, const char * buf) {
  return runtime_handle_cmd(env, tok, buf);
}

state_t runtime_handle_memtest(env_t * env, const char * tok, const char * buf) {

  /* Reset memory reference */
  env->mem_addr = 0;
  env->mem_offset = 0;

  return STATE_MEMTEST_ADDR;
}

state_t runtime_handle_memtest_addr(env_t * env, const char * tok, const char * buf) {

  /* Validate address */
  value_t val;
  unsigned int addr;

  if (runtime_parse_value(env, tok, &val) == RC_OK) {
    addr = val.data;
  } else {
    runtime_error(env, "Expected address, found '%s'\n", tok);
    return STATE_ERR;
  }

  /* Register reference address */
  env->mem_addr = addr;

  runtime_print(env, STYLE_CMD, "MEMTEST\t");
  runtime_print_addr(env, STYLE_NONE, addr);
  runtime_print(env, STYLE_NONE, "\n");

  return STATE_MEMTEST_DATA;
}

state_t runtime_handle_memtest_data(env_t * env, const char * tok, const char * buf) {

  /* Check for end condition */
  if (tok[0] == '.') {
    return runtime_handle_memtest_end(env, tok, buf);
  }

  /* Validate test word */
  test_t test;

  if (runtime_parse_test(env, tok, &test) != RC_OK) {
    runtime_error(env, "Expected test word, found '%s'\n", tok);
    return STATE_ERR;
  }

  /* Read from memory */
  unsigned int addr = env->mem_addr + env->mem_offset++;
  value_t val = env->adapter->read_mem(env->instance, addr);

  /* Compare */
  bool match = (test.data == (val.data & test.mask));

  env->success = env->success && match;

  runtime_print(env, STYLE_NONE, "\t");
  runtime_print_addr(env, STYLE_NONE, addr);
  runtime_print(env, STYLE_NONE, "\t");
  runtime_print_test(env, STYLE_NONE, test);
  runtime_print(env, STYLE_NONE, "\t");
  runtime_print_value(env, match ? STYLE_OK : STYLE_ERR, val);
  runtime_print(env, STYLE_NONE, "\n");

  return STATE_MEMTEST_DATA;
}

state_t runtime_handle_memtest_end(env_t * env, const char * tok, const char * buf) {
  return runtime_handle_cmd(env, tok, buf);
}

state_t runtime_handle_reset(env_t * env, const char * tok, const char * buf) {

  /* Reset intance */
  env->adapter->reset(env->instance);

  runtime_print(env, STYLE_CMD, "RESET\n");

  return STATE_CMD;
}

state_t runtime_handle_step(env_t * env, const char * tok, const char * buf) {

  /* Step instance */
  env->adapter->step(env->instance);

  runtime_print(env, STYLE_CMD, "STEP\n");

  return STATE_CMD;
}


state_t runtime_handle_run(env_t * env, const char * tok, const char * buf) {
  return STATE_RUN_CYCLES;
}

state_t runtime_handle_run_cycles(env_t * env, const char * tok, const char * buf) {

  /* Validate cycle count */
  value_t val;
  unsigned int cycles;

  if (runtime_parse_value(env, tok, &val) == RC_OK) {
    cycles = val.data;
  } else {
    runtime_error(env, "Expected cycle count, found '%s'\n", tok);
    return STATE_ERR;
  }

  /* Start run clock */
  clock_t start = clock();

  /* Run instance */
  env->adapter->run(env->instance, cycles);

  /* Capture clock speed */
  clock_t elapsed = clock() - start;
  double khz = .001 * CLOCKS_PER_SEC * cycles / elapsed;

  runtime_print(env, STYLE_CMD, "RUN\t");
  runtime_print(env, STYLE_NONE, "%zu\t@\t", cycles);

  if (khz > 1000) {
    runtime_print(env, STYLE_INFO, "%.3f MHz\n", khz / 1000);
  } else {
    runtime_print(env, STYLE_INFO, "%.3f kHz\n", khz);
  }

  return STATE_CMD;
}

rc_t runtime_parse_value(const env_t * env, const char * tok, value_t * value) {
  test_t test;

  if (runtime_parse_test(env, tok, &test) == RC_OK) {
    if (~test.mask) {
      return RC_PARSE_ERR;
    } else {
      value->data = test.data;
      value->bits = test.bits;
      value->base = test.base;

      return RC_OK;
    }
  } else {
    return RC_PARSE_ERR;
  }
}

rc_t runtime_parse_test(const env_t * env, const char * tok, test_t * test) {

  /* Handle single bit */
  if (tok[1] == '\0' && (tok[0] == '0' || tok[0] == '1')) {
    test->data = tok[0] - '0';
    test->mask = -1;
    test->bits = 1;
    test->base = 10;

    return RC_OK;
  }

  /* Handle single don't-care bit */
  if (tok[1] == '\0' && (tok[0] == 'X' || tok[0] == 'x')) {
    test->data = 0;
    test->mask = 0;
    test->bits = 1;
    test->base = 10;

    return RC_OK;
  }

  /* Handle variable length */
  char buf[strlen(tok) + 1];
  char * end;

  if (tok[0] == '$') {
    tok++;
    test->bits = strlen(tok) * 4;
    test->base = 16;
  } else if (tok[0] == '%') {
    tok++;
    test->bits = strlen(tok);
    test->base = 2;
  } else {
    test->bits = ceil(strlen(tok) * 3.322); /* log2(X) = log10(X) * log2(10) */
    test->base = 10;
  }

  /* Parse data */
  strcpy(buf, tok);

  for (char * c = buf; *c; c++) {
    if (*c == 'X' || *c == 'x') {

      /* Don't-care values are only supported for power-of-2 bases */
      if (test->base == 10) {
        return RC_PARSE_ERR;
      }

      *c = '0';
    }
  }

  test->data = strtoul(buf, &end, test->base);

  if (end < buf + strlen(buf)) {
    return RC_PARSE_ERR;
  }

  /* Parse (inverse) mask */
  strcpy(buf, tok);

  for (char * c = buf; *c; c++) {
    if (*c == 'X' || *c == 'x') {
      *c = (test->base == 16 ? 'F' : '1');
    } else {
      *c = '0';
    }
  }

  test->mask = ~strtoul(buf, &end, test->base);

  if (end < buf + strlen(buf)) {
    return RC_PARSE_ERR;
  }

  return RC_OK;
}

rc_t runtime_parse_file(const env_t * env, const char * tok, char * file) {

  /* Use absolute paths as-is */
  if (tok[0] == '/') {
    strcpy(file, tok);
  }

  /* Find the base filename of the current path */
  strcpy(file, env->file);

  char * last_slash = strrchr(file, '/');
  char * base = (last_slash == NULL ? file : last_slash + 1);

  /* Copy the new filename over the current filename */
  strcpy(base, tok);

  return RC_OK;
}

void runtime_error(const env_t * env, const char * format, ...) {

  fprintf(stderr, "\033[0;%dm", STYLE_ERR);
  fprintf(stderr, "%s:%zu: ", env->file, env->line);

  va_list args;
  va_start(args, format);

  vfprintf(stderr, format, args);

  va_end(args);

  fprintf(stderr, "\033[0;0m");
  fprintf(stderr, "\n");
}

void runtime_print(const env_t * env, style_t style, const char * format, ...) {

  if (style != STYLE_NONE) {
    printf("\033[0;%dm", style);
  }

  va_list args;
  va_start(args, format);

  vprintf(format, args);

  va_end(args);

  if (style != STYLE_NONE) {
    printf("\033[0;0m");
  }
}

void runtime_print_addr(const env_t * env, style_t style, unsigned int addr) {
  runtime_print(env, style, "(");
  runtime_print_value(env, style, (value_t){ addr, env->adapter->mem_addr_width, 16 });
  runtime_print(env, style, ")");
}

void runtime_print_word(const env_t * env, style_t style, unsigned int word) {
  runtime_print_value(env, style, (value_t){ word, env->adapter->mem_word_width, 16 });
}

void runtime_print_value(const env_t * env, style_t style, value_t val) {
  runtime_print_test(env, style, (test_t){ val.data, -1, val.bits, val.base });
}

void runtime_print_test(const env_t * env, style_t style, test_t test) {

  /* Validate data width */
  if (test.bits > 64) {
    runtime_error(env, "Cannot print %zu-bit value", test.bits);
    return;
  }

  /* Handle single bit */
  if (test.bits == 1) {
    runtime_print(env, style, test.mask ? test.data ? "1" : "0" : "X");
    return;
  }

  /* Handle decimal values */
  if (test.base == 10) {
    runtime_print(env, style, "%u", test.data);
    return;
  }

  /* Handle binary values */
  if (test.base == 2) {
    unsigned int data = test.data;
    unsigned int mask = test.mask;
    char buf[test.bits + 2];

    buf[test.bits + 1] = '\0';

    for (char * c = buf + test.bits; c > buf; c--) {
      if (mask & 1) {
        *c = (data & 1) ? '1' : '0';
      } else {
        *c = 'X';
      }

      data >>= 1;
      mask >>= 1;
    }

    buf[0] = '%';

    runtime_print(env, style, "%s", buf);
    return;
  }

  /* Build format string from base and calculated width */
  int width = 0;
  char format[8] = "";

  if (test.base == 16) {
    int width = ceil(test.bits / 4.0);

    sprintf(format, "$%%0%dX", width);
  } else {
    runtime_error(env, "Cannot print base-%zu value", test.base);
    return;
  }

  /* Construct separate data and mask strings */
  char buf[width + 2];
  char mask[width + 2];

  sprintf(buf, format, test.data);
  sprintf(mask, format, test.mask & ~(-1 << test.bits));

  /* Merge data and mask */
  for (char * c = buf, * m = mask; *c && *m; c++, m++) {
    if (*m == '0') {
      *c = 'X';
    }
  }

  runtime_print(env, style, "%s", buf);
}

env_t * runtime_env_init(const adapter_t * adapter, const char * file) {
  env_t * env = malloc(sizeof(env_t));

  /* Initialize emulator */
  env->adapter = adapter;
  env->instance = adapter->init();

  /* Initialize runtime */
  env->file = file;
  env->line = 1;
  env->success = true;

  /* Initialize pins */
  env->pins = malloc(sizeof(char *) * MAX_PINS);
  env->pins_buf = malloc(sizeof(char) * MAX_PINS * PIN_LEN);
  env->pins_count = 0;
  env->pin_tests = malloc(sizeof(test_t) * MAX_PINS);
  env->pin_tests_count = 0;

  /* Initialize memory */
  env->mem_addr = 0;
  env->mem_offset = 0;

  return env;
}

void runtime_env_destroy(env_t * env) {

  /* Clean up emulator */
  env->adapter->destroy(env->instance);

  /* Clean up runtime */
  free(env->pins);
  free(env->pins_buf);
  free(env->pin_tests);

  free(env);
}
