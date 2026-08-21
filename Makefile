############ C Compiler
CC           := gcc
CLANG_FORMAT := clang-format
TARGET       ?= debug

############ Rust compiler
RUSTC     := rustc
RUSTFLAGS := --edition 2021 --crate-type staticlib -C panic=abort

############ Compile Options
CFLAGS :=
CFLAGS += -Wall
CFLAGS += -Wextra
CFLAGS += -std=c11
CFLAGS += -I$(CURDIR)/src
CFLAGS += -Wno-unused-parameter
ifeq ($(TARGET),release)
CFLAGS += -DNDEBUG
CFLAGS += -DNLOG
CFLAGS += -O3
else ifeq ($(TARGET),debug)
CFLAGS += -DTESTING
CFLAGS += -g
CFLAGS += -fsanitize=address
else
    $(error Invalid TARGET '$(TARGET)' - must be 'debug' or 'release')
endif

############ Output Directories
OUT_DIR  := $(CURDIR)/build/$(TARGET)
BIN_DIR  := $(OUT_DIR)/bin
LIB_DIR  := $(OUT_DIR)/lib
INC_DIR  := $(OUT_DIR)/include
HTML_DIR := $(OUT_DIR)/html
OBJ_DIR  := $(OUT_DIR)/objs

############ Accumulators - each module.mk appends to these
ALL_SRCS    :=
ALL_BINS    :=
ALL_HEADERS :=

############ Targets
TARGET_LIB := $(LIB_DIR)/libnumstore.a

############ Default is all

.DEFAULT_GOAL := all

include src/core/module.mk
include src/nscore/module.mk
include src/numstore/module.mk
include src/smartfiles/module.mk
include src/nsserver/module.mk
ifeq ($(TARGET),debug)
include src/tests/module.mk
endif

ALL_OBJS := $(patsubst src/%.c,$(OBJ_DIR)/%.o,$(ALL_SRCS))

$(OBJ_DIR)/%.o: src/%.c | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET_LIB): $(ALL_OBJS) | $(LIB_DIR)
	$(AR) rcs $@ $(ALL_OBJS)

############ Everything to build
ALL := $(TARGET_LIB) $(ALL_BINS) $(ALL_HEADERS)

.PHONY: all clean format format-check

all: $(ALL)

############ Directories
$(INC_DIR) $(BIN_DIR) $(LIB_DIR) $(OBJ_DIR):
	mkdir -p $@

############ House keeping
clean:
	rm -rf $(OUT_DIR)

format:
	find src -type f \( -name '*.c' -o -name '*.h' \) -print0 \
		| xargs -0 $(CLANG_FORMAT) -i
