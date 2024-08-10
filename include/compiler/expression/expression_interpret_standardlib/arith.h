#pragma once

#include "compiler/arithmetic_expression/arithmetic_expression_interpret.h"
#include "compiler/expression/expression_interpret.h"


// symbol in arithmetic expression which represents the input character
// ptr to static lifetime
static const arith_expr_allowed_symbols* arith_function_allowed_symbols() {
  static const CODE_UNIT c[] = {'c'};
  static arith_expr_symbol allowed_symbol = {c, c + sizeof(c) / sizeof(*c)};
  static arith_expr_allowed_symbols ret = {&allowed_symbol, 1};
  return &ret;
}

// private
typedef struct {
  // expr points to following token range
  arith_expr expr;
  arith_parsed tokens[];
} function_definition_arith_data;

function_presetup_result function_definition_for_arith_presetup(const expr_token** function_start, function_setup_info** presetup_info) {
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

  size_t expr_size = arg_end - arg_begin;
  CODE_UNIT arith_expression_text[expr_size];
  CODE_UNIT* arith_expression_text_ptr = arith_expression_text;
  while (arg_begin != arg_end) {
    *arith_expression_text_ptr++ = arg_begin++->value.literal;
  }
  
  arith_tokenize_capacity cap = tokenize_arithmetic_expression(arith_expression_text, arith_expression_text + expr_size, NULL, arith_function_allowed_symbols());
  if (unlikely(cap.type == ARITH_TOKENIZE_CAPACITY_ERROR)) {
    ret.success = false;
    // translate the offset:
    //  - from relative to the arithmetic expression
    //  - to the overall expression
    ret.value.err.offset = cap.value.err.offset + (*function_start)->offset;
    ret.value.err.reason = cap.value.err.reason;
    return ret;
  }
  
  ret.value.data_size_bytes = sizeof(function_definition_arith_data) + cap.value.capacity * sizeof(arith_parsed);
  (*presetup_info)->function_data_size = ret.value.data_size_bytes;
  (*presetup_info)++;
  (*function_start) = arg_end + 1;
  return ret;
}

static function_setup_result function_definition_for_arith_setup(const expr_token** function_start, const function_setup_info** presetup_info, void* data, size_t data_size_bytes) {
  function_setup_result ret;
  ret.success = true;
  ret.value.ok.max_size_characters = 1;
  ret.value.ok.max_lookbehind_characters = 0;

  (*function_start)++;
  const expr_token* arg_begin = *function_start;
  const expr_token* arg_end = arg_begin;

  while (1) {
    assert(arg_end->type != EXPR_TOKEN_FUNCTION);
    if (arg_end->type == EXPR_TOKEN_ENDARG) {
      break;
    }
    arg_end += 1;
  }

  size_t expr_size = arg_end - arg_begin;
  CODE_UNIT arith_expression_text[expr_size];
  CODE_UNIT* arith_expression_text_ptr = arith_expression_text;
  while (arg_begin != arg_end) {
    *arith_expression_text_ptr++ = arg_begin++->value.literal;
  }

  size_t num_tokens = data_size_bytes - sizeof(function_definition_arith_data);
  assert(num_tokens % sizeof(arith_parsed) == 0);
  num_tokens /= sizeof(arith_parsed);

  arith_token tokens[num_tokens];

  arith_tokenize_capacity cap = tokenize_arithmetic_expression(arith_expression_text, arith_expression_text + expr_size, tokens, arith_function_allowed_symbols());
  assert(cap.type == ARITH_TOKENIZE_FILL_ARRAY);

  arith_expr_result expr_result = parse_arithmetic_expression(tokens, tokens + num_tokens, ((function_definition_arith_data*)data)->tokens);
  if (unlikely(expr_result.type == ARITH_EXPR_ERROR)) {
    ret.success = false;
    ret.value.err.offset = expr_result.value.err.offset + (*function_start)->offset;
    ret.value.err.reason = expr_result.value.err.reason;
    return ret;
  }
  ((function_definition_arith_data*)data)->expr = expr_result.value.expr;
  (*presetup_info)++;
  return ret;
}

static bool function_definition_for_arith_guaranteed_length_interpret(subject_buffer_state* buffer, const void* data, size_t data_size_bytes) {
  assert(subject_buffer_remaining_size(buffer) >= 1);
  const function_definition_arith_data* expr = (const function_definition_arith_data*)data;
  uint_fast32_t character = subject_buffer_start(buffer)[buffer->offset++];
  return interpret_arithmetic_expression(expr->expr, &character);
}

static match_status function_definition_for_arith_interpret(subject_buffer_state* buffer, const void* data, size_t data_size_bytes) {
  size_t characters_remaining = subject_buffer_remaining_size(buffer);
  if (characters_remaining < 1) {
    return MATCH_INCOMPLETE;
  }
  bool success = function_definition_for_literal_guaranteed_length_interpret(buffer, data, data_size_bytes);
  return success ? MATCH_SUCCESS : MATCH_FAILURE;
}

// ptr to static lifetime
const function_definition* function_definition_for_arith() {
  static const CODE_UNIT arith[] = {'a', 'r', 'i', 't', 'h'};
  static function_definition ret = {{arith, arith + sizeof(arith) / sizeof(*arith)}, //
                                    function_definition_for_arith_presetup,
                                    function_definition_for_arith_setup,
                                    function_definition_for_arith_guaranteed_length_interpret,
                                    function_definition_for_arith_interpret};
  return &ret;
}