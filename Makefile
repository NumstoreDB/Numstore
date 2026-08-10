CC      := gcc
CFLAGS  := -Wall -Wextra -std=c11 -Isrc -DTESTING -Wno-unused-parameter
LDFLAGS :=

# Recursively find all .c files under src/
SRCS := $(shell find src -name '*.c')
OBJS := $(patsubst src/%.c,build/%.o,$(SRCS))

# All headers and sources, for the format target
FMT_FILES := $(shell find apps src include -type f \( -name '*.c' -o -name '*.h' \) 2>/dev/null)

.PHONY: all clean format

all: numstore

numstore: $(OBJS)
	$(CC) $(OBJS) apps/numstore.c -o $@ $(LDFLAGS) $(CFLAGS)

build/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf build app

format:
	clang-format -i $(FMT_FILES)
