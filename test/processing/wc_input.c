#include "wc_input.h"
#include <locale.h>
#include <stdio.h>

#include "test_common.h"
extern int has_errors;

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
    size_t expected_output_size = sizeof(expected_output) / sizeof(*expected_output);
    wchar_t output[expected_output_size];
    memset(output, 0, sizeof(output));
    convert_pattern_to_wchar_range_result ret = convert_pattern_to_wchar_range(input, input + input_size, output);
    assert_continue(ret.type == CONVERT_PATTERN_WCHAR_OK);
    assert_continue(ret.value.out_end == output + input_size);
    assert_continue(0 == memcmp(expected_output, output, expected_output_size));
  }
  { // simple utf8 conversion. fits in utf16 or utf32
    const char input[] = {0xE2, 0x9C, 0x93};
    size_t input_size = sizeof(input) / sizeof(*input);
    const wchar_t expected_output[] = {L'✓', 0};
    size_t expected_output_size = sizeof(expected_output) / sizeof(*expected_output);
    wchar_t output[expected_output_size];
    memset(output, 0, sizeof(output));
    convert_pattern_to_wchar_range_result ret = convert_pattern_to_wchar_range(input, input + input_size, output);
    assert_continue(ret.type == CONVERT_PATTERN_WCHAR_OK);
    assert_continue(ret.value.out_end == output + 1);
    assert_continue(0 == memcmp(expected_output, output, expected_output_size));
  }
  { // error from bad utf8
    const char input[] = {0xFF};
    size_t input_size = sizeof(input) / sizeof(*input);
    const wchar_t expected_output[] = {L'\uFFFF', 0};
    size_t expected_output_size = sizeof(expected_output) / sizeof(*expected_output);
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
    size_t expected_output_size = sizeof(expected_output) / sizeof(*expected_output);
    wchar_t output[expected_output_size];
    memset(output, 0, sizeof(output));
    convert_pattern_to_wchar_range_result ret = convert_pattern_to_wchar_range(input, input + input_size, output);
    assert_continue(ret.type == CONVERT_PATTERN_WCHAR_ERROR);
    assert_continue(ret.value.offending_location == input + 1);
    assert_continue(0 == memcmp(expected_output, output, expected_output_size));
  }
  if (has_errors) {
    return has_errors;
  }
  { // empty input output
    assert_continue(NULL == convert_subject_to_wchar_range(NULL, NULL, false, NULL, NULL, false));
    assert_continue(NULL == convert_subject_to_wchar_range(NULL, NULL, true, NULL, NULL, false));
  }
  { // simple (including null)
    mbstate_t ps;
    memset(&ps, 0, sizeof(ps));
    const char input[] = {'a', 0, 'a'};
    size_t input_size = sizeof(input) / sizeof(*input);
    const wchar_t expected_output[] = {L'a', 0, L'a', 0};
    size_t expected_output_size = sizeof(expected_output) / sizeof(*expected_output);
    wchar_t output[expected_output_size];
    memset(output, 0, sizeof(output));
    assert_continue(output + expected_output_size - 1 == convert_subject_to_wchar_range(input, input + input_size, true, &ps, output, false));
    assert_continue(0 == memcmp(expected_output, output, expected_output_size));
  }
  { // retained state between calls
    mbstate_t ps;
    memset(&ps, 0, sizeof(ps));
    {
      const char input[] = {0xE2, 0x9C};
      size_t input_size = sizeof(input) / sizeof(*input);
      assert_continue(NULL == convert_subject_to_wchar_range(input, input + input_size, false, &ps, NULL, false));
    }
    {
      const char input[] = {0x93};
      size_t input_size = sizeof(input) / sizeof(*input);
      const wchar_t expected_output[] = {L'✓', 0};
      size_t expected_output_size = sizeof(expected_output) / sizeof(*expected_output);
      wchar_t output[expected_output_size];
      assert_continue(output + expected_output_size - 1 == convert_subject_to_wchar_range(input, input + input_size, false, &ps, output, false));
    }
  }
  { // decode error
    mbstate_t ps;
    memset(&ps, 0, sizeof(ps));
    const char input[] = {0xFF, 0xFF, 'a'};
    size_t input_size = sizeof(input) / sizeof(*input);
    const wchar_t expected_output[] = {L'\uFFFF', L'\uFFFF', 'a', 0};
    size_t expected_output_size = sizeof(expected_output) / sizeof(*expected_output);
    wchar_t output[expected_output_size];
    memset(output, 0, sizeof(output));
    assert_continue(output + expected_output_size - 1 == convert_subject_to_wchar_range(input, input + input_size, false, &ps, output, false));
    assert_continue(0 == memcmp(expected_output, output, expected_output_size));
  }
  { // decode error - skipped
    mbstate_t ps;
    memset(&ps, 0, sizeof(ps));
    const char input[] = {0xFF, 0xFF, 'a'};
    size_t input_size = sizeof(input) / sizeof(*input);
    const wchar_t expected_output[] = {'a', 0};
    size_t expected_output_size = sizeof(expected_output) / sizeof(*expected_output);
    wchar_t output[expected_output_size];
    memset(output, 0, sizeof(output));
    assert_continue(output + expected_output_size - 1 == convert_subject_to_wchar_range(input, input + input_size, false, &ps, output, true));
    assert_continue(0 == memcmp(expected_output, output, expected_output_size));
  }
  { // incomplete multibyte at end of input
    mbstate_t ps;
    memset(&ps, 0, sizeof(ps));
    const char input[] = {0xE2, 0x9C};
    size_t input_size = sizeof(input) / sizeof(*input);
    const wchar_t expected_output[] = {L'\uFFFF', L'\uFFFF', 0};
    size_t expected_output_size = sizeof(expected_output) / sizeof(*expected_output);
    wchar_t output[expected_output_size];
    memset(output, 0, sizeof(output));
    assert_continue(output + expected_output_size - 1 == convert_subject_to_wchar_range(input, input + input_size, true, &ps, output, false));
    assert_continue(0 == memcmp(expected_output, output, expected_output_size));
  }
  { // incomplete multibyte at end of input - skipped
    mbstate_t ps;
    memset(&ps, 0, sizeof(ps));
    const char input[] = {0xE2, 0x9C};
    size_t input_size = sizeof(input) / sizeof(*input);
    const wchar_t expected_output[] = {0};
    size_t expected_output_size = sizeof(expected_output) / sizeof(*expected_output);
    wchar_t output[expected_output_size];
    memset(output, 0, sizeof(output));
    assert_continue(output + expected_output_size - 1 == convert_subject_to_wchar_range(input, input + input_size, true, &ps, output, true));
    assert_continue(0 == memcmp(expected_output, output, expected_output_size));
  }
  return has_errors;
}
