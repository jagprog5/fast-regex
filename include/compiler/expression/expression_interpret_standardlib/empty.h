#pragma once
#include "compiler/expression/expression_interpret.h"

static function_presetup_result function_definition_for_empty_presetup(const expr_token** function_start, function_setup_info** presetup_info) {
  function_presetup_result ret;
  ret.success = true;

  // points to function start
  assert((*function_start)->type == EXPR_TOKEN_FUNCTION);
  // no way of typing this
  assert((*function_start)->value.function.end_marker.present == false);

  if (unlikely((*function_start)->value.function.begin_marker.present == false)) {
    ret.success = false;
    ret.value.err.offset = (*function_start)->offset;
    ret.value.err.reason = "function with empty name should have marker";
    return ret;
  }

  if (unlikely((*function_start)->value.function.num_args != 0)) {
    ret.success = false;
    ret.value.err.offset = (*function_start)->offset;
    ret.value.err.reason = "function with empty name should not have arguments";
    return ret;
  }

  ret.value.data_size = 0;
  (*presetup_info)->function_data_size = 0;
  (*presetup_info)->num_args = 0;
  (*presetup_info)->function_start = *function_start;
  (*presetup_info)++;
  (*function_start)++;
  return ret;
}

static function_setup_result function_definition_for_empty_setup(const expr_token** function_start, const function_setup_info** presetup_info, void*, size_t) {
  function_setup_result ret;
  ret.success = true;
  ret.value.ok.max_lookbehind_characters = 0;
  ret.value.ok.max_size_characters = 0;
  (*presetup_info)++;
  (*function_start)++;
  return ret;
}

static bool function_definition_for_empty_guaranteed_length_interpret(subject_buffer_state*, const void*, size_t) {
  return true;
}

static match_status function_definition_for_empty_interpret(subject_buffer_state*, const void*, size_t) {
  return MATCH_SUCCESS;
}

// ptr to static lifetime
// an empty function name should be used for markers
// {0}hello{1}
const function_definition* function_definition_for_empty() {
  static function_definition ret = {{NULL, NULL}, // empty string
                                    function_definition_for_empty_presetup,
                                    function_definition_for_empty_setup,
                                    function_definition_for_empty_interpret,
                                    function_definition_for_empty_guaranteed_length_interpret,
                                    NULL};
  return &ret;
}
