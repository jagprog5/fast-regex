#pragma once

#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include "basic/likely_unlikely.h"

// this lib is intended to work with either char or wchar_t.

#ifdef USE_WCHAR
#include <wchar.h>
typedef wchar_t CODE_UNIT;

#define CODE_UNIT_LITERAL(str) L##str

size_t code_unit_strlen(const CODE_UNIT* s) {
  return wcslen(s);
}

int code_unit_memcmp(const CODE_UNIT* str1, const CODE_UNIT* str2, size_t n) {
  return wmemcmp(str1, str2, n);
}

int code_unit_strcmp(const CODE_UNIT* str1, const CODE_UNIT* str2) {
  return wcscmp(str1, str2);
}

const CODE_UNIT* code_unit_memmem(const CODE_UNIT* haystack, size_t haystack_len, const CODE_UNIT* needle, size_t needle_len) {
  // It's surprising that this function doesn't exist in the standard lib.

  if (unlikely(haystack_len < needle_len)) {
    return NULL;
  }

  const CODE_UNIT* last_position = haystack + haystack_len - needle_len;

  while(1) {
    if (wmemcmp(haystack, needle, needle_len) == 0) {
      return haystack;
    }
    ++haystack;
    if (haystack > last_position) return NULL;
  }
}

const CODE_UNIT* code_unit_memchr(const CODE_UNIT *ptr, CODE_UNIT value, size_t num) {
  return (const CODE_UNIT*)wmemchr((const void*)ptr, value, num);
}

#else
typedef char CODE_UNIT;

#define CODE_UNIT_LITERAL(str) str

size_t code_unit_strlen(const CODE_UNIT* s) {
  return strlen(s);
}

int code_unit_memcmp(const CODE_UNIT* str1, const CODE_UNIT* str2, size_t n) {
  return memcmp(str1, str2, n);
}

int code_unit_strcmp(const CODE_UNIT* str1, const CODE_UNIT* str2) {
  return strcmp(str1, str2);
}

const CODE_UNIT* code_unit_memmem(const CODE_UNIT* haystack, size_t haystacklen, const CODE_UNIT* needle, size_t needlelen) {
    return (const CODE_UNIT*)memmem((void*)haystack, haystacklen, (void*)needle, needlelen);
}

const CODE_UNIT* code_unit_memchr(const CODE_UNIT *ptr, CODE_UNIT value, size_t num) {
  return (const CODE_UNIT*)memchr((const void*)ptr, value, num);
}
#endif

// a comparison function suitable for sorting code unit ranges
int code_unit_range_cmp(const CODE_UNIT* begin1, const CODE_UNIT* end1, const CODE_UNIT* begin2, const CODE_UNIT* end2) {
  size_t len1 = end1 - begin1;
  size_t len2 = end2 - begin2;
  size_t len_diff = len1 - len2;
  if (len_diff != 0) return len_diff;
  return code_unit_memcmp(begin1, begin2, len1);
}

// compare range to cstring. returns zero if they match, else nonzero
int code_unit_range_equal_to_string(const CODE_UNIT* begin1, const CODE_UNIT* end1, const CODE_UNIT* begin2) {
  size_t len1 = end1 - begin1;
  size_t len2 = code_unit_strlen(begin2);
  size_t len_diff = len1 - len2;
  if (len_diff != 0) return len_diff;
  return code_unit_memcmp(begin1, begin2, len1);
}

// checks for haystack ending with incomplete needle
// returns pointer to beginning of largest possible incomplete needle, NULL otherwise
// undefined for needle or haystack of zero length (this implementation returns NULL)
const CODE_UNIT* code_unit_incomplete_suffix(const CODE_UNIT* haystack, size_t haystack_len, const CODE_UNIT* needle, size_t needle_len) {
  const CODE_UNIT* pos;
  size_t smallest_len;
  if (unlikely(haystack_len < needle_len)) {
    pos = haystack;
    smallest_len = haystack_len;
  } else {
    pos = (haystack + haystack_len) - needle_len;
    smallest_len = needle_len;
  }

  while (1) {
      if (smallest_len == 0) {
        return NULL;
      }
      pos += 1;
      smallest_len -= 1;
      if (0 == code_unit_memcmp(pos, needle, smallest_len)) {
        return pos;
      }
  }
}
