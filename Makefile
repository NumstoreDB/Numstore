############ Executables

CC           := gcc
PANDOC       := pandoc
CLANG_FORMAT := clang-format
PYTHON       := python3
RUSTC        := rustc

############ User Config

TARGET       ?= debug

############ Output Directories
#
# C/C++ build tree, keyed by TARGET (debug/release):
#   build/<target>/target/{bin,lib,include,html,samples}
#   build/<target>/objs
#
# Python build tree is independent of TARGET - it always lives under
# build/python, mirroring the same target/objs split:
#   build/python/target   - the built extension module (release-related output)
#   build/python/objs     - object files used to build it

OUT_DIR  := $(CURDIR)/build/$(TARGET)
BIN_DIR  := $(OUT_DIR)/target/bin
LIB_DIR  := $(OUT_DIR)/target/lib
INC_DIR  := $(OUT_DIR)/target/include
HTML_DIR := $(OUT_DIR)/target/html
OBJ_DIR  := $(OUT_DIR)/objs
SMP_DIR  := $(OUT_DIR)/target/samples

PY_OUT_DIR    := $(CURDIR)/build/python
PY_TARGET_DIR := $(PY_OUT_DIR)/target
PY_OBJ_DIR    := $(PY_OUT_DIR)/objs

############ C Flags

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
#CFLAGS += -fsanitize=address
else
    $(error Invalid TARGET '$(TARGET)' - must be 'debug' or 'release')
endif

############ Rust Flags

RUSTFLAGS := --edition 2021 --crate-type staticlib -C panic=abort

############ Python Flags
#
# PY_CFLAGS   - flags needed to *compile* against Python/numpy headers
# PY_LDFLAGS  - flags needed to *link* a Python extension module (must come
#               after the object files on the command line so the linker
#               can resolve symbols against them correctly)
#
# These call out to python3-config / python3 -c ..., which is real work and
# requires a working python environment. Only pay that cost when 'python' is
# actually one of the goals being built.

ifneq (,$(filter python,$(MAKECMDGOALS)))
PY_CFLAGS  := $(shell $(PYTHON)-config --includes)
PY_CFLAGS  += -fPIC
PY_CFLAGS  += -DNLOG
PY_CFLAGS  += -DNDEBUG
PY_CFLAGS  += -I$(CURDIR)/src
PY_CFLAGS  += -I$(CURDIR)/src/numstore
PY_CFLAGS  += -I$(shell \
	$(PYTHON) -c "import numpy; print(numpy.get_include())" \
)

PY_LDFLAGS := -shared
PY_LDFLAGS += $(shell \
	$(PYTHON)-config --ldflags --embed 2>/dev/null || $(PYTHON)-config --ldflags \
)

PY_SOABI     := $(shell \
	$(PYTHON) -c "import sysconfig; print(sysconfig.get_config_var('EXT_SUFFIX'))" \
)
TARGET_PYLIB := $(PY_TARGET_DIR)/_pynumstore$(PY_SOABI)
endif

############ Accumulators - each module.mk appends to these

ALL_SRCS    :=
ALL_BINS    :=
ALL_HEADERS :=
ALL_SAMPLES :=
ALL_PYSRCS  :=

# Accumulate all modules
include src/core/module.mk
include src/nscore/module.mk
include src/numstore/module.mk
include src/smartfiles/module.mk
include bindings/python/module.mk
ifeq ($(TARGET),debug)
include src/tests/module.mk
endif

# Objects for the static library (built from ALL_SRCS, plain CFLAGS)
ALL_OBJS := $(patsubst src/%.c,$(OBJ_DIR)/%.o,$(ALL_SRCS))

# PIC objects for the Python extension: the core sources must be recompiled
# with -fPIC before they can be linked into a shared object; the plain
# ALL_OBJS above are not safe to reuse here. These, plus the python binding
# objects, both live under PY_OBJ_DIR.
ALL_PIC_OBJS := $(patsubst src/%.c,$(PY_OBJ_DIR)/%.o,$(ALL_SRCS))
ALL_PYOBJS   := $(patsubst %.c,$(PY_OBJ_DIR)/%.o,$(ALL_PYSRCS))

############ Targets

TARGET_LIB := $(LIB_DIR)/libnumstore.a

############ Default is all

$(OBJ_DIR)/%.o: src/%.c | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET_LIB): $(ALL_OBJS) | $(LIB_DIR)
	$(AR) rcs $@ $(ALL_OBJS)

# These rules reference PY_CFLAGS/PY_LDFLAGS/TARGET_PYLIB, which are only
# defined (see Python Flags above) when 'python' is one of the goals - so
# the rules themselves must be guarded the same way, otherwise the
# $(TARGET_PYLIB) rule below would expand to a rule with an empty target.
ifneq (,$(filter python,$(MAKECMDGOALS)))
$(PY_OBJ_DIR)/%.o: src/%.c | $(PY_OBJ_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

$(PY_OBJ_DIR)/bindings/python/%.o: bindings/python/%.c | $(PY_OBJ_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(PY_CFLAGS) -c $< -o $@

$(TARGET_PYLIB): $(ALL_PIC_OBJS) $(ALL_PYOBJS) | $(PY_TARGET_DIR)
	$(CC) $(PY_CFLAGS) -o $@ $(ALL_PIC_OBJS) $(ALL_PYOBJS) $(PY_LDFLAGS)
endif

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

# Map docs/foo/bar.md -> build/<target>/target/html/foo/bar.html
#
# The 'find' below is real filesystem work, so only run it when 'docs' is
# actually one of the goals being built.
ifneq (,$(filter docs,$(MAKECMDGOALS)))
MD_FILES     := $(shell find docs -name '*.md' | sed 's|^\./||')
HTML_OUTPUTS := $(patsubst docs/%.md,$(HTML_DIR)/%.html,$(MD_FILES))
endif

$(HTML_DIR)/%.html: docs/%.md $(PANDOC_DEPS) | $(HTML_DIR)
	mkdir -p $(dir $@)
	$(PANDOC) $(PANDOC_ARGS) --output $@ $<

############ Default target
#
# make                  - debug numstore library and binaries
# make TARGET=release   - release numstore library and binaries
# make docs             - build docs
# make python           - build python extension

.PHONY: all clean format format-check docs python

.DEFAULT_GOAL := all

ALL := $(TARGET_LIB) $(ALL_BINS) $(ALL_HEADERS) $(ALL_SAMPLES)

all: $(ALL)

docs: $(HTML_OUTPUTS)

python: $(TARGET_PYLIB)

############ Directories

$(INC_DIR) $(BIN_DIR) $(LIB_DIR) $(OBJ_DIR) $(SMP_DIR) $(HTML_DIR) $(PY_TARGET_DIR) $(PY_OBJ_DIR):
	mkdir -p $@

############ Package Management

release-package:
	$(MAKE) TARGET=debug clean
	$(MAKE) TARGET=debug
	./build/debug/target/bin/unit_tests
	$(MAKE) TARGET=release clean
	$(MAKE) TARGET=release
	cp docs/release_docs.md build/release/target/README.md
	tar -czf build/release.tar.gz -C build/release target
	cd build/release && zip -r ../release.zip target

############ Housekeeping

clean:
	rm -rf $(OUT_DIR) $(PY_OUT_DIR)

format:
	find src -type f \( -name '*.c' -o -name '*.h' \) -print0 \
		| xargs -0 $(CLANG_FORMAT) -i
