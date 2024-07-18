CFLAGS = -std=c99
# no heap
LDFLAGS =-Wl,--wrap=malloc,--wrap=calloc,--wrap=realloc,--wrap=free

TEST_SOURCES = $(shell find test -type f -name '*.c')
TEST_OBJS = $(patsubst test/%.c,build/test/%.o,$(TEST_SOURCES))
TEST_BINARIES = $(patsubst test/%.c,build/test/%,$(TEST_SOURCES))

.PHONY: all debug test clean

# MAIN BINARY

all: CFLAGS += -O3 -DNDEBUG
all: build/regex

debug: CFLAGS += -O0 -g
debug: build/regex

build/regex: build/main.o
	$(CC) $^ -o $@ $(LDFLAGS)

build/main.o: src/main.c
	$(CC) -c $< -o $@ $(CFLAGS) -Iinclude

# TEST SUITE

test: CFLAGS += -O0 -g
test: $(TEST_BINARIES)
	test/run_tests.sh

# base test compile
build/test/%.o: test/%.c test/test_common.h include/%.h include/code_unit.h
	mkdir -p -- $$(dirname '$@')
	$(CC) -Itest -Iinclude -Iinclude/"$$(dirname -- '$<' | cut -c 6-)" -c $< -o $@ $(CFLAGS)

# base test link
build/test/%: build/test/%.o
	$(CC) $^ -o $@ $(LDFLAGS)

# compilation test compile
build/test/%_compile.o: test/%_compile.c test/test_common.h include/%_compile.h include/code_unit.h
	mkdir -p -- $$(dirname '$@')
	@#                     VVVVVVVVVVVVVVVVVV
	$(CC) -Itest -Iinclude -Iinclude/compiler -Iinclude/"$$(dirname -- '$<' | cut -c 6-)" -c $< -o $@ $(CFLAGS) -D_GNU_SOURCE -DCOMPILER_USED=$(CC)

# compilation test link
build/test/%_compile: build/test/%_compile.o
	@#                        VVVV
	$(CC) $^ -o $@ $(LDFLAGS) -ldl 

# MISC

.PRECIOUS: build/test/%.o
.PRECIOUS: build/test/%_compile.o

clean:
	rm -rvf build/*
