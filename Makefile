CC      := gcc
CFLAGS  := -Wall -Wextra -std=c11 -I$(shell pwd)/src -DTESTING -Wno-unused-parameter -g

RUSTC     := rustc
RUSTFLAGS := --edition 2021 --crate-type staticlib -C panic=abort

SUBDIRS := src/core src/nscore src/nsserver

OUT_DIR := $(CURDIR)/build
BIN_DIR := $(OUT_DIR)/bin
LIB_DIR := $(OUT_DIR)/lib

export CC CFLAGS LIB_DIR BIN_DIR RUSTC RUSTFLAGS

.PHONY: all $(SUBDIRS) clean

all: $(SUBDIRS)

$(SUBDIRS):
	mkdir -p $(LIB_DIR)
	mkdir -p $(BIN_DIR)
	$(MAKE) -C $@

clean:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done
