#define USE_WCHAR

#include "arithmetic_expression.h"

#include "test_common.h"
extern int has_errors;

// helper for testing
arith_tokenize_capacity token_exp_no_sym(const CODE_UNIT* begin, //
                                         const CODE_UNIT* end,
                                         arith_token* output) {
  arith_expr_allowed_symbols allowed_symbols;
  allowed_symbols.size = 0;
  allowed_symbols.symbols = NULL;
  return tokenize_arithmetic_expression(begin, end, output, &allowed_symbols);
}

int main(void) {
  // ===========================================================================
  // =============================== tokenization test =========================
  // ===========================================================================

  { // empty input output
    arith_tokenize_capacity ret = token_exp_no_sym(NULL, NULL, NULL);
    assert_continue(ret.type == ARITH_TOKENIZE_CAPACITY_OK);
    assert_continue(ret.value.capacity == 0);
  }
  { // basic operators, whitespace ignored. unary has additional zeros inserted
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("+ -\n");
    arith_token array_output[4];
    memset(array_output, 0, sizeof(array_output));
    arith_tokenize_capacity ret = token_exp_no_sym(expr, expr + code_unit_strlen(expr), array_output);
    assert_continue(ret.type == ARITH_TOKENIZE_FILL_ARRAY);
    assert_continue(array_output[0].type == ARITH_U32);
    assert_continue(array_output[0].offset == 0);
    assert_continue(array_output[0].value.u32 == 0);

    assert_continue(array_output[1].type == ARITH_UNARY_ADD);
    assert_continue(array_output[1].offset == 0);

    assert_continue(array_output[2].type == ARITH_SUB);
    assert_continue(array_output[2].offset == 2);

    assert_continue(array_output[3].type == ARITH_INVALID);
    assert_continue(array_output[3].offset == 0);
    assert_continue(array_output[3].value.symbol_value_lookup == 0);
  }
  { // a number which end with the end of the string
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("1234");
    arith_token array_output[2];
    memset(array_output, 0, sizeof(array_output));
    token_exp_no_sym(expr, expr + code_unit_strlen(expr), array_output);

    assert_continue(array_output[0].type == ARITH_U32);
    assert_continue(array_output[0].offset == 0);
    assert_continue(array_output[0].value.u32 == 1234);

    assert_continue(array_output[1].type == ARITH_INVALID);
    assert_continue(array_output[1].offset == 0);
    assert_continue(array_output[1].value.symbol_value_lookup == 0);
  }
  { // a number which ends for any other reason
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("1234\n\r\t");
    arith_token array_output[2];
    memset(array_output, 0, sizeof(array_output));
    token_exp_no_sym(expr, expr + code_unit_strlen(expr), array_output);
    assert_continue(array_output[0].type == ARITH_U32);
    assert_continue(array_output[0].offset == 0);
    assert_continue(array_output[0].value.u32 == 1234);

    assert_continue(array_output[1].type == ARITH_INVALID);
    assert_continue(array_output[1].offset == 0);
    assert_continue(array_output[1].value.symbol_value_lookup == 0);
  }
  { // maximum value
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("4294967295");
    arith_token array_output[2];
    memset(array_output, 0, sizeof(array_output));
    token_exp_no_sym(expr, expr + code_unit_strlen(expr), array_output);
    assert_continue(array_output[0].type == ARITH_U32);
    assert_continue(array_output[0].offset == 0);
    assert_continue(array_output[0].value.u32 == 4294967295);

    assert_continue(array_output[1].type == ARITH_INVALID);
    assert_continue(array_output[1].offset == 0);
    assert_continue(array_output[1].value.symbol_value_lookup == 0);
  }
  { // overflow from addition in number parse
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("4294967296");
    arith_tokenize_capacity ret = token_exp_no_sym(expr, expr + code_unit_strlen(expr), NULL);
    assert_continue(ret.type == ARITH_TOKENIZE_CAPACITY_ERROR);
    assert_continue(ret.value.err.offset == 0);
    assert_continue(0 == strcmp(ret.value.err.reason, "num too large"));
  }
  { // overflow from multiply in number parse
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("42949672950");
    arith_tokenize_capacity ret = token_exp_no_sym(expr, expr + code_unit_strlen(expr), NULL);
    assert_continue(ret.type == ARITH_TOKENIZE_CAPACITY_ERROR);
    assert_continue(ret.value.err.offset == 0);
    assert_continue(0 == strcmp(ret.value.err.reason, "num too large"));
  }
  { // a symbol that ends with the end of the string
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("abz9_hello");
    arith_token array_output[2];
    memset(array_output, 0, sizeof(array_output));

    arith_expr_symbol one_symbol;
    one_symbol.begin = expr;
    one_symbol.end = expr + code_unit_strlen(expr);
    arith_expr_allowed_symbols allowed_symbols;
    allowed_symbols.size = 1;
    allowed_symbols.symbols = &one_symbol;

    tokenize_arithmetic_expression(expr, expr + code_unit_strlen(expr), array_output, &allowed_symbols);
    assert_continue(array_output[0].type == ARITH_SYMBOL);
    assert_continue(array_output[0].offset == 0);
    assert_continue(array_output[0].value.symbol_value_lookup == 0);

    assert_continue(array_output[1].type == ARITH_INVALID);
    assert_continue(array_output[1].offset == 0);
    assert_continue(array_output[1].value.symbol_value_lookup == 0);
  }
  { // a symbol that ends for any other reason
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("abz9_hello\n\t\r");
    arith_token array_output[2];
    memset(array_output, 0, sizeof(array_output));

    arith_expr_symbol one_symbol;
    one_symbol.begin = expr;
    one_symbol.end = expr + 10;
    arith_expr_allowed_symbols allowed_symbols;
    allowed_symbols.size = 1;
    allowed_symbols.symbols = &one_symbol;

    tokenize_arithmetic_expression(expr, expr + code_unit_strlen(expr), array_output, &allowed_symbols);
    assert_continue(array_output[0].type == ARITH_SYMBOL);
    assert_continue(array_output[0].offset == 0);
    assert_continue(array_output[0].value.symbol_value_lookup == 0);

    assert_continue(array_output[1].type == ARITH_INVALID);
    assert_continue(array_output[1].offset == 0);
    assert_continue(array_output[1].value.symbol_value_lookup == 0);
  }
  { // invalid character
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("@ 123");

    arith_token array_output[2];
    memset(array_output, 0, sizeof(array_output));

    arith_tokenize_capacity ret = token_exp_no_sym(expr, expr + code_unit_strlen(expr), array_output);
    assert_continue(ret.type = ARITH_TOKENIZE_CAPACITY_ERROR);
    assert_continue(ret.value.err.offset == 0);
    assert_continue(0 == strcmp(ret.value.err.reason, "invalid character in arith expr"));
  }
  { // < from end of string
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("<");

    arith_token array_output[2];
    memset(array_output, 0, sizeof(array_output));

    token_exp_no_sym(expr, expr + code_unit_strlen(expr), array_output);
    assert_continue(array_output[0].type == ARITH_LESS_THAN);
    assert_continue(array_output[0].offset == 0);

    assert_continue(array_output[1].type == ARITH_INVALID);
    assert_continue(array_output[1].offset == 0);
    assert_continue(array_output[1].value.symbol_value_lookup == 0);
  }
  { // < from end of operation
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("<\r\n\t");

    arith_token array_output[2];
    memset(array_output, 0, sizeof(array_output));

    token_exp_no_sym(expr, expr + code_unit_strlen(expr), array_output);
    assert_continue(array_output[0].type == ARITH_LESS_THAN);
    assert_continue(array_output[0].offset == 0);

    assert_continue(array_output[1].type == ARITH_INVALID);
    assert_continue(array_output[1].offset == 0);
    assert_continue(array_output[1].value.symbol_value_lookup == 0);
  }
  { // <= from end of string
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("<=");

    arith_token array_output[2];
    memset(array_output, 0, sizeof(array_output));

    token_exp_no_sym(expr, expr + code_unit_strlen(expr), array_output);
    assert_continue(array_output[0].type == ARITH_LESS_THAN_EQUAL);
    assert_continue(array_output[0].offset == 0);

    assert_continue(array_output[1].type == ARITH_INVALID);
    assert_continue(array_output[1].offset == 0);
    assert_continue(array_output[1].value.symbol_value_lookup == 0);
  }
  { // <= from end of operation
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("<=\n\t\r");

    arith_token array_output[2];
    memset(array_output, 0, sizeof(array_output));

    token_exp_no_sym(expr, expr + code_unit_strlen(expr), array_output);
    assert_continue(array_output[0].type == ARITH_LESS_THAN_EQUAL);
    assert_continue(array_output[0].offset == 0);

    assert_continue(array_output[1].type == ARITH_INVALID);
    assert_continue(array_output[1].offset == 0);
    assert_continue(array_output[1].value.symbol_value_lookup == 0);
  }
  { // >
    const CODE_UNIT* expr = CODE_UNIT_LITERAL(">\r\n\t");

    arith_token array_output[2];
    memset(array_output, 0, sizeof(array_output));

    token_exp_no_sym(expr, expr + code_unit_strlen(expr), array_output);
    assert_continue(array_output[0].type == ARITH_GREATER_THAN);
    assert_continue(array_output[0].offset == 0);

    assert_continue(array_output[1].type == ARITH_INVALID);
    assert_continue(array_output[1].offset == 0);
    assert_continue(array_output[1].value.symbol_value_lookup == 0);
  }
  { // >=
    const CODE_UNIT* expr = CODE_UNIT_LITERAL(">=\r\n\t");

    arith_token array_output[2];
    memset(array_output, 0, sizeof(array_output));

    token_exp_no_sym(expr, expr + code_unit_strlen(expr), array_output);
    assert_continue(array_output[0].type == ARITH_GREATER_THAN_EQUAL);
    assert_continue(array_output[0].offset == 0);

    assert_continue(array_output[1].type == ARITH_INVALID);
    assert_continue(array_output[1].offset == 0);
    assert_continue(array_output[1].value.symbol_value_lookup == 0);
  }
  {
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("<<>><>");

    arith_token array_output[5];
    memset(array_output, 0, sizeof(array_output));

    token_exp_no_sym(expr, expr + code_unit_strlen(expr), array_output);
    assert_continue(array_output[0].type == ARITH_LEFT_SHIFT);
    assert_continue(array_output[0].offset == 0);

    assert_continue(array_output[1].type == ARITH_RIGHT_SHIFT);
    assert_continue(array_output[1].offset == 2);

    assert_continue(array_output[2].type == ARITH_LESS_THAN);
    assert_continue(array_output[2].offset == 4);

    assert_continue(array_output[3].type == ARITH_GREATER_THAN);
    assert_continue(array_output[3].offset == 5);

    assert_continue(array_output[4].type == ARITH_INVALID);
    assert_continue(array_output[4].offset == 0);
    assert_continue(array_output[4].value.symbol_value_lookup == 0);
  }
  { // & from end of string
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("&");

    arith_token array_output[2];
    memset(array_output, 0, sizeof(array_output));

    token_exp_no_sym(expr, expr + code_unit_strlen(expr), array_output);
    assert_continue(array_output[0].type == ARITH_BITWISE_AND);
    assert_continue(array_output[0].offset == 0);

    assert_continue(array_output[1].type == ARITH_INVALID);
    assert_continue(array_output[1].offset == 0);
    assert_continue(array_output[1].value.symbol_value_lookup == 0);
  }
  { // & from end of operation
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("&\r\n\t");

    arith_token array_output[2];
    memset(array_output, 0, sizeof(array_output));

    token_exp_no_sym(expr, expr + code_unit_strlen(expr), array_output);
    assert_continue(array_output[0].type == ARITH_BITWISE_AND);
    assert_continue(array_output[0].offset == 0);

    assert_continue(array_output[1].type == ARITH_INVALID);
    assert_continue(array_output[1].offset == 0);
    assert_continue(array_output[1].value.symbol_value_lookup == 0);
  }
  { // != fail from end of string
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("!");

    arith_token array_output[2];
    memset(array_output, 0, sizeof(array_output));

    arith_tokenize_capacity ret = token_exp_no_sym(expr, expr + code_unit_strlen(expr), array_output);
    assert_continue(ret.type == ARITH_TOKENIZE_CAPACITY_ERROR);
    assert_continue(ret.value.err.offset == 0);
    assert_continue(0 == strcmp(ret.value.err.reason, "operator incomplete (!=) from end of string"));
  }
  { // != fail
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("!5");

    arith_token array_output[2];
    memset(array_output, 0, sizeof(array_output));

    arith_tokenize_capacity ret = token_exp_no_sym(expr, expr + code_unit_strlen(expr), array_output);
    assert_continue(ret.type == ARITH_TOKENIZE_CAPACITY_ERROR);
    assert_continue(ret.value.err.offset == 0);
    assert_continue(0 == strcmp(ret.value.err.reason, "operator incomplete (!=)"));
  }
  {
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("!==");

    arith_token array_output[3];
    memset(array_output, 0, sizeof(array_output));

    token_exp_no_sym(expr, expr + code_unit_strlen(expr), array_output);
    assert_continue(array_output[0].type == ARITH_NOT_EQUAL);
    assert_continue(array_output[0].offset == 0);

    assert_continue(array_output[1].type == ARITH_EQUAL);
    assert_continue(array_output[1].offset == 2);

    assert_continue(array_output[2].type == ARITH_INVALID);
    assert_continue(array_output[2].offset == 0);
    assert_continue(array_output[2].value.symbol_value_lookup == 0);
  }
  { // ~ fail
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("15~3");

    arith_token array_output[3];
    memset(array_output, 0, sizeof(array_output));

    arith_tokenize_capacity ret = token_exp_no_sym(expr, expr + code_unit_strlen(expr), array_output);
    assert_continue(ret.type == ARITH_TOKENIZE_CAPACITY_ERROR);
    assert_continue(ret.value.err.offset == 2);
    assert_continue(0 == strcmp(ret.value.err.reason, "unary op mustn't follow value"));
  }
  { // ~ ok
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("(~3)");

    arith_token array_output[6];
    memset(array_output, 0, sizeof(array_output));

    token_exp_no_sym(expr, expr + code_unit_strlen(expr), array_output);
    assert_continue(array_output[0].type == ARITH_LEFT_BRACKET);
    assert_continue(array_output[0].offset == 0);

    assert_continue(array_output[1].type == ARITH_U32);
    assert_continue(array_output[1].offset == 1);
    assert_continue(array_output[1].value.u32 == 0);

    assert_continue(array_output[2].type == ARITH_BITWISE_COMPLEMENT);
    assert_continue(array_output[2].offset == 1);

    assert_continue(array_output[3].type == ARITH_U32);
    assert_continue(array_output[3].offset == 2);
    assert_continue(array_output[3].value.u32 == 3);

    assert_continue(array_output[4].type == ARITH_RIGHT_BRACKET);
    assert_continue(array_output[4].offset == 3);

    assert_continue(array_output[5].type == ARITH_INVALID);
    assert_continue(array_output[5].offset == 0);
    assert_continue(array_output[5].value.symbol_value_lookup == 0);
  }
  {
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("\'");
    arith_token array_output[2];
    memset(array_output, 0, sizeof(array_output));

    arith_tokenize_capacity ret = token_exp_no_sym(expr, expr + code_unit_strlen(expr), array_output);
    assert_continue(ret.type == ARITH_TOKENIZE_CAPACITY_ERROR);
    assert_continue(ret.value.err.offset == 0);
    assert_continue(0 == strcmp(ret.value.err.reason, "literal ended without content"));
  }
  {
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("'a");
    arith_token array_output[2];
    memset(array_output, 0, sizeof(array_output));

    arith_tokenize_capacity ret = token_exp_no_sym(expr, expr + code_unit_strlen(expr), array_output);
    assert_continue(ret.type == ARITH_TOKENIZE_CAPACITY_ERROR);
    assert_continue(ret.value.err.offset == 0);
    assert_continue(0 == strcmp(ret.value.err.reason, "literal ended without closing quote"));
  }
  {
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("'ab");
    arith_token array_output[2];
    memset(array_output, 0, sizeof(array_output));

    arith_tokenize_capacity ret = token_exp_no_sym(expr, expr + code_unit_strlen(expr), array_output);
    assert_continue(ret.type == ARITH_TOKENIZE_CAPACITY_ERROR);
    assert_continue(ret.value.err.offset == 2);
    assert_continue(0 == strcmp(ret.value.err.reason, "expecting end of literal"));
  }
  {
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("'a'");
    arith_token array_output[2];
    memset(array_output, 0, sizeof(array_output));

    token_exp_no_sym(expr, expr + code_unit_strlen(expr), array_output);
    assert_continue(array_output[0].type == ARITH_U32);
    assert_continue(array_output[0].offset == 0);
    assert_continue(array_output[0].value.u32 == 'a');

    assert_continue(array_output[1].type == ARITH_INVALID);
    assert_continue(array_output[1].offset == 0);
    assert_continue(array_output[1].value.symbol_value_lookup == 0);
  }
  {
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("'\\z");
    arith_token array_output[2];
    memset(array_output, 0, sizeof(array_output));

    arith_tokenize_capacity ret = token_exp_no_sym(expr, expr + code_unit_strlen(expr), array_output);
    assert_continue(ret.type == ARITH_TOKENIZE_CAPACITY_ERROR);
    assert_continue(ret.value.err.offset == 2);
    assert_continue(0 == strcmp(ret.value.err.reason, "invalid escaped character"));
  }
  {
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("'\\n'");
    arith_token array_output[2];
    memset(array_output, 0, sizeof(array_output));

    token_exp_no_sym(expr, expr + code_unit_strlen(expr), array_output);

    assert_continue(array_output[0].type == ARITH_U32);
    assert_continue(array_output[0].offset == 0);
    assert_continue(array_output[0].value.u32 == '\n');

    assert_continue(array_output[1].type == ARITH_INVALID);
    assert_continue(array_output[1].offset == 0);
    assert_continue(array_output[1].value.symbol_value_lookup == 0);
  }
  {
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("'\\xaF'");
    arith_token array_output[2];
    memset(array_output, 0, sizeof(array_output));

    token_exp_no_sym(expr, expr + code_unit_strlen(expr), array_output);

    assert_continue(array_output[0].type == ARITH_U32);
    assert_continue(array_output[0].offset == 0);
    assert_continue(array_output[0].value.u32 == 0xAF);

    assert_continue(array_output[1].type == ARITH_INVALID);
    assert_continue(array_output[1].offset == 0);
    assert_continue(array_output[1].value.symbol_value_lookup == 0);
  }
  {
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("'\\x'");
    arith_token array_output[2];
    memset(array_output, 0, sizeof(array_output));

    token_exp_no_sym(expr, expr + code_unit_strlen(expr), array_output);

    assert_continue(array_output[0].type == ARITH_U32);
    assert_continue(array_output[0].offset == 0);
    assert_continue(array_output[0].value.u32 == 0);

    assert_continue(array_output[1].type == ARITH_INVALID);
    assert_continue(array_output[1].offset == 0);
    assert_continue(array_output[1].value.symbol_value_lookup == 0);
  }
  {
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("'\\xFFFFFFFFF'"); // overlong
    arith_token array_output[2];
    memset(array_output, 0, sizeof(array_output));

    arith_tokenize_capacity ret = token_exp_no_sym(expr, expr + code_unit_strlen(expr), array_output);
    assert_continue(ret.type == ARITH_TOKENIZE_CAPACITY_ERROR);
    assert_continue(ret.value.err.offset == 11);
    assert_continue(0 == strcmp(ret.value.err.reason, "expecting end of literal"));
  }
  {
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("'\\xQ'");
    arith_token array_output[2];
    memset(array_output, 0, sizeof(array_output));

    arith_tokenize_capacity ret = token_exp_no_sym(expr, expr + code_unit_strlen(expr), array_output);
    assert_continue(ret.type == ARITH_TOKENIZE_CAPACITY_ERROR);
    assert_continue(ret.value.err.offset == 3);
    assert_continue(0 == strcmp(ret.value.err.reason, "invalid hex escaped character"));
  }
  { // consecutive value from symbol
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("'a' tester");
    arith_token array_output[3];
    memset(array_output, 0, sizeof(array_output));

    arith_tokenize_capacity ret = token_exp_no_sym(expr, expr + code_unit_strlen(expr), array_output);
    assert_continue(ret.type == ARITH_TOKENIZE_CAPACITY_ERROR);
    assert_continue(ret.value.err.offset == 4);
    assert_continue(0 == strcmp(ret.value.err.reason, "consecutive value not allowed"));
  }
  { // consecutive value from char literal
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("5 'a'");
    arith_token array_output[3];
    memset(array_output, 0, sizeof(array_output));

    arith_tokenize_capacity ret = token_exp_no_sym(expr, expr + code_unit_strlen(expr), array_output);
    assert_continue(ret.type == ARITH_TOKENIZE_CAPACITY_ERROR);
    assert_continue(ret.value.err.offset == 2);
    assert_continue(0 == strcmp(ret.value.err.reason, "consecutive value not allowed"));
  }
  { // consecutive value from begin of expression
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("5 (stuff)");
    arith_token array_output[3];
    memset(array_output, 0, sizeof(array_output));

    arith_tokenize_capacity ret = token_exp_no_sym(expr, expr + code_unit_strlen(expr), array_output);
    assert_continue(ret.type == ARITH_TOKENIZE_CAPACITY_ERROR);
    assert_continue(ret.value.err.offset == 2);
    assert_continue(0 == strcmp(ret.value.err.reason, "consecutive value not allowed"));
  }
  { // invalid symbol emitted from end of string
    arith_expr_symbol allowed_symbol_arr[2];
    allowed_symbol_arr[0].begin = CODE_UNIT_LITERAL("abc");
    allowed_symbol_arr[0].end = allowed_symbol_arr[0].begin + code_unit_strlen(allowed_symbol_arr[0].begin);
    allowed_symbol_arr[1].begin = CODE_UNIT_LITERAL("test");
    allowed_symbol_arr[1].end = allowed_symbol_arr[1].begin + code_unit_strlen(allowed_symbol_arr[1].begin);

    arith_expr_allowed_symbols allowed_symbols;
    allowed_symbols.size = sizeof(allowed_symbol_arr) / sizeof(*allowed_symbol_arr);
    allowed_symbols.symbols = allowed_symbol_arr;

    const CODE_UNIT* expr = CODE_UNIT_LITERAL("abc + test + blah");
    arith_token array_output[6];
    memset(array_output, 0, sizeof(array_output));

    arith_tokenize_capacity ret = tokenize_arithmetic_expression(expr, expr + code_unit_strlen(expr), array_output, &allowed_symbols);
    assert_continue(ret.type == ARITH_TOKENIZE_CAPACITY_ERROR);
    assert_continue(ret.value.err.offset == 13);
    assert_continue(0 == strcmp(ret.value.err.reason, "invalid symbol"));
  }
  { // invalid symbol typical
    arith_expr_symbol allowed_symbol_arr[0];
    arith_expr_allowed_symbols allowed_symbols;
    allowed_symbols.size = sizeof(allowed_symbol_arr) / sizeof(*allowed_symbol_arr);
    allowed_symbols.symbols = allowed_symbol_arr;

    const CODE_UNIT* expr = CODE_UNIT_LITERAL("blah");
    arith_token array_output[2];
    memset(array_output, 0, sizeof(array_output));

    arith_tokenize_capacity ret = token_exp_no_sym(expr, expr + code_unit_strlen(expr), array_output);
    assert_continue(ret.type == ARITH_TOKENIZE_CAPACITY_ERROR);
    assert_continue(ret.value.err.offset == 0);
    assert_continue(0 == strcmp(ret.value.err.reason, "invalid symbol"));
  }
  { // consecutive value from literal number
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("(1) 17");
    arith_token array_output[5];
    memset(array_output, 0, sizeof(array_output));

    arith_tokenize_capacity ret = token_exp_no_sym(expr, expr + code_unit_strlen(expr), array_output);
    assert_continue(ret.type == ARITH_TOKENIZE_CAPACITY_ERROR);
    assert_continue(ret.value.err.offset == 4);
    assert_continue(0 == strcmp(ret.value.err.reason, "consecutive value not allowed"));
  }
  { // test wholistic
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("(variable & 6) = (32 * -'a')");

    arith_expr_symbol one_symbol;
    one_symbol.begin = CODE_UNIT_LITERAL("variable");
    one_symbol.end = one_symbol.begin + code_unit_strlen(one_symbol.begin);
    arith_expr_allowed_symbols allowed_symbols;
    allowed_symbols.size = 1;
    allowed_symbols.symbols = &one_symbol;

    arith_tokenize_capacity ret_first_pass = tokenize_arithmetic_expression(expr, expr + code_unit_strlen(expr), NULL, &allowed_symbols);
    assert_continue(ret_first_pass.type == ARITH_TOKENIZE_CAPACITY_OK);
    assert_continue(ret_first_pass.value.capacity == 13);

    arith_token array_output[ret_first_pass.value.capacity + 1];
    memset(array_output, 0, sizeof(array_output));

    arith_tokenize_capacity ret = tokenize_arithmetic_expression(expr, expr + code_unit_strlen(expr), array_output, &allowed_symbols);
    assert_continue(ret.type == ARITH_TOKENIZE_FILL_ARRAY); // not necessary to check in normal operation

    assert_continue(array_output[0].type == ARITH_LEFT_BRACKET);
    assert_continue(array_output[0].offset == 0);

    assert_continue(array_output[1].type == ARITH_SYMBOL);
    assert_continue(array_output[1].offset == 1);
    assert_continue(array_output[1].value.symbol_value_lookup == 0);

    assert_continue(array_output[2].type == ARITH_BITWISE_AND);
    assert_continue(array_output[2].offset == 10);

    assert_continue(array_output[3].type == ARITH_U32);
    assert_continue(array_output[3].offset == 12);
    assert_continue(array_output[3].value.u32 == 6);

    assert_continue(array_output[4].type == ARITH_RIGHT_BRACKET);
    assert_continue(array_output[4].offset == 13);

    assert_continue(array_output[5].type == ARITH_EQUAL);
    assert_continue(array_output[5].offset == 15);

    assert_continue(array_output[6].type == ARITH_LEFT_BRACKET);
    assert_continue(array_output[6].offset == 17);

    assert_continue(array_output[7].type == ARITH_U32);
    assert_continue(array_output[7].offset == 18);
    assert_continue(array_output[7].value.u32 == 32);

    assert_continue(array_output[8].type == ARITH_MUL);
    assert_continue(array_output[8].offset == 21);

    assert_continue(array_output[9].type == ARITH_U32);
    assert_continue(array_output[9].offset == 23);
    assert_continue(array_output[9].value.u32 == 0);

    assert_continue(array_output[10].type == ARITH_UNARY_SUB);
    assert_continue(array_output[10].offset == 23);

    assert_continue(array_output[11].type == ARITH_U32);
    assert_continue(array_output[11].offset == 24);
    assert_continue(array_output[11].value.u32 == 'a');

    assert_continue(array_output[12].type == ARITH_RIGHT_BRACKET);
    assert_continue(array_output[12].offset == 27);

    assert_continue(array_output[13].type == ARITH_INVALID);
    assert_continue(array_output[13].offset == 0);
    assert_continue(array_output[13].value.symbol_value_lookup == 0);
  }

  if (has_errors) {
    // stop suite early before moving on to more complex stuff
    return has_errors;
  }

  // ===========================================================================
  // ==================================== parsing ==============================
  // ===========================================================================

  { // wholistic simple binary ops test
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("1 * ( 2 + 3 * 4 ) + 5");

    arith_tokenize_capacity ret_first_pass = token_exp_no_sym(expr, expr + code_unit_strlen(expr), NULL);
    assert_continue(ret_first_pass.type == ARITH_TOKENIZE_CAPACITY_OK);
    assert_continue(ret_first_pass.value.capacity == 11);

    union {
      arith_token tokens[ret_first_pass.value.capacity];
      arith_parsed parsed[ret_first_pass.value.capacity];
    } array_output;

    arith_tokenize_capacity ret_fill = token_exp_no_sym(expr, expr + code_unit_strlen(expr), array_output.tokens);
    assert_continue(ret_fill.type == ARITH_TOKENIZE_FILL_ARRAY); // not necessary to check in normal operation

    arith_expr_result ret = parse_arithmetic_expression(array_output.tokens, array_output.tokens + ret_first_pass.value.capacity, array_output.parsed);
    assert_continue(ret.type == ARITH_EXPR_OK);
    assert_continue(ret.value.expr.stack_required == 4);

    assert_continue(array_output.parsed[0].type == ARITH_U32);
    assert_continue(array_output.parsed[0].value.u32 == 1);
    assert_continue(array_output.parsed[1].type == ARITH_U32);
    assert_continue(array_output.parsed[1].value.u32 == 2);
    assert_continue(array_output.parsed[2].type == ARITH_U32);
    assert_continue(array_output.parsed[2].value.u32 == 3);
    assert_continue(array_output.parsed[3].type == ARITH_U32);
    assert_continue(array_output.parsed[3].value.u32 == 4);
    assert_continue(array_output.parsed[4].type == ARITH_MUL);
    assert_continue(array_output.parsed[5].type == ARITH_ADD);
    assert_continue(array_output.parsed[6].type == ARITH_MUL);
    assert_continue(array_output.parsed[7].type == ARITH_U32);
    assert_continue(array_output.parsed[7].value.u32 == 5);
    assert_continue(array_output.parsed[8].type == ARITH_ADD);
  }

  { // missing )
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("( 52");
    union {
      arith_token tokens[2];
      arith_parsed parsed[2];
    } array_output;
    arith_tokenize_capacity ret_fill = token_exp_no_sym(expr, expr + code_unit_strlen(expr), array_output.tokens);
    assert_continue(ret_fill.type == ARITH_TOKENIZE_FILL_ARRAY);

    assert_continue(array_output.tokens[0].type == ARITH_LEFT_BRACKET);
    assert_continue(array_output.tokens[0].offset == 0);
    assert_continue(array_output.tokens[1].type == ARITH_U32);
    assert_continue(array_output.tokens[1].offset == 2);
    assert_continue(array_output.tokens[1].value.u32 == 52);

    arith_expr_result ret = parse_arithmetic_expression(array_output.tokens, array_output.tokens + sizeof(array_output.tokens) / sizeof(*array_output.tokens), array_output.parsed);
    assert_continue(ret.type == ARITH_EXPR_ERROR);
    assert_continue(ret.value.err.offset == 0);
    assert_continue(0 == strcmp(ret.value.err.reason, "no matching right bracket"));
  }

  { // missing (
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("52 )");
    union {
      arith_token tokens[2];
      arith_parsed parsed[2];
    } array_output;
    arith_tokenize_capacity ret_fill = token_exp_no_sym(expr, expr + code_unit_strlen(expr), array_output.tokens);

    assert_continue(array_output.tokens[0].type == ARITH_U32);
    assert_continue(array_output.tokens[0].offset == 0);
    assert_continue(array_output.tokens[0].value.u32 == 52);
    assert_continue(array_output.tokens[1].type == ARITH_RIGHT_BRACKET);
    assert_continue(array_output.tokens[1].offset == 3);

    arith_expr_result ret = parse_arithmetic_expression(array_output.tokens, array_output.tokens + sizeof(array_output.tokens) / sizeof(*array_output.tokens), array_output.parsed);
    assert_continue(ret.type == ARITH_EXPR_ERROR);
    assert_continue(ret.value.err.offset == 3);
    assert_continue(0 == strcmp(ret.value.err.reason, "no matching left bracket"));
  }

  { // unary op parse
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("-5 * 2");
    union {
      arith_token tokens[5];
      arith_parsed parsed[5];
    } array_output;
    arith_tokenize_capacity ret_fill = token_exp_no_sym(expr, expr + code_unit_strlen(expr), array_output.tokens);

    assert_continue(array_output.tokens[0].type == ARITH_U32);
    assert_continue(array_output.tokens[0].offset == 0);
    assert_continue(array_output.tokens[0].value.u32 == 0);
    assert_continue(array_output.tokens[1].type == ARITH_UNARY_SUB);
    assert_continue(array_output.tokens[1].offset == 0);
    assert_continue(array_output.tokens[2].type == ARITH_U32);
    assert_continue(array_output.tokens[2].offset == 1);
    assert_continue(array_output.tokens[2].value.u32 == 5);
    assert_continue(array_output.tokens[3].type == ARITH_MUL);
    assert_continue(array_output.tokens[3].offset == 3);
    assert_continue(array_output.tokens[4].type == ARITH_U32);
    assert_continue(array_output.tokens[4].offset == 5);
    assert_continue(array_output.tokens[4].value.u32 == 2);

    arith_expr_result ret = parse_arithmetic_expression(array_output.tokens, array_output.tokens + sizeof(array_output.tokens) / sizeof(*array_output.tokens), array_output.parsed);
    assert_continue(ret.type = ARITH_EXPR_OK);
    // TODO check ret value expr begin, end, after optimization is added
    assert_continue(array_output.parsed[0].type == ARITH_U32);
    assert_continue(array_output.parsed[0].value.u32 == 0);
    assert_continue(array_output.parsed[1].type == ARITH_U32);
    assert_continue(array_output.parsed[1].value.u32 == 5);
    assert_continue(array_output.parsed[2].type == ARITH_SUB);
    assert_continue(array_output.parsed[3].type == ARITH_U32);
    assert_continue(array_output.parsed[3].value.u32 == 2);
    assert_continue(array_output.parsed[4].type == ARITH_MUL);
  }

  { // too many ops
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("1 * * 3");
    union {
      arith_token tokens[4];
      arith_parsed parsed[4];
    } array_output;
    arith_tokenize_capacity ret_fill = token_exp_no_sym(expr, expr + code_unit_strlen(expr), array_output.tokens);

    assert_continue(array_output.tokens[0].type == ARITH_U32);
    assert_continue(array_output.tokens[0].offset == 0);
    assert_continue(array_output.tokens[0].value.u32 == 1);
    assert_continue(array_output.tokens[1].type == ARITH_MUL);
    assert_continue(array_output.tokens[1].offset == 2);
    assert_continue(array_output.tokens[2].type == ARITH_MUL);
    assert_continue(array_output.tokens[2].offset == 4);
    assert_continue(array_output.tokens[3].type == ARITH_U32);
    assert_continue(array_output.tokens[3].offset == 6);
    assert_continue(array_output.tokens[3].value.u32 == 3);

    arith_expr_result ret = parse_arithmetic_expression(array_output.tokens, array_output.tokens + sizeof(array_output.tokens) / sizeof(*array_output.tokens), array_output.parsed);
    assert_continue(ret.type == ARITH_EXPR_ERROR);
    assert_continue(ret.value.err.offset == 2);
    assert_continue(0 == strcmp(ret.value.err.reason, "expression stack exhausted by op"));
  }

  { // two + is ok (first is binary, second is unary)
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("1 + + 3");
    union {
      arith_token tokens[5];
      arith_parsed parsed[5];
    } array_output;
    arith_tokenize_capacity ret_fill = token_exp_no_sym(expr, expr + code_unit_strlen(expr), array_output.tokens);

    assert_continue(array_output.tokens[0].type == ARITH_U32);
    assert_continue(array_output.tokens[0].offset == 0);
    assert_continue(array_output.tokens[0].value.u32 == 1);
    assert_continue(array_output.tokens[1].type == ARITH_ADD);
    assert_continue(array_output.tokens[1].offset == 2);
    assert_continue(array_output.tokens[2].type == ARITH_U32);
    assert_continue(array_output.tokens[2].offset == 4);
    assert_continue(array_output.tokens[2].value.u32 == 0);
    assert_continue(array_output.tokens[3].type == ARITH_UNARY_ADD);
    assert_continue(array_output.tokens[3].offset == 4);
    assert_continue(array_output.tokens[4].type == ARITH_U32);
    assert_continue(array_output.tokens[4].offset == 6);
    assert_continue(array_output.tokens[4].value.u32 == 3);

    arith_expr_result ret = parse_arithmetic_expression(array_output.tokens, array_output.tokens + sizeof(array_output.tokens) / sizeof(*array_output.tokens), array_output.parsed);
    assert_continue(ret.type == ARITH_EXPR_OK);
    assert_continue(array_output.parsed[0].type == ARITH_U32);
    assert_continue(array_output.parsed[0].value.u32 == 1);
    assert_continue(array_output.parsed[1].type == ARITH_U32);
    assert_continue(array_output.parsed[1].value.u32 == 0);
    assert_continue(array_output.parsed[2].type == ARITH_U32);
    assert_continue(array_output.parsed[2].value.u32 == 3);
    assert_continue(array_output.parsed[3].type == ARITH_ADD);
    assert_continue(array_output.parsed[4].type == ARITH_ADD);
  }

  { // three + is not ok
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("1 + + + 3");
    union {
      arith_token tokens[6];
      arith_parsed parsed[6];
    } array_output;
    arith_tokenize_capacity ret_fill = token_exp_no_sym(expr, expr + code_unit_strlen(expr), array_output.tokens);

    assert_continue(array_output.tokens[0].type == ARITH_U32);
    assert_continue(array_output.tokens[0].offset == 0);
    assert_continue(array_output.tokens[0].value.u32 == 1);
    assert_continue(array_output.tokens[1].type == ARITH_ADD);
    assert_continue(array_output.tokens[1].offset == 2);
    assert_continue(array_output.tokens[2].type == ARITH_U32);
    assert_continue(array_output.tokens[2].offset == 4);
    assert_continue(array_output.tokens[2].value.u32 == 0);
    assert_continue(array_output.tokens[3].type == ARITH_UNARY_ADD);
    assert_continue(array_output.tokens[3].offset == 4);
    assert_continue(array_output.tokens[4].type == ARITH_ADD);
    assert_continue(array_output.tokens[4].offset == 6);
    assert_continue(array_output.tokens[5].type == ARITH_U32);
    assert_continue(array_output.tokens[5].offset == 8);
    assert_continue(array_output.tokens[5].value.u32 == 3);

    arith_expr_result ret = parse_arithmetic_expression(array_output.tokens, array_output.tokens + sizeof(array_output.tokens) / sizeof(*array_output.tokens), array_output.parsed);
    assert_continue(ret.type == ARITH_EXPR_ERROR);
    assert_continue(ret.value.err.offset == 2);
    assert_continue(0 == strcmp(ret.value.err.reason, "expression stack exhausted by op"));
  }

  return has_errors;
}
