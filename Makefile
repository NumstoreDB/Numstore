############ Executables

CC           		:= gcc
PANDOC       		:= pandoc
CLANG_FORMAT 		:= clang-format
PYTHON       		:= python3
RUSTC        		:= rustc
CLANG_TIDY 			:= clang-tidy
BEAR       			:= bear

############ User Config

TARGET 				?= debug
PLATFORM 			?=
CROSS_GOAL 	  ?= all

# Default to 1 on release builds 0 on debug builds
NLOG_DEFAULT := 0
ifeq ($(TARGET),release)
NLOG_DEFAULT := 1
endif

ASAN   				?= 0
NLOG 					?= $(NLOG_DEFAULT)
CFLAGS_USER 	?=

############ Output Directories

CROSS_SUFFIX := $(if $(PLATFORM),-$(PLATFORM))
OUT_DIR  		 := $(CURDIR)/build/$(TARGET)$(CROSS_SUFFIX)

# Not included in the output
OBJ_DIR  		 := $(OUT_DIR)/objs

# Included in the output
BIN_DIR  		 := $(OUT_DIR)/target/bin
LIB_DIR  		 := $(OUT_DIR)/target/lib
INC_DIR  		 := $(OUT_DIR)/target/include
HTML_DIR 		 := $(OUT_DIR)/target/html
SMP_DIR  		 := $(OUT_DIR)/target/samples

# Python directory
PY_OUT_DIR    := $(CURDIR)/build/python$(CROSS_SUFFIX)
PY_TARGET_DIR := $(PY_OUT_DIR)/target
PY_OBJ_DIR    := $(PY_OUT_DIR)/objs

############ C Flags

# Common Flags
CFLAGS_COMMON :=
CFLAGS_COMMON += -MMD
CFLAGS_COMMON += -MP
CFLAGS_COMMON += -Wall
CFLAGS_COMMON += -Wextra
# CFLAGS_COMMON += -Werror
CFLAGS_COMMON += -I$(CURDIR)/src

# Debug flags
CFLAGS_DEBUG :=
CFLAGS_DEBUG += -DTESTING
CFLAGS_DEBUG += -g

# Release flags
CFLAGS_RELEASE :=
CFLAGS_RELEASE += -DNDEBUG
CFLAGS_RELEASE += -O3

# Asan flags
CFLAGS_ASAN :=
CFLAGS_ASAN += -g
CFLAGS_ASAN += -fsanitize=address,undefined
CFLAGS_ASAN += -fno-omit-frame-pointer

# No Logs
CFLAGS_NLOG := -DNLOG

# Combine all of them
ifeq ($(TARGET),release)
CFLAGS := $(CFLAGS_COMMON) $(CFLAGS_RELEASE)
else ifeq ($(TARGET),debug)
CFLAGS := $(CFLAGS_COMMON) $(CFLAGS_DEBUG)
else
    $(error Invalid TARGET '$(TARGET)' - must be 'debug' or 'release')
endif

# Address sanitizer
ifeq ($(ASAN),1)
CFLAGS += $(CFLAGS_ASAN)
endif

# No logs
ifeq ($(NLOG),1)
CFLAGS += $(CFLAGS_NLOG)
endif

# Add user flags
CFLAGS += $(CFLAGS_USER)

############ Rust Flags

RUSTFLAGS := --edition 2021 --crate-type staticlib -C panic=abort

############ Accumulators - each module.mk appends to these

TARGET_LIB := $(LIB_DIR)/libnumstore.a
LIBNS_SRCS    :=
ALL_PYSRCS  	:=
ALL 					:= $(TARGET_LIB)

# Each module appends to these lists
include src/core/module.mk
include src/nscore/module.mk
include src/numstore/module.mk
include src/smartfiles/module.mk
include bindings/python/module.mk
ifeq ($(TARGET),debug)
include src/tests/module.mk
else ifeq ($(ASAN),1)
include src/tests/module.mk
endif

# Derived from sources above
LIBNS_OBJS := $(patsubst src/%.c,$(OBJ_DIR)/%.o,$(LIBNS_SRCS))

# Default target
.DEFAULT_GOAL := all

############ Python Flags

PY_SOURCES_FILE := bindings/python/sources.txt

$(PY_SOURCES_FILE): $(LIBNS_SRCS) $(ALL_PYSRCS)
	@rm -f $@
	@echo "# IGNORE: Sources for pynumstore" >> $@
	@echo "# IGNORE: Generate with make python-sources" >> $@
	@for f in $(LIBNS_SRCS); do echo "../../$$f" >> $@; done
	@for f in $(ALL_PYSRCS); do echo "../../$$f" >> $@; done

.PHONY: python-sources
python-sources: $(PY_SOURCES_FILE) $(PY_HEADERS_FILE)

python-package: python-sources | $(PY_TARGET_DIR)
	PYNUMSTORE_BUILD_BASE=$(PY_OBJ_DIR) \
		$(PYTHON) -m build $(CURDIR)/bindings/python \
			--wheel \
			--no-isolation \
			--outdir $(PY_TARGET_DIR)

python-test: python-package
	$(PYTHON) -m pip install --force-reinstall --no-build-isolation $(PY_TARGET_DIR)/pynumstore-*.whl
	$(PYTHON) -m pip install pytest
	$(PYTHON) -m pytest $(CURDIR)/bindings/python/tests

############ Targets

$(OBJ_DIR)/%.o: src/%.c | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	@echo "  CC       $< -> $(patsubst $(CURDIR)/%,%,$@)"
	@$(CC) $(CFLAGS) -c $< -o $@

$(TARGET_LIB): $(LIBNS_OBJS) | $(LIB_DIR)
	@echo "  AR       $(patsubst $(CURDIR)/%,%,$(TARGET_LIB)) ($(words $(LIBNS_OBJS)) objs)"
	@$(AR) rcs $@ $(LIBNS_OBJS)

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
	@mkdir -p $(dir $@)
	$(PANDOC) $(PANDOC_ARGS) --output $@ $

############ Default target

.PHONY: all clean format docs python python-package python-test

all: $(ALL)

docs: $(HTML_OUTPUTS)

python: $(TARGET_PYLIB)

############ Directories

$(INC_DIR) $(BIN_DIR) $(LIB_DIR) $(OBJ_DIR) $(SMP_DIR) $(HTML_DIR) $(PY_TARGET_DIR) $(PY_OBJ_DIR): 
	@mkdir -p $@

############ Package Management

TEST_BIN := build/debug$(CROSS_SUFFIX)/target/bin/unit_tests$(if $(findstring windows,$(PLATFORM)),.exe)

release-package:
	$(MAKE) clean
	$(MAKE) TARGET=debug
	./$(TEST_BIN)
	$(MAKE) TARGET=release
	cp docs/release_docs.md build/release$(CROSS_SUFFIX)/target/README.md
	tar -czf build/release$(CROSS_SUFFIX).tar.gz -C build/release$(CROSS_SUFFIX) target
	cd build/release$(CROSS_SUFFIX) && zip -r ../release$(CROSS_SUFFIX).zip target

############ Cross-compilation via dockcross
# Examples:
# 	make cross PLATFORM=windows-static-x64
# 	make cross PLATFORM=windows-static-x64 CROSS_GOAL=release-package-windows-cross
# 	make cross PLATFORM=linux-arm64 CROSS_GOAL="TARGET=release all"

docker/dockcross-%: 
	@echo "  DOCKCROSS $*"
	@docker run --rm dockcross/$* > $@
	@chmod u+x $@

ifneq ($(filter cross,$(MAKECMDGOALS)),)
ifeq ($(PLATFORM),)
$(error PLATFORM is required, e.g. make cross PLATFORM=windows-static-x64)
endif
endif

.PHONY: cross
cross: docker/dockcross-$(PLATFORM)
	./$< bash -c 'make $(CROSS_GOAL) PLATFORM=$(PLATFORM) CC=$$CC AR=$$AR'

# Just a list of platforms to try
PACKAGE_PLATFORMS := \
	windows-static-x64 \
	windows-static-x86 \
	linux-x64 \
	linux-x86 \
	linux-arm64 \
	linux-armv6 \
	linux-armv7a \
	manylinux2014-x64 \
	manylinux2014-x86

.PHONY: package-release-all-platforms
package-release-all-platforms:
	@for p in $(PACKAGE_PLATFORMS); do \
		echo "  RELEASE  $$p"; \
		$(MAKE) cross PLATFORM=$$p CROSS_GOAL=release-package-windows-cross || exit 1; \
	done
	@echo "  DONE     built $(words $(PACKAGE_PLATFORMS)) platform(s):"
	@for p in $(PACKAGE_PLATFORMS); do echo "             - build/release-$$p"; done

############ Housekeeping

compile_commands.json:
	@echo "  BEAR     compile_commands.json"
	@$(BEAR) -- $(MAKE) TARGET=debug clean all > /dev/null

.PHONY: lint
lint: compile_commands.json
	@echo "  TIDY     $(words $(LIBNS_SRCS))"
	$(CLANG_TIDY) -p . $(LIBNS_SRCS)

.PHONY: lint-fix
lint-fix: compile_commands.json
	@echo "  TIDY-FIX $(words $(LIBNS_SRCS)) files"
	@$(CLANG_TIDY) -p . --fix --fix-errors $(LIBNS_SRCS)

############ Housekeeping

clean:
	rm -rf build
	rm -rf bindings/python/build bindings/python/dist
	rm -rf bindings/python/*.egg-info bindings/python/src/*.egg-info
	rm -rf bindings/python/.pytest_cache
	find bindings/python -type d -name __pycache__ -prune -exec rm -rf {} +
	rm -f *.wal 
	rm -f *test*
	rm -f *sample*
	rm -f *.db

format:
	find src bindings -type f \( -name '*.c' -o -name '*.h' \) -print0 \
		| xargs -0 $(CLANG_FORMAT) -i

example:
	echo "Testing CI JOB - this will be deleted"

############ Header dependencies

-include $(LIBNS_OBJS:.o=.d)
-include $(ALL_PYOBJS:.o=.d)
