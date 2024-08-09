#pragma once

#include "compiler/expression/expression.h"
#include "character/subject_buffer.h"

#include <stdlib.h> // qsort

// ========================= function definition ===============================

// result of presetup.
// on success, yields the size of data needed for the setup phase
// on failure, gives offending location and reason
typedef struct {
  bool success;
  union {
    size_t data_size_bytes; // success
    struct { // failure
      size_t offset; // offending location relative to beginning of expression
      const char* reason;
    } err;
  } value;
} function_presetup_result;

// result of setup.
// on success, yields information about the function
// on failure, gives offending location and reason
typedef struct {
  bool success;
  union {
    struct { // success
      size_t max_size_characters;
      size_t max_lookbehind_characters;
    } ok;
    struct { // failure
      size_t offset; // offending location relative to beginning of expression
      const char* reason;
    } err;
  } value;
} function_setup_result;

typedef enum { MATCH_SUCCESS, MATCH_FAILURE, MATCH_INCOMPLETE } match_status;

// associates a function name with it's behaviour
typedef struct {
  expr_token_string_range name;

  // presetup must validate arguments and indicate the allocation size needed for the next phase
  // it must also move function_start to the next function
  function_presetup_result (*presetup)(size_t num_args, const expr_token** function_start);

  // data points to a number of bytes indicates by the presetup
  // this phase must do parsing and overall setup
  function_setup_result (*setup)(size_t num_args, const expr_token* function_start, void* data, size_t data_size_bytes);

  // this function is called at runtime during pattern matching, in a context in
  // which there can't be an incomplete match because there's enough content
  // left in the match buffer (bound check is not required).
  // returning true indicates a successful match, false otherwise
  // it must also move the subject buffer's offset forward past the matched content
  bool (*guaranteed_length_interpret)(subject_buffer_state* buffer, void* data, size_t data_size_bytes);

  // this function is called at runtime during pattern matching, in a context in
  // which there might be an incomplete match because there's not enough content left in the match buffer.
  // it must also move the subject buffer's offset forward past the matched content
  match_status (*interpret)(subject_buffer_state* buffer, void* data, size_t data_size_bytes);
} function_definition;

// ===================== definition for literal function (built in) ============

static function_presetup_result function_definition_for_literal_presetup(size_t num_args, const expr_token** function_start) {
  // literal function is special because it doesn't need to validate its arguments.
  // this is caused from the overall logic in interpret_presetup
  assert(num_args == 1);
  assert((*function_start)->type == EXPR_TOKEN_LITERAL);
  function_presetup_result ret;
  ret.success = true;
  ret.value.data_size_bytes = sizeof(CODE_UNIT);
  (*function_start)++; // move to end of args
  return ret;
}

static function_setup_result function_definition_for_literal_setup(size_t num_args, const expr_token* function_start, void* data, size_t data_size_bytes) {
  assert(num_args == 1);
  assert(function_start->type == EXPR_TOKEN_LITERAL);
  assert(data_size_bytes == sizeof(CODE_UNIT));
  function_setup_result ret;
  ret.success = true;
  ret.value.ok.max_lookbehind_characters = 0;
  ret.value.ok.max_size_characters = 1;
  *(CODE_UNIT*)data = function_start->data.literal;
  return ret;
}

static bool function_definition_for_literal_guaranteed_length_interpret(subject_buffer_state* buffer, void* data, size_t data_size_bytes) {
  assert(data_size_bytes == sizeof(CODE_UNIT));
  assert(subject_buffer_remaining_size(buffer) >= 1);
  return subject_buffer_start(buffer)[buffer->offset++] == *(CODE_UNIT*)data;
}

static match_status function_definition_for_literal_interpret(subject_buffer_state* buffer, void* data, size_t data_size_bytes) {
  size_t characters_remaining = subject_buffer_remaining_size(buffer);
  if (characters_remaining < 1) {
    return MATCH_INCOMPLETE;
  }
  bool success = function_definition_for_literal_guaranteed_length_interpret(buffer, data, data_size_bytes);
  return success ? MATCH_SUCCESS : MATCH_FAILURE;
}

// pointer to static lifetime
static const function_definition* function_definition_for_literal() {
  static function_definition ret = {{NULL, NULL}, //
                                    function_definition_for_literal_presetup,
                                    function_definition_for_literal_setup,
                                    function_definition_for_literal_guaranteed_length_interpret,
                                    function_definition_for_literal_interpret};
  return &ret;
}

// =============== helper functions (quick function name lookup) ===============

static int function_definition_sort_comp(const void* lhs_arg, const void* rhs_arg) {
  const function_definition* lhs = (const function_definition*)lhs_arg;
  const function_definition* rhs = (const function_definition*)rhs_arg;
  return code_unit_range_cmp(lhs->name.begin, lhs->name.end, rhs->name.begin, rhs->name.end);
}

// binary search to find function with name, NULL if not found
static const function_definition* function_definition_lookup(expr_token_string_range name, size_t num_function, const function_definition* functions) {
  size_t left = 0;
  size_t right = num_function;
  while (left < right) {
    size_t mid = left + (right - left) / 2;
    int cmp = code_unit_range_cmp(name.begin, name.end, functions[mid].name.begin, functions[mid].name.end);
    if (cmp < 0) {
      right = mid;
    } else if (cmp > 0) {
      left = mid + 1;
    } else {
      return &functions[mid];
    }
  }
  return NULL;
}

// sort functions before use in interpret_presetup
void function_definition_sort(size_t num_function, function_definition* functions) {
  qsort(functions, num_function, sizeof(*functions), function_definition_sort_comp);
}
