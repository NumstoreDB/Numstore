############ Targets Reference
#
# make 																		build lib + tool binaries (default)
# make all                          			build lib + tool binaries (default)
# make docs                         			build docs/*.md -> html
# make python                       			alias for python-package
# make python-sources               			regen bindings/python/sources.txt
# make python-package               			build wheel
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
#   make ASAN=1
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
#   make lint
#   make lint-fix
#   make clean
#   make format
############

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
ASAN   				?= 0
NLOG 					?= 1
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
python-sources: $(PY_SOURCES_FILE) 

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

MD_DIR := $(DOC_DIR)/markdown

# docs/foo/bar.md -> $(HTML_DIR)/foo/bar.html and $(MD_DIR)/foo/bar.md
MD_FILES     := $(shell find docs -name '*.md' -not -path 'docs/pandoc/*')
HTML_OUTPUTS := $(patsubst docs/%.md,$(HTML_DIR)/%.html,$(MD_FILES))
MD_OUTPUTS   := $(patsubst docs/%.md,$(MD_DIR)/%.md,$(MD_FILES))

$(HTML_DIR)/%.html: docs/%.md $(PANDOC_DEPS) | $(HTML_DIR)
	@mkdir -p $(dir $@)
	@echo "  PANDOC   $< -> $(patsubst $(CURDIR)/%,%,$@)"
	@PANDOC_SRC_REL=$(patsubst docs/%,%,$<) $(PANDOC) $(PANDOC_ARGS) --output $@ $<

$(MD_DIR)/%.md: docs/%.md | $(MD_DIR)
	@mkdir -p $(dir $@)
	@echo "  CP       $< -> $(patsubst $(CURDIR)/%,%,$@)"
	@cp $< $@

MAN_DIR  := $(PKG_DIR)/share/man
MAN_SRCS := $(shell find docs/man -name '*.md')

PANDOC_MAN_ARGS := \
	--from=markdown \
	--to=man \
	--standalone

MAN_OUTPUTS := $(patsubst docs/man/%.md,$(MAN_DIR)/%,$(MAN_SRCS))

$(MAN_DIR)/%: docs/man/%.md
	@mkdir -p $(dir $@)
	@echo "  MAN      $< -> $(patsubst $(CURDIR)/%,%,$@)"
	@PANDOC_SRC_REL=man/$(patsubst docs/man/%,%,$<) $(PANDOC) $(PANDOC_MAN_ARGS) --output $@ $<

.PHONY: docs
docs: $(HTML_OUTPUTS) $(MD_OUTPUTS) $(MAN_OUTPUTS)

############ Packaging Targets

PKG_TEMPLATES_DIR := $(CURDIR)/packaging

PC_TEMPLATES := \
	$(PKG_TEMPLATES_DIR)/pkgconfig/numstore.pc.in \
	$(PKG_TEMPLATES_DIR)/pkgconfig/smartfiles.pc.in

SAMPLES_MAKEFILE_IN := $(PKG_TEMPLATES_DIR)/samples/Makefile.in

PKG_SUBST := \
	-e 's|@VERSION@|$(VERSION)|g' \
	-e 's|@PROJECT_NAME@|$(PROJECT_NAME)|g'

.PHONY: release-package
release-package: all docs | $(PC_DIR) $(SMP_DIR)
	@echo "  PKG      $(ARTIFACT_NAME)"
	@cp $(CURDIR)/LICENSE $(PKG_DIR)/LICENSE
	@cp $(CURDIR)/CHANGELOG.md $(PKG_DIR)/CHANGELOG.md
	@cp $(CURDIR)/docs/release_docs.md $(PKG_DIR)/README.md
	@for t in $(PC_TEMPLATES); do \
		out=$(PC_DIR)/$$(basename $$t .in); \
		sed $(PKG_SUBST) $$t > $$out; \
	done
	@sed $(PKG_SUBST) $(SAMPLES_MAKEFILE_IN) > $(SMP_DIR)/Makefile
	@cp samples/*.c $(SMP_DIR)/ 2>/dev/null || true
	@echo "  DONE     $(PKG_DIR)"

.PHONY: release-tarball
release-tarball: release-package
ifeq ($(ARCHIVE_EXT),zip)
	cd $(OUT_DIR) && zip -r -q $(ARTIFACT_NAME).zip $(ARTIFACT_NAME)
	@echo "  ZIP      $(OUT_DIR)/$(ARTIFACT_NAME).zip"
else
	tar -C $(OUT_DIR) -czf $(OUT_DIR)/$(ARTIFACT_NAME).tar.gz $(ARTIFACT_NAME)
	@echo "  TAR      $(OUT_DIR)/$(ARTIFACT_NAME).tar.gz"
endif

############ Default target

.PHONY: all clean format docs python python-package python-test

all: $(ALL)

docs: $(HTML_OUTPUTS)

python: $(TARGET_PYLIB)

############ Directories

$(INC_DIR) $(BIN_DIR) $(LIB_DIR) $(PC_DIR) $(OBJ_DIR) $(SMP_DIR) $(HTML_DIR) $(MD_DIR) $(DOC_DIR) $(PY_TARGET_DIR) $(PY_OBJ_DIR):
	@mkdir -p $@

$(MAN_DIR)/man%:
	@mkdir -p $@

############ Cross-compilation via dockcross

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
		$(MAKE) cross PLATFORM=$$p CROSS_GOAL="TARGET=release release-tarball" || exit 1; \
	done
	@echo "  DONE     built $(words $(PACKAGE_PLATFORMS)) platform(s) under build/release-*/"

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
