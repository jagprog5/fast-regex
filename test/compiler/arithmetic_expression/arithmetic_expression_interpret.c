#include "compiler/arithmetic_expression/arithmetic_expression_interpret.h"

#include "test_common.h"
extern int has_errors;

uint_fast32_t run_test(const CODE_UNIT* expr_begin, const CODE_UNIT* expr_end, arith_expr_allowed_symbols* allowed_symbols, const uint_fast32_t* symbol_values) {
  // tokenize first pass - get output size
  arith_tokenize_capacity cap = tokenize_arithmetic_expression(expr_begin, expr_end, NULL, allowed_symbols);
  assert_continue(cap.type == ARITH_TOKENIZE_CAPACITY_OK);

  union {
    arith_token tokens[cap.value.capacity];
    arith_parsed parsed[cap.value.capacity];
  } array_output;

  // tokenize second pass, fill output tokens
  tokenize_arithmetic_expression(expr_begin, expr_end, array_output.tokens, allowed_symbols);

  // ret depends on the lifetime of array_output.parsed
  arith_expr_result ret = parse_arithmetic_expression(array_output.tokens, array_output.tokens + cap.value.capacity, array_output.parsed);

  assert_continue(ret.type == ARITH_EXPR_OK);

  return interpret_arithmetic_expression(ret.value.expr, symbol_values);
}

uint_fast32_t run_test_no_vars(const CODE_UNIT* expr_begin, const CODE_UNIT* expr_end) {
  arith_expr_allowed_symbols allowed_symbols;
  allowed_symbols.size = 0;
  allowed_symbols.symbols = NULL;
  return run_test(expr_begin, expr_end, &allowed_symbols, NULL);
}

int main(void) {
  {
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("2 * 4 - 1");
    assert_continue(7 == run_test_no_vars(expr, expr + code_unit_strlen(expr)));
  }
  {
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("+1");
    assert_continue(1 == run_test_no_vars(expr, expr + code_unit_strlen(expr)));
  }
  {
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("~1");
    assert_continue(~(uint_fast32_t)1 == run_test_no_vars(expr, expr + code_unit_strlen(expr)));
  }
  {
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("3");
    assert_continue(3 == run_test_no_vars(expr, expr + code_unit_strlen(expr)));
  }
  {
    const CODE_UNIT* first_str = CODE_UNIT_LITERAL("first");
    arith_expr_symbol first;
    first.begin = first_str;
    first.end = first_str + code_unit_strlen(first_str);

    const CODE_UNIT* second_str = CODE_UNIT_LITERAL("second");
    arith_expr_symbol second;
    second.begin = second_str;
    second.end = second_str + code_unit_strlen(second_str);

    const CODE_UNIT* third_str = CODE_UNIT_LITERAL("third");
    arith_expr_symbol third;
    third.begin = third_str;
    third.end = third_str + code_unit_strlen(third_str);

    const uint_fast32_t symbol_values[] = {1, 2, 3};
    const arith_expr_symbol symbol_names[] = {first, second, third};
    arith_expr_allowed_symbols allowed_symbols;
    allowed_symbols.size = sizeof(symbol_names) / sizeof(*symbol_names);
    allowed_symbols.symbols = symbol_names;
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("first * second + third");
    assert_continue(5 == run_test(expr, expr + code_unit_strlen(expr), &allowed_symbols, symbol_values));
  }
  return has_errors;
}
