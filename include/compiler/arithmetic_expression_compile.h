#pragma once

#include "arithmetic_expression.h"

// TODO

typedef enum {
  ARITH_TOKEN_COMPILE_ARG_GET_CAPACITY,
  ARITH_TOKEN_COMPILE_ARG_FILL_CAPACITY,
} arith_token_compile_result_type;

typedef union {
  size_t* capacity; // ARITH_TOKEN_COMPILE_ARG_GET_CAPACITY
  char* program; // ARITH_TOKEN_COMPILE_ARG_FILL_CAPACITY
} arith_token_compile_result_value;

typedef struct {
  arith_token_compile_result_type type;
  arith_token_compile_result_value value;
} arith_token_compile_result;

static void send_output(arith_token_compile_result* dst, const char* begin, const char* end) {
  assert(begin <= end);
  switch (dst->type) {
    case ARITH_TOKEN_COMPILE_ARG_GET_CAPACITY:
      (*dst->value.capacity) += end - begin;
      break;
    case ARITH_TOKEN_COMPILE_ARG_FILL_CAPACITY:
    default:
      assert(dst->type == ARITH_TOKEN_COMPILE_ARG_FILL_CAPACITY);
      while (begin != end) {
        *dst->value.program++ = *begin++;
      }
      break;
  }
}

static void send_cstr_output(arith_token_compile_result* dst, const char* c) {
  send_output(dst, c, c + strlen(c));
}

// generate c code implementing arithmetic expression. the written code is in a single
// block. it relies on the following:
//  - uf32_t is defined to uint_fast32_t
//  - the variable "arith_in" with type (const uf32_t*) is in scope.
//    it's analogous to interpret_arithmetic_expression's arg.
//  - the variable "arith_out" with type (uf32_t) is in scope
// begin to end points to an arithmetic expression that has been parsed by
// parse_arithmetic_expression, then validated with validate_arithmetic_expression.
static void generate_block_arithmetic_expression(const arith_token* begin, const arith_token* end, arith_token_compile_result arg) {
  assert(begin < end); // empty not allowed. handled prior by validate_arithmetic_expression
  send_cstr_output(&arg, "{"); // open block
  do {
    arith_token token = *begin;
    switch (token.type) {
        
    }
    ++begin;
  } while (begin != end);
  send_cstr_output(&arg, "}"); // close block
}