#pragma once

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "code_unit.h"

// =============================================================================
// ================================ tokenizer ==================================

// ============================== arith_token ==================================

typedef enum {
  ARITH_TOKEN_ERROR,
  ARITH_TOKEN_ADD = '+', // binary op
  ARITH_TOKEN_SUB = '-', // binary op
  ARITH_TOKEN_MUL = '*',
  ARITH_TOKEN_DIV = '/',
  ARITH_TOKEN_MOD = '%',
  ARITH_TOKEN_LEFT_BRACKET = '(',
  ARITH_TOKEN_RIGHT_BRACKET = ')',
  ARITH_TOKEN_BITWISE_XOR = '^',
  ARITH_TOKEN_BITWISE_COMPLEMENT, // only unary
  ARITH_TOKEN_EQUAL,              // ==
  ARITH_TOKEN_NOT_EQUAL,          // !=
  ARITH_TOKEN_LESS_THAN,          // <
  ARITH_TOKEN_LESS_THAN_EQUAL,    // <=
  ARITH_TOKEN_LEFT_SHIFT,         // <<
  ARITH_TOKEN_GREATER_THAN,       // >
  ARITH_TOKEN_GREATER_THAN_EQUAL, // >=
  ARITH_TOKEN_RIGHT_SHIFT,        // >>
  ARITH_TOKEN_BITWISE_AND,        // &
  ARITH_TOKEN_LOGICAL_AND,        // &&
  ARITH_TOKEN_BITWISE_OR,         // |
  ARITH_TOKEN_LOGICAL_OR,         // ||
  ARITH_TOKEN_U32,                // u32 is the max size of CODE_UNIT
  ARITH_TOKEN_SYMBOL,
  ARITH_TOKEN_UNARY_ADD = -ARITH_TOKEN_ADD,
  ARITH_TOKEN_UNARY_SUB = -ARITH_TOKEN_SUB,
} arith_token_type;

typedef union {
  uint32_t u32;                // ARITH_TOKEN_U32
  const CODE_UNIT* symbol_end; // ARITH_TOKEN_SYMBOL
  const char* error_msg;       // ARITH_TOKEN_ERROR
} arith_token_value;

// token variant. token are created from an input string
typedef struct {
  arith_token_type type;
  const CODE_UNIT* begin;
  arith_token_value value;
} arith_token;

// ============================ arith_token_tokenize_result ====================

typedef enum { //
  ARITH_TOKEN_TOKENIZE_ARG_GET_CAPACITY,
  ARITH_TOKEN_TOKENIZE_ARG_FILL_ARRAY
} arith_token_tokenize_result_type;

typedef union {
  size_t* capacity;    // ARITH_TOKEN_TOKENIZE_ARG_GET_CAPACITY
  arith_token* tokens; // ARITH_TOKEN_TOKENIZE_ARG_FILL_ARRAY
} arith_token_tokenize_result_value;

typedef struct {
  arith_token_tokenize_result_type type;
  arith_token_tokenize_result_value value;
} arith_token_tokenize_result;

// =========================== functions =======================================

static void send_output(arith_token_tokenize_result* dst, arith_token token) {
  assert(dst->type == ARITH_TOKEN_TOKENIZE_ARG_GET_CAPACITY || dst->type == ARITH_TOKEN_TOKENIZE_ARG_FILL_ARRAY);
  switch (dst->type) {
    case ARITH_TOKEN_TOKENIZE_ARG_GET_CAPACITY:
      (*dst->value.capacity) += 1;
      break;
    case ARITH_TOKEN_TOKENIZE_ARG_FILL_ARRAY:
    default:
      *dst->value.tokens++ = token;
      break;
  }
}

// private
typedef struct {
  bool unary_allowed_next;
  bool value_allowed_next;
} arithmetic_expression_h_parse_state;

static void send_output_update_state(arith_token_tokenize_result* dst, arith_token token, arithmetic_expression_h_parse_state* state) {
  send_output(dst, token);
  switch (token.type) {
    case ARITH_TOKEN_RIGHT_BRACKET:
    case ARITH_TOKEN_U32:
    case ARITH_TOKEN_SYMBOL:
    case ARITH_TOKEN_UNARY_ADD:
    case ARITH_TOKEN_UNARY_SUB:
    case ARITH_TOKEN_BITWISE_COMPLEMENT:
      state->unary_allowed_next = false;
      break;
    default:
      state->unary_allowed_next = true;
      break;
  }

  switch (token.type) {
    case ARITH_TOKEN_RIGHT_BRACKET:
    case ARITH_TOKEN_U32:
    case ARITH_TOKEN_SYMBOL:
      state->value_allowed_next = false;
      break;
    default:
      state->value_allowed_next = true;
      break;
  }
}

// send a 0 before send_output_update_state.
// the shunting yard method can't handle unary, so all unary is converted to a binary op
static void send_zero_before_unary(arith_token_tokenize_result* dst, arith_token unary, arithmetic_expression_h_parse_state* state) {
  arith_token zero;
  zero.type = ARITH_TOKEN_U32;
  zero.begin = unary.begin;
  zero.value.u32 = 0;
  send_output(dst, zero);
  send_output_update_state(dst, unary, state);
}

// split the input into arith_token tokens.
// use of this function should be completed in two passes.
//  - the first pass gets the capacity required to store the array of tokens.
//  - the second pass fills the capacity.
// unary ops get an imaginary zero before them, so they are now a binary op instead
void tokenize_arithmetic_expression(const CODE_UNIT* begin, const CODE_UNIT* end, arith_token_tokenize_result arg) {
  assert(begin <= end);
  // given the previous token, what is allowed next?
  arithmetic_expression_h_parse_state state;
  state.unary_allowed_next = true;
  state.value_allowed_next = true;

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
        token.begin = begin;
        token.type = ch == '+' ? ARITH_TOKEN_UNARY_ADD : ARITH_TOKEN_UNARY_SUB;
        send_zero_before_unary(&arg, token, &state);
      } break;
      case '~': {
        arith_token token;
        token.begin = begin;
        if (!state.unary_allowed_next) {
          token.type = ARITH_TOKEN_ERROR;
          token.value.error_msg = "unary op mustn't follow value";
          send_output(&arg, token);
          goto end;
        } else {
          token.type = ARITH_TOKEN_BITWISE_COMPLEMENT;
          send_zero_before_unary(&arg, token, &state);
        }
      } break;
      case '(': {
        if (!state.value_allowed_next) {
          arith_token token;
          token.begin = begin;
          token.type = ARITH_TOKEN_ERROR;
          token.value.error_msg = "consecutive value now allowed";
          send_output(&arg, token);
          goto end;
        }
        goto simple_token;
      } break;
      case '*':
      case '/':
      case '%':
      case ')':
      case '^': {
simple_token:
        arith_token token;
        token.begin = begin;
        token.type = (arith_token_type)ch;
        send_output_update_state(&arg, token, &state);
      } break;
      case '<':
      case '>': {
        bool is_lt = ch == '<';
        arith_token token;
        token.begin = begin;
        ++begin;
        if (begin == end) {
          // end of operation from end of string
          token.type = is_lt ? ARITH_TOKEN_LESS_THAN : ARITH_TOKEN_GREATER_THAN;
          send_output_update_state(&arg, token, &state);
          goto end;
        }

        ch = *begin;
        if (ch == '=') {
          // <= found.
          token.type = is_lt ? ARITH_TOKEN_LESS_THAN_EQUAL : ARITH_TOKEN_GREATER_THAN_EQUAL;
          send_output_update_state(&arg, token, &state);
          // this character is consumed. continue normally in loop
        } else if (ch == '<' && is_lt) {
          token.type = ARITH_TOKEN_LEFT_SHIFT;
          send_output_update_state(&arg, token, &state);
          // this character is consumed. continue normally in loop
        } else if (ch == '>' && !is_lt) {
          token.type = ARITH_TOKEN_RIGHT_SHIFT;
          send_output_update_state(&arg, token, &state);
          // this character is consumed. continue normally in loop
        } else {
          // end of operation since next character gives something else
          token.type = is_lt ? ARITH_TOKEN_LESS_THAN : ARITH_TOKEN_GREATER_THAN;
          send_output_update_state(&arg, token, &state);
          goto begin_iter;
        }
      } break;
      case '=':
      case '!': {
        bool maybe_equality = ch == '=';
        arith_token token;
        token.begin = begin;
        ++begin;
        if (begin == end) {
          token.type = ARITH_TOKEN_ERROR;
          token.value.error_msg = "operator incomplete from end of string";
          send_output(&arg, token);
          goto end;
        }
        ch = *begin;
        if (ch == '=') {
          token.type = maybe_equality ? ARITH_TOKEN_EQUAL : ARITH_TOKEN_NOT_EQUAL;
          send_output_update_state(&arg, token, &state);
          // this character is consumed. continue normally in loop
        } else {
          token.type = ARITH_TOKEN_ERROR;
          token.value.error_msg = "operator incomplete";
          send_output(&arg, token);
          goto end;
        }
      } break;
      case '&':
      case '|': {
        bool is_and = ch == '&';
        arith_token token;
        token.begin = begin;
        ++begin;
        if (begin == end) {
          // end of operation from end of string
          token.type = is_and ? ARITH_TOKEN_BITWISE_AND : ARITH_TOKEN_BITWISE_OR;
          send_output_update_state(&arg, token, &state);
          goto end;
        }

        ch = *begin;
        if (is_and && ch == '&') {
          token.type = ARITH_TOKEN_LOGICAL_AND;
          send_output_update_state(&arg, token, &state);
          // this character is consumed. continue normally in loop
        } else if (!is_and && ch == '|') {
          token.type = ARITH_TOKEN_LOGICAL_OR;
          send_output_update_state(&arg, token, &state);
          // this character is consumed. continue normally in loop
        } else {
          // end of operation since next character gives something else
          token.type = is_and ? ARITH_TOKEN_BITWISE_AND : ARITH_TOKEN_BITWISE_OR;
          send_output_update_state(&arg, token, &state);
          goto begin_iter;
        }
      } break;
      case '\'': {
        arith_token token;
        token.begin = begin;
        if (!state.value_allowed_next) {
          token.type = ARITH_TOKEN_ERROR;
          token.value.error_msg = "consecutive value now allowed";
          send_output(&arg, token);
          goto end;
        }
        ++begin;
        if (begin == end) {
          token.type = ARITH_TOKEN_ERROR;
          token.value.error_msg = "literal ended without content";
          send_output(&arg, token);
          goto end;
        }

        ch = *begin;

        if (ch == '\\') {
          // escaped character
          ++begin;
          if (begin == end) {
            token.begin = begin;
            token.type = ARITH_TOKEN_ERROR;
            token.value.error_msg = "escape character ended without content";
            send_output(&arg, token);
            goto end;
          }
          ch = *begin;
          switch (ch) {
            default:
              token.begin = begin;
              token.type = ARITH_TOKEN_ERROR;
              token.value.error_msg = "invalid escaped character";
              send_output(&arg, token);
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
              for (int i = 0; i < 8; ++i) { // 4 bytes (8 nibbles) in u32
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
                  token.begin = begin;
                  token.type = ARITH_TOKEN_ERROR;
                  token.value.error_msg = "invalid hex escaped character";
                  send_output(&arg, token);
                  goto end;
                }
              }
            } break;
          }
        } else {
          token.value.u32 = ch;
        }
        ++begin;
        if (begin == end) {
literal_no_close_quote:
          token.type = ARITH_TOKEN_ERROR;
          token.value.error_msg = "literal ended without closing quote";
          send_output(&arg, token);
          goto end;
        }
        ch = *begin;
        if (ch != '\'') {
          token.type = ARITH_TOKEN_ERROR;
          token.begin = begin;
          token.value.error_msg = "expecting end of literal";
          send_output(&arg, token);
          goto end;
        }
past_literal_close_quote:
        token.type = ARITH_TOKEN_U32;
        send_output_update_state(&arg, token, &state);
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
        token.begin = begin;
        if (!state.value_allowed_next) {
          token.type = ARITH_TOKEN_ERROR;
          token.value.error_msg = "consecutive value now allowed";
          send_output(&arg, token);
          goto end;
        }
        token.type = ARITH_TOKEN_U32;
        token.value.u32 = ch - '0';
        ++begin;
        while (1) {
          if (begin == end) {
            // end of number from end of string
            send_output_update_state(&arg, token, &state);
            goto end;
          }

          ch = *begin;

          if (ch < '0' || ch > '9') {
            // end of number from end of digits
            send_output_update_state(&arg, token, &state);
            goto begin_iter;
          }

          // handle digit

          { // checked multiply
            uint64_t next_value = token.value.u32;
            assert(sizeof(next_value) > sizeof(token.value.u32)); // static
            next_value *= 10;
            if (next_value > UINT32_MAX) {
              goto handle_value_overflow;
            }
            token.value.u32 = next_value;
          }

          { // checked add
            uint32_t next_value = token.value.u32 + (ch - '0');
            if (next_value < token.value.u32) {
handle_value_overflow:
              arith_token err_token;
              err_token.type = ARITH_TOKEN_ERROR;
              err_token.begin = token.begin;
              err_token.value.error_msg = "num too large";
              send_output(&arg, err_token);
              goto end;
            }
            token.value.u32 = next_value;
          }

          ++begin;
        }
      } break;
      default: {
        if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z' || ch == '_')) {
          if (!state.value_allowed_next) {
            arith_token token;
            token.begin = begin;
            token.type = ARITH_TOKEN_ERROR;
            token.value.error_msg = "consecutive value now allowed";
            send_output(&arg, token);
            goto end;
          }
          arith_token token;
          token.type = ARITH_TOKEN_SYMBOL;
          token.begin = begin;

          ++begin;
          while (1) {
            if (begin == end) {
              // emit symbol, end of string reached
              token.value.symbol_end = begin;
              send_output_update_state(&arg, token, &state);
              goto end;
            }
            ch = *begin;
            if (!((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '_')) {
              // emit symbol, end of symbol reached
              token.value.symbol_end = begin;
              send_output_update_state(&arg, token, &state);
              goto begin_iter;
            }
            ++begin;
          }

        } else {
          arith_token token;
          token.type = ARITH_TOKEN_ERROR;
          token.begin = begin;
          token.value.error_msg = "invalid unit in arith expr";
          send_output(&arg, token);
          goto end;
        }
      } break;
    }
    ++begin;
  }
end:
}

// all operations are left to right associative
// lower number indicates higher precedence.
static unsigned int operation_precedence(arith_token_type type) {
  switch (type) {
    case ARITH_TOKEN_UNARY_ADD:
    case ARITH_TOKEN_UNARY_SUB:
    case ARITH_TOKEN_BITWISE_COMPLEMENT:
      return 0; // unary must have highest
      break;
    case ARITH_TOKEN_MUL:
    case ARITH_TOKEN_DIV:
    case ARITH_TOKEN_MOD:
      return 1;
      break;
    case ARITH_TOKEN_ADD:
    case ARITH_TOKEN_SUB:
      return 2;
      break;
    case ARITH_TOKEN_LEFT_SHIFT:
    case ARITH_TOKEN_RIGHT_SHIFT:
      return 3;
      break;
    case ARITH_TOKEN_LESS_THAN:
    case ARITH_TOKEN_LESS_THAN_EQUAL:
      return 4;
      break;
    case ARITH_TOKEN_GREATER_THAN:
    case ARITH_TOKEN_GREATER_THAN_EQUAL:
      return 5;
      break;
    case ARITH_TOKEN_BITWISE_AND:
      return 6;
      break;
    case ARITH_TOKEN_BITWISE_XOR:
      return 7;
      break;
    case ARITH_TOKEN_BITWISE_OR:
      return 8;
      break;
    case ARITH_TOKEN_EQUAL:
    case ARITH_TOKEN_NOT_EQUAL:
      return 9;
      break;
    case ARITH_TOKEN_LOGICAL_AND:
      return 10;
      break;
    case ARITH_TOKEN_LOGICAL_OR:
    default:
      assert(type == ARITH_TOKEN_LOGICAL_OR);
      return 11;
      break;
  }
}

// overwrites the range begin to end with its reverse polish notation equivalent
// returns the new end of the range
// https://math.oxford.emory.edu/site/cs171/shuntingYardAlgorithm/
// unary add and sub are converted to binary add and sub
// (since they got an extra zero in the tokenization step)
arith_token* parse_arithmetic_expression(arith_token* begin, arith_token* end) {
  assert(begin <= end);
  arith_token* begin_copy = begin; // save for below

  size_t input_size = end - begin;
  arith_token operator_stack[input_size];
  arith_token* operator_stack_top = operator_stack;

  arith_token* write_pos = begin;

  while (begin != end) {
    arith_token token = *begin;

    switch (token.type) {
      case ARITH_TOKEN_ERROR:
        *write_pos++ = token;
        goto end;
        break;
      case ARITH_TOKEN_U32:
      case ARITH_TOKEN_SYMBOL:
        // "If the incoming symbol is an operand, print it."
        *write_pos++ = token;
        break;
      case ARITH_TOKEN_LEFT_BRACKET:
        // "If the incoming symbol is a left parenthesis, push it on the stack"
        *operator_stack_top++ = token;
        break;
      case ARITH_TOKEN_RIGHT_BRACKET: {
        // "If the incoming symbol is a right parenthesis: discard the right
        // parenthesis, pop and print the stack symbols until you see a left
        // parenthesis. Pop the left parenthesis and discard it."
        while (1) {
          if (operator_stack == operator_stack_top) {
            token.type = ARITH_TOKEN_ERROR;
            token.value.error_msg = "no matching left bracket";
            *write_pos++ = token;
            goto end;
          }

          arith_token stack_top = *--operator_stack_top;
          if (stack_top.type == ARITH_TOKEN_LEFT_BRACKET) {
            break;
          }
          *write_pos++ = stack_top;
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
          if (stack_top.type == ARITH_TOKEN_LEFT_BRACKET) {
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
            *write_pos++ = stack_top;
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
    if (stack_top.type == ARITH_TOKEN_LEFT_BRACKET) {
      stack_top.type = ARITH_TOKEN_ERROR;
      stack_top.value.error_msg = "no matching right bracket";
      *write_pos++ = stack_top;
      goto end;
    }
    *write_pos++ = stack_top;
  }

  // verify pass and fixup: replace unary add/sub with bin (since they are now)
  begin = begin_copy;
  end = write_pos;

  unsigned int stack_size = 0;
  while (begin != end) {
    switch (begin->type) {
      case ARITH_TOKEN_U32:
      case ARITH_TOKEN_SYMBOL:
        stack_size += 1;
        break;
      case ARITH_TOKEN_UNARY_ADD:
      case ARITH_TOKEN_UNARY_SUB:
        // unary was converted to binary op. label it correctly
        begin->type *= -1;
      default:
        if (stack_size < 2) {
          arith_token err;
          err.type = ARITH_TOKEN_ERROR;
          err.begin = begin->begin;
          err.value.error_msg = "expression stack exhausted by op";
          *begin = err;
          write_pos = begin + 1;
          goto end;
        }
        stack_size -= 1;
        break;
    }
    ++begin;
  }

end:

  assert(write_pos >= begin_copy);
  assert(write_pos <= end);
  return write_pos;
}
