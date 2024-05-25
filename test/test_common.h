#pragma once
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

// assert, but continue to remaining code.
// if assertion isn't satisfied, prints an error, and sets `has_errors` to 1
#define assert_continue(predicate) \
    do { \
        assert_continue___((predicate), "Failed at: "  __FILE__ ":" STRINGIFY(__LINE__) "\n"); \
    } while (0)

#define STRINGIFY(x) STRINGIFY2(x)
#define STRINGIFY2(x) #x

int has_errors = 0;

void assert_continue___(bool predicate, const char* msg) {
    if (!predicate) {
        fputs(msg, stderr);
        has_errors = 1;
    }
}
