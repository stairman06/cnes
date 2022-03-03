#include "util.h"
#include <stdbool.h>
#include <stdint.h>

void set_flag(uint8_t* val, uint8_t flag, bool condition) {
  if (condition) {
    *val |= flag;
  } else {
    *val &= ~flag;
  }
}
