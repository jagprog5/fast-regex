#pragma once

#include "character/subject_buffer.h"
#include "compiler/expression/expression.h"
#include "compiler/expression/expression_interpret/expression_interpret_definition.h"

// ========================== interpret presetup ===============================

// an output of interpret_presetup, used as input to interpret_setup
typedef struct {
  size_t function_data_size; // bytes
  const function_definition* definition;
  size_t num_args;
  const expr_token* function_start;
} function_setup_info;

// overall return status from interpret_presetup.
// the return status should only be viewed on the first pass
typedef struct {
  bool success;
  // if OK, indicates the number of bytes in the output data
  // if ERR, indicates the number of characters in the output error message
  size_t size;
} interpret_presetup_result;

// needed prior to interpret_presetup
size_t interpret_presetup_get_number_of_function_calls(const expr_token* begin, //
                                                       const expr_token* const end) {
  size_t ret = 0;
  while (begin != end) {
    if (begin++->type != EXPR_TOKEN_ENDARG) ret += 1;
  }
  return ret;
}

// associates function names with their definitions and invokes the presetup on each function
//
// functions points to num_function defined functions, sorted by function_definition_sort
//
// presetup_info_output points to n elements (interpret_presetup_get_number_of_function_calls)
//
// on first pass, error_msg_output should be null. if the return value indicates a failure,
// then it should be called again, pointing to an appropriate number of elements (indicated in return value);
// it will be populated with a null terminating message.
//
// error_offset_output should also be null on the first pass, and non null on second pass error (similar to error_msg_output)
interpret_presetup_result interpret_presetup(const expr_token* begin, //
                                             const expr_token* const end,
                                             size_t num_function,
                                             const function_definition* functions,
                                             function_setup_info* presetup_info_output,
                                             CODE_UNIT* error_msg_output,
                                             size_t* error_offset_output) {
  assert(begin <= end);
  size_t presetup_info_index = 0;
  interpret_presetup_result ret;
  ret.size = 0;

  while (begin != end) {
    expr_token token = *begin;

    if (token.type == EXPR_TOKEN_ENDARG) continue;

    const function_definition* definition;
    function_presetup_result presetup_result;

    expr_token* begin_before_call = begin;
    if (token.type == EXPR_TOKEN_LITERAL) {
      definition = function_definition_for_literal();
      presetup_result = definition->presetup(1, &begin);
    } else {
      assert(token.type == EXPR_TOKEN_FUNCTION);
      // check if function has been registered
      definition = function_definition_lookup(token.data.function.name, num_function, functions);
      if (unlikely(definition == NULL)) {
        ret.success = false;
        ret.size = 0;
        const CODE_UNIT* msg_prefix = "function name not registered: ";
        if (!error_msg_output) {
          // first pass. getting size of error message
          ret.size += code_unit_strlen(msg_prefix);
          // yes, the error string can contain a null character (in the pattern, \0).
          // this will cut off the string when printing, but that's ok
          assert(token.data.function.name.end >= token.data.function.name.begin);
          ret.size += token.data.function.name.end - token.data.function.name.begin;
          ret.size += 1; // for trailing null
        } else {
          // second pass. populate the output error details
          *error_offset_output = token.offset;
          while (1) {
            CODE_UNIT cu = *msg_prefix++;
            if (cu == '\0') break;
            *error_msg_output++ = cu;
          }
          const CODE_UNIT* name_pos = token.data.function.name.begin;
          while (name_pos != token.data.function.name.end) {
            *error_msg_output++ = *name_pos++;
          }
          *error_msg_output++ = '\0';
        }
        return ret;
      }

      presetup_result = definition->presetup(token.data.function.num_args, &begin);
    }

    // ensure that the presetup actually moved the pointer forward.
    // otherwise, this would be an infinite loop
    assert(begin > begin_before_call);

    if (unlikely(presetup_result.success == false)) {
      ret.success = false;
      ret.size = 0;
      const CODE_UNIT* msg_prefix = "function presetup error: ";
      if (!error_msg_output) {
        ret.size += code_unit_strlen(msg_prefix);
        ret.size += strlen(presetup_result.value.err.reason);
        ret.size += 1; // for trailing null
      } else {
        // second pass. populate the output error details
        *error_offset_output = token.offset;
        while (1) {
          CODE_UNIT cu = *msg_prefix++;
          if (cu == '\0') break;
          *error_msg_output++ = cu;
        }
        while (1) {
          char ch = *presetup_result.value.err.reason++;
          if (ch == '\0') break;
          *error_msg_output++ = ch;
        }
        *error_msg_output++ = '\0';
      }
      return ret;
    }

    // presetup was successful
    presetup_info_output[presetup_info_index].function_data_size = presetup_result.value.data_size_bytes;
    presetup_info_output[presetup_info_index].definition = definition;
    presetup_info_output[presetup_info_index].num_args = token.data.function.num_args;
    presetup_info_output[presetup_info_index].function_start = begin_before_call;
    ++presetup_info_index;
    ret.size += presetup_result.value.data_size_bytes;
  }

  ret.success = true;
  return ret;
}

// ========================== interpret setup ==================================

typedef struct {
  bool success; 

  // only applicable on failure. returns the number of elements in the error message
  size_t size;
} interpret_setup_result;

// presetup_info_output points to n elements (interpret_presetup_get_number_of_function_calls)
// both presetup_info and data were populated by interpret_presetup
//
// data points to n elements, as specified by a successful return result from interpret_presetup
void interpret_setup(size_t num_function_calls, function_setup_info* presetup_info, void* data) {
  size_t total_max_size_characters;
  size_t total_max_lookbehind_characters;
  for (size_t i = 0; i < presetup_info; ++i) {
    function_setup_result result = presetup_info[i].definition->setup(presetup_info[i].num_args, presetup_info[i].function_start, data, presetup_info[i].function_data_size);
    
    // TODO
    
        // result.


    // presetup_info.
  }
}
 