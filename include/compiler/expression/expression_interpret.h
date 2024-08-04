#pragma once

#include "character/subject_buffer.h"
#include "compiler/expression/expression.h"

// for qsort
#include <stdlib.h>

// IN PROGRESS

typedef enum {
  FUNCTION_SETUP_OK,
  FUNCTION_SETUP_ERR,
} function_init_status;

typedef struct {
  const char* reason;
  size_t offset; // offset is relative to entire program
} function_init_value_error;

typedef struct {
  size_t data_size_bytes;
} function_presetup_value_ok;

typedef union {
  function_init_value_error err; // FUNCTION_SETUP_ERR
  function_presetup_value_ok ok; // FUNCTION_SETUP_OK
} function_presetup_value;

typedef struct {
  function_init_status type;
  function_presetup_value value;
} function_presetup_result;

typedef struct {
  size_t max_characters_forward;
  size_t max_characters_backward;
} function_setup_value_ok;

typedef union {
  function_init_value_error err; // FUNCTION_SETUP_ERR
  function_setup_value_ok ok;    // FUNCTION_SETUP_OK
} function_setup_value;

typedef struct {
  function_init_status type;
  function_setup_value value;
} function_setup_result;

typedef enum {
  MATCH_SUCCESS,
  MATCH_FAILURE,
  MATCH_INCOMPLETE // pcre2 partial hard - match is incomplete if it could be extended by more content
} interpret_result;

// associates a function name with it's behaviour
typedef struct {
  expr_token_string_range name;

  // presetup should validate arguments and indicate how much data is needed for the next phase
  function_presetup_result (*presetup)(size_t num_args, const expr_token* arg_start);

  // data points to a number of bytes indicates by the presetup.
  // this phase should do parsing and overall setup
  function_setup_result (*setup)(size_t num_args, const expr_token* arg_start, char* data);

  // this is the function used at runtime.
  // TODO more info from result
  interpret_result (*interpret)(subject_buffer_state* buffer);
} function_definition;

static int function_definition_sort_comp(const void* lhs_arg, const void* rhs_arg) {
  const function_definition* lhs = (const function_definition*)lhs_arg;
  const function_definition* rhs = (const function_definition*)rhs_arg;
  return code_unit_range_cmp(lhs->name.begin, lhs->name.end, rhs->name.begin, rhs->name.end);
}

void function_definition_sort(size_t num_function, function_definition* functions) {
  qsort(functions, num_function, sizeof(*functions), function_definition_sort_comp);
}

static const function_definition* function_definition_lookup(expr_token_string_range name, size_t num_function, const function_definition* functions) {
  size_t left = 0;
  size_t right = num_function;
  while (left < right) {
    size_t mid = left + (right - left) / 2;
    int cmp = code_unit_range_cmp(name.begin, name.end, functions[mid].name.begin, functions[mid].name.end);
    if (cmp < 0) {
      right = mid;
    } else if (cmp > 0) {
      left = mid + 1;
    } else {
      return &functions[mid];
    }
  }
  return NULL;
}

typedef enum { FUNCTION_INTERPRET_SETUP_OK, FUNCTION_INTERPRET_SETUP_ERR } interpret_setup_expression_result_type;

// TODO error string needs to be dynamically generated, should have two phase for that as well. error strings can be long
typedef struct {
  interpret_setup_expression_result_type type;
  // fields are applicable if error
  size_t offset; // offset within entire expression
  // to get the error string, call
  size_t error_string_size; // number of elements in buffer in next call to interpret_setup_expression, to get the error reason string
} interpret_setup_expression_result;

// setup expression to be interpreted.
// functions points to a number of function definitions. they must be sorted by function_definition_sort.
// error_msg should be NULL on first pass. if there is an error, it should be non null and point to an allocation with size indicated by the returned value
interpret_setup_expression_result interpret_setup_expression(const expr_token* begin, const expr_token* const end, size_t num_function, const function_definition* functions, CODE_UNIT* error_msg) {
  assert(begin <= end);
  interpret_setup_expression_result ret;
  while (begin != end) {
    expr_token token = *begin++;

    if (token.type == EXPR_TOKEN_LITERAL) {
    } else if (token.type == EXPR_TOKEN_ENDARG) {
      // return from function...
    } else { // EXPR_TOKEN_FUNCTION
      // check if function has been registered

      const function_definition* definition = function_definition_lookup(token.data.function.name, num_function, functions);
      if (unlikely(definition == NULL)) {
        ret.type = FUNCTION_INTERPRET_SETUP_ERR;
        ret.offset = token.offset;
        const CODE_UNIT* msg_prefix = "function name not registered: ";
        ret.error_string_size = 0;
        if (!error_msg) {
          ret.error_string_size += code_unit_strlen(msg_prefix);
          // yes, the error string can contain a null character (in the pattern, \0).
          // this will cut off the string when printing, but that's ok
          ret.error_string_size += token.data.function.name.end - token.data.function.name.begin;
          ret.error_string_size += 1; // for trailing null
        } else {
          while (1) {
            CODE_UNIT cu = *msg_prefix++;
            if (cu == '\0') break;
            *error_msg++ = cu;
          }
          const CODE_UNIT* pos = token.data.function.name.begin;
          while (pos != token.data.function.name.end) {
            *error_msg++ = *pos++;
          }
          *error_msg++ = '\0';
        }
        return ret;
      }

      function_presetup_result presetup_result = definition->presetup(token.data.function.num_args, begin);
      if (unlikely(presetup_result.type == FUNCTION_SETUP_ERR)) {
        const CODE_UNIT* msg_prefix = "function presetup error: ";
        ret.error_string_size = 0;
        if (!error_msg) {
          ret.error_string_size += code_unit_strlen(msg_prefix);
          ret.error_string_size += strlen(presetup_result.value.err.reason);
          ret.error_string_size += 1; // for trailing null
        } else {
          while (1) {
            CODE_UNIT cu = *msg_prefix++;
            if (cu == '\0') break;
            *error_msg++ = cu;
          }
          while (1) {
            char ch = *presetup_result.value.err.reason++;
            if (ch == '\0') break;
            *error_msg++ = ch;
          }
          *error_msg++ = '\0';
        }
        return ret;
      }

      // presetup was successful (arguments ok)
      size_t setup_data_size = presetup_result.value.ok.data_size_bytes;

      
    }
  }
}