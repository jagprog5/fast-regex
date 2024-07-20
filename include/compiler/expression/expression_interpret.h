#pragma once

#include <expression.h>

typedef enum {
    FUNCTION_SETUP_ERROR,
    FUNCTION_SETUP_OK,
} function_setup_type;

typedef union {
    size_t capacity; // number of bytes. FUNCTION_SETUP_OK
    const char* reason; // FUNCTION_SETUP_ERROR
} function_setup_value;

typedef struct {
    function_setup_type type;
    function_setup_value value;
} function_setup_result;

// associates a function name with it's behaviour
typedef struct function_definition {
    const CODE_UNIT* name;

    // setup is called in two passes.
    // the first pass is called with data NULL
    // the second pass is called with data pointing to an allocation with size indicated by the result of the first pass
    function_setup_result (*setup)(size_t num_args, expr_token* arg_start, char* data);

    // returns TRUE if the 
    bool (*interpret)();
};

// typedef struct function_table {

// };