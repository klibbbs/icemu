#include "runtime.h"

#include <dlfcn.h>

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


/* --- Constants --- */

enum {
    BUF_LEN  = 4096,
    PIN_LEN  = 16,
    MAX_PINS = 64
};

/* --- Types --- */

typedef enum {
    RC_OK,
    RC_ERR
} rc_t;

typedef enum {
    STYLE_NONE = 0,
    STYLE_TEXT = 1,
    STYLE_OK   = 32,
    STYLE_ERR  = 31,
    STYLE_INFO = 35,
    STYLE_CMD  = 34
} style_t;

typedef enum {
    STATE_NONE,
    STATE_ERR,
    STATE_SUCCESS,
    STATE_FAILURE,
    STATE_START,
    STATE_NEXT,
    STATE_CMD,
    STATE_DEVICE_FILE,
    STATE_EXEC_FILE,
    STATE_PINDEF_PIN,
    STATE_PINTEST_DATA,
    STATE_MEMSET_ADDR,
    STATE_MEMSET_DATA,
    STATE_MEMTEST_ADDR,
    STATE_MEMTEST_DATA,
    STATE_RUN_CYCLES
} state_t;

typedef struct {
    unsigned int data;
    unsigned int mask;
    size_t bits;
    size_t base;
} test_t;

typedef struct {
    void * dl;
    const adapter_t * adapter;
    void * instance;
} device_t;

typedef struct {
    const char * file;
    size_t line;
    rc_t success;

    device_t * device;

    char ** pins;
    char * pins_buf;
    size_t pins_count;

    test_t * pin_tests;
    size_t pin_tests_count;

    unsigned int mem_addr;
    size_t mem_offset;
} env_t;

static state_t runtime_exec_repl(void);
static state_t runtime_exec_file(const char * file);
static state_t runtime_exec_stream(env_t * env, FILE * stream);
static state_t runtime_exec_line(env_t * env, char * buf, state_t state);
static state_t runtime_exec_flush(env_t * env, state_t state);
static state_t runtime_exec_eof(env_t * env, state_t state);
static state_t runtime_exec_token(env_t * env, const char * tok, const char * buf, state_t state);

static state_t runtime_handle_start(env_t * env, const char * tok, const char * buf);
static state_t runtime_handle_cmd(env_t * env, const char * tok, const char * buf);
static state_t runtime_handle_nop(env_t * env, const char * tok, const char * buf);
static state_t runtime_handle_exit(env_t * env, const char * tok, const char * buf);
static state_t runtime_handle_device(env_t * env, const char * tok, const char * buf);
static state_t runtime_handle_device_file(env_t * env, const char * tok, const char * buf);
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

static rc_t runtime_test(env_t * env, value_t val, test_t test);

static rc_t runtime_parse_value(const env_t * env, const char * tok, value_t * val);
static rc_t runtime_parse_test(const env_t * env, const char * tok, test_t * test);
static rc_t runtime_parse_test_hex(const env_t * env, const char * tok, test_t * test);
static rc_t runtime_parse_test_bin(const env_t * env, const char * tok, test_t * test);
static rc_t runtime_parse_test_dec(const env_t * env, const char * tok, test_t * test);
static rc_t runtime_parse_file(const env_t * env, const char * tok, char * file);
static rc_t runtime_parse_device(const env_t * env, const char * tok, char * file, char * id);

static void runtime_error(const env_t * env, const char * format, ...);

static void runtime_print(const env_t * env, style_t style, const char * format, ...);
static void runtime_print_addr(const env_t * env, style_t style, unsigned int addr);
static void runtime_print_word(const env_t * env, style_t style, unsigned int word);
static void runtime_print_value(const env_t * env, style_t style, value_t val);
static void runtime_print_test(const env_t * env, style_t style, test_t test);
static void runtime_print_test_hex(const env_t * env, style_t style, test_t test);
static void runtime_print_test_bin(const env_t * env, style_t style, test_t test);
static void runtime_print_test_dec(const env_t * env, style_t style, test_t test);

static device_t * runtime_device_init(const char * file, const char * id);
static void runtime_device_destroy(device_t * device);

static env_t * runtime_env_init(const char * file);
static void runtime_env_destroy(env_t * env);

/* --- Main --- */

int main (int argc, char * argv[]) {
    int i;
    state_t state = STATE_NONE;

    /* Execute files sequentially if specified, otherwise open the REPL */
    if (argc > 1) {
        for (i = 1; i < argc; i++) {
            state = runtime_exec_file(argv[i]);

            /* Stop execution on error or failure */
            if (state == STATE_FAILURE || state == STATE_ERR) {
                break;
            }
        }
    } else {
        state = runtime_exec_repl();
    }

    if (state == STATE_SUCCESS) {
        return EXIT_SUCCESS;
    } else {
        return EXIT_FAILURE;
    }
}

/* --- Private functions --- */

state_t runtime_exec_repl(void) {

    /* Initialize environment */
    env_t * env = runtime_env_init("shell");

    /* Parse and execute line by line */
    char buf[BUF_LEN] = "";
    state_t state = STATE_START;

    do {
        state = runtime_exec_line(env, buf, state);

        /* Stop execution only on exit */
        if (state == STATE_SUCCESS || state == STATE_FAILURE) {
            break;
        }

        /* On error, read the next command */
        if (state == STATE_ERR) {
            state = STATE_CMD;
        }

        /* Flush unterminated commands on EOL */
        state = runtime_exec_flush(env, state);

        /* Print a command prompt */
        runtime_print(env, STYLE_INFO, "> ");
    } while (fgets(buf, BUF_LEN, stdin) != NULL);

    /* Finish execution on EOF */
    if (feof(stdin)) {
        state = runtime_exec_eof(env, state);
    }

    if (ferror(stdin)) {
        runtime_error(env, "%s", strerror(errno));
        state = STATE_ERR;
    }

    /* Clean up environment */
    runtime_env_destroy(env);

    return state;
}

state_t runtime_exec_file(const char * file) {
    env_t * env;
    state_t state = STATE_NONE;

    /* Open file for reading */
    FILE * f = fopen(file, "r");

    if (f == NULL) {
        fprintf(stderr, "Error opening '%s': %s\n", file, strerror(errno));
        return STATE_ERR;
    }

    /* Initialize runtime environment */
    env = runtime_env_init(file);

    /* Execute contents of file */
    state = runtime_exec_stream(env, f);

    /* Clean up environment */
    runtime_env_destroy(env);

    /* Close stream */
    fclose(f);

    return state;
}

state_t runtime_exec_stream(env_t * env, FILE * stream) {

    /* Parse and execute line by line */
    char buf[BUF_LEN] = "";
    state_t state = STATE_START;

    do {
        state = runtime_exec_line(env, buf, state);

        /* Stop execution on exit or on error */
        if (state == STATE_SUCCESS || state == STATE_FAILURE || state == STATE_ERR) {
            break;
        }

        /* Increment line counter */
        env->line++;
    } while (fgets(buf, BUF_LEN, stream) != NULL);

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
        if (state == STATE_SUCCESS || state == STATE_FAILURE || state == STATE_ERR) {
            return state;
        }

        /* Advance to the next token */
        ptr = NULL;
    }

    return state;
}

state_t runtime_exec_flush(env_t * env, state_t state) {

    /* Execute a .nop to finish any unterminated commands */
    return runtime_exec_token(env, ".nop", "", state);
}

state_t runtime_exec_eof(env_t * env, state_t state) {

    /* If already in an exit state, nothing more to do */
    if (state == STATE_SUCCESS || state == STATE_FAILURE) {
        return state;
    }

    /* Execute an implicit .exit to finish unterminated commands and run exit sequence */
    return runtime_exec_token(env, ".exit", "", state);
}

state_t runtime_exec_token(env_t * env, const char * tok, const char * buf, state_t state) {

    /* Handle token based on state */
    switch (state) {
        case STATE_START:
            return runtime_handle_start(env, tok, buf);
        case STATE_CMD:
            return runtime_handle_cmd(env, tok, buf);
        case STATE_DEVICE_FILE:
            return runtime_handle_device_file(env, tok, buf);
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
        case STATE_SUCCESS:
        case STATE_FAILURE:
        case STATE_ERR:
            return state;
        default:
            runtime_error(env, "Unexpected state %d", state);
            return STATE_ERR;
    }
}

state_t runtime_handle_start(env_t * env, const char * tok, const char * buf) {
    runtime_print(env, STYLE_CMD, "START\t");
    runtime_print(env, STYLE_NONE, "%s\n", env->file);

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
    } else if (strcmp(tok, ".device") == 0) {
        return runtime_handle_device(env, tok, buf);
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
    runtime_print(env, STYLE_CMD, "EXIT\t");

    if (env->success == RC_OK) {
        runtime_print(env, STYLE_OK, "SUCCESS\n");
        return STATE_SUCCESS;
    } else {
        runtime_print(env, STYLE_ERR, "FAILURE\n");
        return STATE_FAILURE;
    }
}

state_t runtime_handle_device(env_t * env, const char * tok, const char * buf) {
    return STATE_DEVICE_FILE;
}

state_t runtime_handle_device_file(env_t * env, const char * tok, const char * buf) {
    device_t * device;

    /* Validate device library */
    char file[BUF_LEN];
    char id[BUF_LEN];

    if (runtime_parse_device(env, tok, file, id) != RC_OK) {
        runtime_error(env, "Expected device library, found '%s'\n", tok);
        return STATE_ERR;
    }

    /* Load device */
    device = runtime_device_init(file, id);

    if (device == NULL) {
        runtime_error(env, "Error loading device '%s' from library '%s'", id, file);
        return STATE_ERR;
    }

    /* Clean up old device and register new one */
    if (env->device) {
        runtime_device_destroy(env->device);
    }

    env->device = device;

    runtime_print(env, STYLE_CMD, "DEVICE\t");
    runtime_print(env, STYLE_INFO, "%s\t", env->device->adapter->id);
    runtime_print(env, STYLE_NONE, "(%s)\n", env->device->adapter->name);

    return STATE_CMD;
}

state_t runtime_handle_exec(env_t * env, const char * tok, const char * buf) {
    return STATE_EXEC_FILE;
}

state_t runtime_handle_exec_file(env_t * env, const char * tok, const char * buf) {
    state_t state;
    char file[BUF_LEN];
    FILE * f;

    /* Store current filename and line number */
    const char * old_file = env->file;
    size_t old_line = env->line;

    /* Validate filename */
    if (runtime_parse_file(env, tok, file) != RC_OK) {
        runtime_error(env, "Expected filename, found '%s'\n", tok);
        return STATE_ERR;
    }

    /* Open file for reading */
    f = fopen(file, "r");

    if (f == NULL) {
        runtime_error(env, "Error opening file '%s': %s", tok, strerror(errno));
        return STATE_ERR;
    }

    /* Initialize new filename and line number */
    env->file = file;
    env->line = 1;

    /* Execute contents of file */
    runtime_print(env, STYLE_CMD, "EXEC\t");
    runtime_print(env, STYLE_NONE, "%s\n", file);
    state = runtime_exec_stream(env, f);

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
    size_t p;

    /* Validate device */
    if (env->device == NULL) {
        runtime_error(env, "No device configured");
        return STATE_ERR;
    }

    /* Print pin values */
    runtime_print(env, STYLE_CMD, "PINS\t");

    for (p = 0; p < env->pins_count; p++) {

        /* Read from pin */
        value_t val = env->device->adapter->read_pin(env->device->instance, env->pins[p]);

        if (p > 0) {
            runtime_print(env, STYLE_NONE, " ");
        }

        runtime_print(env, STYLE_NONE, "%s[", env->pins[p]);
        runtime_print_value(env, STYLE_NONE, val);
        runtime_print(env, STYLE_NONE, "]");
    }

    runtime_print(env, STYLE_NONE, "\n");

    return STATE_CMD;
}

state_t runtime_handle_pindef(env_t * env, const char * tok, const char * buf) {

    /* Validate device */
    if (env->device == NULL) {
        runtime_error(env, "No device configured");
        return STATE_ERR;
    }

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

    if (!env->device->adapter->can_read_pin(env->device->instance, tok)) {
        runtime_error(env, "Unreadable pin '%s'", tok);
        return STATE_ERR;
    }

    /* Register pin */
    env->pins[env->pins_count++] = strcpy(&env->pins_buf[env->pins_count * PIN_LEN], tok);

    return STATE_PINDEF_PIN;
}

state_t runtime_handle_pindef_end(env_t * env, const char * tok, const char * buf) {
    size_t p;

    /* Print pin list */
    runtime_print(env, STYLE_CMD, "PINDEF\t");

    for (p = 0; p < env->pins_count; p++) {
        if (p > 0) {
            runtime_print(env, STYLE_NONE, " ");
        }

        runtime_print(env, STYLE_NONE, "%s", env->pins[p]);
    }

    runtime_print(env, STYLE_NONE, "\n");

    return runtime_handle_cmd(env, tok, buf);
}

state_t runtime_handle_pintest(env_t * env, const char * tok, const char * buf) {

    /* Validate device */
    if (env->device == NULL) {
        runtime_error(env, "No device configured");
        return STATE_ERR;
    }

    /* Reset pin tests */
    env->pin_tests_count = 0;

    return STATE_PINTEST_DATA;
}

state_t runtime_handle_pintest_data(env_t * env, const char * tok, const char * buf) {
    test_t test;

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
    if (runtime_parse_test(env, tok, &test) != RC_OK) {
        runtime_error(env, "Expected pin test value, found '%s'\n", tok);
        return STATE_ERR;
    }

    /* Register test value */
    env->pin_tests[env->pin_tests_count++] = test;

    return STATE_PINTEST_DATA;
}

state_t runtime_handle_pintest_end(env_t * env, const char * tok, const char * buf) {
    size_t p;

    /* Print test values */
    runtime_print(env, STYLE_CMD, "PINTEST\t");

    for (p = 0; p < env->pin_tests_count; p++) {
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

    for (p = 0; p < env->pin_tests_count; p++) {

        /* Read from pin */
        value_t val = env->device->adapter->read_pin(env->device->instance, env->pins[p]);

        /* Compare */
        rc_t rc = runtime_test(env, val, env->pin_tests[p]);
        style_t style = (rc == RC_OK) ? STYLE_OK : STYLE_ERR;

        if (p > 0) {
            runtime_print(env, STYLE_NONE, " ");
        }

        runtime_print(env, style, "%s[", env->pins[p]);
        runtime_print_value(env, style, val);
        runtime_print(env, style, "]");
    }

    runtime_print(env, STYLE_NONE, "\n");

    return runtime_handle_cmd(env, tok, buf);
}

state_t runtime_handle_memset(env_t * env, const char * tok, const char * buf) {

    /* Validate device */
    if (env->device == NULL) {
        runtime_error(env, "No device configured");
        return STATE_ERR;
    }

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
    value_t val;
    unsigned int word;
    unsigned int addr;

    /* Check for end condition */
    if (tok[0] == '.') {
        return runtime_handle_memset_end(env, tok, buf);
    }

    /* Validate word */
    if (runtime_parse_value(env, tok, &val) == RC_OK) {
        word = val.data;
    } else {
        runtime_error(env, "Expected word, found '%s'\n", tok);
        return STATE_ERR;
    }

    /* Write to memory */
    addr = env->mem_addr + env->mem_offset++;

    env->device->adapter->write_mem(env->device->instance, addr, word);

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

    /* Validate device */
    if (env->device == NULL) {
        runtime_error(env, "No device configured");
        return STATE_ERR;
    }

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
    test_t test;
    value_t val;
    unsigned int addr;
    rc_t rc;
    style_t style;

    /* Check for end condition */
    if (tok[0] == '.') {
        return runtime_handle_memtest_end(env, tok, buf);
    }

    /* Validate test word */
    if (runtime_parse_test(env, tok, &test) != RC_OK) {
        runtime_error(env, "Expected test word, found '%s'\n", tok);
        return STATE_ERR;
    }

    /* Read from memory */
    addr = env->mem_addr + env->mem_offset++;
    val = env->device->adapter->read_mem(env->device->instance, addr);

    /* Compare */
    rc = runtime_test(env, val, test);
    style = (rc == RC_OK) ? STYLE_OK : STYLE_ERR;

    runtime_print(env, STYLE_NONE, "\t");
    runtime_print_addr(env, STYLE_NONE, addr);
    runtime_print(env, STYLE_NONE, "\t");
    runtime_print_test(env, STYLE_NONE, test);
    runtime_print(env, STYLE_NONE, "\t");
    runtime_print_value(env, style, val);
    runtime_print(env, STYLE_NONE, "\n");

    return STATE_MEMTEST_DATA;
}

state_t runtime_handle_memtest_end(env_t * env, const char * tok, const char * buf) {
    return runtime_handle_cmd(env, tok, buf);
}

state_t runtime_handle_reset(env_t * env, const char * tok, const char * buf) {

    /* Validate device */
    if (env->device == NULL) {
        runtime_error(env, "No device configured");
        return STATE_ERR;
    }

    /* Reset intance */
    env->device->adapter->reset(env->device->instance);

    runtime_print(env, STYLE_CMD, "RESET\n");

    return STATE_CMD;
}

state_t runtime_handle_step(env_t * env, const char * tok, const char * buf) {

    /* Validate device */
    if (env->device == NULL) {
        runtime_error(env, "No device configured");
        return STATE_ERR;
    }

    /* Step instance */
    env->device->adapter->step(env->device->instance);

    runtime_print(env, STYLE_CMD, "STEP\n");

    return STATE_CMD;
}


state_t runtime_handle_run(env_t * env, const char * tok, const char * buf) {

    /* Validate device */
    if (env->device == NULL) {
        runtime_error(env, "No device configured");
        return STATE_ERR;
    }

    return STATE_RUN_CYCLES;
}

state_t runtime_handle_run_cycles(env_t * env, const char * tok, const char * buf) {
    value_t val;
    unsigned int cycles;
    clock_t start, elapsed;
    double khz;

    /* Validate cycle count */
    if (runtime_parse_value(env, tok, &val) == RC_OK) {
        cycles = val.data;
    } else {
        runtime_error(env, "Expected cycle count, found '%s'\n", tok);
        return STATE_ERR;
    }

    /* Start run clock */
    start = clock();

    /* Run instance */
    env->device->adapter->run(env->device->instance, cycles);

    /* Capture clock speed */
    elapsed = clock() - start;
    khz = .001 * CLOCKS_PER_SEC * cycles / elapsed;

    runtime_print(env, STYLE_CMD, "RUN\t");
    runtime_print(env, STYLE_NONE, "%zu\t@\t", cycles);

    if (khz > 1000) {
        runtime_print(env, STYLE_INFO, "%.3f MHz\n", khz / 1000);
    } else {
        runtime_print(env, STYLE_INFO, "%.3f kHz\n", khz);
    }

    return STATE_CMD;
}

rc_t runtime_test(env_t * env, value_t val, test_t test) {
    if (test.data == (val.data & test.mask)) {
        return RC_OK;
    } else {
        env->success = RC_ERR;

        return RC_ERR;
    }
}

rc_t runtime_parse_value(const env_t * env, const char * tok, value_t * value) {
    test_t test;

    if (runtime_parse_test(env, tok, &test) != RC_OK) {
        return RC_ERR;
    }

    if (~test.mask) {
        return RC_ERR;
    }

    value->data = test.data;
    value->bits = test.bits;
    value->base = test.base;

    return RC_OK;
}

rc_t runtime_parse_test(const env_t * env, const char * tok, test_t * test) {

    /* Check sigil to determine base */
    if (tok[0] == '$') {
        return runtime_parse_test_hex(env, tok, test);
    } else if (tok[0] == '%') {
        return runtime_parse_test_bin(env, tok, test);
    } else {
        return runtime_parse_test_dec(env, tok, test);
    }
}

rc_t runtime_parse_test_hex(const env_t * env, const char * tok, test_t * test) {
    char buf[BUF_LEN];
    char * c, * end;

    /* Check sigil */
    if (tok[0] != '$') {
        return RC_ERR;
    }

    tok++;
    test->base = 16;
    test->bits = strlen(tok) * 4;

    /* Parse data */
    strcpy(buf, tok);

    for (c = buf; *c; c++) {
        *c = (*c == 'X' || *c == 'x') ? '0' : *c;
    }

    test->data = strtoul(buf, &end, 16);

    if (end < buf + strlen(buf)) {
        return RC_ERR;
    }

    /* Parse (inverse) mask */
    strcpy(buf, tok);

    for (c = buf; *c; c++) {
        *c = (*c == 'X' || *c == 'x') ? 'F' : '0';
    }

    test->mask = ~strtoul(buf, &end, 16);

    if (end < buf + strlen(buf)) {
        return RC_ERR;
    }

    return RC_OK;
}

rc_t runtime_parse_test_bin(const env_t * env, const char * tok, test_t * test) {
    char buf[BUF_LEN];
    char * c, * end;

    /* Check sigil */
    if (tok[0] != '%') {
        return RC_ERR;
    }

    tok++;
    test->base = 2;
    test->bits = strlen(tok);

    /* Parse data */
    strcpy(buf, tok);

    for (c = buf; *c; c++) {
        *c = (*c == 'X' || *c == 'x') ? '0' : *c;
    }

    test->data = strtoul(buf, &end, 2);

    if (end < buf + strlen(buf)) {
        return RC_ERR;
    }

    /* Parse (inverse) mask */
    strcpy(buf, tok);

    for (c = buf; *c; c++) {
        *c = (*c == 'X' || *c == 'x') ? '1' : '0';
    }

    test->mask = ~strtoul(buf, &end, 2);

    if (end < buf + strlen(buf)) {
        return RC_ERR;
    }

    return RC_OK;
}

rc_t runtime_parse_test_dec(const env_t * env, const char * tok, test_t * test) {
    char * end;

    test->base = 10;
    test->mask = -1; /* Don't-care expressions are not supported for multi-bit decimal values */

    /* Handle single bit */
    if (tok[1] == '\0' && (tok[0] == '0' || tok[0] == '1')) {
        test->data = tok[0] - '0';
        test->mask = -1;
        test->bits = 1;

        return RC_OK;
    }

    if (tok[1] == '\0' && (tok[0] == 'X' || tok[0] == 'x')) {
        test->data = 0;
        test->mask = 0;
        test->bits = 1;

        return RC_OK;
    }

    /* Parse as decimal */
    test->bits = ceil(strlen(tok) * 3.322); /* log2(X) = log10(X) * log2(10) */
    test->data = strtoul(tok, &end, test->base);

    if (end < tok + strlen(tok)) {
        return RC_ERR;
    }

    return RC_OK;
}

rc_t runtime_parse_file(const env_t * env, const char * tok, char * file) {
    char * last_slash;
    char * base;

    /* Use absolute paths as-is */
    if (tok[0] == '/') {
        strcpy(file, tok);
        return RC_OK;
    }

    /* Find the base filename of the current path */
    strcpy(file, env->file);

    last_slash = strrchr(file, '/');
    base = (last_slash == NULL ? file : last_slash + 1);

    /* Copy the new filename over the current filename */
    strcpy(base, tok);

    return RC_OK;
}

rc_t runtime_parse_device(const env_t * env, const char * tok, char * file, char * id) {
    char * last_slash;
    char * base;
    size_t len;

    /* Determine file path */
    if (runtime_parse_file(env, tok, file) != RC_OK) {
        return RC_ERR;
    }

    /* Extract ID from base name of file path */
    last_slash = strrchr(file, '/');
    base = (last_slash == NULL ? file : last_slash + 1);

    strcpy(id, base);

    /* Validate and strip ".so" file extension */
    len = strlen(id);

    if (len < 4) {
        return RC_ERR;
    }

    if (strcmp(id + len - 3, ".so") != 0) {
        return RC_ERR;
    }

    id[len - 3] = '\0';

    return RC_OK;
}

void runtime_error(const env_t * env, const char * format, ...) {
    va_list args;

    fprintf(stderr, "\033[0;%dm", STYLE_ERR);
    fprintf(stderr, "%s:%zu: ", env->file, env->line);

    va_start(args, format);

    vfprintf(stderr, format, args);

    va_end(args);

    fprintf(stderr, "\033[0;0m");
    fprintf(stderr, "\n");
}

void runtime_print(const env_t * env, style_t style, const char * format, ...) {
    va_list args;

    if (style != STYLE_NONE) {
        printf("\033[0;%dm", style);
    }

    va_start(args, format);

    vprintf(format, args);

    va_end(args);

    if (style != STYLE_NONE) {
        printf("\033[0;0m");
    }
}

void runtime_print_addr(const env_t * env, style_t style, unsigned int addr) {
    value_t val;

    val.data = addr;
    val.bits = env->device->adapter->mem_addr_width;
    val.base = 16;

    runtime_print(env, style, "(");
    runtime_print_value(env, style, val);
    runtime_print(env, style, ")");
}

void runtime_print_word(const env_t * env, style_t style, unsigned int word) {
    value_t val;

    val.data = word;
    val.bits = env->device->adapter->mem_word_width;
    val.base = 16;

    runtime_print_value(env, style, val);
}

void runtime_print_value(const env_t * env, style_t style, value_t val) {
    test_t test;

    test.data = val.data;
    test.mask = -1;
    test.bits = val.bits;
    test.base = val.base;

    runtime_print_test(env, style, test);
}

void runtime_print_test(const env_t * env, style_t style, test_t test) {

    if (test.base == 10 || test.bits == 1) {
        /* Use decimal for displaying single bits */
        runtime_print_test_dec(env, style, test);
    } else if (test.base == 2) {
        runtime_print_test_bin(env, style, test);
    } else {
        /* Print all other bases as hex */
        runtime_print_test_hex(env, style, test);
    }
}

void runtime_print_test_hex(const env_t * env, style_t style, test_t test) {
    char buf[BUF_LEN];
    char mask[BUF_LEN];
    char * c, * m;

    /* Build format string from base and calculated width */
    int width = ceil(test.bits / 4.0);
    char format[8] = "";

    sprintf(format, "$%%0%dX", width);

    /* Construct separate data and mask strings */
    sprintf(buf, format, test.data);
    sprintf(mask, format, test.mask & ~(-1 << test.bits));

    /* Merge data and mask */
    for (c = buf, m = mask; *c && *m; c++, m++) {
        if (*m == '0') {
            *c = 'X';
        }
    }

    runtime_print(env, style, "%s", buf);
}

void runtime_print_test_bin(const env_t * env, style_t style, test_t test) {
    char buf[BUF_LEN];
    char * c;
    unsigned int data = test.data;
    unsigned int mask = test.mask;

    buf[test.bits + 1] = '\0';

    for (c = buf + test.bits; c > buf; c--) {
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
}

void runtime_print_test_dec(const env_t * env, style_t style, test_t test) {

    /* Handle single bit */
    if (test.bits == 1) {
        runtime_print(env, style, test.mask ? test.data ? "1" : "0" : "X");
        return;
    }

    runtime_print(env, style, "%u", test.data);
}

device_t * runtime_device_init(const char * file, const char * id) {
    char adapter_sym[BUF_LEN] = "";
    adapter_func adapter;
    device_t * device;

    /* Load dynamic library */
    void * dl = dlopen(file, RTLD_NOW);

    if (dl == NULL) {
        fprintf(stderr, "Error loading library '%s': %s\n", file, dlerror());
        return NULL;
    }

    /* Load adapter symbol */
    sprintf(adapter_sym, "%s_adapter", id);
    adapter = (adapter_func)dlsym(dl, adapter_sym);

    if (adapter == NULL) {
        dlclose(dl);
        fprintf(stderr, "Error loading symbol '%s': %s\n", adapter_sym, dlerror());
        return NULL;
    }

    /* Initialize device */
    device = malloc(sizeof(device_t));

    device->dl = dl;
    device->adapter = adapter();
    device->instance = device->adapter->init();

    return device;
}

void runtime_device_destroy(device_t * device) {

    /* Clean up instance and adapter */
    device->adapter->destroy(device->instance);

    /* Close dynamic library */
    dlclose(device->dl);

    free(device);
}

env_t * runtime_env_init(const char * file) {
    env_t * env = malloc(sizeof(env_t));

    /* Initialize runtime */
    env->file = file;
    env->line = 1;
    env->success = RC_OK;

    /* Initialize device emulator */
    env->device = NULL;

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

    /* Clean up device emulator if defined */
    if (env->device) {
        runtime_device_destroy(env->device);
    }

    /* Clean up runtime */
    free(env->pins);
    free(env->pins_buf);
    free(env->pin_tests);

    free(env);
}
