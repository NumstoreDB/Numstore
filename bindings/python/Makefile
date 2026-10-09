#############################################
# A thin wrapper around the python build tools
#
# Run from bindings/python, or from anywhere with make -C bindings/python.
# Override any variable on the command line, e.g.
#
#   make install PYTHON=python3.12
#   make wheel BUILD_TYPE=Debug

PYTHON     ?= python3
BUILD_TYPE ?= Release
DIST       ?= dist
WHEELHOUSE ?= wheelhouse

PIP   = $(PYTHON) -m pip
BUILD = $(PYTHON) -m build -C cmake.build-type=$(BUILD_TYPE)

# Newest wheel in $(DIST) -- several can pile up across python versions
WHEEL = $$(ls -t $(DIST)/pynumstore-*.whl | head -1)

.PHONY: all help deps wheel sdist vendor install dev test samples check \
        format clean upload-test upload

all: wheel

help:
	@echo "make deps         install build/test tooling into the current python"
	@echo "make wheel        build a wheel into $(DIST)/"
	@echo "make install      build a wheel and pip install it"
	@echo "make dev          editable install with test extras"
	@echo "make test         run pytest"
	@echo "make samples      run every script in samples/"
	@echo "make check        install, then test and samples"
	@echo "make sdist        vendor numstore and build a source distribution"
	@echo "make format       ruff format"
	@echo "make clean        remove build artifacts"
	@echo "make upload-test  upload $(WHEELHOUSE)/*.whl to TestPyPI"
	@echo "make upload       upload $(WHEELHOUSE)/*.whl to PyPI"


############ Build
deps:
	$(PIP) install --upgrade build pytest ruff twine

wheel:
	$(BUILD) --wheel . --outdir $(DIST)

vendor:
	scripts/vendor.sh

sdist: vendor
	$(BUILD) --sdist . --outdir $(DIST)


############ Install
install: wheel
	$(PIP) install --force-reinstall --no-deps $(WHEEL)
	$(PIP) install numpy

dev:
	$(PIP) install -e '.[test]'


############ Test
test:
	$(PYTHON) -m pytest tests

# Samples write database files into the working directory, so run them in a
# scratch directory rather than the source tree
samples:
	@mkdir -p build/samples
	@set -e; for s in samples/*.py; do \
	    echo "== $$s"; \
	    (cd build/samples && $(PYTHON) ../../$$s); \
	done

check: install test samples


#############################################
### Housekeeping

format:
	scripts/format.sh

clean:
	scripts/clean.sh

# Wheels for PyPI must be manylinux/macOS/Windows builds -- download them from
# the Wheels workflow's artifacts into $(WHEELHOUSE) first. A local wheel from
# `make wheel` is tagged linux_x86_64 and PyPI rejects it.
upload-test:
	scripts/upload_python.sh $(WHEELHOUSE)/*.whl

upload:
	scripts/upload_python.sh --prod $(WHEELHOUSE)/*.whl
