#############################################
# A thin wrapper around cmake
#
# Don't use this for production builds - just
# eliminates keystrokes while developing

.PHONY: all release release-package python python-upload python-upload-prod clean format

all: build/debug/CMakeCache.txt
	cmake --build build/debug

release: build/release/CMakeCache.txt
	cmake --build build/release

release-package: release
	rm -rf build/release/package
	cmake --install build/release --prefix build/release/package

# Debug configuration
build/debug/CMakeCache.txt:
	cmake -S numstore -B build/debug -DCMAKE_BUILD_TYPE=Debug

# Release configuration
build/release/CMakeCache.txt:
	cmake -S numstore -B build/release -DCMAKE_BUILD_TYPE=Release

#############################################
### Python bindings

# The cleaned wheel
python: wheelhouse/pynumstore-*.whl

# The main wheel (python step)
bindings/python/dist/pynumstore-*.whl:
	python3 -m build --wheel bindings/python --outdir bindings/python/dist

# Clean the wheel
wheelhouse/pynumstore-*.whl: bindings/python/dist/pynumstore-*.whl
	mkdir -p wheelhouse
ifeq ($(shell uname -s),Linux)
	auditwheel repair $^ -w wheelhouse
else
	cp $^ wheelhouse/
endif

# Upload the clean wheel
python-upload: wheelhouse/pynumstore-*.whl
	scripts/upload_python.sh $^

# Upload the clean wheel to production
python-upload-prod: wheelhouse/pynumstore-*.whl
	scripts/upload_python.sh --prod $^

#############################################
### Housekeeping

clean:
	scripts/clean_all.sh

format:
	scripts/format.sh
