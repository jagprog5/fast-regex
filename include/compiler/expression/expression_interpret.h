#pragma once

#include "character/subject_buffer.h"
#include "compiler/expression/expression.h"

#include <stdlib.h> // qsort

// ========================= function definition ===============================

// result of presetup.
// on success, yields the size of data needed for the setup phase
// on failure, gives offending location and reason
typedef struct {
  bool success;
  union {
    size_t data_size_bytes; // success
    struct {                // failure
      size_t offset;        // offending location relative to beginning of expression
      const char* reason;
    } err;
  } value;
} function_presetup_result;

// result of setup.
// on success, yields information about the function
// on failure, gives offending location and reason
typedef struct {
  bool success;
  union {
    struct { // success
      size_t max_size_characters;
      size_t max_lookbehind_characters;
    } ok;
    struct {         // failure
      size_t offset; // offending location relative to beginning of expression
      const char* reason;
    } err;
  } value;
} function_setup_result;

typedef enum { MATCH_SUCCESS, MATCH_FAILURE, MATCH_INCOMPLETE } match_status;

struct function_definition; // forward declare

typedef struct {
  size_t function_data_size; // bytes
  const struct function_definition* definition;
  size_t num_args;
  const expr_token* function_start;
} function_setup_info;

// associates a function name with it's behaviour
typedef struct function_definition {
  expr_token_string_range name;

  // validate function arguments, and indicate data size needed for the next phase.
  //
  // move function_start to the next function (typically one past end_arg)
  //
  // place information in presetup_info, and move forward to next element.
  // requires populating all fields except for definition (which is already set)
  function_presetup_result (*presetup)(const expr_token** function_start, function_setup_info** presetup_info);

  // data points to a number of bytes indicates by the presetup
  // this phase must do parsing and overall setup
  //
  // move function_start and presetup_info forward in the same way as the presetup
  function_setup_result (*setup)(const expr_token** function_start, const function_setup_info** presetup_info, void* data, size_t data_size_bytes);

  // search for a match that starts at the buffer's match offset
  //
  // this function is called at runtime during pattern matching, in a context in
  // which there might be an incomplete match because there's not enough content left in the match buffer.
  // this uncertainty applies in both the forward and backward direction
  //
  // return INCOMPLETE if a match could be completed or extended with additional content
  // otherwise, return SUCCESS on successful match, and FAILURE on no match.
  //
  // the function must move the subject buffer's offset depending on the result:
  //  - INCOMPLETE : don't move the offset
  //  - SUCCESS : move the offset forward to one character past the matched content
  //  - FAILURE : (doesn't matter) 
  match_status (*interpret)(subject_buffer_state* buffer, const void* data, size_t data_size_bytes);

  // search for a match that starts at the buffer's match offset
  //
  // this function is called at runtime during pattern matching, in a context in
  // which there can't be an incomplete match because there's enough content
  // left in the match buffer (bound check is not required).
  // this guaranteed length applies in both the forward and backward direction.
  //
  // returning true indicates a successful match, false otherwise
  //
  // the function must move the subject buffer's offset depending on the result:
  //  - SUCCESS : move the offset forward to one character past the matched content
  //  - FAILURE : (doesn't matter) 
  bool (*guaranteed_length_interpret)(subject_buffer_state* buffer, const void* data, size_t data_size_bytes);

  // search for a match anywhere in the buffer
  //
  // this is called for an expression element that is at the beginning of the pattern.
  // there is no previously matched element to build off of
  //
  // this function pointer should be NULL for elements which don't match any content (like comments)
  //
  // return INCOMPLETE if a match could be completed or extended with additional content
  // otherwise, return SUCCESS on successful match, and FAILURE on no match.
  //
  // the function must move the subject buffer's offset depending on the result:
  //  - INCOMPLETE : move to the beginning of the incomplete match
  //  - SUCCESS : move the offset forward to one character past the matched content
  //  - FAILURE : move to the end of the match buffer (buffer->size)
  match_status (*entrypoint_interpret)(subject_buffer_state* buffer, const void* data, size_t data_size_bytes);
} function_definition;

// ===================== definition for literal function (built in) ============

static function_presetup_result function_definition_for_literal_presetup(const expr_token** function_start, function_setup_info** presetup_info) {
  // literal function is special because it doesn't need to validate its arguments.
  // this is caused from the overall logic in interpret_presetup
  assert((*function_start)->type == EXPR_TOKEN_LITERAL);
  function_presetup_result ret;
  ret.success = true;
  ret.value.data_size_bytes = sizeof(CODE_UNIT);
  (*presetup_info)->function_data_size = sizeof(CODE_UNIT);
  (*presetup_info)->num_args = 0;
  (*presetup_info)->function_start = *function_start;
  (*presetup_info)++;
  (*function_start)++;
  return ret;
}

static function_setup_result function_definition_for_literal_setup(const expr_token** function_start, const function_setup_info** presetup_info, void* data, size_t data_size_bytes) {
  assert((*function_start)->type == EXPR_TOKEN_LITERAL);
  function_setup_result ret;
  ret.success = true;
  ret.value.ok.max_lookbehind_characters = 0;
  ret.value.ok.max_size_characters = 1;
  assert(data_size_bytes == sizeof(CODE_UNIT));
  #ifdef NDEBUG
    (void)(data_size_bytes);
  #endif
  *(CODE_UNIT*)data = (*function_start)->value.literal;
  (*presetup_info)++;
  (*function_start)++;
  return ret;
}

static bool function_definition_for_literal_guaranteed_length_interpret(subject_buffer_state* buffer, const void* data, size_t data_size_bytes) {
  assert(data_size_bytes == sizeof(CODE_UNIT));
  #ifdef NDEBUG
    (void)(data_size_bytes);
  #endif
  assert(subject_buffer_remaining_size(buffer) >= 1);
  return subject_buffer_start(buffer)[buffer->offset++] == *(const CODE_UNIT*)data;
}

static match_status function_definition_for_literal_interpret(subject_buffer_state* buffer, const void* data, size_t data_size_bytes) {
  size_t characters_remaining = subject_buffer_remaining_size(buffer);
  if (characters_remaining < 1) {
    return MATCH_INCOMPLETE;
  }
  bool success = function_definition_for_literal_guaranteed_length_interpret(buffer, data, data_size_bytes);
  return success ? MATCH_SUCCESS : MATCH_FAILURE;
}

static match_status function_definition_for_literal_entrypoint_interpret(subject_buffer_state* buffer, const void* data, size_t data_size_bytes) {
  assert(data_size_bytes == sizeof(CODE_UNIT));
  #ifdef NDEBUG
    (void)(data_size_bytes);
  #endif
  CODE_UNIT find = *((const CODE_UNIT*)data);
  const CODE_UNIT* ptr = code_unit_memchr(subject_buffer_offset(buffer), find, subject_buffer_remaining_size(buffer));
  if (ptr == NULL) {
    buffer->offset = buffer->size;
    return MATCH_FAILURE;
  } else {
    buffer->offset = (ptr - subject_buffer_start(buffer)) + 1;
    return MATCH_SUCCESS;
  }
}

// ptr to static lifetime
const function_definition* function_definition_for_literal() {
  static function_definition ret = {{NULL, NULL}, //
                                    function_definition_for_literal_presetup,
                                    function_definition_for_literal_setup,
                                    function_definition_for_literal_interpret,
                                    function_definition_for_literal_guaranteed_length_interpret,
                                    function_definition_for_literal_entrypoint_interpret};
  return &ret;
}

// =============== helper functions ===============

// wrapper, providing abstraction which reduces complexity of defined entrypoint function
// if true is returned, the entrypoint successfully matched, and the offset points one after the match
// false otherwise
static bool entrypoint_interpret_wrapper(subject_buffer_state* buffer, //
                                         const void* data,
                                         size_t data_size_bytes,
                                         match_status (*entrypoint_interpret)(subject_buffer_state* buffer, const void* data, size_t data_size_bytes)) {
  while (1) {
    size_t offset_before = buffer->offset;
    match_status result = entrypoint_interpret(buffer, data, data_size_bytes);
    // assertions
    if (result == MATCH_SUCCESS) {
      assert(buffer->offset >= offset_before); // must have been moved to end of match
    } else if (result == MATCH_FAILURE) {
      assert(buffer->offset == buffer->size); // moved to end of buffer
    } else if (result == MATCH_INCOMPLETE) {
      assert(offset_before == buffer->offset); // shouldn't have moved
    }
    #ifdef NDEBUG
      (void)(offset_before);
    #endif

    if (result == MATCH_SUCCESS) {
      return true;
    }

    bool input_complete = subject_buffer_shift_and_get_input(buffer); // get more input
    if (likely(!input_complete)) {
      continue; // more input has been received. try again to find a match
    }

    // no more input. this is the last segment to find a match in
    result = entrypoint_interpret(buffer, data, data_size_bytes);
    return result == MATCH_SUCCESS;
  }
}

static int function_definition_sort_comp(const void* lhs_arg, const void* rhs_arg) {
  const function_definition* lhs = (const function_definition*)lhs_arg;
  const function_definition* rhs = (const function_definition*)rhs_arg;
  return code_unit_range_cmp(lhs->name.begin, lhs->name.end, rhs->name.begin, rhs->name.end);
}

// binary search to find function with name, NULL if not found
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

// sort functions before use in interpret_presetup
void function_definition_sort(function_definition* functions, size_t num_function) {
  qsort(functions, num_function, sizeof(*functions), function_definition_sort_comp);
}

// ========================== interpret presetup ===============================

// overall return status from interpret_presetup.
// the return status should only be viewed on the first pass
typedef struct {
  bool success;

  union {
    size_t data_size_bytes;
    struct {
      size_t offset;
      size_t size; // the number of characters in the output error message
    } err;
  } value;
} interpret_presetup_result;

// needed prior to interpret_presetup
size_t interpret_presetup_get_number_of_function_calls(const expr_token* begin, //
                                                       const expr_token* const end) {
  size_t ret = 0;
  while (begin < end) {
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
typedef struct {
  const expr_token* begin;
  const expr_token* end;
  size_t num_function;
  const function_definition* functions;
  function_setup_info* presetup_info_output;
  CODE_UNIT* error_msg_output;
} interpret_presetup_arg;

interpret_presetup_result interpret_presetup(interpret_presetup_arg* arg) {
  assert(arg->begin <= arg->end);
  interpret_presetup_result ret;
  ret.value.data_size_bytes = 0;

  while (arg->begin != arg->end) {
    assert(arg->begin < arg->end);
    expr_token token = *arg->begin;

    if (token.type == EXPR_TOKEN_ENDARG) {
      break;
    }

    const function_definition* definition;
    const expr_token* begin_before_call = arg->begin;
    if (token.type == EXPR_TOKEN_LITERAL) {
      definition = function_definition_for_literal();
    } else {
      assert(token.type == EXPR_TOKEN_FUNCTION);
      // check if function has been registered
      definition = function_definition_lookup(token.value.function.name, arg->num_function, arg->functions);
      if (unlikely(definition == NULL)) {
        ret.success = false;
        ret.value.err.size = 0;
        ret.value.err.offset = token.offset;
        const CODE_UNIT* msg_prefix = CODE_UNIT_LITERAL("function name not registered: ");
        if (!arg->error_msg_output) {
          // first pass. getting size of error message
          ret.value.err.size += code_unit_strlen(msg_prefix);
          assert(token.value.function.name.end >= token.value.function.name.begin);
          ret.value.err.size += token.value.function.name.end - token.value.function.name.begin;
          ret.value.err.size += 1; // for trailing null
        } else {
          while (1) {
            CODE_UNIT cu = *msg_prefix++;
            if (cu == '\0') break;
            *arg->error_msg_output++ = cu;
          }
          const CODE_UNIT* name_pos = token.value.function.name.begin;
          while (name_pos != token.value.function.name.end) {
            *arg->error_msg_output++ = *name_pos++;
          }
          *arg->error_msg_output++ = '\0';
        }
        return ret;
      }
    }
    arg->presetup_info_output->definition = definition;
    function_presetup_result presetup_result = definition->presetup(&arg->begin, &arg->presetup_info_output);

    if (unlikely(presetup_result.success == false)) {
      ret.success = false;
      ret.value.err.size = 0;
      ret.value.err.offset = presetup_result.value.err.offset;
      const CODE_UNIT* msg_prefix = CODE_UNIT_LITERAL("function presetup error ("); //)
      if (!arg->error_msg_output) {
        ret.value.err.size += code_unit_strlen(msg_prefix);
        assert(definition->name.end >= definition->name.begin);
        ret.value.err.size += definition->name.end - definition->name.begin;
        ret.value.err.size += 3; // ):
        ret.value.err.size += strlen(presetup_result.value.err.reason);
        ret.value.err.size += 1; // for trailing null
      } else {
        while (1) {
          CODE_UNIT cu = *msg_prefix++;
          if (cu == '\0') break;
          *arg->error_msg_output++ = cu;
        }
        const CODE_UNIT* pos = definition->name.begin;
        while (pos != definition->name.end) {
          *arg->error_msg_output++ = *pos++;
        }
        *arg->error_msg_output++ = ')';
        *arg->error_msg_output++ = ':';
        *arg->error_msg_output++ = ' ';
        while (1) {
          char ch = *presetup_result.value.err.reason++;
          if (ch == '\0') break;
          *arg->error_msg_output++ = ch;
        }
        *arg->error_msg_output++ = '\0';
      }
      return ret;
    }
    // ensure that the presetup actually moved the pointer forward.
    // otherwise, this would be an infinite loop
    assert(arg->begin > begin_before_call);
    #ifdef NDEBUG
      (void)(begin_before_call);
    #endif

    ret.value.data_size_bytes += presetup_result.value.data_size_bytes;
  }

  ret.success = true;
  return ret;
}

// ========================== interpret setup ==================================

typedef struct {
  bool success;

  union {
    struct {
      size_t max_size_characters;
      size_t max_lookbehind_characters;
    } ok;
    struct {
      size_t size;   // msg. number of elements
      size_t offset; // offending location relative to expression begin
    } err;
  } value;
} interpret_setup_result;

// both presetup_info and data were populated by interpret_presetup
//
// data points to n bytes, as specified by a successful return result from
// interpret_presetup
//
// error_msg_output should point to null on first pass, and to an appropriate
// sized allocation on second pass as indicated by a first pass failure return
// result
typedef struct {
  const expr_token* begin;
  const expr_token* end;
  const function_setup_info* presetup_info;
  void* data;
  CODE_UNIT* error_msg_output;
} interpret_setup_arg;

interpret_setup_result interpret_setup(interpret_setup_arg* arg) {
  interpret_setup_result ret;
  ret.value.ok.max_lookbehind_characters = 0;
  ret.value.ok.max_size_characters = 0;
  while (arg->begin != arg->end) {
    assert(arg->begin < arg->end);
    expr_token token = *arg->begin;

    if (token.type == EXPR_TOKEN_ENDARG) {
      break;
    }

    const function_definition* definition = arg->presetup_info->definition;
    size_t data_size = arg->presetup_info->function_data_size;

    const expr_token* begin_before_call = arg->begin;
    function_setup_result result = definition->setup(&arg->begin, &arg->presetup_info, arg->data, data_size);

    if (unlikely(!result.success)) {
      ret.success = false;
      ret.value.err.size = 0;
      ret.value.err.offset = result.value.err.offset;
      const CODE_UNIT* msg_prefix = CODE_UNIT_LITERAL("function setup error ("); //)
      if (!arg->error_msg_output) {
        ret.value.err.size += code_unit_strlen(msg_prefix);
        assert(definition->name.end >= definition->name.begin);
        ret.value.err.size += definition->name.end - definition->name.begin;
        ret.value.err.size += 3; // ):
        ret.value.err.size += strlen(result.value.err.reason);
        ret.value.err.size += 1; // for trailing null
      } else {
        while (1) {
          CODE_UNIT cu = *msg_prefix++;
          if (cu == '\0') break;
          *arg->error_msg_output++ = cu;
        }
        const CODE_UNIT* pos = definition->name.begin;
        while (pos != definition->name.end) {
          *arg->error_msg_output++ = *pos++;
        }
        *arg->error_msg_output++ = ')';
        *arg->error_msg_output++ = ':';
        *arg->error_msg_output++ = ' ';
        while (1) {
          char ch = *result.value.err.reason++;
          if (ch == '\0') break;
          *arg->error_msg_output++ = ch;
        }
        *arg->error_msg_output++ = '\0';
      }
      return ret;
    }
    assert(arg->begin > begin_before_call); // ensure progress
    #ifdef NDEBUG
      (void)(begin_before_call);
    #endif

    arg->data = (char*)arg->data + data_size;

    ret.value.ok.max_size_characters += result.value.ok.max_size_characters;
    ret.value.ok.max_lookbehind_characters += result.value.ok.max_lookbehind_characters;
  }
  ret.success = true;
  return ret;
}
