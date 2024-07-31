#pragma once

#include "character/subject_buffer.h"
#include "compiler/expression/expression.h"

// IN PROGRESS

typedef enum {
  FUNCTION_SETUP_OK,
  FUNCTION_SETUP_ERR,
} function_init_status;

typedef struct {
  const char* reason;
  size_t offset;
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

// typedef enum {
  // MATCHED
// } interpret_result;

// associates a function name with it's behaviour
typedef struct function_definition {
  const CODE_UNIT* name;

  // presetup should validate arguments and indicate how much data is needed for the next phase
  function_presetup_result (*presetup)(size_t num_args, const expr_token* arg_start);

  // data points to a number of bytes indicates by the presetup.
  // this phase should do parsing and overall setup
  function_setup_result (*setup)(size_t num_args, const expr_token* arg_start, char* data);

  // this is the function used at runtime.
  // TODO more info from result
  interpret_result (*interpret)(subject_buffer_state* buffer);
};

