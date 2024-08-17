#pragma once

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "basic/ascii_int.h"
#include "basic/likely_unlikely.h"
#include "character/code_unit.h"

typedef struct {
  const CODE_UNIT* begin;
  const CODE_UNIT* end;
} expr_token_string_range;

typedef struct {
  bool present;
  uint32_t marker_number; // which marker is this?
  int offset;             // placement of marker relative to beginning or end expression function
} expr_marker;

typedef struct {
  expr_token_string_range name;
  size_t num_args;
  expr_marker begin_marker;
  expr_marker end_marker;
} expr_token_function_data;

typedef enum { EXPR_TOKEN_LITERAL, EXPR_TOKEN_FUNCTION, EXPR_TOKEN_ENDARG } expr_token_type;

typedef struct {
  expr_token_type type;
  union {
    CODE_UNIT literal;                 // EXPR_TOKEN_LITERAL
    expr_token_function_data function; // EXPR_TOKEN_FUNCTION
  } value;
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

  // the number of markers in the expression
  size_t num_markers;

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
  ret.num_markers = 0;
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
  arg->num_markers = 0;
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
  token.value.literal = ch;
  token.type = EXPR_TOKEN_LITERAL;
  token.offset = output->pos - output->begin;
  send_to_output(output, token);
}

// offset points to character before
static void send_escaped_to_output(expr_tokenize_arg* output, CODE_UNIT ch) {
  expr_token token;
  token.value.literal = ch;
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
          case '0':
            send_escaped_to_output(arg, '\0');
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
          case 'x': {
            expr_token token;
            token.type = EXPR_TOKEN_LITERAL;
            token.offset = (arg->pos - arg->begin) - 1;
            // escaped hex string.
            arg->pos++;
            if (unlikely(arg->pos == arg->end)) {
              ret.ret.offset = arg->pos - arg->begin;
              ret.ret.reason = "expecting content for hex literal";
              goto end;
            }

            ch = *arg->pos;

            if (unlikely(ch != '{')) {
              ret.ret.offset = arg->pos - arg->begin;
              ret.ret.reason = "expecting '{', hex syntax is: \\x{abc}";
              goto end;
            }

            uint32_t hex_value = 0;
            for (size_t i = 0; i < 8; ++i) { // 4 bytes (8 nibbles) in u32
              ++arg->pos;
              if (unlikely(arg->pos == arg->end)) {
                ret.ret.offset = arg->pos - arg->begin;
                ret.ret.reason = "hex content not finished";
                goto end;
              }

              ch = *arg->pos;

              if (ch >= '0' && ch <= '9') {
                hex_value <<= 4;
                hex_value += ch - '0';
              } else if (ch >= 'a' && ch <= 'f') {
                hex_value <<= 4;
                hex_value += (ch - 'a') + 10;
              } else if (ch >= 'A' && ch <= 'F') {
                hex_value <<= 4;
                hex_value += (ch - 'A') + 10;
              } else if (ch == '}') {
                goto past_hex_completion;
              } else {
                ret.ret.offset = arg->pos - arg->begin;
                ret.ret.reason = "invalid hex escaped character";
                goto end;
              }
            }

            ++arg->pos;

            if (unlikely(arg->pos == arg->end)) {
              ret.ret.offset = arg->pos - arg->begin;
              ret.ret.reason = "hex content not finished";
              goto end;
            }

            ch = *arg->pos;

            if (unlikely(ch != '}')) {
              ret.ret.offset = arg->pos - arg->begin;
              ret.ret.reason = "hex content overlong. expecting '}'";
              goto end;
            }
past_hex_completion:
            token.value.literal = hex_value;
            send_to_output(arg, token);
          } break;
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
          function_token->value.function.num_args = 0;
        }
        // parse the function name and markers
        enum {
          MARKER_PARSE_PHASE_BEGIN_PARSING_ID, // must be at least one digit proceeding +-
          MARKER_PARSE_PHASE_PARSING_ID,       // digits for the marker id (before +-)
          MARKER_PARSE_PHASE_BEGIN_OFFSET,     // must be at least one digit after +-
          MARKER_PARSE_PHASE_PARSING_OFFSET    // digits for offset (after +-)
        } marker_parse_phase = MARKER_PARSE_PHASE_BEGIN_PARSING_ID;
        bool offset_is_positive = true;
        uint32_t marker_number = 0;
        int marker_offset = 0;
        bool marker_present = false;

        while (1) {
          ++arg->pos;
          if (unlikely(arg->pos == arg->end)) {
            ret.ret.offset = function_begin - arg->begin;
            ret.ret.reason = "function name ended abruptly";
            goto end;
          }
          ch = *arg->pos;

          switch (marker_parse_phase) {
            case MARKER_PARSE_PHASE_BEGIN_PARSING_ID: {
              // only digit allowed at beginning
              if (ch >= '0' && ch <= '9') {
                // overflow impossible on first digit
                append_ascii_to_uint32(&marker_number, ch);
                marker_present = true;
              } else {
                goto after_begin_marker; // any other character is part of the function name
              }
              marker_parse_phase = MARKER_PARSE_PHASE_PARSING_ID; // next phase
            } break;
            case MARKER_PARSE_PHASE_PARSING_ID: {
              // digit or plus minus allowed
              if (ch == '-' || ch == '+') {
                offset_is_positive = ch == '+';
                marker_parse_phase = MARKER_PARSE_PHASE_BEGIN_OFFSET;
              } else if (ch >= '0' && ch <= '9') {
                if (!append_ascii_to_uint32(&marker_number, ch)) {
                  ret.ret.offset = function_begin - arg->begin;
                  ret.ret.reason = "overflow on marker begin value";
                  goto end;
                }
              } else {
                goto after_begin_marker;
              }
            } break;
            case MARKER_PARSE_PHASE_BEGIN_OFFSET: {
              if (ch >= '0' && ch <= '9') {
                // overflow impossible on first digit
                append_ascii_to_int(&marker_offset, ch);
                marker_parse_phase = MARKER_PARSE_PHASE_PARSING_OFFSET;
              } else {
                ret.ret.offset = arg->pos - arg->begin;
                ret.ret.reason = "marker offset must be non-empty if following sign";
                goto end;
              }
            } break;
            case MARKER_PARSE_PHASE_PARSING_OFFSET: {
              if (ch >= '0' && ch <= '9') {
                if (!append_ascii_to_int(&marker_offset, ch)) {
                  ret.ret.offset = function_begin - arg->begin;
                  ret.ret.reason = "overflow on marker begin offset value";
                  goto end;
                }
              } else {
                goto after_begin_marker;
              }
            } break;
          }
        }
after_begin_marker:
        if (function_token) {
          function_token->value.function.begin_marker.marker_number = marker_number;
          function_token->value.function.begin_marker.offset = marker_offset * (offset_is_positive ? 1 : -1);
          function_token->value.function.begin_marker.present = marker_present;
          function_token->value.function.name.begin = arg->pos;
        }
        // reset and reuse for end marker as well
        marker_parse_phase = MARKER_PARSE_PHASE_PARSING_ID;
        marker_number = 0;
        marker_offset = 0;
        offset_is_positive = true;
        marker_present = false;
        // parsing the function name.
        // looking for }, ',', or a digit to signify the end of the function name
        while (1) {
          switch (ch) {
            case '}':
            case ',':
              if (function_token) {
                function_token->value.function.name.end = arg->pos;
              }
              if (ch == '}') {
                goto end_of_function_parse; // {function_name}
              } else {
                goto after_end_marker; // break outer
              }
              break;
            default:
              if (ch >= '0' && ch <= '9') {
                // counts as MARKER_PARSE_PHASE_BEGIN_PARSING_ID
                if (function_token) {
                  function_token->value.function.name.end = arg->pos;
                }
                marker_present = true;
                append_ascii_to_uint32(&marker_number, ch);
                goto before_end_marker;
              }
              break;
          }
          ++arg->pos;
          if (unlikely(arg->pos == arg->end)) {
            ret.ret.offset = function_begin - arg->begin;
            ret.ret.reason = "function name ended abruptly";
            goto end;
          }
          ch = *arg->pos;
        }
before_end_marker:
        while (1) {
          ++arg->pos;
          if (unlikely(arg->pos == arg->end)) {
            ret.ret.offset = function_begin - arg->begin;
            ret.ret.reason = "function name ended abruptly";
            goto end;
          }
          ch = *arg->pos;
          if (ch == '}' || ch == ',') {
            if (marker_parse_phase == MARKER_PARSE_PHASE_BEGIN_OFFSET) {
              ret.ret.offset = arg->pos - arg->begin;
              ret.ret.reason = "unexpected character in end marker offset start";
              goto end;
            }
            if (ch == '}') {
              goto end_of_function_parse; // {function_name}
            } else {
              goto after_end_marker; // break outer
            }
          }
          switch (marker_parse_phase) {
            default:
              // handled above: MARKER_PARSE_PHASE_BEGIN_PARSING_ID
              assert(marker_parse_phase == MARKER_PARSE_PHASE_PARSING_ID);
              // digit or plus minus allowed
              if (ch >= '0' && ch <= '9') {
                if (!append_ascii_to_uint32(&marker_number, ch)) {
                  ret.ret.offset = function_begin - arg->begin;
                  ret.ret.reason = "overflow on marker end value";
                  goto end;
                }
              } else if (ch == '+' || ch == '-') {
                offset_is_positive = ch == '+';
                marker_parse_phase = MARKER_PARSE_PHASE_BEGIN_OFFSET;
              } else {
                ret.ret.offset = arg->pos - arg->begin;
                ret.ret.reason = "unexpected character in end marker";
                goto end;
              }
              break;
            case MARKER_PARSE_PHASE_BEGIN_OFFSET:
            case MARKER_PARSE_PHASE_PARSING_OFFSET:
              if (ch >= '0' && ch <= '9') {
                if (!append_ascii_to_int(&marker_offset, ch)) {
                  ret.ret.offset = function_begin - arg->begin;
                  ret.ret.reason = "overflow on marker end value offset";
                  goto end;
                }
                marker_parse_phase = MARKER_PARSE_PHASE_PARSING_OFFSET;
              } else {
                ret.ret.offset = arg->pos - arg->begin;
                ret.ret.reason = "unexpected character in end marker offset start";
                goto end;
              }
              break;
          }
        }
after_end_marker: // also end of function name
        // parse the arguments
        while (1) {
          if (function_token) {
            function_token->value.function.num_args += 1;
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
        if (function_token) {
          function_token->value.function.end_marker.present = marker_present;
          function_token->value.function.end_marker.marker_number = marker_number;
          function_token->value.function.end_marker.offset = marker_offset * (offset_is_positive ? 1 : -1);
        }
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

typedef enum { EXPR_TOKEN_MARKERS_LOW_OK, EXPR_TOKEN_MARKERS_LOW_ERR } expr_token_marker_low_type;

typedef struct {
  expr_token_marker_low_type type;
  // if OK, indicates the number of output markers
  // if ERR, indicates the offending offset
  size_t value;
} expr_token_marker_low_result;

// markers should be the lowest values possible
expr_token_marker_low_result tokenize_expression_check_markers_lowest(const expr_token* const begin, const expr_token* const end) {
  assert(begin <= end);

  expr_token_marker_low_result ret;

  size_t num_markers = 0;
  const expr_token* pos = begin;
  while (pos != end) {
    expr_token t = *pos;
    if (t.type == EXPR_TOKEN_FUNCTION) {
      if (t.value.function.begin_marker.present) {
        ++num_markers;
      }

      if (t.value.function.end_marker.present) {
        ++num_markers;
      }
    }
    ++pos;
  }

  pos = begin;

  // in this array, using -1 to indicate marker was not seen,
  // and other values indicates the offset where the marker was seen
  size_t marker_seen[num_markers];
  for (size_t i = 0; i < num_markers; ++i) {
    marker_seen[i] = -1;
  }

  while (pos != end) {
    expr_token t = *pos;
    if (t.type == EXPR_TOKEN_FUNCTION) {
      if (t.value.function.begin_marker.present) {
        size_t marker_number = t.value.function.begin_marker.marker_number;
        size_t offset = t.offset;
        if (marker_number >= num_markers) {
          // this is both a bound check, additionally it's known early that this
          // is invalid via pigeon hole principal
          ret.type = EXPR_TOKEN_MARKERS_LOW_ERR;
          ret.value = offset;
          return ret;
        }
        marker_seen[marker_number] = offset;
      }

      if (t.value.function.end_marker.present) {
        size_t marker_number = t.value.function.end_marker.marker_number;
        size_t offset = t.offset;
        if (marker_number >= num_markers) {
          ret.type = EXPR_TOKEN_MARKERS_LOW_ERR;
          ret.value = offset;
          return ret;
        }
        marker_seen[marker_number] = offset;
      }
    }
    ++pos;
  }

  size_t num_unique_markers = 0;

  // marker numbers should be compacted to the lowest available numbers
  bool no_more = false;
  for (size_t i = 0; i < num_markers; ++i) {
    if (no_more) {
      if (marker_seen[i] != (size_t)-1) {
        ret.type = EXPR_TOKEN_MARKERS_LOW_ERR;
        ret.value = marker_seen[i];
        return ret;
      }
    } else {
      if (marker_seen[i] == (size_t)-1) {
        no_more = true;
      } else {
        num_unique_markers += 1;
      }
    }
  }

  ret.type = EXPR_TOKEN_MARKERS_LOW_OK;
  ret.value = num_unique_markers;
  return ret;
}
