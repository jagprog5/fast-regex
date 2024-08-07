#include <assert.h>
#include <stdbool.h>

#define TESTING_HOOK
bool marker_increment_check = false;

#include "compiler/expression/expression.h"

#include "test_common.h"
extern int has_errors;

int main(void) {
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

    for (size_t i = 0; i < output_size; ++i) {
      assert_continue(tokens[i].type == EXPR_TOKEN_LITERAL);
      assert_continue(tokens[i].offset == i);
      assert_continue(tokens[i].data.literal == 'a' + i);
    }
  }
  {
    const CODE_UNIT* program = CODE_UNIT_LITERAL("\\");
    expr_tokenize_arg arg = expr_tokenize_arg_init(program, program + code_unit_strlen(program));
    expr_tokenize_result cap = tokenize_expression(&arg);
    assert_continue(0 == strcmp(cap.reason, "literal ended without content"));
    assert_continue(cap.offset == 0);
  }
  {
    const CODE_UNIT* program = CODE_UNIT_LITERAL("\\q");
    expr_tokenize_arg arg = expr_tokenize_arg_init(program, program + code_unit_strlen(program));
    expr_tokenize_result cap = tokenize_expression(&arg);
    assert_continue(0 == strcmp(cap.reason, "invalid escaped character"));
    assert_continue(cap.offset == 1);
  }
  {
    const CODE_UNIT* program = CODE_UNIT_LITERAL("\\n\\{");
    expr_tokenize_arg arg = expr_tokenize_arg_init(program, program + code_unit_strlen(program));
    expr_tokenize_result cap = tokenize_expression(&arg);
    assert_continue(cap.reason == NULL);
    size_t output_size = expr_tokenize_arg_get_cap(&arg);
    assert_continue(output_size == 2);
    expr_token tokens[output_size];
    expr_tokenize_arg_set_to_fill(&arg, tokens);
    cap = tokenize_expression(&arg);
    assert_continue(cap.reason == NULL);
    assert_continue(arg.out - tokens == output_size);

    assert_continue(tokens[0].type == EXPR_TOKEN_LITERAL);
    assert_continue(tokens[0].data.literal == '\n');
    assert_continue(tokens[0].offset == 0);
    assert_continue(tokens[1].type == EXPR_TOKEN_LITERAL);
    assert_continue(tokens[1].data.literal == '{');
    assert_continue(tokens[1].offset == 2);
  }
  {
    const CODE_UNIT* program = CODE_UNIT_LITERAL("}");
    expr_tokenize_arg arg = expr_tokenize_arg_init(program, program + code_unit_strlen(program));
    expr_tokenize_result cap = tokenize_expression(&arg);
    assert_continue(0 == strcmp(cap.reason, "must be escaped"));
    assert_continue(cap.offset == 0);
  }
  { // abrupt end during function name
    const CODE_UNIT* program = CODE_UNIT_LITERAL("{");
    expr_tokenize_arg arg = expr_tokenize_arg_init(program, program + code_unit_strlen(program));
    expr_tokenize_result cap = tokenize_expression(&arg);
    assert_continue(0 == strcmp(cap.reason, "function name ended abruptly"));
    assert_continue(cap.offset == 0);
  }
  { // abrupt end during function arg
    const CODE_UNIT* program = CODE_UNIT_LITERAL("{,");
    expr_tokenize_arg arg = expr_tokenize_arg_init(program, program + code_unit_strlen(program));
    expr_tokenize_result cap = tokenize_expression(&arg);
    assert_continue(0 == strcmp(cap.reason, "expecting function end"));
    assert_continue(cap.offset == 2); // points at end
  }
  {
    const CODE_UNIT* program = CODE_UNIT_LITERAL("{}");
    expr_tokenize_arg arg = expr_tokenize_arg_init(program, program + code_unit_strlen(program));
    expr_tokenize_result cap = tokenize_expression(&arg);
    assert_continue(cap.reason == NULL);
    size_t output_size = expr_tokenize_arg_get_cap(&arg);
    assert_continue(output_size == 1);
    expr_token tokens[output_size];
    expr_tokenize_arg_set_to_fill(&arg, tokens);
    cap = tokenize_expression(&arg);
    assert_continue(cap.reason == NULL);
    assert_continue(arg.out - tokens == output_size);

    assert_continue(tokens[0].type == EXPR_TOKEN_FUNCTION);
    assert_continue(tokens[0].offset == 0);
    assert_continue(tokens[0].data.function.name.begin == tokens[0].data.function.name.end);
    assert_continue(tokens[0].data.function.num_args == 0);
  }
  {
    const CODE_UNIT* program = CODE_UNIT_LITERAL("{abcd}");
    expr_tokenize_arg arg = expr_tokenize_arg_init(program, program + code_unit_strlen(program));
    expr_tokenize_result cap = tokenize_expression(&arg);
    assert_continue(cap.reason == NULL);
    size_t output_size = expr_tokenize_arg_get_cap(&arg);
    assert_continue(output_size == 1);
    expr_token tokens[output_size];
    expr_tokenize_arg_set_to_fill(&arg, tokens);
    cap = tokenize_expression(&arg);
    assert_continue(cap.reason == NULL);
    assert_continue(arg.out - tokens == output_size);

    assert_continue(tokens[0].type == EXPR_TOKEN_FUNCTION);
    assert_continue(tokens[0].offset == 0);
    assert_continue(0 == code_unit_range_equal2(tokens[0].data.function.name.begin, tokens[0].data.function.name.end, CODE_UNIT_LITERAL("abcd")));
    assert_continue(tokens[0].data.function.num_args == 0);
  }
  {
    const CODE_UNIT* program = CODE_UNIT_LITERAL("{abcd,}");
    expr_tokenize_arg arg = expr_tokenize_arg_init(program, program + code_unit_strlen(program));
    expr_tokenize_result cap = tokenize_expression(&arg);
    assert_continue(cap.reason == NULL);
    size_t output_size = expr_tokenize_arg_get_cap(&arg);
    assert_continue(output_size == 2);
    expr_token tokens[output_size];
    expr_tokenize_arg_set_to_fill(&arg, tokens);
    cap = tokenize_expression(&arg);
    assert_continue(cap.reason == NULL);
    assert_continue(arg.out - tokens == output_size);

    assert_continue(tokens[0].type == EXPR_TOKEN_FUNCTION);
    assert_continue(tokens[0].offset == 0);
    assert_continue(0 == code_unit_range_equal2(tokens[0].data.function.name.begin, tokens[0].data.function.name.end, CODE_UNIT_LITERAL("abcd")));
    assert_continue(tokens[0].data.function.num_args == 1);
    assert_continue(tokens[1].type == EXPR_TOKEN_ENDARG);
  }
  {
    const CODE_UNIT* program = CODE_UNIT_LITERAL("{ab,12}");
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

    assert_continue(tokens[0].type == EXPR_TOKEN_FUNCTION);
    assert_continue(tokens[0].offset == 0);
    assert_continue(0 == code_unit_range_equal2(tokens[0].data.function.name.begin, tokens[0].data.function.name.end, CODE_UNIT_LITERAL("ab")));
    assert_continue(tokens[0].data.function.num_args == 1);

    assert_continue(tokens[1].offset == 4);
    assert_continue(tokens[1].type == EXPR_TOKEN_LITERAL);
    assert_continue(tokens[1].data.literal == '1');

    assert_continue(tokens[2].offset == 5);
    assert_continue(tokens[2].type == EXPR_TOKEN_LITERAL);
    assert_continue(tokens[2].data.literal == '2');

    assert_continue(tokens[3].offset == 6);
    assert_continue(tokens[3].type == EXPR_TOKEN_ENDARG);
  }
  {
    const CODE_UNIT* program = CODE_UNIT_LITERAL("{ab,\\,\\}}");
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

    assert_continue(tokens[0].offset == 0);
    assert_continue(tokens[0].type == EXPR_TOKEN_FUNCTION);
    assert_continue(0 == code_unit_range_equal2(tokens[0].data.function.name.begin, tokens[0].data.function.name.end, CODE_UNIT_LITERAL("ab")));
    assert_continue(tokens[0].data.function.num_args == 1);

    assert_continue(tokens[1].offset == 4);
    assert_continue(tokens[1].type == EXPR_TOKEN_LITERAL);
    assert_continue(tokens[1].data.literal == ',');

    assert_continue(tokens[2].offset == 6);
    assert_continue(tokens[2].type == EXPR_TOKEN_LITERAL);
    assert_continue(tokens[2].data.literal == '}');

    assert_continue(tokens[3].offset == 8);
    assert_continue(tokens[3].type == EXPR_TOKEN_ENDARG);
  }
  {
    const CODE_UNIT* program = CODE_UNIT_LITERAL("{ab,12,34}");
    expr_tokenize_arg arg = expr_tokenize_arg_init(program, program + code_unit_strlen(program));
    expr_tokenize_result cap = tokenize_expression(&arg);
    assert_continue(cap.reason == NULL);
    size_t output_size = expr_tokenize_arg_get_cap(&arg);
    assert_continue(output_size == 7);
    expr_token tokens[output_size];
    expr_tokenize_arg_set_to_fill(&arg, tokens);
    cap = tokenize_expression(&arg);
    assert_continue(cap.reason == NULL);
    assert_continue(arg.out - tokens == output_size);

    assert_continue(tokens[0].offset == 0);
    assert_continue(tokens[0].type == EXPR_TOKEN_FUNCTION);
    assert_continue(0 == code_unit_range_equal2(tokens[0].data.function.name.begin, tokens[0].data.function.name.end, CODE_UNIT_LITERAL("ab")));
    assert_continue(tokens[0].data.function.num_args == 2);

    assert_continue(tokens[1].offset == 4);
    assert_continue(tokens[1].type == EXPR_TOKEN_LITERAL);
    assert_continue(tokens[1].data.literal == '1');

    assert_continue(tokens[2].offset == 5);
    assert_continue(tokens[2].type == EXPR_TOKEN_LITERAL);
    assert_continue(tokens[2].data.literal == '2');

    assert_continue(tokens[3].offset == 6);
    assert_continue(tokens[3].type == EXPR_TOKEN_ENDARG);

    assert_continue(tokens[4].offset == 7);
    assert_continue(tokens[4].type == EXPR_TOKEN_LITERAL);
    assert_continue(tokens[4].data.literal == '3');

    assert_continue(tokens[5].offset == 8);
    assert_continue(tokens[5].type == EXPR_TOKEN_LITERAL);
    assert_continue(tokens[5].data.literal == '4');

    assert_continue(tokens[6].offset == 9);
    assert_continue(tokens[6].type == EXPR_TOKEN_ENDARG);
  }
  {
    const CODE_UNIT* program = CODE_UNIT_LITERAL("start{outer,ab,{inner,cd},ef}");
    expr_tokenize_arg arg = expr_tokenize_arg_init(program, program + code_unit_strlen(program));
    expr_tokenize_result cap = tokenize_expression(&arg);
    assert_continue(cap.reason == NULL);
    size_t output_size = expr_tokenize_arg_get_cap(&arg);
    assert_continue(output_size == 17);
    expr_token tokens[output_size];
    expr_tokenize_arg_set_to_fill(&arg, tokens);
    cap = tokenize_expression(&arg);
    assert_continue(cap.reason == NULL);
    assert_continue(arg.out - tokens == output_size);

    assert_continue(tokens[0].type == EXPR_TOKEN_LITERAL);
    assert_continue(tokens[0].data.literal == 's');

    assert_continue(tokens[1].type == EXPR_TOKEN_LITERAL);
    assert_continue(tokens[1].data.literal == 't');

    assert_continue(tokens[2].type == EXPR_TOKEN_LITERAL);
    assert_continue(tokens[2].data.literal == 'a');

    assert_continue(tokens[3].type == EXPR_TOKEN_LITERAL);
    assert_continue(tokens[3].data.literal == 'r');

    assert_continue(tokens[4].type == EXPR_TOKEN_LITERAL);
    assert_continue(tokens[4].data.literal == 't');

    assert_continue(tokens[5].type == EXPR_TOKEN_FUNCTION);
    assert_continue(0 == code_unit_range_equal2(tokens[5].data.function.name.begin, tokens[5].data.function.name.end, CODE_UNIT_LITERAL("outer")));
    assert_continue(tokens[5].data.function.num_args == 3);

    assert_continue(tokens[6].type == EXPR_TOKEN_LITERAL);
    assert_continue(tokens[6].data.literal == 'a');

    assert_continue(tokens[7].type == EXPR_TOKEN_LITERAL);
    assert_continue(tokens[7].data.literal == 'b');

    assert_continue(tokens[8].type == EXPR_TOKEN_ENDARG);

    assert_continue(tokens[9].offset == 15);
    assert_continue(tokens[9].type == EXPR_TOKEN_FUNCTION);
    assert_continue(0 == code_unit_range_equal2(tokens[9].data.function.name.begin, tokens[9].data.function.name.end, CODE_UNIT_LITERAL("inner")));
    assert_continue(tokens[9].data.function.num_args == 1);

    assert_continue(tokens[10].offset == 22);
    assert_continue(tokens[10].type == EXPR_TOKEN_LITERAL);
    assert_continue(tokens[10].data.literal == 'c');

    assert_continue(tokens[11].type == EXPR_TOKEN_LITERAL);
    assert_continue(tokens[11].data.literal == 'd');

    assert_continue(tokens[12].type == EXPR_TOKEN_ENDARG);
    assert_continue(tokens[13].type == EXPR_TOKEN_ENDARG);

    assert_continue(tokens[14].type == EXPR_TOKEN_LITERAL);
    assert_continue(tokens[14].data.literal == 'e');

    assert_continue(tokens[15].type == EXPR_TOKEN_LITERAL);
    assert_continue(tokens[15].data.literal == 'f');

    assert_continue(tokens[16].type == EXPR_TOKEN_ENDARG);
  }
  {
    const CODE_UNIT* program = CODE_UNIT_LITERAL("\\x");
    expr_tokenize_arg arg = expr_tokenize_arg_init(program, program + code_unit_strlen(program));
    expr_tokenize_result cap = tokenize_expression(&arg);
    assert_continue(0 == strcmp(cap.reason, "expecting content for hex literal"));
    assert_continue(cap.offset == 2);
  }
  {
    const CODE_UNIT* program = CODE_UNIT_LITERAL("\\xq");
    expr_tokenize_arg arg = expr_tokenize_arg_init(program, program + code_unit_strlen(program));
    expr_tokenize_result cap = tokenize_expression(&arg);
    assert_continue(0 == strcmp(cap.reason, "expecting '{', hex syntax is: \\x{abc}"));
    assert_continue(cap.offset == 2);
  }
  {
    const CODE_UNIT* program = CODE_UNIT_LITERAL("\\x{");
    expr_tokenize_arg arg = expr_tokenize_arg_init(program, program + code_unit_strlen(program));
    expr_tokenize_result cap = tokenize_expression(&arg);
    assert_continue(0 == strcmp(cap.reason, "hex content not finished"));
    assert_continue(cap.offset == 3);
  }
  {
    const CODE_UNIT* program = CODE_UNIT_LITERAL("\\x{a");
    expr_tokenize_arg arg = expr_tokenize_arg_init(program, program + code_unit_strlen(program));
    expr_tokenize_result cap = tokenize_expression(&arg);
    assert_continue(0 == strcmp(cap.reason, "hex content not finished"));
    assert_continue(cap.offset == 4);
  }
  {
    const CODE_UNIT* program = CODE_UNIT_LITERAL("\\x{q");
    expr_tokenize_arg arg = expr_tokenize_arg_init(program, program + code_unit_strlen(program));
    expr_tokenize_result cap = tokenize_expression(&arg);
    assert_continue(0 == strcmp(cap.reason, "invalid hex escaped character"));
    assert_continue(cap.offset == 3);
  }
  {
    const CODE_UNIT* program = CODE_UNIT_LITERAL("\\x{aaaaaaaaa}");
    expr_tokenize_arg arg = expr_tokenize_arg_init(program, program + code_unit_strlen(program));
    expr_tokenize_result cap = tokenize_expression(&arg);
    assert_continue(0 == strcmp(cap.reason, "hex content overlong. expecting '}'"));
    assert_continue(cap.offset == 11);
  }
  {
    const CODE_UNIT* program = CODE_UNIT_LITERAL("\\x{aaaaaaaa}");
    expr_tokenize_arg arg = expr_tokenize_arg_init(program, program + code_unit_strlen(program));
    expr_tokenize_result cap = tokenize_expression(&arg);
    assert_continue(cap.reason == NULL);
    size_t output_size = expr_tokenize_arg_get_cap(&arg);
    assert_continue(output_size == 1);
    expr_token tokens[output_size];
    expr_tokenize_arg_set_to_fill(&arg, tokens);
    cap = tokenize_expression(&arg);
    assert_continue(cap.reason == NULL);
    assert_continue(arg.out - tokens == output_size);

    assert_continue(tokens[0].type == EXPR_TOKEN_LITERAL);
    assert_continue(tokens[0].offset == 0);
    assert_continue(tokens[0].data.literal == (CODE_UNIT)0xaaaaaaaa);
  }
  {
    const CODE_UNIT* program = CODE_UNIT_LITERAL("\\x{aa}");
    expr_tokenize_arg arg = expr_tokenize_arg_init(program, program + code_unit_strlen(program));
    expr_tokenize_result cap = tokenize_expression(&arg);
    assert_continue(cap.reason == NULL);
    size_t output_size = expr_tokenize_arg_get_cap(&arg);
    assert_continue(output_size == 1);
    expr_token tokens[output_size];
    expr_tokenize_arg_set_to_fill(&arg, tokens);
    cap = tokenize_expression(&arg);
    assert_continue(cap.reason == NULL);
    assert_continue(arg.out - tokens == output_size);

    assert_continue(tokens[0].type == EXPR_TOKEN_LITERAL);
    assert_continue(tokens[0].offset == 0);
    assert_continue(tokens[0].data.literal == (CODE_UNIT)0xaa);
  }
  if (has_errors) return has_errors;

  { // marker
    const CODE_UNIT* program = CODE_UNIT_LITERAL("{0}");
    expr_tokenize_arg arg = expr_tokenize_arg_init(program, program + code_unit_strlen(program));
    expr_tokenize_result cap = tokenize_expression(&arg);
    assert_continue(cap.reason == NULL);
    size_t output_size = expr_tokenize_arg_get_cap(&arg);
    assert_continue(output_size == 1);
    expr_token tokens[output_size];
    expr_tokenize_arg_set_to_fill(&arg, tokens);
    cap = tokenize_expression(&arg);
    assert_continue(cap.reason == NULL);
    assert_continue(arg.out - tokens == output_size);

    assert_continue(tokens[0].type == EXPR_TOKEN_FUNCTION);
    assert_continue(tokens[0].offset == 0);
    assert_continue(tokens[0].data.function.name.begin == tokens[0].data.function.name.end);
    assert_continue(tokens[0].data.function.begin_marker.marker_number == 0);
    assert_continue(tokens[0].data.function.begin_marker.offset == 0);
    assert_continue(tokens[0].data.function.begin_marker.present == true);
    assert_continue(tokens[0].data.function.end_marker.present == false);
  }

  { // marker
    const CODE_UNIT* program = CODE_UNIT_LITERAL("{512abc}");
    expr_tokenize_arg arg = expr_tokenize_arg_init(program, program + code_unit_strlen(program));
    expr_tokenize_result cap = tokenize_expression(&arg);
    assert_continue(cap.reason == NULL);
    size_t output_size = expr_tokenize_arg_get_cap(&arg);
    assert_continue(output_size == 1);
    expr_token tokens[output_size];
    expr_tokenize_arg_set_to_fill(&arg, tokens);
    cap = tokenize_expression(&arg);
    assert_continue(cap.reason == NULL);
    assert_continue(arg.out - tokens == output_size);

    assert_continue(tokens[0].type == EXPR_TOKEN_FUNCTION);
    assert_continue(tokens[0].offset == 0);
    assert_continue(0 == code_unit_range_equal2(tokens[0].data.function.name.begin, tokens[0].data.function.name.end, CODE_UNIT_LITERAL("abc")));
    assert_continue(tokens[0].data.function.begin_marker.marker_number == 512);
    assert_continue(tokens[0].data.function.begin_marker.offset == 0);
    assert_continue(tokens[0].data.function.begin_marker.present == true);
    assert_continue(tokens[0].data.function.end_marker.present == false);
  }

  { // marker number overflow
    const CODE_UNIT* program = CODE_UNIT_LITERAL("{999999999999999999999999999999999999999999}");
    expr_tokenize_arg arg = expr_tokenize_arg_init(program, program + code_unit_strlen(program));
    expr_tokenize_result cap = tokenize_expression(&arg);
    assert_continue(0 == strcmp(cap.reason, "overflow on marker begin value"));
    assert_continue(cap.offset == 0);
  }
  {
    const CODE_UNIT* program = CODE_UNIT_LITERAL("{0-tester}");
    expr_tokenize_arg arg = expr_tokenize_arg_init(program, program + code_unit_strlen(program));
    expr_tokenize_result cap = tokenize_expression(&arg);
    assert_continue(0 == strcmp(cap.reason, "marker offset must be non-empty if following sign"));
    assert_continue(cap.offset == 3);
  }

  { // marker
    const CODE_UNIT* program = CODE_UNIT_LITERAL("{512-3abc}");
    expr_tokenize_arg arg = expr_tokenize_arg_init(program, program + code_unit_strlen(program));
    expr_tokenize_result cap = tokenize_expression(&arg);
    assert_continue(cap.reason == NULL);
    size_t output_size = expr_tokenize_arg_get_cap(&arg);
    assert_continue(output_size == 1);
    expr_token tokens[output_size];
    expr_tokenize_arg_set_to_fill(&arg, tokens);
    cap = tokenize_expression(&arg);
    assert_continue(cap.reason == NULL);
    assert_continue(arg.out - tokens == output_size);

    assert_continue(tokens[0].type == EXPR_TOKEN_FUNCTION);
    assert_continue(tokens[0].offset == 0);
    assert_continue(tokens[0].data.function.begin_marker.marker_number == 512);
    assert_continue(tokens[0].data.function.begin_marker.offset == -3);
    assert_continue(0 == code_unit_range_equal2(tokens[0].data.function.name.begin, tokens[0].data.function.name.end, CODE_UNIT_LITERAL("abc")));
    assert_continue(tokens[0].data.function.begin_marker.present == true);
    assert_continue(tokens[0].data.function.end_marker.present == false);
  }

  { // marker
    const CODE_UNIT* program = CODE_UNIT_LITERAL("{512+99abc}");
    expr_tokenize_arg arg = expr_tokenize_arg_init(program, program + code_unit_strlen(program));
    expr_tokenize_result cap = tokenize_expression(&arg);
    assert_continue(cap.reason == NULL);
    size_t output_size = expr_tokenize_arg_get_cap(&arg);
    assert_continue(output_size == 1);
    expr_token tokens[output_size];
    expr_tokenize_arg_set_to_fill(&arg, tokens);
    cap = tokenize_expression(&arg);
    assert_continue(cap.reason == NULL);
    assert_continue(arg.out - tokens == output_size);

    assert_continue(tokens[0].type == EXPR_TOKEN_FUNCTION);
    assert_continue(tokens[0].offset == 0);
    assert_continue(tokens[0].data.function.begin_marker.marker_number == 512);
    assert_continue(tokens[0].data.function.begin_marker.offset == 99);
    assert_continue(0 == code_unit_range_equal2(tokens[0].data.function.name.begin, tokens[0].data.function.name.end, CODE_UNIT_LITERAL("abc")));
    assert_continue(tokens[0].data.function.begin_marker.present == true);
    assert_continue(tokens[0].data.function.end_marker.present == false);
  }

  {
    const CODE_UNIT* program = CODE_UNIT_LITERAL("{0-999999999999999999999999999999999999999999tester}");
    expr_tokenize_arg arg = expr_tokenize_arg_init(program, program + code_unit_strlen(program));
    expr_tokenize_result cap = tokenize_expression(&arg);
    assert_continue(0 == strcmp(cap.reason, "overflow on marker begin offset value"));
    assert_continue(cap.offset == 0);
  }

  { // marker
    const CODE_UNIT* program = CODE_UNIT_LITERAL("{abc512}");
    expr_tokenize_arg arg = expr_tokenize_arg_init(program, program + code_unit_strlen(program));
    expr_tokenize_result cap = tokenize_expression(&arg);
    assert_continue(cap.reason == NULL);
    size_t output_size = expr_tokenize_arg_get_cap(&arg);
    assert_continue(output_size == 1);
    expr_token tokens[output_size];
    expr_tokenize_arg_set_to_fill(&arg, tokens);
    cap = tokenize_expression(&arg);
    assert_continue(cap.reason == NULL);
    assert_continue(arg.out - tokens == output_size);

    assert_continue(tokens[0].type == EXPR_TOKEN_FUNCTION);
    assert_continue(tokens[0].offset == 0);
    assert_continue(0 == code_unit_range_equal2(tokens[0].data.function.name.begin, tokens[0].data.function.name.end, CODE_UNIT_LITERAL("abc")));
    assert_continue(tokens[0].data.function.end_marker.present == true);
    assert_continue(tokens[0].data.function.end_marker.marker_number == 512);
    assert_continue(tokens[0].data.function.end_marker.offset == 0);
    assert_continue(tokens[0].data.function.begin_marker.present == false);
  }

  {
    const CODE_UNIT* program = CODE_UNIT_LITERAL("{abc5");
    expr_tokenize_arg arg = expr_tokenize_arg_init(program, program + code_unit_strlen(program));
    expr_tokenize_result cap = tokenize_expression(&arg);
    assert_continue(0 == strcmp(cap.reason, "function name ended abruptly"));
    assert_continue(cap.offset == 0);
  }

  {
    const CODE_UNIT* program = CODE_UNIT_LITERAL("{abc5a}");
    expr_tokenize_arg arg = expr_tokenize_arg_init(program, program + code_unit_strlen(program));
    expr_tokenize_result cap = tokenize_expression(&arg);
    assert_continue(0 == strcmp(cap.reason, "unexpected character in end marker"));
    assert_continue(cap.offset == 5);
  }

  {
    const CODE_UNIT* program = CODE_UNIT_LITERAL("{abc5-,}");
    expr_tokenize_arg arg = expr_tokenize_arg_init(program, program + code_unit_strlen(program));
    expr_tokenize_result cap = tokenize_expression(&arg);
    assert_continue(0 == strcmp(cap.reason, "unexpected character in end marker offset start"));
    assert_continue(cap.offset == 6);
  }

  {
    const CODE_UNIT* program = CODE_UNIT_LITERAL("{abc5-z}");
    expr_tokenize_arg arg = expr_tokenize_arg_init(program, program + code_unit_strlen(program));
    expr_tokenize_result cap = tokenize_expression(&arg);
    assert_continue(0 == strcmp(cap.reason, "unexpected character in end marker offset start"));
    assert_continue(cap.offset == 6);
  }

  {
    const CODE_UNIT* program = CODE_UNIT_LITERAL("{abc999999999999999999999999999999999999999999}");
    expr_tokenize_arg arg = expr_tokenize_arg_init(program, program + code_unit_strlen(program));
    expr_tokenize_result cap = tokenize_expression(&arg);
    assert_continue(0 == strcmp(cap.reason, "overflow on marker end value"));
    assert_continue(cap.offset == 0);
  }

  {
    const CODE_UNIT* program = CODE_UNIT_LITERAL("{abc5-3}");
    expr_tokenize_arg arg = expr_tokenize_arg_init(program, program + code_unit_strlen(program));
    expr_tokenize_result cap = tokenize_expression(&arg);
    assert_continue(cap.reason == NULL);
    size_t output_size = expr_tokenize_arg_get_cap(&arg);
    assert_continue(output_size == 1);
    expr_token tokens[output_size];
    expr_tokenize_arg_set_to_fill(&arg, tokens);
    cap = tokenize_expression(&arg);
    assert_continue(cap.reason == NULL);
    assert_continue(arg.out - tokens == output_size);

    assert_continue(tokens[0].type == EXPR_TOKEN_FUNCTION);
    assert_continue(tokens[0].offset == 0);
    assert_continue(0 == code_unit_range_equal2(tokens[0].data.function.name.begin, tokens[0].data.function.name.end, CODE_UNIT_LITERAL("abc")));
    assert_continue(tokens[0].data.function.end_marker.present == true);
    assert_continue(tokens[0].data.function.end_marker.marker_number == 5);
    assert_continue(tokens[0].data.function.end_marker.offset == -3);
    assert_continue(tokens[0].data.function.begin_marker.present == false);
  }

  {
    const CODE_UNIT* program = CODE_UNIT_LITERAL("{abc5+99}");
    expr_tokenize_arg arg = expr_tokenize_arg_init(program, program + code_unit_strlen(program));
    expr_tokenize_result cap = tokenize_expression(&arg);
    assert_continue(cap.reason == NULL);
    size_t output_size = expr_tokenize_arg_get_cap(&arg);
    assert_continue(output_size == 1);
    expr_token tokens[output_size];
    expr_tokenize_arg_set_to_fill(&arg, tokens);
    cap = tokenize_expression(&arg);
    assert_continue(cap.reason == NULL);
    assert_continue(arg.out - tokens == output_size);

    assert_continue(tokens[0].type == EXPR_TOKEN_FUNCTION);
    assert_continue(tokens[0].offset == 0);
    assert_continue(0 == code_unit_range_equal2(tokens[0].data.function.name.begin, tokens[0].data.function.name.end, CODE_UNIT_LITERAL("abc")));
    assert_continue(tokens[0].data.function.end_marker.present == true);
    assert_continue(tokens[0].data.function.end_marker.marker_number == 5);
    assert_continue(tokens[0].data.function.end_marker.offset == 99);
    assert_continue(tokens[0].data.function.begin_marker.present == false);
  }

  {
    const CODE_UNIT* program = CODE_UNIT_LITERAL("{abc1-999999999999999999999999999999999999999999}");
    expr_tokenize_arg arg = expr_tokenize_arg_init(program, program + code_unit_strlen(program));
    expr_tokenize_result cap = tokenize_expression(&arg);
    assert_continue(0 == strcmp(cap.reason, "overflow on marker end value offset"));
    assert_continue(cap.offset == 0);
  }

  { // wholistic test
    const CODE_UNIT* program = CODE_UNIT_LITERAL("{abc,{0+2str1-1,hello}}");
    expr_tokenize_arg arg = expr_tokenize_arg_init(program, program + code_unit_strlen(program));
    expr_tokenize_result cap = tokenize_expression(&arg);
    assert_continue(cap.reason == NULL);
    size_t output_size = expr_tokenize_arg_get_cap(&arg);
    assert_continue(output_size == 9);
    expr_token tokens[output_size];
    expr_tokenize_arg_set_to_fill(&arg, tokens);
    cap = tokenize_expression(&arg);
    assert_continue(cap.reason == NULL);
    assert_continue(arg.out - tokens == output_size);

    assert_continue(tokens[0].type == EXPR_TOKEN_FUNCTION);
    assert_continue(tokens[0].offset == 0);
    assert_continue(0 == code_unit_range_equal2(tokens[0].data.function.name.begin, tokens[0].data.function.name.end, CODE_UNIT_LITERAL("abc")));
    assert_continue(tokens[0].data.function.end_marker.present == false);
    assert_continue(tokens[0].data.function.begin_marker.present == false);
    assert_continue(tokens[0].data.function.num_args == 1);

    assert_continue(tokens[1].type == EXPR_TOKEN_FUNCTION);
    assert_continue(tokens[1].offset == 5);
    assert_continue(0 == code_unit_range_equal2(tokens[1].data.function.name.begin, tokens[1].data.function.name.end, CODE_UNIT_LITERAL("str")));
    assert_continue(tokens[1].data.function.begin_marker.present == true);
    assert_continue(tokens[1].data.function.begin_marker.marker_number == 0);
    assert_continue(tokens[1].data.function.begin_marker.offset == 2);
    assert_continue(tokens[1].data.function.end_marker.present == true);
    assert_continue(tokens[1].data.function.end_marker.marker_number == 1);
    assert_continue(tokens[1].data.function.end_marker.offset == -1);
    assert_continue(tokens[1].data.function.num_args == 1);

    assert_continue(tokens[2].type == EXPR_TOKEN_LITERAL);
    assert_continue(tokens[2].offset == 16);
    assert_continue(tokens[2].data.literal == 'h');

    assert_continue(tokens[3].type == EXPR_TOKEN_LITERAL);
    assert_continue(tokens[3].offset == 17);
    assert_continue(tokens[3].data.literal == 'e');

    assert_continue(tokens[4].type == EXPR_TOKEN_LITERAL);
    assert_continue(tokens[4].offset == 18);
    assert_continue(tokens[4].data.literal == 'l');

    assert_continue(tokens[5].type == EXPR_TOKEN_LITERAL);
    assert_continue(tokens[5].offset == 19);
    assert_continue(tokens[5].data.literal == 'l');

    assert_continue(tokens[6].type == EXPR_TOKEN_LITERAL);
    assert_continue(tokens[6].offset == 20);
    assert_continue(tokens[6].data.literal == 'o');
    
    assert_continue(tokens[7].type == EXPR_TOKEN_ENDARG);
    assert_continue(tokens[8].type == EXPR_TOKEN_ENDARG);
  }

  if (has_errors) return has_errors;

  marker_increment_check = true;

  { // needs marker 0 defined
    const CODE_UNIT* program = CODE_UNIT_LITERAL("{1tester}");
    expr_tokenize_arg arg = expr_tokenize_arg_init(program, program + code_unit_strlen(program));
    expr_tokenize_result cap = tokenize_expression(&arg);
    assert_continue(0 == strcmp(cap.reason, "begin marker number didn't increment from previous. should start at 0 and increase by 1 for each marker"));
    assert_continue(cap.offset == 0);
  }

  { // ok
    const CODE_UNIT* program = CODE_UNIT_LITERAL("{0tester}");
    expr_tokenize_arg arg = expr_tokenize_arg_init(program, program + code_unit_strlen(program));
    expr_tokenize_result cap = tokenize_expression(&arg);
    assert_continue(cap.reason == NULL);
  }

  { // last marker listed not ok (missing marker 2)
    const CODE_UNIT* program = CODE_UNIT_LITERAL("{0tester}{1tester}{0tester}{3tester}");
    expr_tokenize_arg arg = expr_tokenize_arg_init(program, program + code_unit_strlen(program));
    expr_tokenize_result cap = tokenize_expression(&arg);
    assert_continue(0 == strcmp(cap.reason, "begin marker number didn't increment from previous. should start at 0 and increase by 1 for each marker"));
    assert_continue(cap.offset == 27);
  }

  { // needs marker 0 defined
    const CODE_UNIT* program = CODE_UNIT_LITERAL("{tester1}");
    expr_tokenize_arg arg = expr_tokenize_arg_init(program, program + code_unit_strlen(program));
    expr_tokenize_result cap = tokenize_expression(&arg);
    assert_continue(0 == strcmp(cap.reason, "end marker number didn't increment from previous. should start at 0 and increase by 1 for each marker"));
    assert_continue(cap.offset == 0);
  }

  { // ok
    const CODE_UNIT* program = CODE_UNIT_LITERAL("{tester0}");
    expr_tokenize_arg arg = expr_tokenize_arg_init(program, program + code_unit_strlen(program));
    expr_tokenize_result cap = tokenize_expression(&arg);
    assert_continue(cap.reason == NULL);
  }

  { // last marker listed not ok (missing marker 2)
    const CODE_UNIT* program = CODE_UNIT_LITERAL("{tester0}{tester1}{tester0}{tester3}");
    expr_tokenize_arg arg = expr_tokenize_arg_init(program, program + code_unit_strlen(program));
    expr_tokenize_result cap = tokenize_expression(&arg);
    assert_continue(0 == strcmp(cap.reason, "end marker number didn't increment from previous. should start at 0 and increase by 1 for each marker"));
    assert_continue(cap.offset == 27);
  }


  return has_errors;
}