#pragma once

#include <string.h>

// this lib is intended to work with either char or wchar_t.

#ifdef USE_WCHAR
    #include <wchar.h>
    typedef wchar_t CODE_UNIT;

    #define CODE_UNIT_LITERAL(str) L##str

    size_t code_unit_strlen(const CODE_UNIT* s) {
        return wcslen(s);
    }

    int code_unit_memcmp(const CODE_UNIT* str1, const CODE_UNIT* str2, size_t n) {
        return wmemcmp(str1, str2, n);
    }
#else
    typedef char CODE_UNIT;

    #define CODE_UNIT_LITERAL(str) str

    size_t code_unit_strlen(const CODE_UNIT* s) {
        return strlen(s);
    }

    int code_unit_memcmp(const char* str1, const char* str2, size_t n) {
        return memcmp(str1, str2, n);
    }
#endif

// returns zero if they match, else nonzero
int code_unit_range_equal(const CODE_UNIT* begin1, const CODE_UNIT* end1, const CODE_UNIT* begin2, const CODE_UNIT* end2) {
    size_t len1 = end1 - begin1;
    size_t len2 = end2 - begin2;
    if (len1 != len2) return 1;
    return code_unit_memcmp(begin1, begin2, len1);
}

// compare range to cstring. returns zero if they match, else nonzero
int code_unit_range_equal2(const CODE_UNIT* begin1, const CODE_UNIT* end1, const CODE_UNIT* begin2) {
    size_t len1 = end1 - begin1;
    size_t len2 = code_unit_strlen(begin2);
    if (len1 != len2) return 1;
    return code_unit_memcmp(begin1, begin2, len1);
}