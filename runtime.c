#include "runtime.h"

#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* --- Macros --- */

#define str_ok(str)  "\033[0;32m" str "\033[0;0m"
#define str_err(str) "\033[0;31m" str "\033[0;0m"


/* --- Private declarations --- */

typedef enum {
  STATE_START,
  STATE_CMD,
  STATE_NEXT,
  STATE_PINSET,
  STATE_MEMSET_ADDR,
  STATE_MEMSET_DATA,
  STATE_MEMTEST_ADDR,
  STATE_MEMTEST_DATA,
} state_t;

typedef struct {
  const adapter_t * adapter;
  void * instance;

  state_t state;
  bool success;

  char ** pins;
  char * pins_buf;
  size_t pins_count;
  size_t pins_cursor;

  unsigned int mem_addr;
  size_t mem_offset;
} env_t;

enum {
  BUF_LEN  = 4096,
  PIN_LEN  = 32,
  MAX_PINS = 64,
};

static rc_t runtime_exec_line(env_t * env, size_t line, char * buf);
static rc_t runtime_exec_cmd(env_t * env, size_t line, const char * tok, char * buf);
static rc_t runtime_exec_pinset(env_t * env, size_t line, const char * tok, char * buf);
static rc_t runtime_exec_memset_addr(env_t * env, size_t line, const char * tok, char * buf);
static rc_t runtime_exec_memset_data(env_t * env, size_t line, const char * tok, char * buf);
static rc_t runtime_exec_memtest_addr(env_t * env, size_t line, const char * tok, char * buf);
static rc_t runtime_exec_memtest_data(env_t * env, size_t line, const char * tok, char * buf);

static rc_t parse_number(const char * tok, unsigned int * num);
static rc_t parse_decimal(const char * tok, unsigned int * num);
static rc_t parse_hex(const char * tok, unsigned int * num);

static env_t * runtime_env_init();
static void runtime_env_destroy(env_t * env);

/* --- Public functions --- */

rc_t runtime_exec_file(const adapter_t * adapter, const char * file) {
  rc_t rc = RC_OK;
  env_t * env;

  FILE * f;
  size_t line = 0;
  char buf[BUF_LEN];

  /* Open stream for reading */
  f = fopen(file, "r");

  if (f == NULL) {
    fprintf(stderr, "Error opening '%s': %s\n", file, strerror(errno));

    return RC_FILE_ERR;
  }

  /* Initialize environment */
  env = runtime_env_init(adapter);

  /* Parse and execute line by line */
  while (fgets(buf, BUF_LEN,f) != NULL) {
    rc = runtime_exec_line(env, line, buf);

    /* Stop execution if parsing fails at any point */
    if (rc != RC_OK) {
      break;
    }

    line++;
  }

  if (ferror(f)) {
    fprintf(stderr, "Error reading '%s', line %zu: %s\n", file, line, strerror(errno));
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

rc_t runtime_exec_line(env_t * env, size_t line, char * buf) {
  rc_t rc = RC_OK;
  char * ptr;
  char * tok;

  /* Strip comments */
  if ((ptr = strchr(buf, '#')) != NULL) {
    *(ptr + 0) = '\n';
    *(ptr + 1) = '\0';
  }

  /* Tokenize on whitespace */
  ptr = buf;

  while ((tok = strtok(ptr, " \n\t\r\v")) != NULL) {

    if (strlen(tok) == 0) {
      fprintf(stderr, "Error on line %zu: unexpected empty token\n", line);
      return RC_ERR;
    }

    /* Capture the remainder of the line */
    char * remainder = buf + strlen(tok);

    if (*remainder == '\0') {
      remainder++;
    }

    /* Handle state machine */
    switch (env->state) {
      case STATE_START:
      case STATE_CMD:
        /* Parse command */
        rc = runtime_exec_cmd(env, line, tok, remainder);
        break;
      case STATE_PINSET:
        /* Parse output pin */
        rc = runtime_exec_pinset(env, line, tok, remainder);
        break;
      case STATE_MEMSET_ADDR:
        /* Parse reference address */
        rc = runtime_exec_memset_addr(env, line, tok, remainder);
        break;
      case STATE_MEMSET_DATA:
        /* Parse memory word */
        rc = runtime_exec_memset_data(env, line, tok, remainder);
        break;
      case STATE_MEMTEST_ADDR:
        /* Parse reference address */
        rc = runtime_exec_memtest_addr(env, line, tok, remainder);
        break;
      case STATE_MEMTEST_DATA:
        /* Parse memory word */
        rc = runtime_exec_memtest_data(env, line, tok, remainder);
        break;
      case STATE_NEXT:
        /* Advance to next line */
        env->state = STATE_CMD;
        return RC_OK;
      default:
        fprintf(stderr, "Error on line %zu: Unexpected parse state %d", line, env->state);
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

rc_t runtime_exec_cmd(env_t * env, size_t line, const char * tok, char * buf) {
  rc_t rc = RC_OK;

  /* Confirm that the token is a command */
  if (tok[0] != '.') {
    fprintf(stderr, "Parse error on line %zu: expected command, found '%s'\n", line, tok);
    return RC_PARSE_ERR;
  }

  /* Execute command */
  if (strcmp(tok, ".print") == 0) {

    /* Print remaining buffer and advance to next line */
    printf("%s", buf);

    env->state = STATE_NEXT;
  } else if (strcmp(tok, ".pinset") == 0) {

    /* Reset pinset state */
    env->pins_count = 0;
    env->pins_cursor = 0;
    env->state = STATE_PINSET;
  } else if (strcmp(tok, ".memset") == 0) {

    /* Reset memory state */
    env->mem_addr = 0;
    env->mem_offset = 0;
    env->state = STATE_MEMSET_ADDR;
  } else if (strcmp(tok, ".memtest") == 0) {

    /* Reset memory state */
    env->mem_addr = 0;
    env->mem_offset = 0;
    env->state = STATE_MEMTEST_ADDR;
  } else if (strcmp(tok, ".reset") == 0) {

    /* Reset instance */
    env->adapter->reset(env->instance);
  } else {
    fprintf(stderr, "Parse error on line %zu: unrecognized command '%s'\n", line, tok);
    return RC_PARSE_ERR;
  }

  return RC_OK;
}

rc_t runtime_exec_pinset(env_t * env, size_t line, const char * tok, char * buf) {

  /* If the token is a command, pin-setting is complete */
  if (tok[0] == '.') {
    env->state = STATE_CMD;
    return runtime_exec_cmd(env, line, tok, buf);
  }

  /* Validate pin */
  if (strlen(tok) + 1 > PIN_LEN) {
    fprintf(stderr, "Parse error on line %zu: invalid pin name '%s'\n", line, tok);
    return RC_PARSE_ERR;
  }

  if (!env->adapter->can_read_pin(env->instance, tok)) {
    fprintf(stderr, "Error on line %zu: unreadable pin '%s'\n", line, tok);
    return RC_EMULATOR_ERR;
  }

  /* Register new pin */
  env->pins[env->pins_count++] = strcpy(&env->pins_buf[env->pins_count * PIN_LEN], tok);

  return RC_OK;
}

rc_t runtime_exec_memset_addr(env_t * env, size_t line, const char * tok, char * buf) {

  /* Validate address */
  unsigned int addr = 0;

  if (parse_hex(tok, &addr) != RC_OK) {
    fprintf(stderr, "Parse error on line %zu: expected hex address, found '%s'\n", line, tok);
    return RC_PARSE_ERR;
  }

  /* Register reference address */
  env->mem_addr = addr;
  env->state = STATE_MEMSET_DATA;

  return RC_OK;
}

rc_t runtime_exec_memset_data(env_t * env, size_t line, const char * tok, char * buf) {

  /* If the token is a command, memory-setting is complete */
  if (tok[0] == '.') {
    env->state = STATE_CMD;
    return runtime_exec_cmd(env, line, tok, buf);
  }

  /* Validate data */
  unsigned int data = 0;

  if (parse_number(tok, &data) != RC_OK) {
    fprintf(stderr, "Parse error on line %zu: expected data, found '%s'\n", line, tok);
    return RC_PARSE_ERR;
  }

  /* Write memory */
  unsigned int addr = env->mem_addr + env->mem_offset++;

  env->adapter->write_mem(env->instance, addr, data);

  return RC_OK;
}

rc_t runtime_exec_memtest_addr(env_t * env, size_t line, const char * tok, char * buf) {

  /* Validate address */
  unsigned int addr = 0;

  if (parse_hex(tok, &addr) != RC_OK) {
    fprintf(stderr, "Parse error on line %zu: expected hex address, found '%s'\n", line, tok);
    return RC_PARSE_ERR;
  }

  /* Register reference address */
  env->mem_addr = addr;
  env->state = STATE_MEMTEST_DATA;

  return RC_OK;
}

rc_t runtime_exec_memtest_data(env_t * env, size_t line, const char * tok, char * buf) {

  /* If the token is a command, memory-setting is complete */
  if (tok[0] == '.') {
    env->state = STATE_CMD;
    return runtime_exec_cmd(env, line, tok, buf);
  }

  /* Validate data */
  unsigned int data = 0;

  if (parse_number(tok, &data) != RC_OK) {
    fprintf(stderr, "Parse error on line %zu: expected data, found '%s'\n", line, tok);
    return RC_PARSE_ERR;
  }

  /* Read memory */
  unsigned int addr = env->mem_addr + env->mem_offset++;
  output_t out = env->adapter->read_mem(env->instance, addr);

  /* Compare */
  if (data == out.data) {
    printf("$%04X: $%02X " str_ok("$%02X") "\n", addr, data, out.data);
  } else {
    printf("$%04X: $%02X " str_err("$%02X") "\n", addr, data, out.data);
    env->success = false;
  }



  return RC_OK;
}

rc_t parse_number(const char * tok, unsigned int * num) {
  if (parse_hex(tok, num) == RC_OK) {
    return RC_OK;
  }

  if (parse_decimal(tok, num) == RC_OK) {
    return RC_OK;
  }

  return RC_PARSE_ERR;
}

rc_t parse_decimal(const char * tok, unsigned int * num) {
  for (size_t i = 0; i < strlen(tok); i++) {
    if (!isdigit(tok[i])) {
      return RC_PARSE_ERR;
    }
  }

  *num = strtoul(tok + 1, NULL, 10);

  return RC_OK;
}

rc_t parse_hex(const char * tok, unsigned int * num) {
  if (tok[0] != '$') {
    return RC_PARSE_ERR;
  }

  for (size_t i = 1; i < strlen(tok); i++) {
    if (!isxdigit(tok[i])) {
      return RC_PARSE_ERR;
    }
  }

  *num = strtoul(tok + 1, NULL, 16);

  return RC_OK;
}

env_t * runtime_env_init(const adapter_t * adapter) {
  env_t * env = malloc(sizeof(env_t));

  /* Initialize emulator */
  env->adapter = adapter;
  env->instance = adapter->init();

  /* Initialize runtime state */
  env->state = STATE_START;
  env->success = true;
  env->pins = malloc(sizeof(char *) * MAX_PINS);
  env->pins_buf = malloc(sizeof(char) * MAX_PINS * PIN_LEN);
  env->pins_count = 0;
  env->pins_cursor = 0;

  /* Initialize memory state */
  env->mem_addr = 0;
  env->mem_offset = 0;

  return env;
}

void runtime_env_destroy(env_t * env) {
  size_t i;

  /* Clean up emulator */
  env->adapter->destroy(env->instance);

  /* Clean up parser state */
  free(env->pins);
  free(env->pins_buf);

  free(env);
}
