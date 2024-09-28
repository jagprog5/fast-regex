#define TESTING_HOOK

#include <assert.h>
#include <locale.h>
#include <stddef.h>
#include <string.h>

char fread_wrapper_data_arr[100];
char* fread_wrapper_data = fread_wrapper_data_arr;
size_t fread_wrapper_data_size = 0;

void set_data_to_read_next(const char* data) {
  size_t len = strlen(data);
  assert(len <= sizeof(fread_wrapper_data_arr) / sizeof(*fread_wrapper_data));
  fread_wrapper_data_size = len;
  memcpy(fread_wrapper_data, data, len * sizeof(*fread_wrapper_data));
}

#include "character/subject_buffer.h"

#include "test_common.h"
extern int has_errors;

int main() {
  setlocale(LC_ALL, "");
  { // simple
    set_data_to_read_next("1234");
    subject_buffer_state buf;
    size_t capacity = 3;
    char read_buffer[capacity];
#ifdef USE_WCHAR
    wchar_t character_buffer[capacity];
    unsigned int character_sizes[capacity];
    init_subject_buffer(&buf, NULL, capacity, read_buffer, capacity, character_buffer, character_sizes, 0);
#else
    init_subject_buffer(&buf, NULL, capacity, read_buffer, 0);
#endif

    assert_continue(false == subject_buffer_get_input(&buf, true));
    assert_continue(subject_buffer_begin(&buf)[buf.offset++] == '1');
    assert_continue(subject_buffer_begin(&buf)[buf.offset++] == '2');
    assert_continue(subject_buffer_begin(&buf)[buf.offset++] == '3');

#ifdef USE_WCHAR
    assert_continue(character_sizes[0] == 1);
    assert_continue(character_sizes[1] == 1);
    assert_continue(character_sizes[2] == 1);
#endif

  }
//   { // simple read
//     subject_buffer_state buf;
//     size_t capacity = 3;
//     char byte_buffer[capacity];
// #ifdef USE_WCHAR
//     wchar_t character_buffer[capacity];
//     unsigned char character_sizes[capacity];
//     init_subject_buffer(&buf, NULL, capacity, byte_buffer, character_buffer, character_sizes, 0);
// #else
//     init_subject_buffer(&buf, NULL, capacity, byte_buffer, 0);
// #endif
//     set_data_to_read_next("1234561");
//     assert_continue(false == subject_buffer_get_first_input(&buf));
//     assert_continue(buf.offset == 0);
//     assert_continue(buf.size == capacity);
//     assert_continue(buf.file_offset == 0);

// #ifdef USE_WCHAR
//     assert_continue(character_sizes[0] == 1);
//     assert_continue(character_sizes[1] == 1);
//     assert_continue(character_sizes[2] == 1);
// #endif

//     // simulating pattern matching happened
//     assert_continue(subject_buffer_begin(&buf)[buf.offset++] == '1');
//     assert_continue(subject_buffer_begin(&buf)[buf.offset++] == '2');
//     assert_continue(subject_buffer_begin(&buf)[buf.offset++] == '3');
//     assert_continue(false == subject_buffer_shift_and_get_input(&buf));
//     assert_continue(buf.offset == 0);
//     assert_continue(buf.size == capacity);
//     assert_continue(buf.file_offset == 3);
//     assert_continue(subject_buffer_begin(&buf)[buf.offset++] == '4');
//     assert_continue(subject_buffer_begin(&buf)[buf.offset++] == '5');
//     assert_continue(subject_buffer_begin(&buf)[buf.offset++] == '6');
//     assert_continue(true == subject_buffer_shift_and_get_input(&buf));
//     assert_continue(buf.offset == 0);
//     assert_continue(buf.size == 1);
//     assert_continue(buf.file_offset == 6);
//     assert_continue(subject_buffer_begin(&buf)[buf.offset++] == '1');
//   }
//   { // try with lookbehind
//     // under normally circumstances this initialization if buf is bad,
//     // since a capacity of 3 bytes can't contain a single 
//     subject_buffer_state buf;
//     size_t capacity = 3;
//     char byte_buffer[capacity];
// #ifdef USE_WCHAR
//     wchar_t character_buffer[capacity];
//     unsigned char character_sizes[capacity];
//     init_subject_buffer(&buf, NULL, capacity, byte_buffer, character_buffer, character_sizes, 1);
// #else
//     init_subject_buffer(&buf, NULL, capacity, byte_buffer, 1);
// #endif
//     set_data_to_read_next("123456");
//     assert_continue(false == subject_buffer_get_first_input(&buf));
//     assert_continue(buf.offset == 0);
//     assert_continue(buf.size == capacity);
//     assert_continue(buf.file_offset == 0);

// #ifdef USE_WCHAR
//     assert_continue(character_sizes[0] == 1);
//     assert_continue(character_sizes[1] == 1);
//     assert_continue(character_sizes[2] == 1);
// #endif

//     assert_continue(subject_buffer_begin(&buf)[buf.offset++] == '1');
//     assert_continue(subject_buffer_begin(&buf)[buf.offset++] == '2');
//     assert_continue(subject_buffer_begin(&buf)[buf.offset++] == '3');
//     assert_continue(false == subject_buffer_shift_and_get_input(&buf));
//     assert_continue(buf.offset == 1);
//     assert_continue(buf.size == capacity);
//     assert_continue(buf.file_offset == 2);
//     assert_continue(subject_buffer_begin(&buf)[buf.offset - 1] == '3');
//     assert_continue(subject_buffer_begin(&buf)[buf.offset++] == '4');
//     assert_continue(subject_buffer_begin(&buf)[buf.offset++] == '5');
//     assert_continue(true == subject_buffer_shift_and_get_input(&buf));
//     assert_continue(buf.file_offset == 4);
//     assert_continue(buf.offset == 1);
//     assert_continue(buf.size == 2);
//     assert_continue(subject_buffer_begin(&buf)[buf.offset - 1] == '5');
//     assert_continue(subject_buffer_begin(&buf)[buf.offset++] == '6');
//   }

// #ifdef USE_WCHAR
//   { // try with more complicated case for wchar_t
//     subject_buffer_state buf;
//     size_t capacity = 2;
//     char byte_buffer[capacity];
//     wchar_t character_buffer[capacity];
//     unsigned char character_sizes[capacity];
//     init_subject_buffer(&buf, NULL, capacity, byte_buffer, character_buffer, character_sizes, 1);

//     // in summary, lookbehind of 1, total capacity of 2, and
//     // 3 characters in the input data (1,3,3 bytes each)
//     char data[] = {'a', 0xE2, 0x9C, 0x93, 0xE2, 0x9C, 0x93, 0x00};
//     set_data_to_read_next(data);
//     assert_continue(false == subject_buffer_get_first_input(&buf));
//     assert_continue(buf.offset == 0);
//     assert_continue(buf.size == 1); // from the 'a'
//     assert_continue(buf.character_sizes[0] == 1);
//     assert_continue(buf.character_buffer[0] == 'a');
//     assert_continue(buf.ps.bytes_consumed_for_ps == 1);
//     assert_continue(buf.file_offset == 0);

//     // simulate pattern matching
//     buf.offset += 1; // offset points after 'a', and at the end of the buffer content

//     // printf("%lu,  %lu\n", buf.offset, buf.max_lookbehind);
//     assert_continue(false == subject_buffer_shift_and_get_input(&buf));
//     // offset still 0, the 'a' is retained from the lookbehind
//     assert_continue(buf.offset == 0);

//     // assert_continue(buf.size == 1); // from the 'a'
//     // assert_continue(buf.character_sizes[0] == 1);
//     // assert_continue(buf.character_buffer[0] == 'a');
//     // assert_continue(buf.ps.bytes_consumed_for_ps == 1);
//     // assert_continue(buf.file_offset == 0);

//     // assert_continue(subject_buffer_begin(&buf)[buf.offset++] == '1');
//     // assert_continue(subject_buffer_begin(&buf)[buf.offset++] == '2');
//     // assert_continue(subject_buffer_begin(&buf)[buf.offset++] == '3');
//     // assert_continue(false == subject_buffer_shift_and_get_input(&buf));
//     // assert_continue(buf.offset == 1);
//     // assert_continue(buf.size == capacity);
//     // assert_continue(buf.file_offset == 2);
//     // assert_continue(subject_buffer_begin(&buf)[buf.offset - 1] == '3');
//     // assert_continue(subject_buffer_begin(&buf)[buf.offset++] == '4');
//     // assert_continue(subject_buffer_begin(&buf)[buf.offset++] == '5');
//     // assert_continue(true == subject_buffer_shift_and_get_input(&buf));
//     // assert_continue(buf.file_offset == 4);
//     // assert_continue(buf.offset == 1);
//     // assert_continue(buf.size == 2);
//     // assert_continue(subject_buffer_begin(&buf)[buf.offset - 1] == '5');
//     // assert_continue(subject_buffer_begin(&buf)[buf.offset++] == '6');
//   }
//   #endif

  return has_errors;
}