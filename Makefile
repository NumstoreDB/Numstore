CC      			:= gcc
CLANG_FORMAT 	:= clang-format
CFLAGS  			:= -Wall -Wextra -std=c11 -I$(shell pwd)/src -DTESTING -Wno-unused-parameter -g

RUSTC     := rustc
RUSTFLAGS := --edition 2021 --crate-type staticlib -C panic=abort

SUBDIRS := src/core src/nscore src/nsserver src/numstore src/smartfiles

OUT_DIR 	:= $(CURDIR)/build
BIN_DIR 	:= $(OUT_DIR)/bin
LIB_DIR 	:= $(OUT_DIR)/lib
HTML_DIR  := $(OUT_DIR)/html

export CC CFLAGS LIB_DIR BIN_DIR RUSTC RUSTFLAGS HTML_DIR

.PHONY: all $(SUBDIRS) clean format

all: $(SUBDIRS)

documentation: 
	$(MAKE) -C docs

src/unit_tests.c: apps/scripts/gen_tests.py 
	python3 apps/scripts/gen_tests.py

$(SUBDIRS): $(LIB_DIR) $(BIN_DIR)
	$(MAKE) -C $@

$(LIB_DIR):
	mkdir -p $@

$(BIN_DIR):
	mkdir -p $@

clean:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done
	$(MAKE) -C docs clean;

clean-all: clean 
	rm -rf build

FORMAT_DIR   := src

.PHONY: format format-check

format:
	find $(FORMAT_DIR) -type f \( -name '*.c' -o -name '*.h' \) -print0 \
		| xargs -0 $(CLANG_FORMAT) -i
