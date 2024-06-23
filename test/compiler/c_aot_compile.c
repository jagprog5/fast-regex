#include <assert.h>
#include <string.h>
#include "c_aot_compile.h"

#ifndef COMPILER_USED
    #error "COMPILER_USED must be defined to the c compiler"
#endif

// Stringify the COMPILER_USED macro
#define STR(x) #x
#define XSTR(x) STR(x)

int main(void) {
    const char* program = "int add(int a, int b) {return a + b;}";
    const char* compiler = XSTR(COMPILER_USED);
    void* dl_handle = compile(compiler, program, program + strlen(program));


    if (dl_handle == NULL) {
        return 1; // error printed by function
    }

    int (*add_symbol)(int, int);
    // https://linux.die.net/man/3/dlopen
    *(void**)(&add_symbol) = dlsym(dl_handle, "add");
    if (add_symbol == NULL) {
        fputs("failed to resolve symbol", stderr);
        return 1;
    }

    assert((*add_symbol)(1, 2) == 3);

    if (dlclose(dl_handle) != 0) {
        char* reason = dlerror();
        if (reason) {
            fprintf(stderr, "dlclose: %s\n", reason);
        } else {
            fputs("dlclose failed for an unknown reason", stderr);
        }
        return 1;
    }
    return 0;
}
