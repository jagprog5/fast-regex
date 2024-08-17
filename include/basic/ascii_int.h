#pragma once

#include <assert.h>
#include <limits.h>
#include <stdint.h>

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
bool append_ascii_to_int(int* value, char ch) {
  assert(ch >= '0' && ch <= '9');

  { // checked multiply for signed type
    if (*value > (INT_MAX / 10)) {
      return false;
    }

    *value *= 10;
  }

  { // checked add for signed type
    if (*value > (INT_MAX - (ch - '0'))) {
      return false;
    }
    *value += (ch - '0');
  }

  return true;
}