#include "runtime.h"

#include <ctype.h>
#include <errno.h>
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
  STATE_CMD,
  STATE_PINSET_PIN,
  STATE_PINTEST_DATA,
  STATE_MEMSET_ADDR,
  STATE_MEMSET_DATA,
  STATE_MEMTEST_ADDR,
  STATE_MEMTEST_DATA,
  STATE_RUN_CYCLES,
} state_t;

typedef struct {
  const adapter_t * adapter;
  void * instance;

  state_t state;
  const char * file;
  size_t line;
  bool success;

  char ** pins;
  char * pins_buf;
  size_t pins_count;
  size_t pins_cursor;
  unsigned int * pins_values;

  unsigned int mem_addr;
  size_t mem_offset;
} env_t;

static rc_t runtime_exec_line(env_t * env, char * buf);

static state_t runtime_handle_start(env_t * env, const char * tok, const char * buf);
static state_t runtime_handle_next(env_t * env, const char * tok, const char * buf);
static state_t runtime_handle_cmd(env_t * env, const char * tok, const char * buf);
static state_t runtime_handle_info(env_t * env, const char * tok, const char * buf);
static state_t runtime_handle_pinset(env_t * env, const char * tok, const char * buf);
static state_t runtime_handle_pinset_pin(env_t * env, const char * tok, const char * buf);
static state_t runtime_handle_pinset_end(env_t * env, const char * tok, const char * buf);
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

static rc_t runtime_parse_val(const char * tok, unsigned int * val);

static void runtime_error(const env_t * env, const char * format, ...);

static void runtime_print(const env_t * env, style_t style, const char * format, ...);
static void runtime_print_addr(const env_t * env, style_t style, unsigned int addr);
static void runtime_print_word(const env_t * env, style_t style, unsigned int word);

static env_t * runtime_env_init();
static void runtime_env_destroy(env_t * env);

/* --- Public functions --- */

rc_t runtime_exec_file(const adapter_t * adapter, const char * file) {
  rc_t rc = RC_OK;
  env_t * env;

  FILE * f;
  char buf[BUF_LEN];

  /* Open stream for reading */
  f = fopen(file, "r");

  if (f == NULL) {
    fprintf(stderr, "Error opening '%s': %s\n", file, strerror(errno));

    return RC_FILE_ERR;
  }

  /* Initialize environment */
  env = runtime_env_init(adapter, file);

  /* Parse and execute line by line */
  while (fgets(buf, BUF_LEN,f) != NULL) {
    rc = runtime_exec_line(env, buf);

    /* Stop execution if parsing fails at any point */
    if (rc != RC_OK) {
      break;
    }

    /* Increment line counter */
    env->line++;
  }

  if (ferror(f)) {
    fprintf(stderr, "Error reading '%s', line %zu: %s\n", file, env->line, strerror(errno));
    rc = RC_FILE_ERR;
  }

  /* Clean up environment */
  runtime_env_destroy(env);

  /* Close stream */
  fclose(f);

  return rc;
}

rc_t runtime_exec_repl(const adapter_t * adapter) {
  rc_t rc = RC_OK;

  /* TODO */

  return rc;
}

/* --- Private functions --- */

rc_t runtime_exec_line(env_t * env, char * buf) {
  rc_t rc = RC_OK;
  char * ptr;
  char * tok;
  char * end;

  /* Strip comments */
  if ((ptr = strchr(buf, '#')) != NULL) {
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
    switch (env->state) {
      case STATE_START:
        env->state = runtime_handle_start(env, tok, buf);
        break;
      case STATE_NEXT:
        env->state = runtime_handle_next(env, tok, buf);
        break;
      case STATE_CMD:
        env->state = runtime_handle_cmd(env, tok, buf);
        break;
      case STATE_PINSET_PIN:
        env->state = runtime_handle_pinset_pin(env, tok, buf);
        break;
      case STATE_PINTEST_DATA:
        env->state = runtime_handle_pintest_data(env, tok, buf);
        break;
      case STATE_MEMSET_ADDR:
        env->state = runtime_handle_memset_addr(env, tok, buf);
        break;
      case STATE_MEMSET_DATA:
        env->state = runtime_handle_memset_data(env, tok, buf);
        break;
      case STATE_MEMTEST_ADDR:
        env->state = runtime_handle_memtest_addr(env, tok, buf);
        break;
      case STATE_MEMTEST_DATA:
        env->state = runtime_handle_memtest_data(env, tok, buf);
        break;
      case STATE_RUN_CYCLES:
        env->state = runtime_handle_run_cycles(env, tok, buf);
        break;
      default:
        runtime_error(env, "Unrecognized state %d", env->state);
        return RC_ERR;
    }

    /* Handle special-case states */
    if (env->state == STATE_NEXT) {
      env->state = STATE_CMD;

      return RC_OK;
    } else if (env->state == STATE_ERR) {
      return RC_ERR;
    }

    /* Advance to the next token */
    ptr = NULL;
  }

  return rc;
}

state_t runtime_handle_start(env_t * env, const char * tok, const char * buf) {
  return runtime_handle_cmd(env, tok, buf);
}

state_t runtime_handle_next(env_t * env, const char * tok, const char * buf) {
  return runtime_handle_cmd(env, tok, buf);
}

state_t runtime_handle_cmd(env_t * env, const char * tok, const char * buf) {

  /* Validate token */
  if (tok[0] != '.') {
    runtime_error(env, "Expected command, found '%s'", tok);
    return STATE_ERR;
  }

  /* Parse command */
  if (strcmp(tok, ".info") == 0) {
    return runtime_handle_info(env, tok, buf);
  } else if (strcmp(tok, ".pinset") == 0) {
    return runtime_handle_pinset(env, tok, buf);
  } else if (strcmp(tok, ".pintest") == 0) {
    return runtime_handle_pintest(env, tok, buf);
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

state_t runtime_handle_info(env_t * env, const char * tok, const char * buf) {

  /* Print the remainder of the line */
  runtime_print(env, STYLE_CMD, "INFO\t");
  runtime_print(env, STYLE_TEXT, "%s", buf);

  return STATE_NEXT;
}

state_t runtime_handle_pinset(env_t * env, const char * tok, const char * buf) {

  /* Reset pin list */
  env->pins_count = 0;
  env->pins_cursor = 0;

  return STATE_PINSET_PIN;
}

state_t runtime_handle_pinset_pin(env_t * env, const char * tok, const char * buf) {

  /* Check for end condition */
  if (tok[0] == '.') {
    return runtime_handle_pinset_end(env, tok, buf);
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

  return STATE_PINSET_PIN;
}

state_t runtime_handle_pinset_end(env_t * env, const char * tok, const char * buf) {

  /* Print pin list */
  runtime_print(env, STYLE_CMD, "PINSET\t");

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

  /* Reset pin cursor */
  env->pins_cursor = 0;

  return STATE_PINTEST_DATA;
}

state_t runtime_handle_pintest_data(env_t * env, const char * tok, const char * buf) {

  /* Check for end condition */
  if (tok[0] == '.') {
    return runtime_handle_pintest_end(env, tok, buf);
  }

  /* Check for an available pin */
  if (env->pins_cursor >= env->pins_count) {
    runtime_error(env, "Only %zu pins specified\n", env->pins_count);
    return STATE_ERR;
  }

  /* Validate test value */
  unsigned int value = 0;

  if (runtime_parse_val(tok, &value) != RC_OK) {
    runtime_error(env, "Expected pin value, found '%s'\n", tok);
    return STATE_ERR;
  }

  /* Register test value */
  env->pins_values[env->pins_cursor++] = value;

  return STATE_PINTEST_DATA;
}

state_t runtime_handle_pintest_end(env_t * env, const char * tok, const char * buf) {

  /* Print test values */
  runtime_print(env, STYLE_CMD, "PINTEST\t");

  for (size_t p = 0; p < env->pins_cursor; p++) {
    if (p > 0) {
      runtime_print(env, STYLE_NONE, " ");
    }

    runtime_print(env, STYLE_NONE, "%s[", env->pins[p]);
    runtime_print(env, STYLE_NONE, "%u", env->pins_values[p]);
    runtime_print(env, STYLE_NONE, "]");
  }

  runtime_print(env, STYLE_NONE, "\n");

  /* Compare pin list */
  runtime_print(env, STYLE_NONE, "       \t");

  for (size_t p = 0; p < env->pins_cursor; p++) {
    if (p > 0) {
      runtime_print(env, STYLE_NONE, " ");
    }

    /* Read from pin */
    output_t out = env->adapter->read_pin(env->instance, env->pins[p]);

    /* Compare */
    bool match = (env->pins_values[p] == out.data);

    env->success = env->success && match;

    runtime_print(env, match ? STYLE_OK : STYLE_ERR, "%s[", env->pins[p]);
    runtime_print(env, match ? STYLE_OK : STYLE_ERR, "%u", out.data);
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
  unsigned int addr = 0;

  if (runtime_parse_val(tok, &addr) != RC_OK) {
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
  unsigned int word = 0;

  if (runtime_parse_val(tok, &word) != RC_OK) {
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
  unsigned int addr = 0;

  if (runtime_parse_val(tok, &addr) != RC_OK) {
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
  unsigned int word = 0;

  if (runtime_parse_val(tok, &word) != RC_OK) {
    runtime_error(env, "Expected word, found '%s'\n", tok);
    return STATE_ERR;
  }

  /* Read from memory */
  unsigned int addr = env->mem_addr + env->mem_offset++;
  output_t out = env->adapter->read_mem(env->instance, addr);

  /* Compare */
  bool match = (word == out.data);

  env->success = env->success && match;

  runtime_print(env, STYLE_NONE, "\t");
  runtime_print_addr(env, STYLE_NONE, addr);
  runtime_print(env, STYLE_NONE, "\t");
  runtime_print_word(env, STYLE_NONE, word);
  runtime_print(env, STYLE_NONE, "\t");
  runtime_print_word(env, match ? STYLE_OK : STYLE_ERR, out.data);
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
  unsigned int cycles = 0;

  if (runtime_parse_val(tok, &cycles) != RC_OK) {
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

rc_t runtime_parse_val(const char * tok, unsigned int * val) {
  int base = 10;
  char * end;

  if (tok[0] == '$') {
    base = 16;
    tok++;
  } else if (tok[0] == '%') {
    base = 2;
    tok++;
  }

  *val = strtoul(tok, &end, base);

  if (end == tok + strlen(tok)) {
    return RC_OK;
  } else {
    return RC_PARSE_ERR;
  }

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
  runtime_print(env, style, "($%04X)", addr);
}

void runtime_print_word(const env_t * env, style_t style, unsigned int word) {
  runtime_print(env, style, "$%02X", word);
}

env_t * runtime_env_init(const adapter_t * adapter, const char * file) {
  env_t * env = malloc(sizeof(env_t));

  /* Initialize emulator */
  env->adapter = adapter;
  env->instance = adapter->init();

  /* Initialize runtime */
  env->state = STATE_START;
  env->file = file;
  env->line = 1;
  env->success = true;

  /* Initialize pins */
  env->pins = malloc(sizeof(char *) * MAX_PINS);
  env->pins_buf = malloc(sizeof(char) * MAX_PINS * PIN_LEN);
  env->pins_count = 0;
  env->pins_cursor = 0;
  env->pins_values = malloc(sizeof(unsigned int) * MAX_PINS);

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
  free(env->pins_values);

  free(env);
}
