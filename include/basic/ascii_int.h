#pragma once

#include "assert.h"
#include "stdint.h"

// false on overflow, true otherwise
bool append_ascii_to_uint32(uint32_t* value, char ch) {
  assert(ch >= '0' && ch <= '9');

  { // checked multiply
    uint32_t next_value = 10 * *value;
    if (next_value / 10 != *value) {
      return false;
    }
    *value = next_value;
  }

  { // checked add
    uint32_t next_value = *value + (ch - '0');
    if (next_value < *value) {
      return false;
    }
    *value = next_value;
  }
  return true;
}

// false on overflow. true otherwise
//  - implementation limit of 10000 as largest value (good enough in context this is used)
//  - only handles digits (no sign)
bool append_ascii_to_int(int* value, char ch) {
    assert(ch >= '0' && ch <= '9');
    *value *= 10;
    *value += (ch - '0');
    if (*value > 10000) {
        return false;
    }
    return true;
}