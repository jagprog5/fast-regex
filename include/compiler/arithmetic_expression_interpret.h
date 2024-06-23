#pragma once

#include "arithmetic_expression.h"

// begin to end points to an arithmetic expression that has been parsed by
// parse_arithmetic_expression, then validated with
// validate_arithmetic_expression. values indicates the value taken on by each
// symbol in the expression, with offset indicated by the same position in the
// allowed_symbols passed to tokenize_arithmetic_expression
uint_fast32_t interpret_arithmetic_expression(const arith_token* begin, const arith_token* end, const uint_fast32_t* values) {
  assert(begin < end); // empty not allowed. handled prior by validate_arithmetic_expression
  size_t input_size = end - begin;
  uint_fast32_t stack[input_size];
  uint_fast32_t* stack_top = stack;
  do {
    arith_token t = *begin;
    switch (t.type) {
      case ARITH_TOKEN_U32:
        *stack_top++ = t.value.u32;
        break;
      case ARITH_TOKEN_SYMBOL:
        *stack_top++ = values[t.value.symbol_value_lookup];
        break;
      default: {
        uint_fast32_t rhs = stack_top[-1];
        uint_fast32_t lhs = stack_top[-2];
        uint_fast32_t* result = stack_top - 2;
        stack_top -= 1;
        switch (t.type) {
          case ARITH_TOKEN_ADD:
            *result = lhs + rhs;
            break;
          case ARITH_TOKEN_SUB:
            *result = lhs - rhs;
            break;
          case ARITH_TOKEN_MUL:
            *result = lhs * rhs;
            break;
          case ARITH_TOKEN_DIV:
            *result = lhs / rhs;
            break;
          case ARITH_TOKEN_MOD:
            *result = lhs % rhs;
            break;
          case ARITH_TOKEN_BITWISE_XOR:
            *result = lhs ^ rhs;
            break;
          case ARITH_TOKEN_BITWISE_COMPLEMENT:
            *result = ~rhs;
            break;
          case ARITH_TOKEN_EQUAL:
            *result = lhs == rhs;
            break;
          case ARITH_TOKEN_NOT_EQUAL:
            *result = lhs != rhs;
            break;
          case ARITH_TOKEN_LESS_THAN:
            *result = lhs < rhs;
            break;
          case ARITH_TOKEN_LESS_THAN_EQUAL:
            *result = lhs <= rhs;
            break;
          case ARITH_TOKEN_LEFT_SHIFT:
            *result = lhs << rhs;
            break;
          case ARITH_TOKEN_GREATER_THAN:
            *result = lhs > rhs;
            break;
          case ARITH_TOKEN_GREATER_THAN_EQUAL:
            *result = lhs >= rhs;
            break;
          case ARITH_TOKEN_RIGHT_SHIFT:
            *result = lhs >> rhs;
            break;
          case ARITH_TOKEN_BITWISE_AND:
            *result = lhs & rhs;
            break;
          default:
          case ARITH_TOKEN_BITWISE_OR:
            assert(t.type == ARITH_TOKEN_BITWISE_OR);
            *result = lhs | rhs;
            break;
        }
      } break;
    }
    ++begin;
  } while (begin != end);
  return stack_top[-1];
}