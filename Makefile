############ Executables

CC           := gcc
PANDOC       := pandoc
CLANG_FORMAT := clang-format
PYTHON       := python3
RUSTC        := rustc

############ User Config

TARGET       ?= debug

############ Output Directories

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

# -MMD -MP makes gcc emit a per-object .d file
# listing every header it actually pulled in; see the `-include` of those
# files near the bottom of this Makefile.
CFLAGS :=
CFLAGS += -MMD
CFLAGS += -MP
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
else ifeq ($(TARGET),asan)
CFLAGS += -DTESTING
CFLAGS += -g
CFLAGS += -fsanitize=address,undefined
CFLAGS += -fno-omit-frame-pointer
else
    $(error Invalid TARGET '$(TARGET)' - must be 'debug', 'release', or 'asan')
endif

############ Rust Flags

RUSTFLAGS := --edition 2021 --crate-type staticlib -C panic=abort

############ Python Flags
#
# PY_CFLAGS   - flags needed to *compile* against Python/numpy headers
# PY_LDFLAGS  - flags needed to *link* a Python extension module (must come
#               after the object files on the command line so the linker
#               can resolve symbols against them correctly)

ifneq (,$(filter python,$(MAKECMDGOALS)))
PY_CFLAGS  := $(shell $(PYTHON)-config --includes)
PY_CFLAGS  += -MMD
PY_CFLAGS  += -MP
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
ifneq (,$(filter debug asan,$(TARGET)))
include src/tests/module.mk
endif

# Objects for the static library (built from ALL_SRCS, plain CFLAGS)
ALL_OBJS := $(patsubst src/%.c,$(OBJ_DIR)/%.o,$(ALL_SRCS))

# PIC objects for the Python extension: the core sources must be recompiled
# with -fPIC before they can be linked into a shared object
ALL_PIC_OBJS := $(patsubst src/%.c,$(PY_OBJ_DIR)/%.o,$(ALL_SRCS))
ALL_PYOBJS   := $(patsubst %.c,$(PY_OBJ_DIR)/%.o,$(ALL_PYSRCS))

############ Targets

TARGET_LIB := $(LIB_DIR)/libnumstore.a

$(OBJ_DIR)/%.o: src/%.c | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET_LIB): $(ALL_OBJS) | $(LIB_DIR)
	$(AR) rcs $@ $(ALL_OBJS)

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
# make TARGET=asan      - debug build + unit tests, instrumented with
#                         AddressSanitizer/UBSan
# make docs             - build docs
# make python           - build the raw _pynumstore extension module
# make python-package   - build the installable pynumstore wheel
# make python-test      - build the wheel, install it, and run the pytest suite
# make release-package-windows-cross
#                       - cross-compile the release package for Windows from
#                         Linux using mingw-w64 (CC/AR overridden by caller)

.PHONY: all clean format docs python python-package python-test

.DEFAULT_GOAL := all

ALL := $(TARGET_LIB) $(ALL_BINS) $(ALL_HEADERS) $(ALL_SAMPLES)

all: $(ALL)

docs: $(HTML_OUTPUTS)

python: $(TARGET_PYLIB)

python-package:
	$(PYTHON) -m pip wheel $(CURDIR)/bindings/python --no-deps -w $(PY_TARGET_DIR)

python-test: python-package
	$(PYTHON) -m pip install --force-reinstall $(PY_TARGET_DIR)/pynumstore-*.whl
	$(PYTHON) -m pip install pytest
	$(PYTHON) -m pytest $(CURDIR)/bindings/python/tests

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
	$(MAKE) python-test

# Cross-compiles the C library/binaries for Windows from Linux (see
# docker/windows-x64.Dockerfile) by pointing CC/AR at the mingw-w64
# toolchain, e.g.:
#   make CC=x86_64-w64-mingw32-gcc AR=x86_64-w64-mingw32-ar release-package-windows-cross
# The resulting .exe binaries can't run on the Linux host that built them,
# so unlike release-package this only proves the build compiles/links --
# it doesn't execute unit_tests, and it skips the Python extension (which
# would need Windows Python headers/import libs to cross-compile against).
release-package-windows-cross:
	$(MAKE) TARGET=debug clean
	$(MAKE) TARGET=debug
	$(MAKE) TARGET=release clean
	$(MAKE) TARGET=release
	cp docs/release_docs.md build/release/target/README.md
	tar -czf build/release.tar.gz -C build/release target
	cd build/release && zip -r ../release.zip target

############ Housekeeping

clean:
	rm -rf $(OUT_DIR) $(PY_OUT_DIR)

format:
	find src bindings -type f \( -name '*.c' -o -name '*.h' \) -print0 \
		| xargs -0 $(CLANG_FORMAT) -i

############ Header dependencies

-include $(ALL_OBJS:.o=.d)
-include $(ALL_PIC_OBJS:.o=.d)
-include $(ALL_PYOBJS:.o=.d)
