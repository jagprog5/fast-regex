#pragma once

#include <stdio.h>
#include <assert.h>
#include "code_unit.h"

// the subject is the input to be processed by a pattern
// this handles the state of that buffer.
typedef struct {
    FILE* input_file;
    size_t capacity; // the number of bytes to read at a time from file

    // bytes read from input_file are placed here
    // points to capacity elements
    char* byte_buffer;

#ifdef USE_WCHAR
    // if using wchar_t, the bytes from byte_buffer are transformed into
    // characters and placed here. points to capacity elements
    wchar_t* character_buffer;
#endif

#ifdef USE_WCHAR
    // the "relevant buffer" is the character_buffer
#else
    // the "relevant buffer" is the byte_buffer
#endif

    // the number of element in the relevant buffer
    size_t size;

    // match offset within buffer
    size_t offset;
} subject_buffer_state;

// this is the beginning of the subject segment
CODE_UNIT* subject_buffer_start(subject_buffer_state* buf) {
#ifdef USE_WCHAR
    return buf->character_buffer;
#else
    return buf->byte_buffer;
#endif
}

CODE_UNIT* subject_buffer_offset(subject_buffer_state* buf) {
    assert(buf->offset < buf->capacity);
    return subject_buffer_start(buf) + buf->offset;
}

// this is the end of the subject segment
CODE_UNIT* subject_buffer_end(subject_buffer_state* buf) {
    return subject_buffer_start(buf) + buf->size;
}

// 1: characters already processed
// 2: lookbehind characters to retain (in this case 5 characters)
// 3: the match offset and proceeding bytes (partial or no match)
// ```
// 1          2    3
// -----------12345xxxxxxxx
// ```
// this function moves back 2 and 3 to the beginning of the buffer, discarding
// 1, and filling the newly available space at the end with more characters
// from the input
void get_more_input(subject_buffer_state* buf, size_t lookbehind) {
    CODE_UNIT* move_begin = subject_buffer_offset(buf);
    if (buf->offset <= lookbehind) {
        // CAN'T HAPPEN as the amount of space needed is precomputed.
        // (also this would mean that no move can happen, infinite loop)
        assert(false);
    }
    move_begin -= lookbehind;

    if (buf->offset >= lookbehind) {
        move_begin -= lookbehind;
    } else {
        move_begin = subject_buffer_start(buf);
    }

    CODE_UNIT* move_end = subject_buffer_end(buf);

    



    if (buf->offset < lookbehind) {
        move_begin = subject_buffer_start(buf);
    }

    // CODE_UNIT* move_begin = subject_buffer_offset(buf);
    // if ()

}
