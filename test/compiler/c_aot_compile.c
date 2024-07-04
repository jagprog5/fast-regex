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
    const char* program = "\
#ifndef MUST_BE_DEFINED\n\
    #error arg pass failure\n\
#endif\n\
#ifndef MUST_BE_DEFINED2\n\
    #error arg pass failure\n\
#endif\n\
int add(int a, int b) {return a + b;}";
    const char* compiler = XSTR(COMPILER_USED);
    const char* const compile_args[] = {"-DMUST_BE_DEFINED", "-DMUST_BE_DEFINED2", NULL};
    void* dl_handle = compile(compiler, program, program + strlen(program), compile_args);


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
