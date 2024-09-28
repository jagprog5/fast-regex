CFLAGS:=-std=c99 -Wall -Wextra

# arg or env provided to make
USE_WCHAR:=
NDEBUG:=

# apply it to CFLAGS if set
ifeq ($(USE_WCHAR),1)
  CFLAGS+=-DUSE_WCHAR
else ifeq ($(USE_WCHAR),t)
  CFLAGS+=-DUSE_WCHAR
else ifeq ($(USE_WCHAR),true)
  CFLAGS+=-DUSE_WCHAR
else ifeq ($(USE_WCHAR),T)
  CFLAGS+=-DUSE_WCHAR
else ifeq ($(USE_WCHAR),TRUE)
  CFLAGS+=-DUSE_WCHAR
endif

ifeq ($(NDEBUG),1)
  CFLAGS+=-DNDEBUG
else ifeq ($(NDEBUG),t)
  CFLAGS+=-DNDEBUG
else ifeq ($(NDEBUG),true)
  CFLAGS+=-DNDEBUG
else ifeq ($(NDEBUG),T)
  CFLAGS+=-DNDEBUG
else ifeq ($(NDEBUG),TRUE)
  CFLAGS+=-DNDEBUG
endif

# no heap
LDFLAGS=-Wl,--wrap=malloc,--wrap=calloc,--wrap=realloc,--wrap=free

TEST_SOURCES:=$(shell find test -type f -name '*.c')
TEST_OBJS:=$(patsubst test/%.c,build/test/%.o,$(TEST_SOURCES))
TEST_BINARIES:=$(patsubst test/%.c,build/test/%,$(TEST_SOURCES))

.PHONY: all debug test clean

# MAIN BINARY

# default does optimization and no debug info
all: CFLAGS+=-O2 -DNDEBUG
all: build/regex

# debug default has debug info and no optimization
debug: CFLAGS+=-O0 -g
debug: build/regex

build/regex: build/main.o
	$(CC) $^ -o $@ $(LDFLAGS)

build/main.o: src/main.c
	$(CC) -c $< -o $@ $(CFLAGS) -Iinclude

# TEST SUITE

test: CFLAGS+=-O0 -g
test: $(TEST_BINARIES)
	test/run_tests.sh

# base test compile
build/test/%.o: test/%.c test/test_common.h include/%.h
	mkdir -p -- $$(dirname '$@')
	$(CC) -Itest -Iinclude -c $< -o $@ $(CFLAGS) -D_GNU_SOURCE

# base test link
build/test/%: build/test/%.o
	$(CC) $^ -o $@ $(LDFLAGS)

# compilation test compile
build/test/%_compile.o: test/%_compile.c test/test_common.h include/%_compile.h
	mkdir -p -- $$(dirname '$@')
	@#                                                         VVVVVVVVVVVVVVVVVVVVV
	$(CC) -Itest -Iinclude -c $< -o $@ $(CFLAGS) -D_GNU_SOURCE -DCOMPILER_USED=$(CC)

# compilation test link
build/test/%_compile: build/test/%_compile.o
	@#                        VVVV
	$(CC) $^ -o $@ $(LDFLAGS) -ldl 

# MISC

.PRECIOUS: build/test/%.o
.PRECIOUS: build/test/%_compile.o

clean:
	rm -rvf build/*
