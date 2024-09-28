#include <locale.h>
#include <stdio.h>
#include <time.h>

#include "character/wc_input.h"

#include "test_common.h"
extern int has_errors;

// in this file, memset typically used to zero out an output buffer.
// 1 more element that is needed was allocated, and should remain zero after being populated
// (it's solely being used here as bound check, but really valgrind should be checking all of this anyways).

int main(void) {
  setlocale(LC_ALL, "");
  { // empty input output
    convert_pattern_to_wchar_range_result ret = convert_pattern_to_wchar_range(NULL, NULL, NULL);
    assert_continue(ret.type == CONVERT_PATTERN_WCHAR_OK);
    assert_continue(ret.value.out_end == NULL);
  }
  { // includes null
    const char input[] = {'a', 'b', '\0', 'w'};
    size_t input_size = sizeof(input) / sizeof(*input);
    const wchar_t expected_output[] = {L'a', L'b', L'\0', L'w', 0};
    size_t expected_output_num_elements = sizeof(expected_output) / sizeof(*expected_output);
    wchar_t output[expected_output_num_elements];
    memset(output, 0, sizeof(output));
    convert_pattern_to_wchar_range_result ret = convert_pattern_to_wchar_range(input, input + input_size, output);
    assert_continue(ret.type == CONVERT_PATTERN_WCHAR_OK);
    assert_continue(ret.value.out_end == output + input_size);
    assert_continue(0 == memcmp(expected_output, output, sizeof(expected_output)));
  }
  { // simple utf8 conversion
    const char input[] = {0xE2, 0x9C, 0x93};
    size_t input_size = sizeof(input) / sizeof(*input);
    const wchar_t expected_output[] = {L'✓', 0};
    size_t expected_output_num_elements = sizeof(expected_output) / sizeof(*expected_output);
    wchar_t output[expected_output_num_elements];
    memset(output, 0, sizeof(output));
    convert_pattern_to_wchar_range_result ret = convert_pattern_to_wchar_range(input, input + input_size, output);
    assert_continue(ret.type == CONVERT_PATTERN_WCHAR_OK);
    assert_continue(ret.value.out_end == output + 1);
    assert_continue(0 == memcmp(expected_output, output, sizeof(expected_output)));
  }
  { // error from bad utf8
    const char input[] = {0xFF};
    size_t input_size = sizeof(input) / sizeof(*input);
    convert_pattern_to_wchar_range_result ret = convert_pattern_to_wchar_range(input, input + input_size, NULL);
    assert_continue(ret.type == CONVERT_PATTERN_WCHAR_ERROR);
    assert_continue(ret.value.offending_location == input);
  }
  { // error from incomplete multibyte
    const char input[] = {0xE2, 0x9C};
    size_t input_size = sizeof(input) / sizeof(*input);
    convert_pattern_to_wchar_range_result ret = convert_pattern_to_wchar_range(input, input + input_size, NULL);
    assert_continue(ret.type == CONVERT_PATTERN_WCHAR_ERROR);
    assert_continue(ret.value.offending_location == input);
  }
  { // same as above, but at a different position
    const char input[] = {'a', 0xE2, 0x9C};
    size_t input_size = sizeof(input) / sizeof(*input);
    const wchar_t expected_output[] = {L'a', 0};
    size_t expected_output_num_elements = sizeof(expected_output) / sizeof(*expected_output);
    wchar_t output[expected_output_num_elements];
    memset(output, 0, sizeof(output));
    convert_pattern_to_wchar_range_result ret = convert_pattern_to_wchar_range(input, input + input_size, output);
    assert_continue(ret.type == CONVERT_PATTERN_WCHAR_ERROR);
    assert_continue(ret.value.offending_location == input + 1);
    assert_continue(0 == memcmp(expected_output, output, sizeof(expected_output)));
  }
  if (has_errors) {
    return has_errors;
  }
  { // nominal case (includes null char in input)
    subject_decode_state state;
    memset(&state, 0, sizeof(state));

    const char input[] = {'a', 'b', 0, 'c'};
    size_t input_size = sizeof(input) / sizeof(*input);

    convert_subject_to_wchar_range_input in;
    in.begin = input;
    in.end = input + input_size;
    in.complete = false;
    in.skip_invalid = false;
    in.state = &state;

    wchar_t output[input_size];
    unsigned int deltas[input_size];
    convert_subject_to_wchar_range_output out;
    out.begin = output;
    out.end = out.begin + input_size; // same as output size
    out.deltas_begin = deltas;

    convert_subject_to_wchar_range_ret ret = convert_subject_to_wchar_range(in, out);
    assert_continue(ret.in_begin == input + input_size);
    assert_continue(ret.out_begin == output + input_size);
    assert_continue(output[0] == 'a');
    assert_continue(output[1] == 'b');
    assert_continue(output[2] == 0);
    assert_continue(output[3] == 'c');
    assert_continue(deltas[0] == 1);
    assert_continue(deltas[1] == 1);
    assert_continue(deltas[2] == 1);
    assert_continue(deltas[3] == 1);
  }
  { // empty output
    subject_decode_state state;
    memset(&state, 0, sizeof(state));

    const char input[] = {'a', 'b', 0, 'c'};
    size_t input_size = sizeof(input) / sizeof(*input);

    convert_subject_to_wchar_range_input in;
    in.begin = input;
    in.end = input + input_size;
    ;
    in.complete = false;
    in.skip_invalid = false;
    in.state = &state;

    convert_subject_to_wchar_range_output out;
    out.begin = NULL;
    out.end = NULL;
    out.deltas_begin = NULL;
    convert_subject_to_wchar_range_ret ret = convert_subject_to_wchar_range(in, out);
    assert_continue(ret.in_begin == input);
    assert_continue(ret.out_begin == NULL);
  }
  { // input which runs out before the output does
    subject_decode_state state;
    memset(&state, 0, sizeof(state));

    const char input[] = {'a', 'b', 0, 'c'};
    size_t input_size = sizeof(input) / sizeof(*input);

    convert_subject_to_wchar_range_input in;
    in.begin = input;
    in.end = input + input_size;
    in.complete = false;
    in.skip_invalid = false;
    in.state = &state;

    size_t output_size = input_size + 1;
    wchar_t output[output_size];
    unsigned int deltas[output_size];
    convert_subject_to_wchar_range_output out;
    out.begin = output;
    out.end = out.begin + output_size;
    out.deltas_begin = deltas;

    convert_subject_to_wchar_range_ret ret = convert_subject_to_wchar_range(in, out);
    assert_continue(ret.in_begin == input + input_size);
    assert_continue(ret.out_begin == output + input_size);
    assert_continue(output[0] == 'a');
    assert_continue(output[1] == 'b');
    assert_continue(output[2] == 0);
    assert_continue(deltas[0] == 1);
    assert_continue(deltas[1] == 1);
    assert_continue(deltas[2] == 1);
  }
  { // conversion of multibyte
    subject_decode_state state;
    memset(&state, 0, sizeof(state));

    const char input[] = {0xE2, 0x9C, 0x93};
    size_t input_size = sizeof(input) / sizeof(*input);

    convert_subject_to_wchar_range_input in;
    in.begin = input;
    in.end = input + input_size;
    in.complete = false;
    in.skip_invalid = false;
    in.state = &state;

    size_t output_size = 1;
    wchar_t output[output_size];
    unsigned int deltas[output_size];
    convert_subject_to_wchar_range_output out;
    out.begin = output;
    out.end = out.begin + output_size;
    out.deltas_begin = deltas;

    convert_subject_to_wchar_range_ret ret = convert_subject_to_wchar_range(in, out);
    assert_continue(ret.in_begin == input + input_size);
    assert_continue(ret.out_begin == output + output_size);
    assert_continue(output[0] == L'✓');
    assert_continue(deltas[0] == 3);
  }
  { // retained stated between multiple calls
    subject_decode_state state;
    memset(&state, 0, sizeof(state));
    { // first call
      const char input[] = {0xE2, 0x9C};
      size_t input_size = sizeof(input) / sizeof(*input);

      convert_subject_to_wchar_range_input in;
      in.begin = input;
      in.end = input + input_size;
      in.complete = false;
      in.skip_invalid = false;
      in.state = &state;

      size_t output_size = 1;
      wchar_t output[output_size];
      unsigned int deltas[output_size];
      convert_subject_to_wchar_range_output out;
      out.begin = output;
      out.end = out.begin + output_size;
      out.deltas_begin = deltas;

      convert_subject_to_wchar_range_ret ret = convert_subject_to_wchar_range(in, out);
      assert_continue(ret.in_begin == input + input_size);
      assert_continue(ret.out_begin == output);
      assert_continue(state.bytes_consumed_for_ps == 2);
    }
    { // second call
      const char input[] = {0x93};
      size_t input_size = sizeof(input) / sizeof(*input);

      convert_subject_to_wchar_range_input in;
      in.begin = input;
      in.end = input + input_size;
      in.complete = false;
      in.skip_invalid = false;
      in.state = &state;

      size_t output_size = 1;
      wchar_t output[output_size];
      unsigned int deltas[output_size];
      convert_subject_to_wchar_range_output out;
      out.begin = output;
      out.end = out.begin + output_size;
      out.deltas_begin = deltas;

      convert_subject_to_wchar_range_ret ret = convert_subject_to_wchar_range(in, out);
      assert_continue(ret.in_begin == input + input_size);
      assert_continue(ret.out_begin == output + output_size);
      assert_continue(output[0] == L'✓');
      assert_continue(deltas[0] == 3);
    }
  }
  { // decode error
    subject_decode_state state;
    memset(&state, 0, sizeof(state));

    const char input[] = {0xFF, 0xFF, 'a'};
    size_t input_size = sizeof(input) / sizeof(*input);

    convert_subject_to_wchar_range_input in;
    in.begin = input;
    in.end = input + input_size;
    in.complete = false;
    in.skip_invalid = false;
    in.state = &state;

    size_t output_size = 3;
    wchar_t output[output_size];
    unsigned int deltas[output_size];
    convert_subject_to_wchar_range_output out;
    out.begin = output;
    out.end = out.begin + output_size;
    out.deltas_begin = deltas;

    convert_subject_to_wchar_range_ret ret = convert_subject_to_wchar_range(in, out);
    assert_continue(ret.in_begin == input + input_size);
    assert_continue(ret.out_begin == output + output_size);
    assert_continue(output[0] == L'\uFFFF');
    assert_continue(output[1] == L'\uFFFF');
    assert_continue(output[2] == 'a');
    assert_continue(deltas[0] == 1);
    assert_continue(deltas[1] == 1);
    assert_continue(deltas[2] == 1);
  }
  { // decode error + skip invalid
    subject_decode_state state;
    memset(&state, 0, sizeof(state));

    const char input[] = {0xFF, 0xFF, 'a'};
    size_t input_size = sizeof(input) / sizeof(*input);

    convert_subject_to_wchar_range_input in;
    in.begin = input;
    in.end = input + input_size;
    in.complete = false;
    in.skip_invalid = true;
    in.state = &state;

    size_t output_size = 1;
    wchar_t output[output_size];
    unsigned int deltas[output_size];
    convert_subject_to_wchar_range_output out;
    out.begin = output;
    out.end = out.begin + output_size;
    out.deltas_begin = deltas;

    convert_subject_to_wchar_range_ret ret = convert_subject_to_wchar_range(in, out);
    assert_continue(ret.in_begin == input + input_size);
    assert_continue(ret.out_begin == output + output_size);
    assert_continue(output[0] == 'a');
    assert_continue(deltas[0] == 3);
  }
  { // incomplete multibyte at end of input
    subject_decode_state state;
    memset(&state, 0, sizeof(state));

    const char input[] = {0xE2, 0x9C};
    size_t input_size = sizeof(input) / sizeof(*input);

    convert_subject_to_wchar_range_input in;
    in.begin = input;
    in.end = input + input_size;
    in.complete = true;
    in.skip_invalid = false;
    in.state = &state;

    size_t output_size = 5;
    wchar_t output[output_size];
    unsigned int deltas[output_size];
    convert_subject_to_wchar_range_output out;
    out.begin = output;
    out.end = out.begin + output_size;
    out.deltas_begin = deltas;

    convert_subject_to_wchar_range_ret ret = convert_subject_to_wchar_range(in, out);
    assert_continue(ret.in_begin == input + input_size);
    assert_continue(ret.out_begin == output + 1);
    assert_continue(output[0] == L'\uFFFF');
    assert_continue(deltas[0] == 2);
    assert_continue(state.bytes_consumed_for_ps == 0);
  }
  { // a fuzzy check for an invariant - ensure file offset correct
    srand(time(NULL));

    for (size_t fuzz_count = 0; fuzz_count < 1000; ++fuzz_count) {
      subject_decode_state state;
      memset(&state, 0, sizeof(state));

      size_t fuzz_input_size = 2 * sizeof(wchar_t) + 1;

      size_t output_size = fuzz_input_size;
      wchar_t output[output_size];
      unsigned int deltas[output_size];
      convert_subject_to_wchar_range_output out;
      out.begin = output;
      out.end = out.begin + output_size;
      out.deltas_begin = deltas;
      
      for (size_t i = 0; i < fuzz_input_size; ++i) {
        char input = rand();

        convert_subject_to_wchar_range_input in;
        in.begin = &input;
        in.end = &input + 1;
        in.complete = i == 2 * sizeof(wchar_t);
        in.skip_invalid = rand() & 0b1;
        in.state = &state;

        convert_subject_to_wchar_range_ret ret = convert_subject_to_wchar_range(in, out);
        // given this test input and output,
        // the input must be consumed, and the output may or may not move forward by 1
        if (out.begin != ret.out_begin && out.begin + 1 != ret.out_begin) {
          assert_continue(false);
          goto break_outer;
        }
        out.deltas_begin += ret.out_begin - out.begin;
        out.begin = ret.out_begin;

        if (ret.in_begin != &input + 1) {
          assert_continue(false);
          goto break_outer;
        }
      }

      size_t sum = 0;
      sum += state.bytes_consumed_for_ps;
      size_t output_sent = out.begin - output;
      for (size_t i = 0; i < output_sent; ++i) {
        sum += deltas[i];
      }
      if (fuzz_input_size != sum) {
        assert_continue(false);
        goto break_outer;
      }
    }
    break_outer:
  }
  return has_errors;
}
