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
#else
    typedef char CODE_UNIT;

    #define CODE_UNIT_LITERAL(str) str

    size_t code_unit_strlen(const CODE_UNIT* s) {
        return strlen(s);
    }
#endif
