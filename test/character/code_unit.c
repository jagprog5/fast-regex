#include "character/code_unit.h"

#include "test_common.h"
extern int has_errors;

int main(void) {
  { // nominal
    const char* haystack = "text that gets trunc";
    size_t haystack_len = code_unit_strlen(haystack);
    const char* needle = "truncated";
    size_t needle_len = code_unit_strlen(needle);
    assert_continue(mem_incomplete_suffix(haystack, haystack_len, needle, needle_len) == haystack + 15);
  }

  { // intentional giving full needle at end, just to see edge case
    // result points to "aa" not "aaa"
    const char* haystack = "123aaa";
    size_t haystack_len = code_unit_strlen(haystack);
    const char* needle = "aaa";
    size_t needle_len = code_unit_strlen(needle);
    assert_continue(mem_incomplete_suffix(haystack, haystack_len, needle, needle_len) == haystack + 4);
  }

  { // empty haystack
    // this should never happen in the context in which this is used.
    // the match buffer will never be empty.
    // (testing anyways)
    const char* haystack = "";
    size_t haystack_len = code_unit_strlen(haystack);
    const char* needle = "123";
    size_t needle_len = code_unit_strlen(needle);
    assert_continue(mem_incomplete_suffix(haystack, haystack_len, needle, needle_len) == NULL);
  }
  { // empty needle
    // this should never happen in the context in which this is used.
    // memmem is used to check for a instance of needle in haystack, and only
    // after that fails, it a incomplete suffix checked for, for a match that is at a segment boundary.
    // since memmem matches on zero element comparison, this function is never reached with a needle of 0
    // (testing anyways)
    const char* haystack = "123";
    size_t haystack_len = code_unit_strlen(haystack);
    const char* needle = "";
    size_t needle_len = code_unit_strlen(needle);
    assert_continue(mem_incomplete_suffix(haystack, haystack_len, needle, needle_len) == NULL);
  }
  { // empty both
    // should never happen (testing anyways)
    const char* haystack = "";
    size_t haystack_len = code_unit_strlen(haystack);
    const char* needle = "";
    size_t needle_len = code_unit_strlen(needle);
    assert_continue(mem_incomplete_suffix(haystack, haystack_len, needle, needle_len) == NULL);
  }
  { // needle larger than haystack
    const char* haystack = "123aaa";
    size_t haystack_len = code_unit_strlen(haystack);
    const char* needle = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    size_t needle_len = code_unit_strlen(needle);
    assert_continue(mem_incomplete_suffix(haystack, haystack_len, needle, needle_len) == haystack + 3);
  }
  return has_errors;
}