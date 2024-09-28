#pragma once

#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <wchar.h>
#include <stdlib.h>

#include "basic/likely_unlikely.h"

typedef enum {
  CONVERT_PATTERN_WCHAR_ERROR,
  CONVERT_PATTERN_WCHAR_OK,
} convert_pattern_to_wchar_range_result_type;

typedef union {
  const char* offending_location; // CONVERT_PATTERN_WCHAR_ERROR
  wchar_t* out_end;               // CONVERT_PATTERN_OK
} convert_pattern_to_wchar_range_result_value;

typedef struct {
  convert_pattern_to_wchar_range_result_type type;
  convert_pattern_to_wchar_range_result_value value;
} convert_pattern_to_wchar_range_result;

// convert a regex pattern to wchar_t range.
// out must point to an allocation greater or equal in number of elements to the input.
convert_pattern_to_wchar_range_result convert_pattern_to_wchar_range(const char* begin, const char* end, wchar_t* out) {
  assert(begin <= end);
  convert_pattern_to_wchar_range_result ret;
  mbstate_t ps;
  memset(&ps, 0, sizeof(ps));
  while (begin != end) {
    size_t num_bytes = mbrtowc(out, begin, end - begin, &ps);
    switch (num_bytes) {
      case 0:
        num_bytes = 1;
        goto suppress_fallthrough_warn;
      default:
suppress_fallthrough_warn:
        begin += num_bytes;
        out += 1;
        break;
      case (size_t)-1: // any error
      case (size_t)-2: {
        ret.type = CONVERT_PATTERN_WCHAR_ERROR;
        ret.value.offending_location = begin;
        return ret;
      } break;
    }
  }
  ret.type = CONVERT_PATTERN_WCHAR_OK;
  ret.value.out_end = out;
  return ret;
}

// returns the number of byes used to encode a character
unsigned int multibyte_character_length(wchar_t character) {
    char mb_buf[MB_CUR_MAX];
    mbstate_t ps;
    memset(&ps, 0, sizeof(ps));
    return wcrtomb(mb_buf, character, &ps);
}

// out points to number of bytes indicated by multibyte_character_length.
// fills out with the 
void multibytes_character_encoding(wchar_t character, char* out) {
  mbstate_t ps;
  memset(&ps, 0, sizeof(ps));
  return wcrtomb(out, character, &ps);
}

// unsigned int multibyte_character_length(wchar_t character) {
//   if (character < 0x80) {
//     return 1;
//   } else if (character < 0x800) {
//     return 2;
//   } else if (character < 0x10000) {
//     return 3;
//   } else if (character < 0x200000) {
//     return 4;
//   } else if (character < 0x4000000) {
//     return 5;
//   } else {
//     return 6;
//   }
// }

// // must be zero set before use. retained between calls
// typedef struct {
//   mbstate_t ps;
//   size_t bytes_consumed_for_ps; // number of bytes since last emitted output character
// } subject_decode_state;

// // begin (inclusive) to end (exclusive) indicates the byte range to convert
// // `complete` indicates that this is the last segment to decode from the input stream
// // `skip_invalid` indicates the behaviour of invalid bytes.
// //  - if true, they are discarded.
// //  - if false, \\uFFFF is sent per invalid byte
// // returns the end of the output.
// typedef struct {
//   const char* begin;
//   const char* end;
//   bool complete;
//   bool skip_invalid;
//   subject_decode_state* state;
// } convert_subject_to_wchar_range_input;

// // begin (inclusive) to end (exclusive) indicates the output range available for conversion
// // deltas_begin must point to the same number of elements at the output range begin to end
// //
// // begin is populated with the converted characters.
// // deltas is populated with the difference in file offset between characters
// typedef struct {
//   wchar_t* begin;
//   wchar_t* end;
//   unsigned int* deltas_begin;
// } convert_subject_to_wchar_range_output;

// // returns the new begin of the input and output (which indicates how many
// // elements were processed / populated, respectively)
// typedef struct {
//   const char* in_begin;
//   wchar_t* out_begin;
// } convert_subject_to_wchar_range_ret;

// // intended for use in converting the subject text from a file before its use by the engine.
// //
// // in an ideal case, the input size should have enough bytes to fill the output size
// // (input_size_bytes >= output_size_characters * MB_CUR_MAX)
// convert_subject_to_wchar_range_ret convert_subject_to_wchar_range(convert_subject_to_wchar_range_input in, convert_subject_to_wchar_range_output out) {
//   assert(in.begin <= in.end);
//   assert(out.begin <= out.end);

//   if (out.begin == out.end) {
//     goto end;
//   }

//   while (in.begin != in.end) {
//     size_t num_bytes = mbrtowc(out.begin, in.begin, in.end - in.begin, &in.state->ps);
//     switch (num_bytes) {
//       case 0:
//         num_bytes = 1;
//         goto suppress_fallthrough_warn;
//       default:
// suppress_fallthrough_warn:
//         in.begin += num_bytes;
//         out.begin += 1;
//         *out.deltas_begin++ = num_bytes + in.state->bytes_consumed_for_ps;
//         in.state->bytes_consumed_for_ps = 0;
//         if (out.begin == out.end) goto end;
//         break;
//       case (size_t)-1:
//         // decode error
//         // this is handled by consuming 1 byte of the input,
//         // and by sending a noncharacter-FFFF to the output. it's important
//         // that something is sent, otherwise characters that aren't adjacent will
//         // appear so from the perspective of the pattern matching downstream
//         in.begin += 1;
//         memset(&in.state->ps, 0, sizeof(in.state->ps)); // make not unspecified
//         if (!in.skip_invalid) {
//           *out.begin++ = L'\uFFFF';
//           *out.deltas_begin++ = 1 + in.state->bytes_consumed_for_ps;
//           in.state->bytes_consumed_for_ps = 0;
//           if (out.begin == out.end) goto end;
//         } else {
//           in.state->bytes_consumed_for_ps += 1;
//         }
//         break;
//       case (size_t)-2:
//         in.state->bytes_consumed_for_ps += in.end - in.begin;
//         in.begin = in.end;
//         if (unlikely(in.complete) && !in.skip_invalid) {
//           // place a single noncharacter-FFFF at the end. this is a weird case,
//           // and can be defined however I want. this is the most convenient way
//           // which preserves lack of adjacency (with the end of the subject)
//           *out.begin++ = L'\uFFFF';
//           *out.deltas_begin++ = in.state->bytes_consumed_for_ps;
//           in.state->bytes_consumed_for_ps = 0;
//         }
//         goto end;
//         break;
//     }
//   }
// end:
//   convert_subject_to_wchar_range_ret ret;
//   ret.in_begin = in.begin;
//   ret.out_begin = out.begin;
//   return ret;
// }
