#include "character/code_unit.h"

#include "test_common.h"
extern int has_errors;

int main(void) {
  { // empty both
    const CODE_UNIT* haystack = CODE_UNIT_LITERAL("");
    size_t haystack_len = code_unit_strlen(haystack);
    const CODE_UNIT* needle = CODE_UNIT_LITERAL("");
    size_t needle_len = code_unit_strlen(needle);
    assert_continue(code_unit_memmem(haystack, haystack_len, needle, needle_len) == haystack);
  }
  { // empty needle
    const CODE_UNIT* haystack = CODE_UNIT_LITERAL("123");
    size_t haystack_len = code_unit_strlen(haystack);
    const CODE_UNIT* needle = CODE_UNIT_LITERAL("");
    size_t needle_len = code_unit_strlen(needle);
    assert_continue(code_unit_memmem(haystack, haystack_len, needle, needle_len) == haystack);
  }
  { // empty haystack
    const CODE_UNIT* needle = CODE_UNIT_LITERAL("123");
    size_t needle_len = code_unit_strlen(needle);
    assert_continue(code_unit_memmem(NULL, 0, needle, needle_len) == NULL);
  }
  { // needle == haystack
    const CODE_UNIT* haystack = CODE_UNIT_LITERAL("123");
    size_t haystack_len = code_unit_strlen(haystack);
    const CODE_UNIT* needle = CODE_UNIT_LITERAL("123");
    size_t needle_len = code_unit_strlen(needle);
    assert_continue(code_unit_memmem(haystack, haystack_len, needle, needle_len) == haystack);
  }
  { // nominal case
    const CODE_UNIT* haystack = CODE_UNIT_LITERAL("abc123abc");
    size_t haystack_len = code_unit_strlen(haystack);
    const CODE_UNIT* needle = CODE_UNIT_LITERAL("123");
    size_t needle_len = code_unit_strlen(needle);
    assert_continue(code_unit_memmem(haystack, haystack_len, needle, needle_len) == haystack + 3);
  }
  { // needle bigger than haystack
    const CODE_UNIT* haystack = CODE_UNIT_LITERAL("123");
    size_t haystack_len = code_unit_strlen(haystack);
    const CODE_UNIT* needle = CODE_UNIT_LITERAL("abc123abc");
    size_t needle_len = code_unit_strlen(needle);
    assert_continue(code_unit_memmem(haystack, haystack_len, needle, needle_len) == NULL);
  }
  { // needle at end of haystack
    const CODE_UNIT* haystack = CODE_UNIT_LITERAL("abc123");
    size_t haystack_len = code_unit_strlen(haystack);
    const CODE_UNIT* needle = CODE_UNIT_LITERAL("123");
    size_t needle_len = code_unit_strlen(needle);
    assert_continue(code_unit_memmem(haystack, haystack_len, needle, needle_len) == haystack + 3);
  }
  { // needle at begin of haystack
    const CODE_UNIT* haystack = CODE_UNIT_LITERAL("123abc");
    size_t haystack_len = code_unit_strlen(haystack);
    const CODE_UNIT* needle = CODE_UNIT_LITERAL("123");
    size_t needle_len = code_unit_strlen(needle);
    assert_continue(code_unit_memmem(haystack, haystack_len, needle, needle_len) == haystack);
  }

  // ====================================

  { // nominal
    const CODE_UNIT* haystack = CODE_UNIT_LITERAL("text that gets trunc");
    size_t haystack_len = code_unit_strlen(haystack);
    const CODE_UNIT* needle = CODE_UNIT_LITERAL("truncated");
    size_t needle_len = code_unit_strlen(needle);
    assert_continue(code_unit_incomplete_suffix(haystack, haystack_len, needle, needle_len) == haystack + 15);
  }

  { // intentional giving full needle at end, just to see edge case
    // result points to "aa" not "aaa"
    const CODE_UNIT* haystack = CODE_UNIT_LITERAL("123aaa");
    size_t haystack_len = code_unit_strlen(haystack);
    const CODE_UNIT* needle = CODE_UNIT_LITERAL("aaa");
    size_t needle_len = code_unit_strlen(needle);
    assert_continue(code_unit_incomplete_suffix(haystack, haystack_len, needle, needle_len) == haystack + 4);
  }

  { // empty haystack
    // this should never happen in the context in which this is used.
    // the match buffer will never be empty.
    // (testing anyways)
    const CODE_UNIT* haystack = CODE_UNIT_LITERAL("");
    size_t haystack_len = code_unit_strlen(haystack);
    const CODE_UNIT* needle = CODE_UNIT_LITERAL("123");
    size_t needle_len = code_unit_strlen(needle);
    assert_continue(code_unit_incomplete_suffix(haystack, haystack_len, needle, needle_len) == NULL);
  }
  { // empty needle
    // this should never happen in the context in which this is used.
    // memmem is used to check for a instance of needle in haystack, and only
    // after that fails, it a incomplete suffix checked for, for a match that is at a segment boundary.
    // since memmem matches on zero element comparison, this function is never reached with a needle of 0
    // (testing anyways)
    const CODE_UNIT* haystack = CODE_UNIT_LITERAL("123");
    size_t haystack_len = code_unit_strlen(haystack);
    const CODE_UNIT* needle = CODE_UNIT_LITERAL("");
    size_t needle_len = code_unit_strlen(needle);
    assert_continue(code_unit_incomplete_suffix(haystack, haystack_len, needle, needle_len) == NULL);
  }
  { // empty both
    // should never happen (testing anyways)
    const CODE_UNIT* haystack = CODE_UNIT_LITERAL("");
    size_t haystack_len = code_unit_strlen(haystack);
    const CODE_UNIT* needle = CODE_UNIT_LITERAL("");
    size_t needle_len = code_unit_strlen(needle);
    assert_continue(code_unit_incomplete_suffix(haystack, haystack_len, needle, needle_len) == NULL);
  }
  { // needle larger than haystack
    const CODE_UNIT* haystack = CODE_UNIT_LITERAL("123aaa");
    size_t haystack_len = code_unit_strlen(haystack);
    const CODE_UNIT* needle = CODE_UNIT_LITERAL("aaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
    size_t needle_len = code_unit_strlen(needle);
    assert_continue(code_unit_incomplete_suffix(haystack, haystack_len, needle, needle_len) == haystack + 3);
  }
  return has_errors;
}