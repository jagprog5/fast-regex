#define TESTING_HOOK

#include <assert.h>
#include <stddef.h>
#include <string.h>

char fread_wrapper_data_arr[100];
char* fread_wrapper_data = fread_wrapper_data_arr;
size_t fread_wrapper_data_size = 0;

void set_data_to_read_next(const char* data) {
  size_t len = strlen(data);
  assert(len <= sizeof(fread_wrapper_data) / sizeof(char));
  fread_wrapper_data_size = len;
  memcpy(fread_wrapper_data, data, len * sizeof(char));
}

#include "subject_buffer.h"

#include <locale.h>

#include "test_common.h"
extern int has_errors;

int main() {
  setlocale(LC_ALL, "");
  { // simple read
    subject_buffer_state buf;
    size_t capacity = 3;
    char byte_buffer[capacity];
#ifdef USE_WCHAR
    wchar_t character_buffer[capacity];
    init_subject_buffer(&buf, capacity, byte_buffer, character_buffer, 0);
#else
    init_subject_buffer(&buf, capacity, byte_buffer, 0);
#endif
    set_data_to_read_next("1234561");
    assert_continue(false == subject_buffer_get_first_input(&buf));
    assert_continue(buf.offset == 0);
    assert_continue(buf.size == capacity);
    // simulating pattern matching happened
    assert_continue(subject_buffer_start(&buf)[buf.offset++] == '1');
    assert_continue(subject_buffer_start(&buf)[buf.offset++] == '2');
    assert_continue(subject_buffer_start(&buf)[buf.offset++] == '3');
    assert_continue(false == subject_buffer_shift_and_get_input(&buf));
    assert_continue(buf.offset == 0);
    assert_continue(buf.size == capacity);
    assert_continue(subject_buffer_start(&buf)[buf.offset++] == '4');
    assert_continue(subject_buffer_start(&buf)[buf.offset++] == '5');
    assert_continue(subject_buffer_start(&buf)[buf.offset++] == '6');
    assert_continue(true == subject_buffer_shift_and_get_input(&buf));
    assert_continue(buf.offset == 0);
    assert_continue(buf.size == 1);
    assert_continue(subject_buffer_start(&buf)[buf.offset++] == '1');
  }
  { // try with lookbehind
    subject_buffer_state buf;
    size_t capacity = 3;
    char byte_buffer[capacity];
#ifdef USE_WCHAR
    wchar_t character_buffer[capacity];
    init_subject_buffer(&buf, capacity, byte_buffer, character_buffer, 1);
#else
    init_subject_buffer(&buf, capacity, byte_buffer, 1);
#endif
    set_data_to_read_next("123456");
    assert_continue(false == subject_buffer_get_first_input(&buf));
    assert_continue(buf.offset == 0);
    assert_continue(buf.size == capacity);
    assert_continue(subject_buffer_start(&buf)[buf.offset++] == '1');
    assert_continue(subject_buffer_start(&buf)[buf.offset++] == '2');
    assert_continue(subject_buffer_start(&buf)[buf.offset++] == '3');
    assert_continue(false == subject_buffer_shift_and_get_input(&buf));
    assert_continue(buf.offset == 1);
    assert_continue(buf.size == capacity);
    assert_continue(subject_buffer_start(&buf)[buf.offset - 1] == '3');
    assert_continue(subject_buffer_start(&buf)[buf.offset++] == '4');
    assert_continue(subject_buffer_start(&buf)[buf.offset++] == '5');
    assert_continue(true == subject_buffer_shift_and_get_input(&buf));
    assert_continue(buf.offset == 1);
    assert_continue(buf.size == 2);
    assert_continue(subject_buffer_start(&buf)[buf.offset++] == '6');
  }

  return has_errors;
}