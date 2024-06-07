CFLAGS = -std=c99
# ban heap usage
LDFLAGS = -Wl,--wrap=malloc,--wrap=calloc,--wrap=realloc,--wrap=free

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

build/test/%.o: test/%.c test/test_common.h include/%.h include/code_unit.h
	mkdir -p -- $$(dirname '$@')
	$(CC) -Itest -Iinclude -Iinclude/"$$(dirname -- '$<' | cut -c 6-)" -c $< -o $@ $(CFLAGS)

build/test/%: build/test/%.o
	$(CC) $^ -o $@ $(LDFLAGS)

# MISC

.PRECIOUS: build/test/%.o

clean:
	rm -rvf build/*
