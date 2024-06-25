#pragma once

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "code_unit.h"
#include "likely_unlikely.hpp"

// ============================== arith_token ==================================

// token variant. token are created from an input string
typedef enum {
  ARITH_INVALID = 0, // never used outside testing
  ARITH_ADD = '+',   // binary op
  ARITH_SUB = '-',   // binary op
  ARITH_MUL = '*',
  ARITH_DIV = '/',
  ARITH_MOD = '%',
  ARITH_LEFT_BRACKET = '(',
  ARITH_RIGHT_BRACKET = ')',
  ARITH_BITWISE_XOR = '^',
  ARITH_BITWISE_AND = '&',
  ARITH_BITWISE_OR = '|',
  ARITH_EQUAL = '=',               // =
  ARITH_BITWISE_COMPLEMENT = 0x80, // only unary. enum values continue past ascii
  ARITH_NOT_EQUAL,                 // !=
  ARITH_LESS_THAN,                 // <
  ARITH_LESS_THAN_EQUAL,           // <=
  ARITH_LEFT_SHIFT,                // <<
  ARITH_GREATER_THAN,              // >
  ARITH_GREATER_THAN_EQUAL,        // >=
  ARITH_RIGHT_SHIFT,               // >>
  ARITH_U32,                       // u32 is the max size of CODE_UNIT
  ARITH_SYMBOL,
  ARITH_UNARY_ADD = -ARITH_ADD,
  ARITH_UNARY_SUB = -ARITH_SUB,
} arith_type;

typedef union {
  uint_fast32_t u32;          // ARITH_U32. will NEVER exceed UINT32_MAX
  size_t symbol_value_lookup; // ARITH_SYMBOL. index of allowed symbol
} arith_value;

typedef struct {
  arith_type type;
  arith_value value;
  // used for diagnostic information on error
  size_t offset;
} arith_token;

// ======================== arith_tokenize_capacity ======================

typedef enum {
  ARITH_TOKENIZE_FILL_ARRAY = 0, // second pass
  ARITH_TOKENIZE_CAPACITY_ERROR, // first pass error
  ARITH_TOKENIZE_CAPACITY_OK,    // first pass success
} arith_tokenize_capacity_type;

typedef struct {
  size_t offset;
  const char* reason;
} arith_tokenize_capacity_error;

typedef union {
  size_t capacity;                   // ARITH_TOKENIZE_CAPACITY_OK
  arith_tokenize_capacity_error err; // ARITH_TOKENIZE_CAPACITY_ERROR
} arith_tokenize_capacity_value;

typedef struct {
  arith_tokenize_capacity_type type;
  arith_tokenize_capacity_value value;
} arith_tokenize_capacity;

// =========================== functions =======================================

// private
typedef struct {
  bool unary_allowed_next;
  bool value_allowed_next;
} arithmetic_expression_h_parse_state;

static void send_output(arith_token** output,
                        arith_tokenize_capacity* dst, //
                        arith_token token,
                        arithmetic_expression_h_parse_state* state) {
  // if *output is null, then this means the capacity is being obtained. in
  // which case it increments dst. if *output is not NULL, then that is the
  // position to write to
  if (*output == NULL) {
    dst->value.capacity += 1;
  } else {
    *(*output)++ = token;
  }

  switch (token.type) {
    case ARITH_RIGHT_BRACKET:
    case ARITH_U32:
    case ARITH_SYMBOL:
    case ARITH_UNARY_ADD:
    case ARITH_UNARY_SUB:
    case ARITH_BITWISE_COMPLEMENT:
      state->unary_allowed_next = false;
      break;
    default:
      state->unary_allowed_next = true;
      break;
  }

  switch (token.type) {
    case ARITH_RIGHT_BRACKET:
    case ARITH_U32:
    case ARITH_SYMBOL:
      state->value_allowed_next = false;
      break;
    default:
      state->value_allowed_next = true;
      break;
  }
}

// send a 0 before sending the token to the output.
// the shunting yard method can't handle unary, so all unary is converted to a binary op
static void send_output_zero_before_unary(arith_token** output,
                                          arith_tokenize_capacity* dst, //
                                          arith_token unary,
                                          arithmetic_expression_h_parse_state* state) {
  arith_token zero;
  zero.type = ARITH_U32;
  zero.offset = unary.offset;
  zero.value.u32 = 0;

  if (*output == NULL) {
    dst->value.capacity += 1;
  } else {
    *(*output)++ = zero;
  }

  send_output(output, dst, unary, state);
}

typedef struct {
  const CODE_UNIT* begin;
  const CODE_UNIT* end;
} arith_expr_symbol;

typedef struct {
  const arith_expr_symbol* symbols;
  size_t size;
} arith_expr_allowed_symbols;

// -1 if not in allowed symbols
static size_t get_arith_expr_allowed_symbol_index(const CODE_UNIT* symbol_begin, //
                                                  const CODE_UNIT* symbol_end,
                                                  const arith_expr_allowed_symbols* allowed) {
  for (size_t i = 0; i < allowed->size; ++i) {
    arith_expr_symbol allowed_symbol = allowed->symbols[i];

    size_t length1 = allowed_symbol.end - allowed_symbol.begin;
    size_t length2 = symbol_end - symbol_begin;
    if (length1 != length2) {
      goto try_next_symbol;
    }

    for (size_t j = 0; j < length1; ++j) {
      if (allowed_symbol.begin[j] != symbol_begin[j]) {
        goto try_next_symbol;
      }
    }

    return i;
try_next_symbol:
  }
  return -1;
}

// split the input into `arith_token` tokens.
// use of this function should be completed in two passes.
//  - the first pass gets the capacity required to store the array of tokens.
//    this is represented by passing NULL the output arg.
//    in this case, the returned value should be inspected for the result
//  - the second pass fills the allocation.
//    this is represented with a non NULL output arg
//    in this case, the returned value is meaningless
// unary ops get an imaginary zero before them, so they are now binary ops instead.
// symbols which aren't specified in `allowed_symbols` throw an error.
arith_tokenize_capacity tokenize_arithmetic_expression(const CODE_UNIT* begin,
                                                       const CODE_UNIT* end, //
                                                       arith_token* output,
                                                       const arith_expr_allowed_symbols* allowed_symbols) {
  assert(begin <= end);
  // given the previous token, what is allowed next?
  arithmetic_expression_h_parse_state state;
  state.unary_allowed_next = true;
  state.value_allowed_next = true;

  arith_tokenize_capacity ret;
  if (output == NULL) {
    ret.type = ARITH_TOKENIZE_CAPACITY_OK;
    ret.value.capacity = 0;
  } else {
    memset(&ret, 0, sizeof(ret));
    ret.type = ARITH_TOKENIZE_FILL_ARRAY; // intentional redundant
  }

  const CODE_UNIT* const original_begin = begin; // for offset calculation

  while (begin != end) {
    CODE_UNIT ch = *begin;
    // begin_iter represents that ch was not consumed in the below parsing
begin_iter:
    switch (ch) {
      case ' ':
      case '\t':
      case '\r':
      case '\n':
        break;
      case '+':
      case '-': {
        if (!state.unary_allowed_next) {
          goto simple_token; // handle normal binary op
        }
        // unary op found
        arith_token token;
        token.offset = begin - original_begin;
        token.type = ch == '+' ? ARITH_UNARY_ADD : ARITH_UNARY_SUB;
        send_output_zero_before_unary(&output, &ret, token, &state);
      } break;
      case '~': {
        arith_token token;
        token.offset = begin - original_begin;
        if (unlikely(!state.unary_allowed_next)) {
          ret.type = ARITH_TOKENIZE_CAPACITY_ERROR;
          ret.value.err.reason = "unary op mustn't follow value";
          ret.value.err.offset = token.offset;
          goto end;
        } else {
          token.type = ARITH_BITWISE_COMPLEMENT;
          send_output_zero_before_unary(&output, &ret, token, &state);
        }
      } break;
      case '(': {
        if (unlikely(!state.value_allowed_next)) {
          ret.type = ARITH_TOKENIZE_CAPACITY_ERROR;
          ret.value.err.reason = "consecutive value not allowed";
          ret.value.err.offset = begin - original_begin;
          goto end;
        }
        goto simple_token;
      } break;
      case '*':
      case '/':
      case '%':
      case ')':
      case '^':
      case '&':
      case '|':
      case '=': {
simple_token:
        arith_token token;
        token.offset = begin - original_begin;
        token.type = (arith_type)ch;
        send_output(&output, &ret, token, &state);
      } break;
      case '<':
      case '>': {
        bool is_lt = ch == '<';
        arith_token token;
        token.offset = begin - original_begin;
        ++begin;
        if (begin == end) {
          // end of operation from end of string
          token.type = is_lt ? ARITH_LESS_THAN : ARITH_GREATER_THAN;
          send_output(&output, &ret, token, &state);
          goto end;
        }

        ch = *begin;
        if (ch == '=') {
          // <= found.
          token.type = is_lt ? ARITH_LESS_THAN_EQUAL : ARITH_GREATER_THAN_EQUAL;
          send_output(&output, &ret, token, &state);
          // this character is consumed.
        } else if (ch == '<' && is_lt) {
          token.type = ARITH_LEFT_SHIFT;
          send_output(&output, &ret, token, &state);
          // this character is consumed.
        } else if (ch == '>' && !is_lt) {
          token.type = ARITH_RIGHT_SHIFT;
          send_output(&output, &ret, token, &state);
          // this character is consumed.
        } else {
          // end of operation since next character gives something else
          token.type = is_lt ? ARITH_LESS_THAN : ARITH_GREATER_THAN;
          send_output(&output, &ret, token, &state);
          goto begin_iter;
        }
      } break;
      case '!': {
        arith_token token;
        token.offset = begin - original_begin;
        ++begin;
        if (unlikely(begin == end)) {
          ret.type = ARITH_TOKENIZE_CAPACITY_ERROR;
          ret.value.err.reason = "operator incomplete (!=) from end of string";
          ret.value.err.offset = token.offset;
          goto end;
        }
        ch = *begin;
        if (likely(ch == '=')) {
          token.type = ARITH_NOT_EQUAL;
          send_output(&output, &ret, token, &state);
          // this character is consumed.
        } else {
          ret.type = ARITH_TOKENIZE_CAPACITY_ERROR;
          ret.value.err.reason = "operator incomplete (!=)";
          ret.value.err.offset = token.offset;
          goto end;
        }
      } break;
      case '\'': {
        arith_token token;
        token.offset = begin - original_begin;
        if (unlikely(!state.value_allowed_next)) {
          ret.type = ARITH_TOKENIZE_CAPACITY_ERROR;
          ret.value.err.reason = "consecutive value not allowed";
          ret.value.err.offset = token.offset;
          goto end;
        }
        ++begin;
        if (unlikely(begin == end)) {
          ret.type = ARITH_TOKENIZE_CAPACITY_ERROR;
          ret.value.err.reason = "literal ended without content";
          ret.value.err.offset = token.offset;
          goto end;
        }

        ch = *begin;

        if (ch == '\\') {
          // escaped character
          ++begin;
          if (unlikely(begin == end)) {
            ret.type = ARITH_TOKENIZE_CAPACITY_ERROR;
            ret.value.err.reason = "escape character ended without content";
            ret.value.err.offset = token.offset;
            goto end;
          }
          ch = *begin;
          switch (ch) {
            default:
              ret.type = ARITH_TOKENIZE_CAPACITY_ERROR;
              ret.value.err.reason = "invalid escaped character";
              ret.value.err.offset = begin - original_begin;
              goto end;
              break;
            case 'a':
              token.value.u32 = '\a';
              break;
            case 'b':
              token.value.u32 = '\b';
              break;
            case 't':
              token.value.u32 = '\t';
              break;
            case 'n':
              token.value.u32 = '\n';
              break;
            case 'v':
              token.value.u32 = '\v';
              break;
            case 'f':
              token.value.u32 = '\f';
              break;
            case 'r':
              token.value.u32 = '\r';
              break;
            case 'e':
              token.value.u32 = '\e';
              break;
            case 'x': {
              token.value.u32 = 0;
              for (size_t i = 0; i < 8; ++i) { // 4 bytes (8 nibbles) in u32
                ++begin;
                if (begin == end) {
                  goto literal_no_close_quote;
                }
                ch = *begin;
                if (ch >= '0' && ch <= '9') {
                  token.value.u32 <<= 4;
                  token.value.u32 += ch - '0';
                } else if (ch >= 'a' && ch <= 'f') {
                  token.value.u32 <<= 4;
                  token.value.u32 += (ch - 'a') + 10;
                } else if (ch >= 'A' && ch <= 'F') {
                  token.value.u32 <<= 4;
                  token.value.u32 += (ch - 'A') + 10;
                } else if (ch == '\'') {
                  goto past_literal_close_quote;
                } else {
                  ret.type = ARITH_TOKENIZE_CAPACITY_ERROR;
                  ret.value.err.reason = "invalid hex escaped character";
                  ret.value.err.offset = begin - original_begin;
                  goto end;
                }
              }
            } break;
          }
        } else {
          token.value.u32 = ch;
        }
        ++begin;
        if (unlikely(begin == end)) {
literal_no_close_quote:
          ret.type = ARITH_TOKENIZE_CAPACITY_ERROR;
          ret.value.err.reason = "literal ended without closing quote";
          ret.value.err.offset = token.offset;
          goto end;
        }
        ch = *begin;
        if (ch != '\'') {
          ret.type = ARITH_TOKENIZE_CAPACITY_ERROR;
          ret.value.err.reason = "expecting end of literal";
          ret.value.err.offset = begin - original_begin;
          goto end;
        }
past_literal_close_quote:
        token.type = ARITH_U32;
        assert(token.value.u32 <= UINT32_MAX);
        send_output(&output, &ret, token, &state);
      } break;
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9': {
        arith_token token;
        token.offset = begin - original_begin;
        if (unlikely(!state.value_allowed_next)) {
          ret.type = ARITH_TOKENIZE_CAPACITY_ERROR;
          ret.value.err.reason = "consecutive value not allowed";
          ret.value.err.offset = begin - original_begin;
          goto end;
        }
        token.type = ARITH_U32;
        token.value.u32 = ch - '0';
        ++begin;
        while (1) {
          assert(token.value.u32 <= UINT32_MAX);
          if (begin == end) {
            // end of number from end of string
            send_output(&output, &ret, token, &state);
            goto end;
          }

          ch = *begin;

          if (ch < '0' || ch > '9') {
            // end of number from end of digits
            send_output(&output, &ret, token, &state);
            goto begin_iter;
          }

          // handle digit
          // token_value is a u32 proxy for token.value.u32
          uint32_t token_value = token.value.u32;

          { // checked multiply
            uint32_t next_value = 10 * token_value;
            if (next_value / 10 != token_value) {
              goto handle_value_overflow;
            }
            token_value = next_value;
          }

          { // checked add
            uint32_t next_value = token_value + (ch - '0');
            if (next_value < token_value) {
handle_value_overflow:
              ret.type = ARITH_TOKENIZE_CAPACITY_ERROR;
              ret.value.err.reason = "num too large";
              ret.value.err.offset = token.offset;
              goto end;
            }
            token_value = next_value;
          }
          token.value.u32 = token_value;
          ++begin;
        }
      } break;
      default: {
        if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z' || ch == '_')) {
          arith_token token;
          token.type = ARITH_SYMBOL;
          token.offset = begin - original_begin;
          if (unlikely(!state.value_allowed_next)) {
            ret.type = ARITH_TOKENIZE_CAPACITY_ERROR;
            ret.value.err.reason = "consecutive value not allowed";
            ret.value.err.offset = token.offset;
            goto end;
          }

          ++begin;
          while (1) {
            if (begin == end) {
              // emit symbol, end of string reached
              size_t symbol_index = get_arith_expr_allowed_symbol_index(original_begin + token.offset, begin, allowed_symbols);
              if (likely(symbol_index != -1)) {
                token.value.symbol_value_lookup = symbol_index;
                send_output(&output, &ret, token, &state);
              } else {
                ret.type = ARITH_TOKENIZE_CAPACITY_ERROR;
                ret.value.err.reason = "invalid symbol";
                ret.value.err.offset = token.offset;
              }
              goto end;
            }
            ch = *begin;
            if (!((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '_')) {
              // emit symbol, end of symbol reached
              size_t symbol_index = get_arith_expr_allowed_symbol_index(original_begin + token.offset, begin, allowed_symbols);
              if (symbol_index != -1) {
                token.value.symbol_value_lookup = symbol_index;
                send_output(&output, &ret, token, &state);
                goto begin_iter;
              } else {
                ret.type = ARITH_TOKENIZE_CAPACITY_ERROR;
                ret.value.err.reason = "invalid symbol";
                ret.value.err.offset = begin - original_begin;
                goto end;
              }
            }
            ++begin;
          }
        } else {
          ret.type = ARITH_TOKENIZE_CAPACITY_ERROR;
          ret.value.err.reason = "invalid character in arith expr";
          ret.value.err.offset = begin - original_begin;
          goto end;
        }
      } break;
    }
    ++begin;
  }
end:
  return ret;
}

// ============================ arith_expr =====================================

// represents a token which is part of a parsed and valid expression.
// similar to arith_token, but doesn't include diagnostic information
typedef struct {
  arith_type type;
  arith_value value;
} arith_parsed;

typedef struct {
  size_t stack_required; // number of stack elements needed to execute the expression
  const arith_parsed* begin;
  const arith_parsed* end;
} arith_expr;

// ================================ arith_expr_result ==========================

typedef enum {
  ARITH_EXPR_ERROR,
  ARITH_EXPR_OK,
} arith_expr_result_type;

typedef struct {
  size_t offset;
  const char* reason;
} arith_expr_error;

typedef union {
  arith_expr_error err; // ARITH_EXPR_ERROR
  arith_expr expr;      // ARITH_EXPR_OK
} arith_expr_result_value;

typedef struct {
  arith_expr_result_type type;
  arith_expr_result_value value;
} arith_expr_result;

// =========================== functions =======================================

// all operations are left to right associative
// smaller number indicates higher precedence.
static unsigned int operation_precedence(arith_type type) {
  switch (type) {
    case ARITH_UNARY_ADD:
    case ARITH_UNARY_SUB:
    case ARITH_BITWISE_COMPLEMENT:
      return 0; // unary must have highest
      break;
    case ARITH_MUL:
    case ARITH_DIV:
    case ARITH_MOD:
      return 1;
      break;
    case ARITH_ADD:
    case ARITH_SUB:
      return 2;
      break;
    case ARITH_LEFT_SHIFT:
    case ARITH_RIGHT_SHIFT:
      return 3;
      break;
    case ARITH_LESS_THAN:
    case ARITH_LESS_THAN_EQUAL:
      return 4;
      break;
    case ARITH_GREATER_THAN:
    case ARITH_GREATER_THAN_EQUAL:
      return 5;
      break;
    case ARITH_BITWISE_AND:
      return 6;
      break;
    case ARITH_BITWISE_XOR:
      return 7;
      break;
    case ARITH_BITWISE_OR:
      return 8;
      break;
    case ARITH_EQUAL:
    case ARITH_NOT_EQUAL:
    default:
      assert(type == ARITH_EQUAL || type == ARITH_NOT_EQUAL);
      return 9;
      break;
  }
}

// out points to a range with an equal number of elements to the input. the
// output range can overwrite the input range if they start at the same
// position. parses the arithmetic expression. unary add and sub are converted
// to binary add and sub (since they got an extra zero in the tokenization
// step).
//
// the return value depends on the lifetime of the input range, but does not
// rely on the original expression string.
arith_expr_result parse_arithmetic_expression(arith_token* begin, arith_token* end, arith_parsed* out) {
  // https://math.oxford.emory.edu/site/cs171/shuntingYardAlgorithm/
  assert(begin <= end);
  assert(sizeof(arith_parsed) <= sizeof(arith_token)); // static

  arith_expr_result ret;
  ret.type = ARITH_EXPR_OK;
  ret.value.expr.stack_required = 0;
  ret.value.expr.begin = out;
  size_t current_stack_size = 0;

  size_t input_size = end - begin;
  arith_token operator_stack[input_size];
  arith_token* operator_stack_top = operator_stack;

  while (begin != end) {
    arith_token token = *begin;

    switch (token.type) {
      case ARITH_U32:
      case ARITH_SYMBOL:
        // "If the incoming symbol is an operand, print it."
        current_stack_size += 1;
        if (current_stack_size > ret.value.expr.stack_required) {
          ret.value.expr.stack_required = current_stack_size;
        }
        out->type = token.type;
        out->value = token.value;
        ++out;
        break;
      case ARITH_LEFT_BRACKET:
        // "If the incoming symbol is a left parenthesis, push it on the stack"
        *operator_stack_top++ = token;
        break;
      case ARITH_RIGHT_BRACKET: {
        // "If the incoming symbol is a right parenthesis: discard the right
        // parenthesis, pop and print the stack symbols until you see a left
        // parenthesis. Pop the left parenthesis and discard it."
        while (1) {
          if (unlikely(operator_stack == operator_stack_top)) {
            ret.type = ARITH_EXPR_ERROR;
            ret.value.err.offset = token.offset;
            ret.value.err.reason = "no matching left bracket";
            goto end;
          }

          arith_token stack_top = *--operator_stack_top;
          if (stack_top.type == ARITH_LEFT_BRACKET) {
            break;
          }
          if (unlikely(current_stack_size < 2)) {
            ret.type = ARITH_EXPR_ERROR;
            ret.value.err.offset = stack_top.offset;
            ret.value.err.reason = "expression stack exhausted by op";
            goto end;
          }
          current_stack_size -= 1;
          switch (stack_top.type) {
            case ARITH_UNARY_ADD:
            case ARITH_UNARY_SUB:
              stack_top.type *= -1; // unary was converted to binary op
          }
          out->type = stack_top.type;
          out->value = stack_top.value;
          ++out;
        }
      } break;
      default:
        // the incoming symbol is an operator
        unsigned int token_precedence = operation_precedence(token.type);
        while (1) {
          // "[If] the stack is empty or contains a left parenthesis on top,
          // push the incoming operator onto the stack"
          if (operator_stack == operator_stack_top) {
            *operator_stack_top++ = token;
            break;
          }
          arith_token stack_top = operator_stack_top[-1];
          if (stack_top.type == ARITH_LEFT_BRACKET) {
            *operator_stack_top++ = token;
            break;
          }

          unsigned int stack_top_precedence = operation_precedence(stack_top.type);
          if (token_precedence <= stack_top_precedence) {
            // "If the incoming symbol is an operator and has either higher
            // precedence than the operator on the top of the stack, or has the
            // same precedence as the operator on the top of the stack and is
            // right associative, or if the stack is empty, or if the top of the
            // stack is "(" (a floor) -- push it on the stack."
            *operator_stack_top++ = token;
            break;
          } else {
            // If the incoming symbol is an operator and has either lower
            // precedence than the operator on the top of the stack, or has the
            // same precedence as the operator on the top of the stack and is
            // left associative -- continue to pop the stack until this is not
            // true. Then, push the incoming operator.
            if (unlikely(current_stack_size < 2)) {
              ret.type = ARITH_EXPR_ERROR;
              ret.value.err.offset = stack_top.offset;
              ret.value.err.reason = "expression stack exhausted by op";
              goto end;
            }
            current_stack_size -= 1;
            switch (stack_top.type) {
              case ARITH_UNARY_ADD:
              case ARITH_UNARY_SUB:
                stack_top.type *= -1; // unary was converted to binary op
            }
            out->type = stack_top.type;
            out->value = stack_top.value;
            ++out;
            --operator_stack_top;
          }
        }
        break;
    }
    ++begin;
  }

  // "At the end of the expression, pop and print all operators on the stack.
  // (No parentheses should remain.)"
  while (operator_stack != operator_stack_top) {
    arith_token stack_top = *--operator_stack_top;
    if (unlikely(stack_top.type == ARITH_LEFT_BRACKET)) {
      ret.type = ARITH_EXPR_ERROR;
      ret.value.err.offset = stack_top.offset;
      ret.value.err.reason = "no matching right bracket";
      goto end;
    }
    if (unlikely(current_stack_size < 2)) {
      ret.type = ARITH_EXPR_ERROR;
      ret.value.err.offset = stack_top.offset;
      ret.value.err.reason = "expression stack exhausted by op";
      goto end;
    }
    current_stack_size -= 1;
    switch (stack_top.type) {
      case ARITH_UNARY_ADD:
      case ARITH_UNARY_SUB:
        stack_top.type *= -1; // unary was converted to binary op
        break;
    }
    out->type = stack_top.type;
    out->value = stack_top.value;
    ++out;
  }

  ret.value.expr.end = out;

  if (unlikely(ret.value.expr.begin == ret.value.expr.end)) {
    ret.type = ARITH_EXPR_ERROR;
    ret.value.err.offset = 0;
    ret.value.err.reason = "empty expression not allowed";
    goto end;
  }

  assert(current_stack_size == 1);
  assert(ret.value.expr.begin < ret.value.expr.end);
  assert(ret.value.expr.end - ret.value.expr.begin <= input_size);
end:
  return ret;
}
