#############################################
# A thin wrapper around $(CMAKE) 
#
# Don't use this for production builds - just 
# eliminates keystrokes while developing

.PHONY: all clean format python python-upload python-upload-test

CMAKE 		 ?= cmake
PYTHON     ?= python3

all: build
	$(CMAKE) --build build 

build:
	$(CMAKE) -S numstore -B build -DCMAKE_BUILD_TYPE=Debug

clean: build
	$(CMAKE) --build build --target clean	
	rm -rf bindings/python/dist 
	rm -rf bindings/python/build 
	rm -rf wheelhouse
	rm -rf bindings/python/*.egg-info 
	rm -rf bindings/python/src/*.egg-info 
	rm -rf bindings/python/.pytest_cache
	find bindings/python -type d -name __pycache__ -prune -exec rm -rf {} +
	find . -maxdepth 1 -type f \( \
		-name '*.wal' -o -name '*.db' -o -name 'testdb*' -o -name '*sample*' -o -name 'test*' \
	\) -delete

python:
	$(PYTHON) -m build --wheel bindings/python --outdir bindings/python/dist

python-repair: python
ifeq ($(shell uname -s),Linux)
	auditwheel repair $(PY_DIST)/*.whl -w wheelhouse
else
	mkdir -p wheelhouse && cp $(PY_DIST)/*.whl wheelhouse/
endif

python-upload-test: python-repair
	twine check wheelhouse/*
	twine upload --repository testpypi wheelhouse/*

python-upload: python-repair
	twine check wheelhouse/*
	twine upload wheelhouse/*

format: format-c format-cmake format-python

PRUNE = -type d \( -name build -o -name dist -o -name .venv -o -name venv -o -name .git \) -prune -o

format-c:
	@if command -v clang-format >/dev/null 2>&1; then \
		find numstore bindings $(PRUNE) -type f \( -name '*.c' -o -name '*.h' \) -print \
			| while read -r f; do echo "formatting: $$f"; clang-format --style=file:.clang-format -i "$$f"; done; \
	else \
		echo "clang-format not found; skipping"; \
	fi

format-cmake:
	@if command -v gersemi >/dev/null 2>&1; then \
		find . $(PRUNE) -type f \( -name CMakeLists.txt -o -name '*.cmake' \) -print \
			| while read -r f; do echo "formatting: $$f"; gersemi -i "$$f"; done; \
	else \
		echo "gersemi not found; skipping"; \
	fi

format-python:
	@if command -v ruff >/dev/null 2>&1; then \
		find bindings/python $(PRUNE) -type f -name '*.py' -print \
			| while read -r f; do echo "formatting: $$f"; ruff format --quiet "$$f"; done; \
	else \
		echo "ruff not found; skipping"; \
	fi
