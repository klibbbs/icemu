#ifndef INCLUDE_RUNTIME_H
#define INCLUDE_RUNTIME_H

#include <stddef.h>

/* --- Object types --- */

typedef struct {
    unsigned int data;
    size_t bits;
    size_t base;
} value_t;

typedef struct {

    /* Constants */
    const char * const id;
    const char * const name;
    const size_t mem_addr_width;
    const size_t mem_word_width;

    /* Functions */
    void * (* init)(void);
    void (* destroy)(void * instance);
    void (* reset)(void * instance);
    void (* step)(void * instance);
    void (* run)(void * instance, size_t cycles);
    int (* can_read_pin)(const void * instance, const char * pin);
    int (* can_write_pin)(const void * instance, const char * pin);
    value_t (* read_pin)(const void * instance, const char * pin);
    void (* write_pin)(void * instance, const char * pin, unsigned int data);
    value_t (* read_mem)(const void * instance, unsigned int addr);
    void (* write_mem)(void * instance, unsigned int addr, unsigned int data);
} adapter_t;

/* --- Function types --- */

typedef const adapter_t * (* adapter_func)(void);

#endif /* INCLUDE_RUNTIME_H */
