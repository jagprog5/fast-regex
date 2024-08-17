#pragma once

#include <stdio.h>

#include <assert.h>
#include <stdbool.h>
#include "code_unit.h"

#include "basic/likely_unlikely.h"

#ifdef USE_WCHAR
#include "character/wc_input.h"
#endif

#ifdef TESTING_HOOK
// rather than using a pipe (which wouldn't be cross platform and requires GNU),
// it's much simpler and faster to type to use a mini mock.
extern char* fread_wrapper_data;
extern size_t fread_wrapper_data_size;

extern size_t fread_wrapper(void* ptr, size_t size, size_t nmemb, FILE* stream) {
  assert(size == sizeof(char));
  assert(stream == NULL);
  #ifdef NDEBUG
    (void)(size);
    (void)(stream);
  #endif
  size_t num_bytes_to_give;
  if (nmemb > fread_wrapper_data_size) {
    // too many to give
    num_bytes_to_give = fread_wrapper_data_size;
  } else {
    num_bytes_to_give = nmemb;
  }
  memcpy(ptr, fread_wrapper_data, num_bytes_to_give);
  fread_wrapper_data_size -= num_bytes_to_give;
  fread_wrapper_data += num_bytes_to_give;
  return num_bytes_to_give;
}
#else
size_t fread_wrapper(void* ptr, size_t size, size_t nmemb, FILE* stream) {
  return fread(ptr, size, nmemb, stream);
}
#endif

// the subject is the input to be processed by a pattern
// this handles the state of that buffer.
typedef struct {
  FILE* input_file;
  size_t capacity;       // the number of bytes to read at a time from the file
  size_t max_lookbehind; // the number of characters before the match offset that the pattern might evaluate

  // bytes read from input_file are placed here
  // points to capacity elements
  char* byte_buffer___;

#ifdef USE_WCHAR
  // if using wchar_t, the bytes from byte_buffer___ are transformed into
  // characters and placed here. points to capacity elements
  wchar_t* character_buffer___;

  // decode state when converting bytes from the byte buffer to characters in
  // the character_buffer___
  mbstate_t ps;
  bool skip_invalid;
#endif

#ifdef USE_WCHAR
  // the "subject buffer" is character_buffer___
  // the "processing buffer" is byte_buffer___
#else
  // the "subject buffer" is byte_buffer___
#endif

  // the number of element in the subject buffer
  size_t size;

  // match offset within the subject buffer
  size_t offset;
} subject_buffer_state;

// the number of characters within the buffer that is at or after the match offset
size_t subject_buffer_remaining_size(const subject_buffer_state* buf) {
  assert(buf->size >= buf->offset);
  return buf->size - buf->offset;
}

// this is the beginning of the subject segment
CODE_UNIT* subject_buffer_start(subject_buffer_state* buf) {
  assert(buf->offset <= buf->size);
  assert(buf->size <= buf->capacity);
#ifdef USE_WCHAR
  return buf->character_buffer___;
#else
  return buf->byte_buffer___;
#endif
}

#ifdef USE_WCHAR
char* subject_processing_buffer(subject_buffer_state* buf) {
  return buf->byte_buffer___;
}
#endif

CODE_UNIT* subject_buffer_offset(subject_buffer_state* buf) {
  return subject_buffer_start(buf) + buf->offset;
}

// this is the end of the subject segment
CODE_UNIT* subject_buffer_end(subject_buffer_state* buf) {
  return subject_buffer_start(buf) + buf->size;
}

#ifdef USE_WCHAR
// both buffers point to allocation with capacity number of elements
// buf->input_file must be set
void init_subject_buffer(subject_buffer_state* buf, size_t capacity, char* byte_buffer, wchar_t* character_buffer, size_t max_lookbehind) {
  buf->input_file = NULL;
  buf->capacity = capacity;
  buf->max_lookbehind = max_lookbehind;
  buf->byte_buffer___ = byte_buffer;
  buf->character_buffer___ = character_buffer;
  // ps zero set on first input
  buf->skip_invalid = false;
  // size, offset set on first input
  assert(buf->capacity > 0);
  assert(buf->max_lookbehind < buf->capacity);
}
#else
// byte_buffer points to allocation with capacity number of elements
// buf->input_file must be set
void init_subject_buffer(subject_buffer_state* buf, size_t capacity, char* byte_buffer, size_t max_lookbehind) {
  buf->input_file = NULL;
  buf->capacity = capacity;
  buf->max_lookbehind = max_lookbehind;
  buf->byte_buffer___ = byte_buffer;
  // ps zero set on first input
  // size, offset set on first input
  assert(buf->capacity > 0);
  assert(buf->max_lookbehind < buf->capacity);
}
#endif

// this must be called the first time input is retrieved.
// return true iff input is complete
bool subject_buffer_get_first_input(subject_buffer_state* buf) {
  buf->size = 0;
  buf->offset = 0;
#ifdef USE_WCHAR
  memset(&buf->ps, 0, sizeof(buf->ps));
  size_t read_ret = fread_wrapper(subject_processing_buffer(buf), sizeof(char), buf->capacity, buf->input_file);
  bool input_complete = read_ret != buf->capacity;                                               // from either eof or error
  CODE_UNIT* new_end = convert_subject_to_wchar_range(subject_processing_buffer(buf),            //
                                                      subject_processing_buffer(buf) + read_ret, //
                                                      input_complete,                            //
                                                      &buf->ps,                                  //
                                                      subject_buffer_start(buf),                 //
                                                      buf->skip_invalid);
  size_t num_new_characters = new_end - subject_buffer_start(buf);
  buf->size += num_new_characters;
#else
  size_t read_ret = fread_wrapper(subject_buffer_start(buf), sizeof(char), buf->capacity, buf->input_file);
  bool input_complete = read_ret != buf->capacity; // from either eof or error
  buf->size += read_ret;
#endif

  return input_complete;
}

// this must be called every time more input is needed,
// after the first call of subject_buffer_get_input
//
// 1: characters already processed
// 2: lookbehind characters to retain (in this case 5 characters)
// 3: the match offset and proceeding bytes (partial or no match)
// 4: newly filled range
// ```
// 1          2    3
// -----------12345xxxxxxxx
//
// ↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓
//
// 2    3       4
// 12345xxxxxxxxNEWNEWNEWNE
// ```
// this function moves back 2 and 3 to the beginning of the buffer, discarding
// 1, and filling the newly available space, 4, at the end with more characters
// from the input. return true iff input is complete
bool subject_buffer_shift_and_get_input(subject_buffer_state* buf) {
  if (unlikely(buf->offset <= buf->max_lookbehind)) {
    // can't shift!
    // region 1 (see doc string) is 0 length. this would likely lead to an
    // infinite loop, since no new characters can be read into the buffer.

    // this should never happen since the buffer capacity required is computed
    // from the pattern (and the match size is bounded due to the nature of
    // expressions)
    assert(false);

    // desperate recovery for NDEBUG

    // force the offset forward by the required amount
    // to make region 1 non empty
    buf->offset = buf->max_lookbehind + 1;
  }

  CODE_UNIT* move_src_begin = subject_buffer_offset(buf) - buf->max_lookbehind;
  CODE_UNIT* move_src_end = subject_buffer_end(buf);
  size_t num_characters = move_src_end - move_src_begin;

  CODE_UNIT* move_dst_begin = subject_buffer_start(buf);
  CODE_UNIT* move_dst_end = move_dst_begin + num_characters;
  memmove(            // range can overlap
      move_dst_begin, //
      move_src_begin, //
      sizeof(CODE_UNIT) * num_characters);
  assert(move_dst_begin < move_src_begin);
  size_t amount_moved_back = move_src_begin - move_dst_begin;
  buf->size -= amount_moved_back;
  buf->offset -= amount_moved_back;

#ifdef USE_WCHAR
  // read bytes into processing buffer
  size_t read_ret = fread_wrapper(subject_processing_buffer(buf), sizeof(char), amount_moved_back, buf->input_file);
  bool input_complete = read_ret != amount_moved_back; // from either eof or error

  // decode processing buffer into characters. fill region 4 with the new characters
  CODE_UNIT* new_end = convert_subject_to_wchar_range(subject_processing_buffer(buf),            //
                                                      subject_processing_buffer(buf) + read_ret, //
                                                      input_complete,                            //
                                                      &buf->ps,                                  //
                                                      move_dst_end,                              //
                                                      buf->skip_invalid);
  size_t num_new_characters = new_end - move_dst_end;
  buf->size += num_new_characters;
#else
  // fill region 4 with new bytes
  size_t read_ret = fread_wrapper(move_dst_end, sizeof(char), amount_moved_back, buf->input_file);
  bool input_complete = read_ret != amount_moved_back; // from either eof or error
  buf->size += read_ret;
#endif
  return input_complete;
}
