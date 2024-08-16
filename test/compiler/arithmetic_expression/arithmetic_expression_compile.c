#include "compiler/arithmetic_expression/arithmetic_expression_compile.h"
#include "compiler/c_aot_compile.h"

#include "test_common.h"
extern int has_errors;

#ifndef COMPILER_USED
    #error "COMPILER_USED must be defined to the c compiler"
#endif

// Stringify the COMPILER_USED macro
#define STR(x) #x
#define XSTR(x) STR(x)

bool do_test(const CODE_UNIT* expression, const arith_expr_allowed_symbols* allowed_symbols, const uint_fast32_t* symbol_values, uint_fast32_t expected_result) {
    arith_tokenize_capacity maybe_cap = tokenize_arithmetic_expression(expression, expression + code_unit_strlen(expression), NULL, allowed_symbols);
    assert_continue(maybe_cap.type == ARITH_TOKENIZE_CAPACITY_OK);
    union {
      arith_token tokens[maybe_cap.value.capacity];
      arith_parsed parsed[maybe_cap.value.capacity];
    } array_output;
    arith_tokenize_capacity fill = tokenize_arithmetic_expression(expression, expression + code_unit_strlen(expression), array_output.tokens, allowed_symbols);
    assert_continue(fill.type == ARITH_TOKENIZE_FILL_ARRAY);
    arith_expr_result maybe_expr = parse_arithmetic_expression(array_output.tokens, array_output.tokens + maybe_cap.value.capacity, array_output.parsed);
    assert_continue(maybe_expr.type == ARITH_EXPR_OK);

    const char* prolog = "\
#include <stdint.h>\n\
typedef uint_fast32_t uf32_t;\
uf32_t evaluate_expr(const uf32_t* arith_in) {\
\
uf32_t arith_out;\
";

    const char* epilog = "\
return arith_out;\
}";

    // first pass, get capacity for program size
    size_t program_size = 0;
    generate_code_cstr(NULL, &program_size, prolog);
    generate_code_arithmetic_expression_block(NULL, &program_size, maybe_expr.value.expr);
    generate_code_cstr(NULL, &program_size, epilog);

    // second pass, fill array
    char code[program_size];
    char* code_fill = code;
    generate_code_cstr(&code_fill, &program_size, prolog);
    generate_code_arithmetic_expression_block(&code_fill, &program_size, maybe_expr.value.expr);
    generate_code_cstr(&code_fill, &program_size, epilog);

    // use
    const char* compiler = XSTR(COMPILER_USED);
    void* dl_handle = compile_no_args(compiler, code, code_fill);
    assert_continue(dl_handle != NULL);
    uint_fast32_t (*evaluate_expr)(const uint_fast32_t*);
    *(void**)(&evaluate_expr) = dlsym(dl_handle, "evaluate_expr");
    assert_continue(evaluate_expr != NULL);
    bool ret = evaluate_expr(symbol_values) == expected_result;
    assert_continue(dlclose(dl_handle) == 0);
    return ret;
}

bool do_test_no_symbols(const CODE_UNIT* expression, uint_fast32_t expected_result) {
  arith_expr_allowed_symbols allowed_symbols;
  allowed_symbols.size = 0;
  allowed_symbols.symbols = NULL;
  return do_test(expression, &allowed_symbols, NULL, expected_result);
}

int main(void) {
  assert_continue(do_test_no_symbols(CODE_UNIT_LITERAL("1 + 3"), 4));
  assert_continue(do_test_no_symbols(CODE_UNIT_LITERAL("3 - 1"), 2));
  assert_continue(do_test_no_symbols(CODE_UNIT_LITERAL("5 - ~3"), (uint_fast32_t)5 - ~(uint_fast32_t)3));

  // stack test
  assert_continue(do_test_no_symbols(CODE_UNIT_LITERAL("5 * 2 + 3 * 7"), 31));

  {
    arith_expr_allowed_symbols allowed_symbols;
    allowed_symbols.size = 2;
    CODE_UNIT* sym0_data = CODE_UNIT_LITERAL("abc");
    arith_expr_symbol sym0;
    sym0.begin = sym0_data;
    sym0.end = sym0_data + code_unit_strlen(sym0_data);
    CODE_UNIT* sym1_data = CODE_UNIT_LITERAL("hi_there");
    arith_expr_symbol sym1;
    sym1.begin = sym1_data;
    sym1.end = sym1_data + code_unit_strlen(sym1_data);

    arith_expr_symbol sym[2] = {sym0, sym1};
    allowed_symbols.symbols = sym;

    uint_fast32_t sym_values[2] = {70, 3};
    assert_continue(do_test(CODE_UNIT_LITERAL("100 - (abc - hi_there)"), &allowed_symbols, sym_values, 33));
  }
  return has_errors;
}
