#include "transistor.h"

#include "node.h"

bool transistor_is_open(const transistor_t * transistor, const node_t * gate) {
  switch (transistor->type) {
    case TRANSISTOR_NMOS:
      return gate->state == BIT_ONE;
  }

  return false;
}
