Header separation is as follows:

`subject_buffer.h` for functions which work with char ranges (doesn't know about wchar_t at all).

`wc_input.h` for functions which work on wchar_t ranges

`code_unit.h` works on CODE_UNIT, and should have a specialization for if CODE_UNIT is char or wchar_t.
