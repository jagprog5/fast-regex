#include <string.h>
#include <uchar.h>

#define USE_WCHAR

#include "arithmetic_expression.h"

#include "test_common.h"
extern int has_errors;

int main(void) {
  { // basic operators, whitespace ignored
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("+ -\n");

    arith_token_tokenize_arg output;
    output.type = ARITH_TOKEN_TOKENIZE_ARG_FILL_ARRAY;
    arith_token array_output[3];
    memset(array_output, 0, sizeof(array_output));
    output.value.tokens = array_output;

    tokenize_arithmetic_expression(expr, &expr[code_unit_strlen(expr)], output);
    assert_continue(array_output[0].type == ARITH_TOKEN_ADD);
    assert_continue(array_output[0].begin == expr);

    assert_continue(array_output[1].type == ARITH_TOKEN_SUB);
    assert_continue(array_output[1].begin == expr + 2);

    assert_continue(array_output[2].type == ARITH_TOKEN_ERROR);
    assert_continue(array_output[2].begin == NULL);
    assert_continue(array_output[2].value.error_msg == NULL);
  }
  { // a number which end with the end of the string
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("1234");

    arith_token_tokenize_arg output;
    output.type = ARITH_TOKEN_TOKENIZE_ARG_FILL_ARRAY;
    arith_token array_output[2];
    memset(array_output, 0, sizeof(array_output));
    output.value.tokens = array_output;

    tokenize_arithmetic_expression(expr, &expr[code_unit_strlen(expr)], output);
    assert_continue(array_output[0].type == ARITH_TOKEN_U32);
    assert_continue(array_output[0].begin == expr);
    assert_continue(array_output[0].value.u32 == 1234);

    assert_continue(array_output[1].type == ARITH_TOKEN_ERROR);
    assert_continue(array_output[1].begin == NULL);
    assert_continue(array_output[1].value.error_msg == NULL);
  }
  { // a number which ends for any other reason
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("1234\n\r\t");

    arith_token_tokenize_arg output;
    output.type = ARITH_TOKEN_TOKENIZE_ARG_FILL_ARRAY;
    arith_token array_output[2];
    memset(array_output, 0, sizeof(array_output));
    output.value.tokens = array_output;

    tokenize_arithmetic_expression(expr, &expr[code_unit_strlen(expr)], output);
    assert_continue(array_output[0].type == ARITH_TOKEN_U32);
    assert_continue(array_output[0].begin == expr);
    assert_continue(array_output[0].value.u32 == 1234);

    assert_continue(array_output[1].type == ARITH_TOKEN_ERROR);
    assert_continue(array_output[1].begin == NULL);
    assert_continue(array_output[1].value.error_msg == NULL);
  }
  { // maximum value
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("4294967295");

    arith_token_tokenize_arg output;
    output.type = ARITH_TOKEN_TOKENIZE_ARG_FILL_ARRAY;
    arith_token array_output[2];
    memset(array_output, 0, sizeof(array_output));
    output.value.tokens = array_output;

    tokenize_arithmetic_expression(expr, &expr[code_unit_strlen(expr)], output);
    assert_continue(array_output[0].type == ARITH_TOKEN_U32);
    assert_continue(array_output[0].begin == expr);
    assert_continue(array_output[0].value.u32 == 4294967295);

    assert_continue(array_output[1].type == ARITH_TOKEN_ERROR);
    assert_continue(array_output[1].begin == NULL);
    assert_continue(array_output[1].value.error_msg == NULL);
  }
  { // overflow from addition in number parse
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("4294967296");

    arith_token_tokenize_arg output;
    output.type = ARITH_TOKEN_TOKENIZE_ARG_FILL_ARRAY;
    arith_token array_output[2];
    memset(array_output, 0, sizeof(array_output));
    output.value.tokens = array_output;

    tokenize_arithmetic_expression(expr, &expr[code_unit_strlen(expr)], output);
    assert_continue(array_output[0].type == ARITH_TOKEN_ERROR);
    assert_continue(array_output[0].begin == expr);
    assert_continue(0==strcmp(array_output[0].value.error_msg, "num too large"));

    assert_continue(array_output[1].type == ARITH_TOKEN_ERROR);
    assert_continue(array_output[1].begin == NULL);
    assert_continue(array_output[1].value.error_msg == NULL);
  }
  { // overflow from multiply in number parse
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("42949672950");

    arith_token_tokenize_arg output;
    output.type = ARITH_TOKEN_TOKENIZE_ARG_FILL_ARRAY;
    arith_token array_output[2];
    memset(array_output, 0, sizeof(array_output));
    output.value.tokens = array_output;

    tokenize_arithmetic_expression(expr, &expr[code_unit_strlen(expr)], output);
    assert_continue(array_output[0].type == ARITH_TOKEN_ERROR);
    assert_continue(array_output[0].begin == expr);
    assert_continue(0==strcmp(array_output[0].value.error_msg, "num too large"));

    assert_continue(array_output[1].type == ARITH_TOKEN_ERROR);
    assert_continue(array_output[1].begin == NULL);
    assert_continue(array_output[1].value.error_msg == NULL);
  }
  { // a symbol that ends with the end of the string
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("abz9_hello");

    arith_token_tokenize_arg output;
    output.type = ARITH_TOKEN_TOKENIZE_ARG_FILL_ARRAY;
    arith_token array_output[2];
    memset(array_output, 0, sizeof(array_output));
    output.value.tokens = array_output;

    tokenize_arithmetic_expression(expr, &expr[code_unit_strlen(expr)], output);
    assert_continue(array_output[0].type == ARITH_TOKEN_SYMBOL);
    assert_continue(array_output[0].begin == expr);
    assert_continue(array_output[0].value.symbol_end == expr + code_unit_strlen(expr));

    assert_continue(array_output[1].type == ARITH_TOKEN_ERROR);
    assert_continue(array_output[1].begin == NULL);
    assert_continue(array_output[1].value.error_msg == NULL);
  }
  { // a symbol that ends for any other reason
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("abz9_hello\n\t\r");

    arith_token_tokenize_arg output;
    output.type = ARITH_TOKEN_TOKENIZE_ARG_FILL_ARRAY;
    arith_token array_output[2];
    memset(array_output, 0, sizeof(array_output));
    output.value.tokens = array_output;

    tokenize_arithmetic_expression(expr, &expr[code_unit_strlen(expr)], output);
    assert_continue(array_output[0].type == ARITH_TOKEN_SYMBOL);
    assert_continue(array_output[0].begin == expr);
    assert_continue(array_output[0].value.symbol_end == expr + (code_unit_strlen(expr) - 3));

    assert_continue(array_output[1].type == ARITH_TOKEN_ERROR);
    assert_continue(array_output[1].begin == NULL);
    assert_continue(array_output[1].value.error_msg == NULL);
  }
  { // invalid character
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("\xFF");

    arith_token_tokenize_arg output;
    output.type = ARITH_TOKEN_TOKENIZE_ARG_FILL_ARRAY;
    arith_token array_output[2];
    memset(array_output, 0, sizeof(array_output));
    output.value.tokens = array_output;

    tokenize_arithmetic_expression(expr, &expr[code_unit_strlen(expr)], output);
    assert_continue(array_output[0].type == ARITH_TOKEN_ERROR);
    assert_continue(array_output[0].begin == expr);
    assert_continue(0==strcmp(array_output[0].value.error_msg, "invalid unit in arith expr"));

    assert_continue(array_output[1].type == ARITH_TOKEN_ERROR);
    assert_continue(array_output[1].begin == NULL);
    assert_continue(array_output[1].value.error_msg == NULL);
  }
  { // < from end of string
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("<");

    arith_token_tokenize_arg output;
    output.type = ARITH_TOKEN_TOKENIZE_ARG_FILL_ARRAY;
    arith_token array_output[2];
    memset(array_output, 0, sizeof(array_output));
    output.value.tokens = array_output;

    tokenize_arithmetic_expression(expr, &expr[code_unit_strlen(expr)], output);
    assert_continue(array_output[0].type == ARITH_TOKEN_LESS_THAN);
    assert_continue(array_output[0].begin == expr);

    assert_continue(array_output[1].type == ARITH_TOKEN_ERROR);
    assert_continue(array_output[1].begin == NULL);
    assert_continue(array_output[1].value.error_msg == NULL);
  }
  { // < from end of operation
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("<\r\n\t");

    arith_token_tokenize_arg output;
    output.type = ARITH_TOKEN_TOKENIZE_ARG_FILL_ARRAY;
    arith_token array_output[2];
    memset(array_output, 0, sizeof(array_output));
    output.value.tokens = array_output;

    tokenize_arithmetic_expression(expr, &expr[code_unit_strlen(expr)], output);
    assert_continue(array_output[0].type == ARITH_TOKEN_LESS_THAN);
    assert_continue(array_output[0].begin == expr);

    assert_continue(array_output[1].type == ARITH_TOKEN_ERROR);
    assert_continue(array_output[1].begin == NULL);
    assert_continue(array_output[1].value.error_msg == NULL);
  }
  { // <= from end of string
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("<=");

    arith_token_tokenize_arg output;
    output.type = ARITH_TOKEN_TOKENIZE_ARG_FILL_ARRAY;
    arith_token array_output[2];
    memset(array_output, 0, sizeof(array_output));
    output.value.tokens = array_output;

    tokenize_arithmetic_expression(expr, &expr[code_unit_strlen(expr)], output);
    assert_continue(array_output[0].type == ARITH_TOKEN_LESS_THAN_EQUAL);
    assert_continue(array_output[0].begin == expr);

    assert_continue(array_output[1].type == ARITH_TOKEN_ERROR);
    assert_continue(array_output[1].begin == NULL);
    assert_continue(array_output[1].value.error_msg == NULL);
  }
  { // <= from end of operation
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("<=\n\t\r");

    arith_token_tokenize_arg output;
    output.type = ARITH_TOKEN_TOKENIZE_ARG_FILL_ARRAY;
    arith_token array_output[2];
    memset(array_output, 0, sizeof(array_output));
    output.value.tokens = array_output;

    tokenize_arithmetic_expression(expr, &expr[code_unit_strlen(expr)], output);
    assert_continue(array_output[0].type == ARITH_TOKEN_LESS_THAN_EQUAL);
    assert_continue(array_output[0].begin == expr);

    assert_continue(array_output[1].type == ARITH_TOKEN_ERROR);
    assert_continue(array_output[1].begin == NULL);
    assert_continue(array_output[1].value.error_msg == NULL);
  }
  { // >
    const CODE_UNIT* expr = CODE_UNIT_LITERAL(">\r\n\t");

    arith_token_tokenize_arg output;
    output.type = ARITH_TOKEN_TOKENIZE_ARG_FILL_ARRAY;
    arith_token array_output[2];
    memset(array_output, 0, sizeof(array_output));
    output.value.tokens = array_output;

    tokenize_arithmetic_expression(expr, &expr[code_unit_strlen(expr)], output);
    assert_continue(array_output[0].type == ARITH_TOKEN_GREATER_THAN);
    assert_continue(array_output[0].begin == expr);

    assert_continue(array_output[1].type == ARITH_TOKEN_ERROR);
    assert_continue(array_output[1].begin == NULL);
    assert_continue(array_output[1].value.error_msg == NULL);
  }
  { // >=
    const CODE_UNIT* expr = CODE_UNIT_LITERAL(">=\r\n\t");

    arith_token_tokenize_arg output;
    output.type = ARITH_TOKEN_TOKENIZE_ARG_FILL_ARRAY;
    arith_token array_output[2];
    memset(array_output, 0, sizeof(array_output));
    output.value.tokens = array_output;

    tokenize_arithmetic_expression(expr, &expr[code_unit_strlen(expr)], output);
    assert_continue(array_output[0].type == ARITH_TOKEN_GREATER_THAN_EQUAL);
    assert_continue(array_output[0].begin == expr);

    assert_continue(array_output[1].type == ARITH_TOKEN_ERROR);
    assert_continue(array_output[1].begin == NULL);
    assert_continue(array_output[1].value.error_msg == NULL);
  }
  { // & from end of string
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("&");

    arith_token_tokenize_arg output;
    output.type = ARITH_TOKEN_TOKENIZE_ARG_FILL_ARRAY;
    arith_token array_output[2];
    memset(array_output, 0, sizeof(array_output));
    output.value.tokens = array_output;

    tokenize_arithmetic_expression(expr, &expr[code_unit_strlen(expr)], output);
    assert_continue(array_output[0].type == ARITH_TOKEN_BITWISE_AND);
    assert_continue(array_output[0].begin == expr);

    assert_continue(array_output[1].type == ARITH_TOKEN_ERROR);
    assert_continue(array_output[1].begin == NULL);
    assert_continue(array_output[1].value.error_msg == NULL);
  }
  { // & from end of operation
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("&\r\n\t");

    arith_token_tokenize_arg output;
    output.type = ARITH_TOKEN_TOKENIZE_ARG_FILL_ARRAY;
    arith_token array_output[2];
    memset(array_output, 0, sizeof(array_output));
    output.value.tokens = array_output;

    tokenize_arithmetic_expression(expr, &expr[code_unit_strlen(expr)], output);
    assert_continue(array_output[0].type == ARITH_TOKEN_BITWISE_AND);
    assert_continue(array_output[0].begin == expr);

    assert_continue(array_output[1].type == ARITH_TOKEN_ERROR);
    assert_continue(array_output[1].begin == NULL);
    assert_continue(array_output[1].value.error_msg == NULL);
  }
  { // && from end of string
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("&&");

    arith_token_tokenize_arg output;
    output.type = ARITH_TOKEN_TOKENIZE_ARG_FILL_ARRAY;
    arith_token array_output[2];
    memset(array_output, 0, sizeof(array_output));
    output.value.tokens = array_output;

    tokenize_arithmetic_expression(expr, &expr[code_unit_strlen(expr)], output);
    assert_continue(array_output[0].type == ARITH_TOKEN_LOGICAL_AND);
    assert_continue(array_output[0].begin == expr);

    assert_continue(array_output[1].type == ARITH_TOKEN_ERROR);
    assert_continue(array_output[1].begin == NULL);
    assert_continue(array_output[1].value.error_msg == NULL);
  }
  { // && from end of operation
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("&&\r\n\t");

    arith_token_tokenize_arg output;
    output.type = ARITH_TOKEN_TOKENIZE_ARG_FILL_ARRAY;
    arith_token array_output[2];
    memset(array_output, 0, sizeof(array_output));
    output.value.tokens = array_output;

    tokenize_arithmetic_expression(expr, &expr[code_unit_strlen(expr)], output);
    assert_continue(array_output[0].type == ARITH_TOKEN_LOGICAL_AND);
    assert_continue(array_output[0].begin == expr);

    assert_continue(array_output[1].type == ARITH_TOKEN_ERROR);
    assert_continue(array_output[1].begin == NULL);
    assert_continue(array_output[1].value.error_msg == NULL);
  }
  {
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("\'");
    arith_token_tokenize_arg output;
    output.type = ARITH_TOKEN_TOKENIZE_ARG_FILL_ARRAY;
    arith_token array_output[2];
    memset(array_output, 0, sizeof(array_output));
    output.value.tokens = array_output;

    tokenize_arithmetic_expression(expr, &expr[code_unit_strlen(expr)], output);
    assert_continue(array_output[0].type == ARITH_TOKEN_ERROR);
    assert_continue(array_output[0].begin == expr);
    assert_continue(array_output[0].value.error_msg == "literal ended without content");

    assert_continue(array_output[1].type == ARITH_TOKEN_ERROR);
    assert_continue(array_output[1].begin == NULL);
    assert_continue(array_output[1].value.error_msg == NULL);
  }
  {
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("'a");
    arith_token_tokenize_arg output;
    output.type = ARITH_TOKEN_TOKENIZE_ARG_FILL_ARRAY;
    arith_token array_output[2];
    memset(array_output, 0, sizeof(array_output));
    output.value.tokens = array_output;

    tokenize_arithmetic_expression(expr, &expr[code_unit_strlen(expr)], output);
    assert_continue(array_output[0].type == ARITH_TOKEN_ERROR);
    assert_continue(array_output[0].begin == expr);
    assert_continue(array_output[0].value.error_msg == "literal ended without closing quote");

    assert_continue(array_output[1].type == ARITH_TOKEN_ERROR);
    assert_continue(array_output[1].begin == NULL);
    assert_continue(array_output[1].value.error_msg == NULL);
  }
  {
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("'ab");
    arith_token_tokenize_arg output;
    output.type = ARITH_TOKEN_TOKENIZE_ARG_FILL_ARRAY;
    arith_token array_output[2];
    memset(array_output, 0, sizeof(array_output));
    output.value.tokens = array_output;

    tokenize_arithmetic_expression(expr, &expr[code_unit_strlen(expr)], output);
    assert_continue(array_output[0].type == ARITH_TOKEN_ERROR);
    assert_continue(array_output[0].begin == expr + 2);
    assert_continue(array_output[0].value.error_msg == "expecting end of literal");

    assert_continue(array_output[1].type == ARITH_TOKEN_ERROR);
    assert_continue(array_output[1].begin == NULL);
    assert_continue(array_output[1].value.error_msg == NULL);
  }
  {
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("'a'");
    arith_token_tokenize_arg output;
    output.type = ARITH_TOKEN_TOKENIZE_ARG_FILL_ARRAY;
    arith_token array_output[2];
    memset(array_output, 0, sizeof(array_output));
    output.value.tokens = array_output;

    tokenize_arithmetic_expression(expr, &expr[code_unit_strlen(expr)], output);
    assert_continue(array_output[0].type == ARITH_TOKEN_U32);
    assert_continue(array_output[0].begin == expr);
    assert_continue(array_output[0].value.u32 = 'a');

    assert_continue(array_output[1].type == ARITH_TOKEN_ERROR);
    assert_continue(array_output[1].begin == NULL);
    assert_continue(array_output[1].value.error_msg == NULL);
  }
  {
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("'\\z");
    arith_token_tokenize_arg output;
    output.type = ARITH_TOKEN_TOKENIZE_ARG_FILL_ARRAY;
    arith_token array_output[2];
    memset(array_output, 0, sizeof(array_output));
    output.value.tokens = array_output;

    tokenize_arithmetic_expression(expr, &expr[code_unit_strlen(expr)], output);
    assert_continue(array_output[0].type == ARITH_TOKEN_ERROR);
    assert_continue(array_output[0].begin == expr + 2);
    assert_continue(array_output[0].value.error_msg == "invalid escaped character");

    assert_continue(array_output[1].type == ARITH_TOKEN_ERROR);
    assert_continue(array_output[1].begin == NULL);
    assert_continue(array_output[1].value.error_msg == NULL);
  }
  {
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("'\\n''\\r''\\xaF''\\x'");
    arith_token_tokenize_arg output;
    output.type = ARITH_TOKEN_TOKENIZE_ARG_FILL_ARRAY;
    arith_token array_output[5];
    memset(array_output, 0, sizeof(array_output));
    output.value.tokens = array_output;

    tokenize_arithmetic_expression(expr, &expr[code_unit_strlen(expr)], output);

    assert_continue(array_output[0].type == ARITH_TOKEN_U32);
    assert_continue(array_output[0].begin == expr);
    assert_continue(array_output[0].value.u32 == '\n');

    assert_continue(array_output[1].type == ARITH_TOKEN_U32);
    assert_continue(array_output[1].begin == expr + 4);
    assert_continue(array_output[1].value.u32 == '\r');

    assert_continue(array_output[2].type == ARITH_TOKEN_U32);
    assert_continue(array_output[2].begin == expr + 8);
    assert_continue(array_output[2].value.u32 == 0xAF);

    assert_continue(array_output[3].type == ARITH_TOKEN_U32);
    assert_continue(array_output[3].begin == expr + 14);
    assert_continue(array_output[3].value.u32 == 0);

    assert_continue(array_output[4].type == ARITH_TOKEN_ERROR);
    assert_continue(array_output[4].begin == NULL);
    assert_continue(array_output[4].value.error_msg == NULL);
  }
  {
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("'\\xFFFFFFFFF'"); // overlong
    arith_token_tokenize_arg output;
    output.type = ARITH_TOKEN_TOKENIZE_ARG_FILL_ARRAY;
    arith_token array_output[2];
    memset(array_output, 0, sizeof(array_output));
    output.value.tokens = array_output;

    tokenize_arithmetic_expression(expr, &expr[code_unit_strlen(expr)], output);

    assert_continue(array_output[0].type == ARITH_TOKEN_ERROR);
    assert_continue(array_output[0].begin == expr + 11);
    assert_continue(array_output[0].value.error_msg == "expecting end of literal");

    assert_continue(array_output[1].type == ARITH_TOKEN_ERROR);
    assert_continue(array_output[1].begin == NULL);
    assert_continue(array_output[1].value.error_msg == NULL);
  }
  {
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("'\\xQ'");
    arith_token_tokenize_arg output;
    output.type = ARITH_TOKEN_TOKENIZE_ARG_FILL_ARRAY;
    arith_token array_output[2];
    memset(array_output, 0, sizeof(array_output));
    output.value.tokens = array_output;

    tokenize_arithmetic_expression(expr, &expr[code_unit_strlen(expr)], output);

    assert_continue(array_output[0].type == ARITH_TOKEN_ERROR);
    assert_continue(array_output[0].begin == expr + 3);
    assert_continue(array_output[0].value.error_msg == "invalid hex escaped character");

    assert_continue(array_output[1].type == ARITH_TOKEN_ERROR);
    assert_continue(array_output[1].begin == NULL);
    assert_continue(array_output[1].value.error_msg == NULL);
  }
  { // test wholistic
    const CODE_UNIT* expr = CODE_UNIT_LITERAL("(variable & 6) = (32 / -'a')");
    arith_token_tokenize_arg output_size_arg;
    output_size_arg.type = ARITH_TOKEN_TOKENIZE_ARG_GET_CAPACITY;
    size_t output_size = 0;
    output_size_arg.value.capacity = &output_size;
    tokenize_arithmetic_expression(expr, &expr[code_unit_strlen(expr)], output_size_arg);
    assert_continue(output_size == 12);

    arith_token array_output[13];
    memset(array_output, 0, sizeof(array_output));

    arith_token_tokenize_arg output;
    output.type = ARITH_TOKEN_TOKENIZE_ARG_FILL_ARRAY;
    output.value.tokens = array_output;
    tokenize_arithmetic_expression(expr, &expr[code_unit_strlen(expr)], output);

    assert_continue(array_output[0].type == ARITH_TOKEN_LEFT_BRACKET);
    assert_continue(array_output[0].begin == expr);

    assert_continue(array_output[1].type == ARITH_TOKEN_SYMBOL);
    assert_continue(array_output[1].begin == expr + 1);
    assert_continue(array_output[1].value.symbol_end == expr + 9);

    assert_continue(array_output[2].type == ARITH_TOKEN_BITWISE_AND);
    assert_continue(array_output[2].begin == expr + 10);

    assert_continue(array_output[3].type == ARITH_TOKEN_U32);
    assert_continue(array_output[3].begin == expr + 12);
    assert_continue(array_output[3].value.u32 == 6);

    assert_continue(array_output[4].type == ARITH_TOKEN_RIGHT_BRACKET);
    assert_continue(array_output[4].begin == expr + 13);

    assert_continue(array_output[5].type == ARITH_TOKEN_EQUALITY);
    assert_continue(array_output[5].begin == expr + 15);

    assert_continue(array_output[6].type == ARITH_TOKEN_LEFT_BRACKET);
    assert_continue(array_output[6].begin == expr + 17);

    assert_continue(array_output[7].type == ARITH_TOKEN_U32);
    assert_continue(array_output[7].begin == expr + 18);
    assert_continue(array_output[7].value.u32 == 32);

    assert_continue(array_output[8].type == ARITH_TOKEN_DIV);
    assert_continue(array_output[8].begin == expr + 21);

    assert_continue(array_output[9].type == ARITH_TOKEN_SUB);
    assert_continue(array_output[9].begin == expr + 23);

    assert_continue(array_output[10].type == ARITH_TOKEN_U32);
    assert_continue(array_output[10].begin == expr + 24);
    assert_continue(array_output[10].value.u32 == 'a');

    assert_continue(array_output[11].type == ARITH_TOKEN_RIGHT_BRACKET);
    assert_continue(array_output[11].begin == expr + 27);

    assert_continue(array_output[12].type == ARITH_TOKEN_ERROR);
    assert_continue(array_output[12].begin == NULL);
    assert_continue(array_output[12].value.error_msg == NULL);
  }
  return has_errors;
}
