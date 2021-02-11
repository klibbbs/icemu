#ifndef INCLUDE_DEBUG_H
#define INCLUDE_DEBUG_H

#include <stdio.h>

#ifdef DEBUG
#define debug(args) printf args
#else
#define debug(args)
#endif

#endif /* INCLUDE_DEBUG_H */
