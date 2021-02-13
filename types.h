#ifndef INCLUDE_TYPES_H
#define INCLUDE_TYPES_H

typedef enum {
  false,
  true
} bool;

typedef enum {
  BIT_ZERO = 0,
  BIT_ONE  = 1,
  BIT_Z    = -1,
  BIT_META = -2
} bit_t;

#endif /* INCLUDE_TYPES_H */
