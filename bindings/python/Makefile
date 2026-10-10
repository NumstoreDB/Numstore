#############################################
# A thin wrapper around the python build tools
#
# Run from bindings/python, or from anywhere with make -C bindings/python.
# Override any variable on the command line, e.g.
#
#   make install PYTHON=python3.12
#   make wheel BUILD_TYPE=Debug

# Newest wheel in dist -- several can pile up across python versions
WHEEL = $$(ls -t dist/pynumstore-*.whl | head -1)

.PHONY: all help deps wheel sdist vendor install dev test samples check \
        format clean upload-test upload

all: wheel

help:
	@echo "make deps         install build/test tooling into the current python"
	@echo "make wheel        build a wheel into dist/"
	@echo "make install      build a wheel and pip install it"
	@echo "make dev          editable install with test extras"
	@echo "make test         run pytest"
	@echo "make samples      run every script in samples/"
	@echo "make check        install, then test and samples"
	@echo "make sdist        vendor numstore and build a source distribution"
	@echo "make format       ruff format"
	@echo "make clean        remove build artifacts"
	@echo "make upload-test  upload wheelhouse/*.whl to TestPyPI"
	@echo "make upload       upload wheelhouse/*.whl to PyPI"


############ Build
deps:
	python3 -m pip install --upgrade build pytest ruff twine

wheel:
	python3 -m build -C cmake.build-type=Release --wheel . --outdir dist

vendor:
	scripts/vendor.sh

sdist: vendor
	python3 -m build -C cmake.build-type=Release --sdist . --outdir dist


############ Install
install: wheel
	python3 -m pip install --force-reinstall --no-deps $(WHEEL)
	python3 -m pip install numpy

dev:
	python3 -m pip install -e '.[test]'


############ Test
test:
	python3 -m pytest tests

# Samples write database files into the working directory, so run them in a
# scratch directory rather than the source tree
samples:
	@mkdir -p build/samples
	@set -e; for s in samples/*.py; do \
	    echo "== $$s"; \
	    (cd build/samples && python3 ../../$$s); \
	done

check: install test samples


#############################################
### Housekeeping

format:
	scripts/format.sh

clean:
	scripts/clean.sh

upload-test:
	scripts/upload_python.sh wheelhouse/*.whl

upload:
	scripts/upload_python.sh --prod wheelhouse/*.whl
