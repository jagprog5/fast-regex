#pragma once
#include <stdio.h>

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#include "basic/likely_unlikely.h"

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

  // match offset within the subject buffer
  size_t offset;

  // indicates the position of the buffer's beginning inside the file (bytes).
  size_t file_offset;

  // the number of bytes before the match offset that can be evaluated
  size_t max_lookbehind;

  // the max number of bytes to read at a time from the file
  size_t capacity;
  // the number of element in the subject buffer
  size_t size;

  // bytes read from the file are placed here.
  // points to capacity elements
  char* subject_buffer;
} subject_buffer_state;

// number of bytes in the subject buffer at or after the match offset
size_t subject_buffer_remaining_size(const subject_buffer_state* buf) {
  assert(buf->size >= buf->offset);
  return buf->size - buf->offset;
}

// input_file must be opened to read bytes and must be managed by the caller (close buf->input_file when done)
// subject_buffer points to allocation with capacity number of elements
// capacity must be non-zero, and the max lookbehind must be less than the capacity
void init_subject_buffer(subject_buffer_state* buf, FILE* input_file, size_t capacity, char* subject_buffer, size_t max_lookbehind) {
  buf->input_file = input_file;
  buf->offset = 0;
  buf->file_offset = 0;
  buf->max_lookbehind = max_lookbehind;
  buf->capacity = capacity;
  buf->size = 0;
  buf->subject_buffer = subject_buffer;
  assert(buf->capacity > 0);
  assert(buf->max_lookbehind < buf->capacity);
}

// first_input must be true only when it's the first time this function is called on this buffer.
// following this function, the match offset must be moved forward appropriately
// returns true only when the input file is exhausted; if exhausted this function must not be called again on this buffer
bool subject_buffer_get_input(subject_buffer_state* buf, bool first_input) {
  // 1: bytes already processed
  // 2: lookbehind bytes to retain (in this case 5 bytes)
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
  // 1, and filling the newly available space, 4, at the end with more bytes
  // from the input. return true iff input is complete (must not be called again on same steam if done)
  if (likely(!first_input)) {
    if (unlikely(buf->offset <= buf->max_lookbehind)) {
      // can't shift! the match offset wasn't moved far enough forward since the
      // previous read. region 1 (see doc string) is 0 length. this could lead to
      // an infinite loop, since no new bytes can be read into the buffer.

      // this should never happen since the buffer capacity required is computed
      // from the pattern (and the match size is bounded due to the nature of
      // expressions). the match offset should always be able to move forward

      assert(false);

      // desperate recovery for NDEBUG:
      // force the offset forward by the required amount
      // to make region 1 non empty
      buf->offset = buf->max_lookbehind + 1;
      if (buf->offset > subject_buffer_size(buf)) {
        // so many things have gone wrong at this point...
        // should never happen due to assertion in init_subject_buffer
        abort();
      }
    }

    // do the move
    char* move_src_begin = (buf->subject_buffer + buf->offset) - buf->max_lookbehind;
    char* move_src_end = buf->subject_buffer + buf->size;
    char* move_dst_begin = buf->subject_buffer;
    
    size_t num_bytes_discarded = move_src_begin - move_dst_begin;

    while (move_src_begin != move_src_end) {
      *move_dst_begin++ = *move_src_end++;
    }

    buf->size -= num_bytes_discarded;
    buf->offset -= num_bytes_discarded;
    buf->file_offset += num_bytes_discarded;
  }

  // read more input (fill region 4)
  size_t fread_num_bytes = buf->capacity - buf->size;
  char* fread_dst = buf->subject_buffer + buf->size;
  size_t read_ret = fread_wrapper(fread_dst, sizeof(char), fread_num_bytes, buf->input_file);
  bool input_complete = read_ret != fread_num_bytes; // eof or error

  return input_complete;
}

// =============================================================================

// checks for haystack ending with incomplete needle
// returns NULL for zero length haystack or needle
// returns pointer to beginning of largest possible incomplete needle, NULL otherwise
const void* mem_incomplete_suffix(const void* haystack, size_t haystack_len, const void* needle, size_t needle_len) {
  const void* pos;
  size_t smallest_len;
  if (unlikely(haystack_len < needle_len)) {
    pos = haystack;
    smallest_len = haystack_len;
  } else {
    pos = (haystack + haystack_len) - needle_len;
    smallest_len = needle_len;
  }

  while (1) {
      if (smallest_len == 0) {
        return NULL;
      }
      pos += 1;
      smallest_len -= 1;
      if (0 == memcmp(pos, needle, smallest_len)) {
        return pos;
      }
  }
}
