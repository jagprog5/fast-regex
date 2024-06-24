#pragma once

#include "arithmetic_expression.h"

// values indicates the value taken on by each symbol in the expression, with
// offset indicated by the same position in the allowed_symbols passed to
// tokenize_arithmetic_expression
uint_fast32_t interpret_arithmetic_expression(arith_expr expr, const uint_fast32_t* values) {
  assert(expr.begin < expr.end); // empty not allowed. case caught during parsing
  uint_fast32_t stack[expr.stack_required];
  uint_fast32_t* stack_top = stack;
  do {
    arith_parsed t = *expr.begin;
    switch (t.type) {
      case ARITH_U32:
        *stack_top++ = t.value.u32;
        break;
      case ARITH_SYMBOL:
        *stack_top++ = values[t.value.symbol_value_lookup];
        break;
      default: {
        uint_fast32_t rhs = stack_top[-1];
        uint_fast32_t lhs = stack_top[-2];
        uint_fast32_t* result = stack_top - 2;
        stack_top -= 1;
        switch (t.type) {
          case ARITH_ADD:
            *result = lhs + rhs;
            break;
          case ARITH_SUB:
            *result = lhs - rhs;
            break;
          case ARITH_MUL:
            *result = lhs * rhs;
            break;
          case ARITH_DIV:
            *result = lhs / rhs;
            break;
          case ARITH_MOD:
            *result = lhs % rhs;
            break;
          case ARITH_BITWISE_XOR:
            *result = lhs ^ rhs;
            break;
          case ARITH_BITWISE_COMPLEMENT:
            *result = ~rhs;
            break;
          case ARITH_EQUAL:
            *result = lhs == rhs;
            break;
          case ARITH_NOT_EQUAL:
            *result = lhs != rhs;
            break;
          case ARITH_LESS_THAN:
            *result = lhs < rhs;
            break;
          case ARITH_LESS_THAN_EQUAL:
            *result = lhs <= rhs;
            break;
          case ARITH_LEFT_SHIFT:
            *result = lhs << rhs;
            break;
          case ARITH_GREATER_THAN:
            *result = lhs > rhs;
            break;
          case ARITH_GREATER_THAN_EQUAL:
            *result = lhs >= rhs;
            break;
          case ARITH_RIGHT_SHIFT:
            *result = lhs >> rhs;
            break;
          case ARITH_BITWISE_AND:
            *result = lhs & rhs;
            break;
          default:
          case ARITH_BITWISE_OR:
            assert(t.type == ARITH_BITWISE_OR);
            *result = lhs | rhs;
            break;
        }
      } break;
    }
    ++expr.begin;
  } while (expr.begin != expr.end);
  return stack_top[-1];
}
