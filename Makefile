############ C Compiler
CC           := gcc
PANDOC 			 := pandoc
CLANG_FORMAT := clang-format
TARGET       ?= debug

############ Rust compiler
RUSTC     := rustc
RUSTFLAGS := --edition 2021 --crate-type staticlib -C panic=abort

############ Compile Options
CFLAGS :=
CFLAGS += -Wall
CFLAGS += -Wextra
CFLAGS += -I$(CURDIR)/src
CFLAGS += -Wno-unused-parameter
CFLAGS += -Wno-unused-variable
CFLAGS += -Wno-unused-but-set-variable
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
BIN_DIR  := $(OUT_DIR)/target/bin
LIB_DIR  := $(OUT_DIR)/target/lib
INC_DIR  := $(OUT_DIR)/target/include
HTML_DIR := $(OUT_DIR)/target/html
OBJ_DIR  := $(OUT_DIR)/objs
SMP_DIR  := $(OUT_DIR)/target/samples

############ Accumulators - each module.mk appends to these
ALL_SRCS    :=
ALL_BINS    :=
ALL_HEADERS :=
ALL_SAMPLES :=

############ Targets
TARGET_LIB := $(LIB_DIR)/libnumstore.a

############ Default is all

.DEFAULT_GOAL := all

include src/core/module.mk
include src/nscore/module.mk
include src/numstore/module.mk
include src/smartfiles/module.mk
ifeq ($(TARGET),debug)
include src/tests/module.mk
endif

ALL_OBJS := $(patsubst src/%.c,$(OBJ_DIR)/%.o,$(ALL_SRCS))

$(OBJ_DIR)/%.o: src/%.c | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET_LIB): $(ALL_OBJS) | $(LIB_DIR)
	$(AR) rcs $@ $(ALL_OBJS)

############ Docs

PANDOC_LUA      := docs/pandoc/md-links.lua
PANDOC_CSS      := docs/pandoc/style.css
PANDOC_TEMPLATE := docs/pandoc/template.html
PANDOC_SIDEBAR  := docs/pandoc/_sidebar.html

PANDOC_ARGS := \
	--from=markdown \
	--to=html5 \
	--standalone \
	--embed-resources \
	--lua-filter=$(PANDOC_LUA) \
	--css=$(PANDOC_CSS) \
	--template=$(PANDOC_TEMPLATE) \
	--include-before-body=$(PANDOC_SIDEBAR)

PANDOC_DEPS := $(PANDOC_CSS) $(PANDOC_TEMPLATE) $(PANDOC_LUA) $(PANDOC_SIDEBAR)

# Map src/foo/bar.md -> build/html/foo/bar.html
MD_FILES := $(shell find docs -name '*.md' | sed 's|^\./||')
HTML_OUTPUTS := $(patsubst docs/%.md,$(HTML_DIR)/%.html,$(MD_FILES))

$(info $(HTML_OUTPUTS))

$(HTML_DIR)/%.html: docs/%.md $(PANDOC_DEPS) | $(HTML_DIR)
	mkdir -p $(dir $@)
	$(PANDOC) $(PANDOC_ARGS) --output $@ $<

############ Everything to build
ALL := $(TARGET_LIB) $(ALL_BINS) $(ALL_HEADERS) $(ALL_SAMPLES) $(HTML_OUTPUTS)

.PHONY: all clean format format-check

all: $(ALL)

release-package:
	$(MAKE) TARGET=debug clean
	$(MAKE) TARGET=debug
	./build/debug/target/bin/unit_tests
	$(MAKE) TARGET=release clean
	$(MAKE) TARGET=release
	cp docs/release_docs.md build/release/target/README.md
	tar -czf build/release.tar.gz -C build/release target
	cd build/release && zip -r ../release.zip target

############ Directories
$(INC_DIR) $(BIN_DIR) $(LIB_DIR) $(OBJ_DIR) $(SMP_DIR) $(HTML_DIR):
	mkdir -p $@


############ House keeping
clean:
	rm -rf $(OUT_DIR)

format:
	find src -type f \( -name '*.c' -o -name '*.h' \) -print0 \
		| xargs -0 $(CLANG_FORMAT) -i
