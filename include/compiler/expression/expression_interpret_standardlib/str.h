#pragma once

#include "compiler/expression/expression_interpret.h"

function_presetup_result function_definition_for_str_presetup(const expr_token** function_start, function_setup_info** presetup_info) {
  function_presetup_result ret;
  ret.success = true;

  // points to function start
  assert((*function_start)->type == EXPR_TOKEN_FUNCTION);
  size_t num_args = (*function_start)->value.function.num_args;
  if (unlikely(num_args != 1)) {
    ret.success = false;
    ret.value.err.offset = (*function_start)->offset;
    ret.value.err.reason = "needs one arg";
    return ret;
  }
  (*presetup_info)->num_args = 1;
  (*presetup_info)->function_start = (*function_start)++;

  //                  V
  // FUNCTION(1 arg), literal, literal, ..., literal, endarg
  const expr_token* arg_begin = *function_start;
  const expr_token* arg_end = arg_begin;

  // move arg_end forward until it hits the end of the arg
  while (1) {
    if (unlikely(arg_end->type == EXPR_TOKEN_FUNCTION)) {
      ret.success = false;
      ret.value.err.offset = arg_end->offset;
      ret.value.err.reason = "this function doesn't support nesting";
      return ret;
    }

    if (arg_end->type == EXPR_TOKEN_ENDARG) {
      break;
    }

    arg_end += 1;
  }

  ret.value.data_size = (arg_end - arg_begin) * sizeof(CODE_UNIT);
  (*presetup_info)->function_data_size = ret.value.data_size;
  (*presetup_info)++;
  (*function_start) = arg_end + 1;
  return ret;
}

static function_setup_result function_definition_for_str_setup(const expr_token** function_start, const function_setup_info** presetup_info, void* data, size_t data_size) {
  function_setup_result ret;
  ret.success = true;
  assert(data_size % sizeof(CODE_UNIT) == 0);
  ret.value.ok.max_size_characters = data_size / sizeof(CODE_UNIT);
  ret.value.ok.max_lookbehind_characters = 0;

  assert((*function_start)->type == EXPR_TOKEN_FUNCTION);
  (*function_start)++;
  for (size_t i = 0; i < ret.value.ok.max_size_characters; ++i) {
    assert((*function_start)->type == EXPR_TOKEN_LITERAL);
    ((CODE_UNIT*)data)[i] = (*function_start)++->value.literal;
  }
  assert((*function_start)->type == EXPR_TOKEN_ENDARG);
  (*function_start)++;
  (*presetup_info)++;
  return ret;
}

static bool function_definition_for_str_guaranteed_length_interpret(subject_buffer_state* buffer, const void* data, size_t data_size) {
  const CODE_UNIT* needle = data;
  size_t needle_len = data_size / sizeof(CODE_UNIT); // number of elements
  assert(subject_buffer_remaining_size(buffer) >= needle_len);
  bool ret = code_unit_memcmp(subject_buffer_begin(buffer) + buffer->offset, needle, needle_len);
  buffer->offset += needle_len;
  return ret;
}

static match_status function_definition_for_str_interpret(subject_buffer_state* buffer, const void* data, size_t data_size) {
  size_t characters_remaining = subject_buffer_remaining_size(buffer);
  size_t needle_len = data_size / sizeof(CODE_UNIT); // number of elements
  if (characters_remaining < needle_len) {
    return MATCH_INCOMPLETE;
  }
  bool success = function_definition_for_str_guaranteed_length_interpret(buffer, data, data_size);
  return success ? MATCH_SUCCESS : MATCH_FAILURE;
}

static match_status function_definition_for_str_entrypoint_interpret(subject_buffer_state* buffer, const void* data, size_t data_size, const CODE_UNIT** success_begin_out) {
  const CODE_UNIT* haystack = subject_buffer_begin(buffer) + buffer->offset;
  size_t haystack_len = subject_buffer_remaining_size(buffer); // number of elements
  const CODE_UNIT* needle = data;
  size_t needle_len = data_size / sizeof(CODE_UNIT); // number of elements
  const CODE_UNIT* result = code_unit_memmem(haystack, haystack_len, needle, needle_len);
  if (result != NULL) {
    *success_begin_out = result;
    buffer->offset = (result - subject_buffer_begin(buffer)) + needle_len;
    return MATCH_SUCCESS;
  }

  result = code_unit_incomplete_suffix(haystack, haystack_len, needle, needle_len);
  if (result == NULL) {
    buffer->offset = subject_buffer_size(buffer);
    return MATCH_FAILURE;
  } else {
    buffer->offset = (result - subject_buffer_begin(buffer));
    return MATCH_INCOMPLETE;
  }
}

// ptr to static lifetime
const function_definition* function_definition_for_str() {
  static const CODE_UNIT s[] = {'s', 't', 'r'};
  static function_definition ret = {{s, s + sizeof(s) / sizeof(*s)}, //
                                    function_definition_for_str_presetup,
                                    function_definition_for_str_setup,
                                    function_definition_for_str_interpret,
                                    function_definition_for_str_guaranteed_length_interpret,
                                    function_definition_for_str_entrypoint_interpret};
  return &ret;
}