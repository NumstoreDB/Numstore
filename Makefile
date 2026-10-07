#############################################
# A thin wrapper around cmake
#
# Don't use this for production builds - just
# eliminates keystrokes while developing

.PHONY: all release release-package clean format

# NLOG=1 compiles out logging (CI uses it to keep expected-error traces out of
# the log). Takes effect at configure time, so it needs a fresh build dir.
CMAKE_CONFIG_FLAGS := $(if $(NLOG),-DNS_NLOG=ON,)

all: build/debug/CMakeCache.txt
	cmake --build build/debug -j12

release: build/release/CMakeCache.txt
	cmake --build build/release -j12

release-package: release
	rm -rf build/release/package
	cmake --install build/release --prefix build/release/package

# Debug configuration
build/debug/CMakeCache.txt:
	cmake -S numstore -B build/debug -DCMAKE_BUILD_TYPE=Debug $(CMAKE_CONFIG_FLAGS)

# Release configuration
build/release/CMakeCache.txt:
	cmake -S numstore -B build/release -DCMAKE_BUILD_TYPE=Release $(CMAKE_CONFIG_FLAGS)

#############################################
### Housekeeping

format:
	scripts/format.sh
	$(MAKE) -C bindings/python format
	$(MAKE) -C bindings/rust format
	$(MAKE) -C nsserver format

# bindings/javascript has no formatter, so it only shows up here.
clean:
	scripts/clean_all.sh
	$(MAKE) -C bindings/python clean
	$(MAKE) -C bindings/rust clean
	$(MAKE) -C bindings/javascript clean
	$(MAKE) -C nsserver clean
