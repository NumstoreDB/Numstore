############ Targets Reference
#
# make 																		build lib + tool binaries (default)
# make all                          			build lib + tool binaries (default)
# make docs                         			build docs/*.md -> html
# make python                       			alias for python-package
# make python-package               			build wheel
# make python-wheels                			cibuildwheel wheels for this host's platform
# make all-libs                     			release archives: native + every cross platform
# make all-wheels                   			every wheel this host can build
# make python-test                  			build + install + pytest
# make release-package              			assemble SDK folder (bin/lib/include/docs/samples)
# make release-tarball              			release-package + tar.gz/zip
# make cross                        			run goal inside dockcross container
# make package-release-all-platforms			cross + release-tarball for every platform
# make compile_commands.json        			generate via bear
# make lint                         			clang-tidy (check only)
# make lint-fix                     			clang-tidy --fix
# make clean                        			rm build/ + py artifacts + stray files
# make format                       			clang-format -i src/ bindings/
#
# Examples:
#   make
#   make TARGET=release
#   make NLOG=1
#   make CFLAGS_USER="-Wpedantic"
#   make docs
#   make python-test
#   make TARGET=release release-tarball
#   make cross PLATFORM=windows-static-x64
#   make cross PLATFORM=linux-arm64 CROSS_GOAL="TARGET=release all"
# 	make cross PLATFORM=windows-static-x64
# 	make cross PLATFORM=windows-static-x64 CROSS_GOAL=release-package-windows-cross
# 	make cross PLATFORM=linux-arm64 CROSS_GOAL="TARGET=release all"
#   make package-release-all-platforms
#   make all-libs
#   make all-wheels
#   scripts/build_all_libs.sh -t debug -g all
#   scripts/build_all_wheels.sh -b 'cp312-*'
#   make lint
#   make lint-fix
#   make clean
#   make format
############

# Remove a target whose recipe failed. Without this a partially written file
# counts as up to date on the next run.
.DELETE_ON_ERROR:

############ Executables

CC           		:= clang
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
NLOG 					?= 0
CFLAGS_USER 	?=


############ Project Name / Identity 

PROJECT_NAME	:= numstore
VERSION 			:= $(shell cat version.txt)

############ Platform -> release os/arch mapping (infers from dockcross PLATFORM)

ifeq ($(PLATFORM),)
  # Native build - fall back to uname
  UNAME_S      := $(shell uname -s)
  UNAME_M      := $(shell uname -m)

  ifeq ($(UNAME_S),Linux)
    RELEASE_OS := linux
  else ifeq ($(UNAME_S),Darwin)
    RELEASE_OS := macos
  else
    RELEASE_OS := $(UNAME_S)
  endif

  RELEASE_ARCH := $(UNAME_M)
else
  # dockcross platform names: 
	# linux-x64
	# linux-x86
	# linux-arm64
	# linux-armv6
	# linux-armv7a
	# windows-static-x64
	# windows-static-x86
	# manylinux2014-x64
	# manylinux2014-x86
  ifneq (,$(findstring windows,$(PLATFORM)))
    RELEASE_OS := windows
  else
    RELEASE_OS := linux
  endif

  PLATFORM_ARCH := $(subst windows-static-,,$(subst manylinux2014-,,$(subst linux-,,$(PLATFORM))))

  ifeq ($(PLATFORM_ARCH),x64)
    RELEASE_ARCH := x86_64
  else ifeq ($(PLATFORM_ARCH),x86)
    RELEASE_ARCH := i686
  else
    RELEASE_ARCH := $(PLATFORM_ARCH)
  endif
endif

ARTIFACT_NAME := $(PROJECT_NAME)-$(VERSION)-$(RELEASE_OS)-$(RELEASE_ARCH)

ARCHIVE_EXT := tar.gz
ifeq ($(RELEASE_OS),windows)
ARCHIVE_EXT := zip
endif

############ Output Directories

CROSS_SUFFIX := $(if $(PLATFORM),-$(PLATFORM))
BUILD_NAME	 := $(TARGET)$(CROSS_SUFFIX)
OUT_DIR  		 := $(CURDIR)/build/$(BUILD_NAME)

# Not included in the output
OBJ_DIR  		 := $(OUT_DIR)/objs

# Included in the output
PKG_DIR      := $(OUT_DIR)/$(ARTIFACT_NAME)
BIN_DIR      := $(PKG_DIR)/bin
LIB_DIR      := $(PKG_DIR)/lib
PC_DIR       := $(LIB_DIR)/pkgconfig
INC_DIR      := $(PKG_DIR)/include
DOC_DIR      := $(PKG_DIR)/share/doc/$(PROJECT_NAME)
HTML_DIR     := $(DOC_DIR)/html
SMP_DIR      := $(PKG_DIR)/share/$(PROJECT_NAME)/examples

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

# No logs
ifeq ($(NLOG),1)
CFLAGS += $(CFLAGS_NLOG)
endif

# Add user flags
CFLAGS += $(CFLAGS_USER)

############ Linker Flags

# On glibc older than 2.34 the pthread entry points live in libpthread rather
# than libc, so anything pulling in libnumstore has to ask for them. Only the
# oldest target exposed this (manylinux2014 is CentOS 7 / glibc 2.17, and it
# failed to link pthread_once and pthread_join); newer glibc resolves them from
# libc and hid the omission. -pthread is correct on every POSIX target.
# Windows uses the Win32 threading backend and needs neither flag.
LDFLAGS_COMMON :=
LDLIBS_COMMON  :=

ifneq ($(RELEASE_OS),windows)
LDFLAGS_COMMON += -pthread
LDLIBS_COMMON  += -lm
endif

LDFLAGS := $(LDFLAGS_COMMON)
LDLIBS  := $(LDLIBS_COMMON)

# Shared recipe for every tool/sample/test binary. The static library has to
# precede the system libraries it depends on.
LINK_BIN = $(CC) $(CFLAGS) -I$(INC_DIR) $< -o $@ $(TARGET_LIB) $(LDFLAGS) $(LDLIBS)

############ Rust Flags

RUSTFLAGS := --edition 2021 --crate-type staticlib -C panic=abort

############ Accumulators - each module.mk appends to these

TARGET_LIB 		:= $(LIB_DIR)/libnumstore.a
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
endif

# Derived from sources above
LIBNS_OBJS := $(patsubst src/%.c,$(OBJ_DIR)/%.o,$(LIBNS_SRCS))

# Default target
.DEFAULT_GOAL := all

############ Python Flags

# setup.py reads the module.mk fragments directly, so there is no generated
# source list to keep in sync (bindings/python/sources.txt used to serve that
# role and went stale across a refactor).

# Documented in the target reference at the top of this file, but never defined.
python: python-package

python-package: | $(PY_TARGET_DIR)
	PYNUMSTORE_BUILD_BASE=$(PY_OBJ_DIR) \
		$(PYTHON) -m build $(CURDIR)/bindings/python \
			--wheel \
			--no-isolation \
			--outdir $(PY_TARGET_DIR)

python-test: python-package
	$(PYTHON) -m pip install --force-reinstall --no-build-isolation $(PY_TARGET_DIR)/pynumstore-*.whl
	$(PYTHON) -m pip install pytest
	$(PYTHON) -m pytest $(CURDIR)/bindings/python/tests

# Redistributable wheels for this host's platform. Must run from the repository
# root: cibuildwheel copies the invocation directory into its build
# environment, and setup.py reaches up to ../../src for the C sources.
python-wheels:
	$(PYTHON) -m cibuildwheel --output-dir $(PY_TARGET_DIR)/wheelhouse bindings/python

############ Targets

$(OBJ_DIR)/%.o: src/%.c | $(OBJ_DIR)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET_LIB): $(LIBNS_OBJS) | $(LIB_DIR)
	$(AR) rcs $@ $(LIBNS_OBJS)

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

MD_DIR := $(DOC_DIR)/markdown

# docs/foo/bar.md -> $(HTML_DIR)/foo/bar.html and $(MD_DIR)/foo/bar.md
MD_FILES     := $(shell find docs -name '*.md' -not -path 'docs/pandoc/*')
HTML_OUTPUTS := $(patsubst docs/%.md,$(HTML_DIR)/%.html,$(MD_FILES))
MD_OUTPUTS   := $(patsubst docs/%.md,$(MD_DIR)/%.md,$(MD_FILES))

$(HTML_DIR)/%.html: docs/%.md $(PANDOC_DEPS) | $(HTML_DIR)
	mkdir -p $(dir $@)
	PANDOC_SRC_REL=$(patsubst docs/%,%,$<) $(PANDOC) $(PANDOC_ARGS) --output $@ $<

$(MD_DIR)/%.md: docs/%.md | $(MD_DIR)
	mkdir -p $(dir $@)
	cp $< $@

MAN_DIR  := $(PKG_DIR)/share/man
# MAN_SRCS := $(shell find docs/man -name '*.md')

PANDOC_MAN_ARGS := \
	--from=markdown \
	--to=man \
	--standalone

# MAN_OUTPUTS := $(patsubst docs/man/%.md,$(MAN_DIR)/%,$(MAN_SRCS))

# $(MAN_DIR)/%: docs/man/%.md
# 	mkdir -p $(dir $@)
# 	PANDOC_SRC_REL=man/$(patsubst docs/man/%,%,$<) $(PANDOC) $(PANDOC_MAN_ARGS) --output $@ $<

# The dockcross images have no pandoc, so a cross release-package could never
# build the HTML docs - it failed with "pandoc: not found" on every platform.
# The markdown docs are plain copies and always ship; HTML is included only
# where pandoc is available.
PANDOC_FOUND := $(shell command -v $(PANDOC) >/dev/null 2>&1 && echo 1)

ifeq ($(PANDOC_FOUND),1)
DOC_OUTPUTS := $(HTML_OUTPUTS) $(MD_OUTPUTS)
else
DOC_OUTPUTS := $(MD_OUTPUTS)
endif

.PHONY: docs
docs: $(DOC_OUTPUTS) #  $(MAN_OUTPUTS)
ifneq ($(PANDOC_FOUND),1)
	@echo "note: $(PANDOC) not found - shipping markdown docs only, no HTML"
endif

############ Packaging Targets

PKG_TEMPLATES_DIR := $(CURDIR)/packaging

PC_TEMPLATES := \
	$(PKG_TEMPLATES_DIR)/pkgconfig/numstore.pc.in \
	# $(PKG_TEMPLATES_DIR)/pkgconfig/smartfiles.pc.in

SAMPLES_MAKEFILE_IN := $(PKG_TEMPLATES_DIR)/samples/Makefile.in

PKG_SUBST := \
	-e 's|@VERSION@|$(VERSION)|g' \
	-e 's|@PROJECT_NAME@|$(PROJECT_NAME)|g'

.PHONY: release-package
release-package: all docs | $(PC_DIR) $(SMP_DIR)
	cp $(CURDIR)/LICENSE $(PKG_DIR)/LICENSE
	cp $(CURDIR)/CHANGELOG.md $(PKG_DIR)/CHANGELOG.md
	cp $(CURDIR)/docs/release_docs.md $(PKG_DIR)/README.md
	for t in $(PC_TEMPLATES); do \
		out=$(PC_DIR)/$$(basename $$t .in); \
		sed $(PKG_SUBST) $$t > $$out; \
	done
	sed $(PKG_SUBST) $(SAMPLES_MAKEFILE_IN) > $(SMP_DIR)/Makefile
	cp samples/*.c $(SMP_DIR)/ 2>/dev/null || true

.PHONY: release-tarball
release-tarball: release-package
ifeq ($(ARCHIVE_EXT),zip)
	cd $(OUT_DIR) && zip -r -q $(ARTIFACT_NAME).zip $(ARTIFACT_NAME)
else
	tar -C $(OUT_DIR) -czf $(OUT_DIR)/$(ARTIFACT_NAME).tar.gz $(ARTIFACT_NAME)
endif

############ Default target

.PHONY: all clean format docs python python-package python-test python-wheels

all: $(ALL)

############ Directories

$(INC_DIR) $(BIN_DIR) $(LIB_DIR) $(PC_DIR) $(OBJ_DIR) $(SMP_DIR) $(HTML_DIR) $(MD_DIR) $(DOC_DIR) $(PY_TARGET_DIR) $(PY_OBJ_DIR):
	@mkdir -p $@

$(MAN_DIR)/man%:
	@mkdir -p $@

############ Cross-compilation via dockcross

# Write via a temp file: `> $@` truncates before docker run executes, so a
# failed image pull used to leave a 0-byte launcher behind. Because the file
# then existed, make never regenerated it and `make cross` silently ran an
# empty script - reporting success while building nothing. Two launchers
# (linux-x86, web-wasm32) were committed in exactly that state.
docker/dockcross-%:
	@rm -f $@.tmp
	docker run --rm dockcross/$* > $@.tmp || { rm -f $@.tmp; exit 1; }
	@[ -s $@.tmp ] || { rm -f $@.tmp; echo "error: dockcross/$* produced no launcher"; exit 1; }
	mv $@.tmp $@
	chmod u+x $@

ifneq ($(filter cross,$(MAKECMDGOALS)),)
ifeq ($(PLATFORM),)
$(error PLATFORM is required, e.g. make cross PLATFORM=windows-static-x64)
endif
endif

.PHONY: cross
cross: docker/dockcross-$(PLATFORM)
	@[ -s ./$< ] || { \
		echo "error: ./$< is empty, so it would build nothing."; \
		echo "       Delete it and retry: rm ./$< && make cross PLATFORM=$(PLATFORM)"; \
		echo "       The 32-bit x86 targets (linux-x86, manylinux2014-x86) only"; \
		echo "       work on an x86_64 host - their entrypoint runs linux32,"; \
		echo "       which cannot set a 32-bit personality under arm64 emulation."; \
		exit 1; \
	}
	./$< bash -c 'make $(CROSS_GOAL) PLATFORM=$(PLATFORM) CC=$$CC AR=$$AR'

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

# Single source of truth for the shipped platform list, so
# scripts/build_all_libs.sh cannot drift from it.
.PHONY: print-package-platforms
print-package-platforms:
	@echo $(PACKAGE_PLATFORMS)

.PHONY: package-release-all-platforms
package-release-all-platforms:
	for p in $(PACKAGE_PLATFORMS); do \
		$(MAKE) cross PLATFORM=$$p CROSS_GOAL="TARGET=release release-tarball" || exit 1; \
	done

############ Build everything

# Thin wrappers around the scripts, which (unlike a make loop) attempt every
# platform, report host-incompatible targets as skips, and print a matrix.
.PHONY: all-libs all-wheels
all-libs:
	./scripts/build_all_libs.sh

all-wheels:
	./scripts/build_all_wheels.sh

############ Housekeeping

compile_commands.json:
	$(BEAR) -- $(MAKE) TARGET=debug clean all 

.PHONY: lint
lint: compile_commands.json
	$(CLANG_TIDY) -p . $(LIBNS_SRCS)

.PHONY: lint-fix
lint-fix: compile_commands.json
	$(CLANG_TIDY) -p . --fix --fix-errors $(LIBNS_SRCS)

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
	rm -f foo

format:
	find src bindings -type f \( -name '*.c' -o -name '*.h' \) -print0 \
		| xargs -0 $(CLANG_FORMAT) -i

############ Header dependencies

-include $(LIBNS_OBJS:.o=.d)
-include $(ALL_PYOBJS:.o=.d)
