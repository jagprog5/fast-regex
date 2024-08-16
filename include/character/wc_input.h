#pragma once

#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <wchar.h>

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

// intended for use in converting the subject text from a file before its use by the engine.
// `out` must point to an allocation greater or equal in number of elements to the input.
// the decode state, `ps`, must be retained between calls.
// `complete` indicates that this is the last segment to decode from the input stream
// `skip_invalid` indicates the behaviour of invalid bytes. if true, they are discarded.
// if false, \\uFFFF is sent per invalid byte
// returns the end of the output.
wchar_t* convert_subject_to_wchar_range(const char* begin, const char* end, bool complete, mbstate_t* ps, wchar_t* out, bool skip_invalid) {
  assert(begin <= end);
  while (begin != end) {
    size_t num_bytes = mbrtowc(out, begin, end - begin, ps);
    switch (num_bytes) {
      case 0:
        num_bytes = 1; // fallthrough with 1 character consumed
        goto suppress_fallthrough_warn;
      default:
suppress_fallthrough_warn:
        begin += num_bytes;
        out += 1;
        break;
      case (size_t)-1:
        // decode error
        // this is handled by consuming 1 byte of the input,
        // and by sending a noncharacter-FFFF to the output. it's important
        // that something is sent, otherwise characters that aren't adjacent will
        // appear so from the perspective of the pattern matching downstream
        if (!skip_invalid) *out++ = L'\uFFFF';
        begin += 1;
        memset(ps, 0, sizeof(*ps)); // make not unspecified
        break;
      case (size_t)-2:
        if (complete) {
          // represent n bytes of the incomplete multibyte with a noncharacter, each
          if (!skip_invalid) {
            while (begin != end) {
              *out++ = L'\uFFFF';
              begin += 1;
            }
          }
        } else {
          // incomplete multibyte.
          // state so far has been stored and will be resumed on next call
        }
        goto end;
        break;
    }
  }
end:
  return out;
}
