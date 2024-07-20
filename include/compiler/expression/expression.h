#pragma once

#include <assert.h>
#include <stdbool.h>
#include "code_unit.h"
#include "likely_unlikely.hpp"

typedef struct {
  const CODE_UNIT* begin;
  const CODE_UNIT* end;
} expr_token_string_range;

typedef enum {
  EXPR_TOKEN_LITERAL,
  EXPR_TOKEN_FUNCTION,
  EXPR_TOKEN_ENDARG
} expr_token_type;

typedef struct {
  expr_token_string_range name;
  size_t num_args;
} expr_token_function_data;

typedef union {
  CODE_UNIT literal; // EXPR_TOKEN_LITERAL
  expr_token_function_data function; // EXPR_TOKEN_FUNCTION
} expr_token_data;

typedef struct {
  expr_token_type type;
  expr_token_data data;
  size_t offset; // for diagnostic information
} expr_token;

typedef struct {
  const char* reason; // NULL indicates success
  size_t offset;      // offending location
} expr_tokenize_result;

// this is the argument passed to tokenize_expression.
// a reference is shared with recursive calls (global within top level call)
typedef struct {
  const CODE_UNIT* pos;   // current parse position. starts at begin and moved forward
  const CODE_UNIT* begin; // beginning of the input range
  const CODE_UNIT* end;   // end of the input range

  // use of the function should be complete in two passes.
  //  - the first pass gets the capacity required to store the array of tokens.
  //    in this case, the returned value should be inspected for the result
  //  - the second pass fills the allocation.
  //    in this case, the returned value is meaningless
  bool getting_capacity;

  // moves forward as the output is populated.
  // point to allocation on the second pass. null on first pass
  expr_token* out;
} expr_tokenize_arg;

// private
typedef struct {
  // ret propagates errors to the top level call
  expr_tokenize_result ret;

  // true iff last argument was parsed.
  bool function_complete;
} expr_tokenize_result_internal;

// initialize for the first pass
expr_tokenize_arg expr_tokenize_arg_init(const CODE_UNIT* begin, const CODE_UNIT* end) {
  assert(begin <= end);
  expr_tokenize_arg ret;
  ret.pos = begin;
  ret.begin = begin;
  ret.end = end;
  ret.getting_capacity = true;
  ret.out = NULL;
  return ret;
}

// after the first pass, get the number of tokens that will be produced
size_t expr_tokenize_arg_get_cap(const expr_tokenize_arg* arg) {
  return (size_t)arg->out / sizeof(*arg->out);
}

// out points to an allocation with size indicated by expr_tokenize_arg_get_cap
void expr_tokenize_arg_set_to_fill(expr_tokenize_arg* arg, expr_token* out) {
  arg->pos = arg->begin;
  arg->getting_capacity = false;
  arg->out = out;
}

static void send_to_output(expr_tokenize_arg* output, expr_token token) {
  if (!output->getting_capacity) {
    *output->out = token;
  }
  ++output->out;
}

static void send_literal_to_output(expr_tokenize_arg* output, CODE_UNIT ch) {
  expr_token token;
  token.data.literal = ch;
  token.type = EXPR_TOKEN_LITERAL;
  token.offset = output->pos - output->begin;
  send_to_output(output, token);
}

// offset points to character before
static void send_escaped_to_output(expr_tokenize_arg* output, CODE_UNIT ch) {
  expr_token token;
  token.data.literal = ch;
  token.type = EXPR_TOKEN_LITERAL;
  token.offset = (output->pos - output->begin) - 1;
  send_to_output(output, token);
}

static void send_endarg_to_output(expr_tokenize_arg* output) {
  expr_token token;
  token.type = EXPR_TOKEN_ENDARG;
  token.offset = output->pos - output->begin;
  send_to_output(output, token);
}

static expr_token* reserve_in_output(expr_tokenize_arg* output) {
  expr_token* ret = NULL;
  if (!output->getting_capacity) {
    ret = output->out;
    ret->offset = output->pos - output->begin;
  }
  ++output->out;
  return ret;
}

// private
expr_tokenize_result_internal tokenize_expression_internal(expr_tokenize_arg* arg, bool top_level) {
  assert(arg->pos <= arg->end);
  expr_tokenize_result_internal ret;
  ret.function_complete = false;
  ret.ret.offset = 0;
  ret.ret.reason = NULL;

  while (arg->pos != arg->end) {
    CODE_UNIT ch = *arg->pos;
    switch (ch) {
      default:
        send_literal_to_output(arg, ch); // most characters are matched literally
        break;
      case '\\': {
        // see what follows escape character
        ++arg->pos;
        if (unlikely(arg->pos == arg->end)) {
          ret.ret.offset = (arg->pos - arg->begin) - 1;
          ret.ret.reason = "literal ended without content";
          goto end;
        }
        ch = *arg->pos;
        switch (ch) {
          default: {
            ret.ret.offset = arg->pos - arg->begin;
            ret.ret.reason = "invalid escaped character";
            goto end;
          } break;
          case '\\':
          case '{':
          case ',':
          case '}':
            send_escaped_to_output(arg, ch);
            break;
          case 'a':
            send_escaped_to_output(arg, '\a');
            break;
          case 'b':
            send_escaped_to_output(arg, '\b');
            break;
          case 't':
            send_escaped_to_output(arg, '\t');
            break;
          case 'n':
            send_escaped_to_output(arg, '\n');
            break;
          case 'v':
            send_escaped_to_output(arg, '\v');
            break;
          case 'f':
            send_escaped_to_output(arg, '\f');
            break;
          case 'r':
            send_escaped_to_output(arg, '\r');
            break;
          case 'e':
            send_escaped_to_output(arg, '\e');
            break;
        }
      } break;
      case '}':
      case ',': {
        if (unlikely(top_level)) {
          // at the top level, this is illegal
          ret.ret.offset = arg->pos - arg->begin;
          ret.ret.reason = "must be escaped";
          goto end;
        } else {
          // at a nested level, this indicates the end of the arg
          send_endarg_to_output(arg);
          ret.function_complete = ch == '}';
          goto end;
        }
      } break;
      case '{': {
        // function call
        const CODE_UNIT* const function_begin = arg->pos; // for offset calculation
        expr_token* function_token = reserve_in_output(arg);
        if (function_token) {
          function_token->type = EXPR_TOKEN_FUNCTION;
          function_token->data.function.name.begin = arg->pos + 1;
          function_token->data.function.num_args = 0;
        }
        // parse the function name
        while (1) {
          ++arg->pos;
          if (unlikely(arg->pos == arg->end)) {
            ret.ret.offset = function_begin - arg->begin;
            ret.ret.reason = "function name ended abruptly";
            goto end;
          }
          ch = *arg->pos;
          switch (ch) {
            case '}':
            case ',':
              if (function_token) {
                function_token->data.function.name.end = arg->pos;
              }
              if (ch == '}') {
                goto end_of_function_parse; // {function_name}
              } else {
                goto end_of_function_name; // break outer
              }
              break;
          }
        }
end_of_function_name:
        // parse the arguments
        while (1) {
          if (function_token) {
            function_token->data.function.num_args += 1;
          }
          ++arg->pos; // skip past the ','
          expr_tokenize_result_internal recurse = tokenize_expression_internal(arg, false);
          if (unlikely(recurse.ret.reason != NULL)) { // propagate error
            ret.ret = recurse.ret;
            goto end;
          }

          if (recurse.function_complete) {
            break;
          }
        }
end_of_function_parse:
      } break;
    }
    ++arg->pos;
  }

  if (unlikely(!top_level)) {
    // if not at the top level, then a unescaped , or } is expect before the end
    ret.ret.reason = "expecting function end";
    ret.ret.offset = arg->end - arg->begin;
    goto end; // intentional redundant
  }
end:
  return ret;
}

expr_tokenize_result tokenize_expression(expr_tokenize_arg* arg) {
  return tokenize_expression_internal(arg, true).ret;
}
