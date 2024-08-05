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

size_t interpret_presetup_get_number_of_function_calls(const expr_token* begin, //
                                                       const expr_token* const end) {
  size_t ret = 0;
  while (begin != end) {
    if (begin++->type == EXPR_TOKEN_FUNCTION) ret += 1;
  }
}

// typedef enum { FUNCTION_INTERPRET_SETUP_OK, FUNCTION_INTERPRET_SETUP_ERR } interpret_setup_expression_result_type;

// typedef struct {
//   size_t offset;       // offending location in the expression
//   size_t err_msg_size; // size required to store error message (including null terminator)
// } interpret_setup_expression_result_value_err;

// typedef union {
//   size_t data_size;                                // FUNCTION_INTERPRET_SETUP_OK
//   interpret_setup_expression_result_value_err err; // FUNCTION_INTERPRET_SETUP_ERR
// } interpret_setup_expression_result_value;

// typedef struct {
//   interpret_setup_expression_result_type type;
//   interpret_setup_expression_result_value value;
// } interpret_setup_expression_result;

// typedef struct {
//   char* data; // points to contiguous data from all the function setup results
//   //
// } interpret_setup_expression_arg_ok;

typedef union {
  char* data;               // FUNCTION_INTERPRET_SETUP_OK
  CODE_UNIT* error_message; // FUNCTION_INTERPRET_SETUP_ERR
} interpret_setup_expression_arg;

// setup expression to be interpreted.
//
// functions points to a number of function definitions. they must be sorted by
// function_definition_sort.
//
// this function should be called in two passes. the first pass gets the output
// capacity needed for the output, indicated by passing out as NULL. the size
// and overall status is indicated in the returned value.
//
// the second pass fills the output capacity, and the return value can be
// discarded.
interpret_setup_expression_result interpret_setup_expression(const expr_token* const begin, //
                                                             const expr_token* const end,
                                                             size_t num_function,
                                                             const function_definition* functions,
                                                             interpret_setup_expression_arg output) {
  assert(begin <= end);
  interpret_setup_expression_result ret;

  size_t num_function_calls = 0;
  const expr_token* pos = begin;

  while (pos != end) {
    if (pos++->type == EXPR_TOKEN_FUNCTION) num_function_calls += 1;
  }

  // stores info retrieved at the presetup stage, for the setup stage
  struct function_presetup_information {
    size_t function_data_size;
    const function_definition* definition;
    size_t num_args;
    const expr_token* arg_start;
  };

  // size of data for each function call
  struct function_presetup_information presetup_info[num_function_calls];
  size_t presetup_info_index = 0;
  // total function call size
  size_t function_total_data_size = 0;

  pos = begin; // reset

  while (pos != end) {
    expr_token token = *pos++;

    if (token.type == EXPR_TOKEN_LITERAL) {
    } else if (token.type == EXPR_TOKEN_ENDARG) {
      // return from function...
    } else { // EXPR_TOKEN_FUNCTION
      // check if function has been registered
      const function_definition* definition = function_definition_lookup(token.data.function.name, num_function, functions);
      if (unlikely(definition == NULL)) {
        ret.type = FUNCTION_INTERPRET_SETUP_ERR;
        ret.value.err.offset = token.offset;
        ret.value.err.err_msg_size = 0;

        const CODE_UNIT* msg_prefix = "function name not registered: ";
        if (!output.error_message) {
          ret.value.err.err_msg_size += code_unit_strlen(msg_prefix);
          // yes, the error string can contain a null character (in the pattern, \0).
          // this will cut off the string when printing, but that's ok
          ret.value.err.err_msg_size += token.data.function.name.end - token.data.function.name.begin;
          ret.value.err.err_msg_size += 1; // for trailing null
        } else {
          while (1) {
            CODE_UNIT cu = *msg_prefix++;
            if (cu == '\0') break;
            *output.error_message++ = cu;
          }
          const CODE_UNIT* pos = token.data.function.name.begin;
          while (pos != token.data.function.name.end) {
            *output.error_message++ = *pos++;
          }
          *output.error_message++ = '\0';
        }
        return ret;
      }

      function_presetup_result presetup_result = definition->presetup(token.data.function.num_args, pos);
      if (unlikely(presetup_result.type == FUNCTION_SETUP_ERR)) {
        ret.type = FUNCTION_INTERPRET_SETUP_ERR;
        ret.value.err.offset = token.offset;
        ret.value.err.err_msg_size = 0;

        const CODE_UNIT* msg_prefix = "function presetup error: ";
        if (!output.error_message) {
          ret.value.err.err_msg_size += code_unit_strlen(msg_prefix);
          ret.value.err.err_msg_size += strlen(presetup_result.value.err.reason);
          ret.value.err.err_msg_size += 1; // for trailing null
        } else {
          while (1) {
            CODE_UNIT cu = *msg_prefix++;
            if (cu == '\0') break;
            *output.error_message++ = cu;
          }
          while (1) {
            char ch = *presetup_result.value.err.reason++;
            if (ch == '\0') break;
            *output.error_message++ = ch;
          }
          *output.error_message++ = '\0';
        }
        return ret;
      }

      // presetup was successful (arguments ok)
      presetup_info[presetup_info_index].function_data_size = presetup_result.value.ok.data_size_bytes;
      presetup_info[presetup_info_index].definition = definition;
      presetup_info[presetup_info_index].num_args = token.data.function.num_args;
      presetup_info[presetup_info_index].arg_start = pos;
      ++presetup_info_index;
      function_total_data_size += presetup_result.value.ok.data_size_bytes;
    }
  }

  assert(presetup_info_index == num_function_calls); // bound check

  presetup_info_index = 0;
  char function_data[function_total_data_size];
  char* function_data_ptr = function_data;

  // at this point, presetup has been cimpleted on all the above functions
  // if this is the first pass, this is done

  while (presetup_info_index != num_function_calls) {
    struct function_presetup_information info = presetup_info[presetup_info_index++];
    function_setup_result result = info.definition->setup(info.num_args, info.arg_start, function_data_ptr);
    if (unlikely(result.type == FUNCTION_SETUP_ERR)) {
      const CODE_UNIT* msg_prefix = "function setup error: ";
      ret.error_string_size = 0;
      if (!error_msg) {
        ret.error_string_size += code_unit_strlen(msg_prefix);
        ret.error_string_size += strlen(result.value.err.reason);
        ret.error_string_size += 1; // for trailing null
      } else {
        while (1) {
          CODE_UNIT cu = *msg_prefix++;
          if (cu == '\0') break;
          *error_msg++ = cu;
        }
        while (1) {
          char ch = *result.value.err.reason++;
          if (ch == '\0') break;
          *error_msg++ = ch;
        }
        *error_msg++ = '\0';
      }
      return ret;
    }
    function_data_ptr += info.function_data_size;
  }

  assert(function_data_ptr - function_data == function_total_data_size);
}