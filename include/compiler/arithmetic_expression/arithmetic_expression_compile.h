#pragma once

#include <stdio.h>
#include <inttypes.h>
#include "compiler/arithmetic_expression/arithmetic_expression.h"

typedef enum {
  ARITH_COMPILE_FILL_ARRAY = 0, // second pass
  ARITH_COMPILE_GET_CAPACITY,   // first pass
} arith_compile_capacity_type;

typedef union {
  size_t capacity; // ARITH_TOKENIZE_CAPACITY
} arith_compile_capacity_value;

typedef struct {
  arith_compile_capacity_type type;
  arith_compile_capacity_value value;
} arith_compile_capacity;

void generate_code_char(char** output, size_t* dst, char ch) {
  if (output == NULL) {
    *dst += 1;
  } else {
    *(*output)++ = ch;
  }
}

void generate_code_range(char** output, size_t* dst, const char* begin, const char* end) {
  assert(begin <= end);
  if (output == NULL) {
    *dst += end - begin;
  } else {
    while (begin != end) {
      *(*output)++ = *begin++;
    }
  }
}

void generate_code_cstr(char** output, size_t* dst, const char* c) {
  generate_code_range(output, dst, c, c + strlen(c));
}

// paranoid since this allocates on the stack. additionally, this give 0 output for 0 input (which is desired)
static size_t log10_manual_size_t(size_t n) {
  size_t log = 0;
  while (n >= 10) {
    n /= 10;
    log++;
  }
  return log;
}

static uint_fast32_t log10_manual_u32(uint_fast32_t n) {
  uint_fast32_t log = 0;
  while (n >= 10) {
    n /= 10;
    log++;
  }
  return log;
}

// get capacity needed to write n as ascii
static size_t ascii_size_size_t(size_t n) {
  return log10_manual_size_t(n) + 1;
}

// get capacity needed to write n as ascii
static size_t ascii_size_u32(size_t n) {
  return log10_manual_u32(n) + 1;
}

void generate_code_size_t(char** output, size_t* dst, size_t n) {
  size_t num_size = ascii_size_size_t(n);
  if (output == NULL) {
    *dst += num_size;
  } else {
    char vals[num_size + 1]; // +1 for null terminator
    snprintf(vals, sizeof(vals), "%zu", n);
    for (size_t i = 0; i < num_size; ++i) {
      *(*output)++ = vals[i];
    }
  }
}

void generate_code_uf32_t(char** output, size_t* dst, uint_fast32_t n) {
  uint_fast32_t num_size = ascii_size_u32(n);
  if (output == NULL) {
    *dst += num_size;
  } else {
    char vals[num_size + 1]; // +1 for null terminator
    snprintf(vals, sizeof(vals), "%" PRIuFAST32, n);
    for (uint_fast32_t i = 0; i < num_size; ++i) {
      *(*output)++ = vals[i];
    }
  }
}

// generate c code implementing arithmetic expression. the written code is in a single
// block. it relies on the following:
//  - uf32_t is defined to uint_fast32_t
//  - the variable "arith_in" with type (const uf32_t*) is in scope.
//    it's analogous to interpret_arithmetic_expression's arg.
//  - the variable "arith_out" with type (uf32_t) is in scope
// use of this function should be completed in two passes.
//  - the first pass gets the capacity required to store the program.
//    this is represented by passing NULL the output arg.
//    dst must point to something already initialized
//  - the second pass fills the allocation.
//    this is represented with a non NULL output arg
void generate_code_arithmetic_expression_block(char** output, size_t* dst, arith_expr expr) {
  assert(expr.begin < expr.end); // empty not allowed. handled prior by parse_arithmetic_expression

  generate_code_char(output, dst, '{'); // open block

  // write the stack variables
  generate_code_cstr(output, dst, "uf32_t ");
  for (size_t i = 0; i < expr.stack_required; ++i) {
    if (i != 0) {
      generate_code_char(output, dst, ',');
    }
    generate_code_char(output, dst, 's');
    generate_code_size_t(output, dst, i);
  }
  
  generate_code_char(output, dst, ';');

  size_t stack_position = 0;
  do {
    arith_parsed token = *expr.begin;
    switch (token.type) {
      case ARITH_U32:
        generate_code_char(output, dst, 's');
        generate_code_size_t(output, dst, stack_position);
        generate_code_char(output, dst, '=');
        generate_code_uf32_t(output, dst, token.value.u32);
        generate_code_char(output, dst, ';');
        stack_position += 1;
        break;
      case ARITH_SYMBOL:
        generate_code_char(output, dst, 's');
        generate_code_size_t(output, dst, stack_position);
        generate_code_char(output, dst, '=');
        generate_code_cstr(output, dst, "arith_in[");
        generate_code_size_t(output, dst, token.value.symbol_value_lookup);
        generate_code_char(output, dst, ']');
        generate_code_char(output, dst, ';');
        stack_position += 1;
        break;
      case ARITH_ADD:
      case ARITH_SUB:
      case ARITH_MUL:
      case ARITH_BITWISE_XOR:
      case ARITH_EQUAL:
      case ARITH_NOT_EQUAL:
      case ARITH_LESS_THAN:
      case ARITH_LESS_THAN_EQUAL:
      case ARITH_LEFT_SHIFT:
      case ARITH_GREATER_THAN:
      case ARITH_GREATER_THAN_EQUAL:
      case ARITH_RIGHT_SHIFT:
      case ARITH_BITWISE_AND:
      case ARITH_BITWISE_OR:
        generate_code_char(output, dst, 's');
        generate_code_size_t(output, dst, stack_position - 2);
        generate_code_char(output, dst, '=');
        generate_code_char(output, dst, 's');
        generate_code_size_t(output, dst, stack_position - 2);
        switch (token.type) {
          case ARITH_ADD:
            generate_code_char(output, dst, '+');
            break;
          case ARITH_SUB:
            generate_code_char(output, dst, '-');
            break;
          case ARITH_MUL:
            generate_code_char(output, dst, '*');
            break;
          case ARITH_BITWISE_XOR:
            generate_code_char(output, dst, '^');
            break;
          case ARITH_EQUAL:
            generate_code_cstr(output, dst, "==");
            break;
          case ARITH_NOT_EQUAL:
            generate_code_cstr(output, dst, "!=");
            break;
          case ARITH_LESS_THAN:
            generate_code_char(output, dst, '<');
            break;
          case ARITH_LESS_THAN_EQUAL:
            generate_code_cstr(output, dst, "<=");
            break;
          case ARITH_LEFT_SHIFT:
            generate_code_cstr(output, dst, "<<");
            break;
          case ARITH_GREATER_THAN:
            generate_code_cstr(output, dst, ">");
            break;
          case ARITH_GREATER_THAN_EQUAL:
            generate_code_cstr(output, dst, ">=");
            break;
          case ARITH_RIGHT_SHIFT:
            generate_code_cstr(output, dst, ">>");
            break;
          case ARITH_BITWISE_AND:
            generate_code_char(output, dst, '&');
            break;
          default:
          case ARITH_BITWISE_OR:
            assert(token.type == ARITH_BITWISE_OR);
            generate_code_char(output, dst, '|');
            break;
        }
        generate_code_char(output, dst, 's');
        generate_code_size_t(output, dst, stack_position - 1);
        generate_code_char(output, dst, ';');
        stack_position -= 1;
        break;
      default:
      case ARITH_BITWISE_COMPLEMENT:
        assert(token.type == ARITH_BITWISE_COMPLEMENT);
        generate_code_char(output, dst, 's');
        generate_code_size_t(output, dst, stack_position - 2);
        generate_code_char(output, dst, '=');
        generate_code_char(output, dst, '~');
        generate_code_char(output, dst, 's');
        generate_code_size_t(output, dst, stack_position - 1);
        generate_code_char(output, dst, ';');
        stack_position -= 1;
        break;
    }
    ++expr.begin;
  } while (expr.begin != expr.end);

  // the output
  generate_code_cstr(output, dst, "arith_out=s0;");
  generate_code_char(output, dst, '}'); // close block
}