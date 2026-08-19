############ C Compiler
CC           := gcc
CLANG_FORMAT := clang-format
TARGET       ?= debug

############ Rust compiler
RUSTC     := rustc
RUSTFLAGS := --edition 2021 --crate-type staticlib -C panic=abort

############ Sub make modules to build
SUBDIRS := src/core src/nscore src/smartfiles src/nsserver src/numstore

############ Compile Options
CFLAGS := 
CFLAGS += -Wall
CFLAGS += -Wextra 
CFLAGS += -std=c11 
CFLAGS += -I$(shell pwd)/src 
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

############ All the Output Directories
OUT_DIR  := $(CURDIR)/build/$(TARGET)
BIN_DIR  := $(OUT_DIR)/bin
LIB_DIR  := $(OUT_DIR)/lib
INC_DIR  := $(OUT_DIR)/include
HTML_DIR := $(OUT_DIR)/html
OBJ_DIR  := $(OUT_DIR)/objs

############ All the libraries
ALL_LIBS :=
ALL_LIBS += $(LIB_DIR)/libcore.a
ALL_LIBS += $(LIB_DIR)/libnscore.a
ALL_LIBS += $(LIB_DIR)/libsmartfiles.a
ALL_LIBS += $(LIB_DIR)/libnumstore.a

############ All the link flags
ALL_LD :=
ALL_LD += -lnumstore
ALL_LD += -lsmartfiles
ALL_LD += -lnscore
ALL_LD += -lcore

############ Test Targets (Debug only)
TEST_BINS      := unit_tests cgd_swarm_test irwr_swarm_test
TEST_BIN_PATHS := $(addprefix $(BIN_DIR)/,$(TEST_BINS))

############ Everything to build
ALL :=
ALL += $(SUBDIRS)
ifeq ($(TARGET),debug)
ALL += $(TEST_BIN_PATHS)
endif

############ Exports
export CC CFLAGS LIB_DIR BIN_DIR RUSTC RUSTFLAGS HTML_DIR INC_DIR TARGET OBJ_DIR

############ PHONY
.PHONY: all $(SUBDIRS) clean clean-all format format-check

############ DEFAULT
all: $(ALL)

############ Build Tests
$(BIN_DIR)/unit_tests: scripts/gen_tests.py $(ALL_LIBS) | $(BIN_DIR)
	python3 scripts/gen_tests.py
	$(CC) $(CFLAGS) -I$(INC_DIR) src/unit_tests.c -o $@ -L$(LIB_DIR) $(ALL_LD)

$(BIN_DIR)/cgd_swarm_test: $(ALL_LIBS) | $(BIN_DIR)
	$(CC) $(CFLAGS) -I$(INC_DIR) src/cgd_swarm_test.c -o $@ -L$(LIB_DIR) $(ALL_LD)

$(BIN_DIR)/irwr_swarm_test: $(ALL_LIBS) | $(BIN_DIR)
	$(CC) $(CFLAGS) -I$(INC_DIR) src/irwr_swarm_test.c -o $@ -L$(LIB_DIR) $(ALL_LD)

############ Build Sub Modules
$(SUBDIRS): $(LIB_DIR) $(BIN_DIR) $(INC_DIR)
	$(MAKE) -C $@

############ Directories
$(INC_DIR) $(BIN_DIR) $(LIB_DIR) $(OBJ_DIR):
	mkdir -p $@

############ House keeping
clean:
	@for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done

format:
	find src -type f \( -name '*.c' -o -name '*.h' \) -print0 \
		| xargs -0 $(CLANG_FORMAT) -i
