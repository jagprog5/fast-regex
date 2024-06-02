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
  ARITH_TOKEN_ADD = '+',
  ARITH_TOKEN_SUB = '-',
  ARITH_TOKEN_MUL = '*',
  ARITH_TOKEN_DIV = '/',
  ARITH_TOKEN_MOD = '%',
  ARITH_TOKEN_LEFT_BRACKET = '(',
  ARITH_TOKEN_RIGHT_BRACKET = ')',
  ARITH_TOKEN_XOR = '^',
  ARITH_TOKEN_EQUALITY = '=',
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
  ARITH_TOKEN_U32,
  ARITH_TOKEN_SYMBOL,
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

// =============================== arith_token_tokenize_result ====================

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

// split the input into arith_token tokens.
// use of this function should be completed in two passes.
//  - the first pass gets the capacity required to store the array of tokens.
//  - the second pass fills the capacity.
void tokenize_arithmetic_expression(const CODE_UNIT* begin, const CODE_UNIT* end, arith_token_tokenize_result arg) {
  assert(begin <= end);
  CODE_UNIT ch;
  while (begin != end) {
    ch = *begin;
    // main_switch represents that ch was not consumed in the below parsing
main_switch:
    switch (ch) {
      case ' ':
      case '\t':
      case '\r':
      case '\n':
        break;
      case '+':
      case '-':
      case '*':
      case '/':
      case '%':
      case '(':
      case ')':
      case '^':
      case '=': {
        arith_token token;
        token.begin = begin;
        token.type = (arith_token_type)ch;
        send_output(&arg, token);
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
          send_output(&arg, token);
          goto end;
        }

        ch = *begin;
        if (ch == '=') {
          // <= found.
          token.type = is_lt ? ARITH_TOKEN_LESS_THAN_EQUAL : ARITH_TOKEN_GREATER_THAN_EQUAL;
          send_output(&arg, token);
          // this character is consumed. continue normally in loop
        } else if (ch == '<' && is_lt) {
          token.type = ARITH_TOKEN_LEFT_SHIFT;
          send_output(&arg, token);
          // this character is consumed. continue normally in loop
        } else if (ch == '>' && !is_lt) {
          token.type = ARITH_TOKEN_RIGHT_SHIFT;
          send_output(&arg, token);
          // this character is consumed. continue normally in loop
        } else {
          // end of operation since next character gives something else
          token.type = is_lt ? ARITH_TOKEN_LESS_THAN : ARITH_TOKEN_GREATER_THAN;
          send_output(&arg, token);
          goto main_switch;
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
          send_output(&arg, token);
          goto end;
        }

        ch = *begin;
        if (is_and && ch == '&') {
          token.type = ARITH_TOKEN_LOGICAL_AND;
          send_output(&arg, token);
          // this character is consumed. continue normally in loop
        } else if (!is_and && ch == '|') {
          token.type = ARITH_TOKEN_LOGICAL_OR;
          send_output(&arg, token);
          // this character is consumed. continue normally in loop
        } else {
          // end of operation since next character gives something else
          token.type = is_and ? ARITH_TOKEN_BITWISE_AND : ARITH_TOKEN_BITWISE_OR;
          send_output(&arg, token);
          goto main_switch;
        }
      } break;
      case '\'': {
        arith_token token;
        token.begin = begin;
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
        send_output(&arg, token);
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
        token.type = ARITH_TOKEN_U32;
        token.value.u32 = ch - '0';
        ++begin;
        while (1) {
          if (begin == end) {
            // end of number from end of string
            send_output(&arg, token);
            goto end;
          }

          ch = *begin;

          if (ch < '0' || ch > '9') {
            // end of number from end of digits
            send_output(&arg, token);
            goto main_switch;
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
          arith_token token;
          token.type = ARITH_TOKEN_SYMBOL;
          token.begin = begin;

          ++begin;
          while (1) {
            if (begin == end) {
              // emit symbol, end of string reached
              token.value.symbol_end = begin;
              send_output(&arg, token);
              goto end;
            }
            ch = *begin;
            if (!((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '_')) {
              // emit symbol, end of symbol reached
              token.value.symbol_end = begin;
              send_output(&arg, token);
              goto main_switch;
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
