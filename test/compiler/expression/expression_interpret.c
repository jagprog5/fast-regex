#include "compiler/expression/expression_interpret.h"
#include "compiler/expression/expression_interpret_standardlib/arith.h"

#include "test_common.h"
extern int has_errors;

int main() {
  { // simple literals
    const CODE_UNIT* program = CODE_UNIT_LITERAL("abcd");
    expr_tokenize_arg arg = expr_tokenize_arg_init(program, program + code_unit_strlen(program));
    expr_tokenize_result cap = tokenize_expression(&arg);
    assert_continue(cap.reason == NULL);
    size_t output_size = expr_tokenize_arg_get_cap(&arg);
    assert_continue(output_size == 4);
    expr_token tokens[output_size];
    expr_tokenize_arg_set_to_fill(&arg, tokens);
    cap = tokenize_expression(&arg);
    assert_continue(cap.reason == NULL);
    assert_continue(arg.out - tokens == output_size);

    size_t num_function_calls = interpret_presetup_get_number_of_function_calls(tokens, tokens + output_size);

    function_setup_info presetup_info[num_function_calls];

    interpret_presetup_arg presetup_arg;
    presetup_arg.begin = tokens;
    presetup_arg.end = tokens + output_size;
    presetup_arg.error_msg_output = NULL;
    presetup_arg.functions = function_definition_for_literal();
    presetup_arg.num_function = 1;
    presetup_arg.presetup_info_output = presetup_info;

    interpret_presetup_result presetup_result = interpret_presetup(&presetup_arg);

    assert_continue(presetup_result.success);
    assert_continue(presetup_result.value.data_size_bytes == sizeof(CODE_UNIT) * 4);

    char data[presetup_result.value.data_size_bytes];

    interpret_setup_arg setup_arg;
    setup_arg.begin = tokens;
    setup_arg.data = data;
    setup_arg.end = tokens + output_size;
    setup_arg.error_msg_output = NULL;
    setup_arg.presetup_info = presetup_info;

    interpret_setup_result setup_result = interpret_setup(&setup_arg);
    assert_continue(setup_result.success);
    assert_continue(setup_result.value.ok.max_size_characters == 4);
    assert_continue(setup_result.value.ok.max_lookbehind_characters == 0);
  }
  { // empty
    assert_continue(interpret_presetup_get_number_of_function_calls(NULL, NULL) == 0);
    interpret_presetup_arg presetup_arg;
    memset(&presetup_arg, 0, sizeof(presetup_arg));

    interpret_presetup_result presetup_result = interpret_presetup(&presetup_arg);
    assert_continue(presetup_result.success);
    assert_continue(presetup_result.value.data_size_bytes == 0);

    interpret_setup_arg setup_arg;
    memset(&setup_arg, 0, sizeof(setup_arg));
    interpret_setup_result setup_result = interpret_setup(&setup_arg);
    assert_continue(setup_result.success);
    assert_continue(setup_result.value.ok.max_size_characters == 0);
    assert_continue(setup_result.value.ok.max_lookbehind_characters == 0);
  }
  { // unknown function
    const CODE_UNIT* program = CODE_UNIT_LITERAL(" {name_not_registered,abc,123}");
    expr_tokenize_arg arg = expr_tokenize_arg_init(program, program + code_unit_strlen(program));
    expr_tokenize_result cap = tokenize_expression(&arg);
    assert_continue(cap.reason == NULL);
    size_t output_size = expr_tokenize_arg_get_cap(&arg);
    expr_token tokens[output_size];
    expr_tokenize_arg_set_to_fill(&arg, tokens);
    cap = tokenize_expression(&arg);
    assert_continue(cap.reason == NULL);
    assert_continue(arg.out - tokens == output_size);

    size_t num_function_calls = interpret_presetup_get_number_of_function_calls(tokens, tokens + output_size);

    function_setup_info presetup_info[num_function_calls];
    interpret_presetup_arg presetup_arg;
    presetup_arg.begin = tokens;
    presetup_arg.end = tokens + output_size;
    presetup_arg.error_msg_output = NULL;
    presetup_arg.functions = function_definition_for_literal();
    presetup_arg.num_function = 1;
    presetup_arg.presetup_info_output = presetup_info;

    interpret_presetup_arg presetup_arg_copy = presetup_arg;

    interpret_presetup_result presetup_result = interpret_presetup(&presetup_arg);

    assert_continue(presetup_result.success == false);
    const CODE_UNIT* msg = CODE_UNIT_LITERAL("function name not registered: name_not_registered");
    assert_continue(presetup_result.value.err.size == code_unit_strlen(msg) + 1);

    CODE_UNIT error_msg[presetup_result.value.err.size];
    presetup_arg_copy.error_msg_output = error_msg;
    interpret_presetup(&presetup_arg_copy);
    assert_continue(0 == code_unit_strcmp(error_msg, msg));
    assert_continue(presetup_result.value.err.offset == 1);
  }

  { // presetup of function failed
    // this expression fails at the tokenization step (invalid symbol)
    const CODE_UNIT* program = CODE_UNIT_LITERAL("{arith, abc < 9}");
    expr_tokenize_arg arg = expr_tokenize_arg_init(program, program + code_unit_strlen(program));
    expr_tokenize_result cap = tokenize_expression(&arg);
    assert_continue(cap.reason == NULL);
    size_t output_size = expr_tokenize_arg_get_cap(&arg);
    expr_token tokens[output_size];
    expr_tokenize_arg_set_to_fill(&arg, tokens);
    cap = tokenize_expression(&arg);
    assert_continue(cap.reason == NULL);
    assert_continue(arg.out - tokens == output_size);

    size_t num_function_calls = interpret_presetup_get_number_of_function_calls(tokens, tokens + output_size);

    function_setup_info presetup_info[num_function_calls];
    interpret_presetup_arg presetup_arg;
    presetup_arg.begin = tokens;
    presetup_arg.end = tokens + output_size;
    presetup_arg.error_msg_output = NULL;
    function_definition definitions[2] = {
      *function_definition_for_literal(),
      *function_definition_for_arith()
    };
    size_t num_functions = sizeof(definitions) / sizeof(*definitions);
    function_definition_sort(definitions, num_functions);
    presetup_arg.functions = definitions;
    presetup_arg.num_function = num_functions;
    presetup_arg.presetup_info_output = presetup_info;

    interpret_presetup_arg presetup_arg_copy = presetup_arg;
    interpret_presetup_result presetup_result = interpret_presetup(&presetup_arg);
    assert_continue(presetup_result.success == false);
    const CODE_UNIT* msg = CODE_UNIT_LITERAL("function presetup error (arith): invalid symbol");
    assert_continue(presetup_result.value.err.size == code_unit_strlen(msg) + 1);

    CODE_UNIT error_msg[presetup_result.value.err.size];
    presetup_arg_copy.error_msg_output = error_msg;
    interpret_presetup(&presetup_arg_copy);
    assert_continue(0 == code_unit_strcmp(error_msg, msg));
    assert_continue(presetup_result.value.err.offset == 8);
  }

  { // setup of function failed
    // this expression fails at the parse step
    const CODE_UNIT* program = CODE_UNIT_LITERAL("{arith, 0 * * 9}");
    expr_tokenize_arg arg = expr_tokenize_arg_init(program, program + code_unit_strlen(program));
    expr_tokenize_result cap = tokenize_expression(&arg);
    assert_continue(cap.reason == NULL);
    size_t output_size = expr_tokenize_arg_get_cap(&arg);
    expr_token tokens[output_size];
    expr_tokenize_arg_set_to_fill(&arg, tokens);
    cap = tokenize_expression(&arg);
    assert_continue(cap.reason == NULL);
    assert_continue(arg.out - tokens == output_size);

    size_t num_function_calls = interpret_presetup_get_number_of_function_calls(tokens, tokens + output_size);

    function_setup_info presetup_info[num_function_calls];
    interpret_presetup_arg presetup_arg;
    presetup_arg.begin = tokens;
    presetup_arg.end = tokens + output_size;
    presetup_arg.error_msg_output = NULL;
    function_definition definitions[2] = {
      *function_definition_for_literal(),
      *function_definition_for_arith()
    };
    size_t num_functions = sizeof(definitions) / sizeof(*definitions);
    function_definition_sort(definitions, num_functions);
    presetup_arg.functions = definitions;
    presetup_arg.num_function = num_functions;
    presetup_arg.presetup_info_output = presetup_info;

    interpret_presetup_arg presetup_arg_copy = presetup_arg;
    interpret_presetup_result presetup_result = interpret_presetup(&presetup_arg);
    assert_continue(presetup_result.success);

    char data[presetup_result.value.data_size_bytes];

    interpret_setup_arg setup_arg;
    setup_arg.begin = tokens;
    setup_arg.data = data;
    setup_arg.end = tokens + output_size;
    setup_arg.error_msg_output = NULL;
    setup_arg.presetup_info = presetup_info;

    interpret_setup_arg setup_arg_copy = setup_arg;
    interpret_setup_result setup_result = interpret_setup(&setup_arg);
    assert_continue(setup_result.success == false);
    assert_continue(setup_result.value.err.offset == 10);

    const CODE_UNIT* msg = "function setup error (arith): expression stack exhausted by op";
    assert_continue(setup_result.value.err.size == code_unit_strlen(msg) + 1);

    CODE_UNIT error_msg[setup_result.value.err.size];
    setup_arg_copy.error_msg_output = error_msg;
    interpret_setup(&setup_arg_copy);
    assert_continue(0 == code_unit_strcmp(error_msg, msg));
  }

  return has_errors;
}