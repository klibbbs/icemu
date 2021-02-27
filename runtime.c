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
  PIN_LEN  = 32,
  MAX_PINS = 64,
};

typedef enum {
  COLOR_NONE = 0,
  COLOR_OK   = 32,
  COLOR_ERR  = 31,
} color_t;

typedef enum {
  STATE_START,
  STATE_CMD,
  STATE_NEXT,
  STATE_PINSET,
  STATE_MEMSET_ADDR,
  STATE_MEMSET_DATA,
  STATE_MEMTEST_ADDR,
  STATE_MEMTEST_DATA,
  STATE_RUN,
} state_t;

typedef struct {
  const adapter_t * adapter;
  void * instance;

  state_t state;
  const char * file;
  size_t line;
  bool success;
  bool verbose;

  char ** pins;
  char * pins_buf;
  size_t pins_count;
  size_t pins_cursor;

  unsigned int mem_addr;
  size_t mem_offset;

  clock_t run_cpu_start;
  clock_t run_cpu_end;
} env_t;

static rc_t runtime_exec_line(env_t * env, char * buf);
static rc_t runtime_exec_cmd(env_t * env, const char * tok, char * buf);
static rc_t runtime_exec_pinset(env_t * env, const char * tok, char * buf);
static rc_t runtime_exec_memset_addr(env_t * env, const char * tok, char * buf);
static rc_t runtime_exec_memset_data(env_t * env, const char * tok, char * buf);
static rc_t runtime_exec_memtest_addr(env_t * env, const char * tok, char * buf);
static rc_t runtime_exec_memtest_data(env_t * env, const char * tok, char * buf);
static rc_t runtime_exec_run(env_t * env, const char * tok, char * buf);

static rc_t runtime_parse_val(const char * tok, unsigned int * val);

static void runtime_debug(const env_t * env, const char * format, ...);
static void runtime_debug_pinset(const env_t * env);
static void runtime_debug_addr(const env_t * env, unsigned int addr);
static void runtime_debug_word(const env_t * env, unsigned int word);

static void runtime_print(const env_t * env, color_t color, const char * format, ...);
static void runtime_print_addr(const env_t * env, color_t color, unsigned int addr);
static void runtime_print_word(const env_t * env, color_t color, unsigned int word);

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

    if (strlen(tok) == 0) {
      fprintf(stderr, "Error on line %zu: unexpected empty token\n", env->line);
      return RC_ERR;
    }

    /* Re-point the buffer to the remainder of the line */
    if (tok + strlen(tok) >= end) {
      buf = end;
    } else {
      buf = tok + strlen(tok) + 1;
    }

    /* Handle state machine */
    switch (env->state) {
      case STATE_START:
        runtime_debug(env, "START\t%s\n", env->file);
        /* Fall through */
      case STATE_CMD:
        /* Parse command */
        rc = runtime_exec_cmd(env, tok, buf);
        break;
      case STATE_PINSET:
        /* Parse output pin */
        rc = runtime_exec_pinset(env, tok, buf);
        break;
      case STATE_MEMSET_ADDR:
        /* Parse reference address */
        rc = runtime_exec_memset_addr(env, tok, buf);
        break;
      case STATE_MEMSET_DATA:
        /* Parse memory word */
        rc = runtime_exec_memset_data(env, tok, buf);
        break;
      case STATE_MEMTEST_ADDR:
        /* Parse reference address */
        rc = runtime_exec_memtest_addr(env, tok, buf);
        break;
      case STATE_MEMTEST_DATA:
        /* Parse memory word */
        rc = runtime_exec_memtest_data(env, tok, buf);
        break;
      case STATE_RUN:
        /* Parse cycle count */
        rc = runtime_exec_run(env, tok, buf);
        break;
      case STATE_NEXT:
        /* Advance to next line */
        env->state = STATE_CMD;
        return RC_OK;
      default:
        fprintf(stderr, "Error on line %zu: Unexpected parse state %d", env->line, env->state);
        return RC_ERR;
    }

    if (rc != RC_OK) {
      break;
    }

    /* Advance to the next token */
    ptr = NULL;
  }

  return rc;
}

rc_t runtime_exec_cmd(env_t * env, const char * tok, char * buf) {
  rc_t rc = RC_OK;

  /* Confirm that the token is a command */
  if (tok[0] != '.') {
    fprintf(stderr, "Parse error on line %zu: expected command, found '%s'\n", env->line, tok);
    return RC_PARSE_ERR;
  }

  /* Execute command */
  if (strcmp(tok, ".verbose") == 0) {
    /* Set verbose flag */
    env->verbose = true;
  } else if (strcmp(tok, ".info") == 0) {

    /* Print remaining buffer */
    runtime_debug(env, "INFO\t");
    printf("%s", buf);

    /* Advance to next line */
    env->state = STATE_NEXT;
  } else if (strcmp(tok, ".pinset") == 0) {

    /* Reset pinset state */
    env->pins_count = 0;
    env->pins_cursor = 0;

    /* Consume pins */
    env->state = STATE_PINSET;
  } else if (strcmp(tok, ".memset") == 0) {

    /* Reset memory state */
    env->mem_addr = 0;
    env->mem_offset = 0;

    /* Consume reference address */
    env->state = STATE_MEMSET_ADDR;
  } else if (strcmp(tok, ".memtest") == 0) {

    /* Reset memory state */
    env->mem_addr = 0;
    env->mem_offset = 0;

    /* Consume reference address */
    env->state = STATE_MEMTEST_ADDR;
  } else if (strcmp(tok, ".reset") == 0) {

    /* Reset instance */
    env->adapter->reset(env->instance);
  } else if (strcmp(tok, ".step") == 0) {

    /* Step instance */
    env->adapter->step(env->instance);
    runtime_debug(env, "STEP\n");
  } else if (strcmp(tok, ".run") == 0) {

    /* Consume cycle count */
    env->state = STATE_RUN;
  } else {
    fprintf(stderr, "Parse error on line %zu: unrecognized command '%s'\n", env->line, tok);
    return RC_PARSE_ERR;
  }

  return RC_OK;
}

rc_t runtime_exec_pinset(env_t * env, const char * tok, char * buf) {

  /* If the token is a command, pin-setting is complete */
  if (tok[0] == '.') {
    runtime_debug(env, "PINSET\t");
    runtime_debug_pinset(env);
    runtime_debug(env, "\n");

    env->state = STATE_CMD;

    return runtime_exec_cmd(env, tok, buf);
  }

  /* Validate pin */
  if (strlen(tok) + 1 > PIN_LEN) {
    fprintf(stderr, "Parse error on line %zu: invalid pin name '%s'\n", env->line, tok);
    return RC_PARSE_ERR;
  }

  if (!env->adapter->can_read_pin(env->instance, tok)) {
    fprintf(stderr, "Error on line %zu: unreadable pin '%s'\n", env->line, tok);
    return RC_EMULATOR_ERR;
  }

  /* Register new pin */
  env->pins[env->pins_count++] = strcpy(&env->pins_buf[env->pins_count * PIN_LEN], tok);

  return RC_OK;
}

rc_t runtime_exec_memset_addr(env_t * env, const char * tok, char * buf) {

  /* Validate address */
  unsigned int addr = 0;

  if (runtime_parse_val(tok, &addr) != RC_OK) {
    fprintf(stderr, "Parse error on line %zu: expected address, found '%s'\n", env->line, tok);
    return RC_PARSE_ERR;
  }

  /* Register reference address */
  env->mem_addr = addr;

  runtime_debug(env, "MEMSET\n");

  /* Consume memory words */
  env->state = STATE_MEMSET_DATA;

  return RC_OK;
}

rc_t runtime_exec_memset_data(env_t * env, const char * tok, char * buf) {

  /* If the token is a command, memory-setting is complete */
  if (tok[0] == '.') {
    env->state = STATE_CMD;
    return runtime_exec_cmd(env, tok, buf);
  }

  /* Validate data */
  unsigned int data = 0;

  if (runtime_parse_val(tok, &data) != RC_OK) {
    fprintf(stderr, "Parse error on line %zu: expected word, found '%s'\n", env->line, tok);
    return RC_PARSE_ERR;
  }

  /* Write memory */
  unsigned int addr = env->mem_addr + env->mem_offset++;

  env->adapter->write_mem(env->instance, addr, data);

  runtime_debug(env, "\t");
  runtime_debug_addr(env, addr);
  runtime_debug(env, "\t");
  runtime_debug_word(env, data);
  runtime_debug(env, "\n");

  return RC_OK;
}

rc_t runtime_exec_memtest_addr(env_t * env, const char * tok, char * buf) {

  /* Validate address */
  unsigned int addr = 0;

  if (runtime_parse_val(tok, &addr) != RC_OK) {
    fprintf(stderr, "Parse error on line %zu: expected address, found '%s'\n", env->line, tok);
    return RC_PARSE_ERR;
  }

  /* Register reference address */
  env->mem_addr = addr;

  runtime_print(env, COLOR_NONE, "MEMTEST\n");

  /* Consume memory words */
  env->state = STATE_MEMTEST_DATA;

  return RC_OK;
}

rc_t runtime_exec_memtest_data(env_t * env, const char * tok, char * buf) {

  /* If the token is a command, memory-setting is complete */
  if (tok[0] == '.') {
    env->state = STATE_CMD;
    return runtime_exec_cmd(env, tok, buf);
  }

  /* Validate data */
  unsigned int data = 0;

  if (runtime_parse_val(tok, &data) != RC_OK) {
    fprintf(stderr, "Parse error on line %zu: expected data, found '%s'\n", env->line, tok);
    return RC_PARSE_ERR;
  }

  /* Read memory */
  unsigned int addr = env->mem_addr + env->mem_offset++;
  output_t out = env->adapter->read_mem(env->instance, addr);

  /* Compare */
  bool match = (data == out.data);

  env->success = env->success && match;

  runtime_print(env, COLOR_NONE, "\t");
  runtime_print_addr(env, COLOR_NONE, addr);
  runtime_print(env, COLOR_NONE, "\t");
  runtime_print_word(env, COLOR_NONE, data);
  runtime_print(env, COLOR_NONE, "\t");
  runtime_print_word(env, match ? COLOR_OK : COLOR_ERR, out.data);
  runtime_print(env, COLOR_NONE, "\n");

  return RC_OK;
}

rc_t runtime_exec_run(env_t * env, const char * tok, char * buf) {

  /* Validate cycles */
  unsigned int cycles = 0;

  if (runtime_parse_val(tok, &cycles) != RC_OK) {
    fprintf(stderr, "Parse error on line %zu: expected cycle count, found '%s'\n", env->line, tok);
    return RC_PARSE_ERR;
  }

  /* Run instance */
  runtime_debug(env, "RUN\t%zu\t...\t", cycles);

  env->adapter->run(env->instance, cycles);

  runtime_debug(env, "DONE\n");

  /* Advance to the next command */
  env->state = STATE_CMD;

  return RC_OK;
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

void runtime_debug(const env_t * env, const char * format, ...) {
  if (!env->verbose) {
    return;
  }

  va_list args;
  va_start(args, format);

  vprintf(format, args);

  va_end(args);
}

void runtime_debug_pinset(const env_t * env) {
  if (!env->verbose) {
    return;
  }

  for (size_t p = 0; p < env->pins_count; p++) {
    if (p > 0) {
      runtime_print(env, COLOR_NONE, " %s", env->pins[p]);
    } else {
      runtime_print(env, COLOR_NONE, "%s", env->pins[p]);
    }
  }
}

void runtime_debug_addr(const env_t * env, unsigned int addr) {
  if (!env->verbose) {
    return;
  }

  runtime_print_addr(env, COLOR_NONE, addr);
}

void runtime_debug_word(const env_t * env, unsigned int word) {
  if (!env->verbose) {
    return;
  }

  runtime_print_word(env, COLOR_NONE, word);
}

void runtime_print(const env_t * env, color_t color, const char * format, ...) {

  if (color != COLOR_NONE) {
    printf("\033[0;%dm", color);
  }

  va_list args;
  va_start(args, format);

  vprintf(format, args);

  va_end(args);

  if (color != COLOR_NONE) {
    printf("\033[0;0m");
  }
}

void runtime_print_addr(const env_t * env, color_t color, unsigned int addr) {
  runtime_print(env, color, "$%04X:", addr);
}

void runtime_print_word(const env_t * env, color_t color, unsigned int word) {
  runtime_print(env, color, "$%02X", word);
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
  env->verbose = false;

  /* Initialize pins */
  env->pins = malloc(sizeof(char *) * MAX_PINS);
  env->pins_buf = malloc(sizeof(char) * MAX_PINS * PIN_LEN);
  env->pins_count = 0;
  env->pins_cursor = 0;

  /* Initialize memory */
  env->mem_addr = 0;
  env->mem_offset = 0;

  /* Initialize benchmarks */
  env->run_cpu_start = 0;
  env->run_cpu_end = 0;

  return env;
}

void runtime_env_destroy(env_t * env) {
  size_t i;

  /* Clean up emulator */
  env->adapter->destroy(env->instance);

  /* Clean up runtime */
  free(env->pins);
  free(env->pins_buf);

  free(env);
}
