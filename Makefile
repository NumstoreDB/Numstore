#############################################
# A thin wrapper around cmake
#
# Don't use this for production builds - just
# eliminates keystrokes while developing

.PHONY: all release release-package clean format

all: debug


############ Debug
debug: build/debug/CMakeCache.txt 
	cmake --build build/debug 

build/debug/CMakeCache.txt:
	cmake -S numstore -B build/debug -DCMAKE_BUILD_TYPE=Debug 


############ Release
release-package: release
	rm -rf build/release/package
	cmake --install build/release --prefix build/release/package

release: build/release/CMakeCache.txt
	cmake --build build/release -j12

build/release/CMakeCache.txt:
	cmake -S numstore -B build/release -DCMAKE_BUILD_TYPE=Release -DNLOG=1


############ Code Coverage
coverage: build/coverage/html/index.html

build/coverage/CMakeCache.txt:
	cmake -S numstore -B build/coverage \
		-DCMAKE_BUILD_TYPE=Debug \
		-DCMAKE_C_FLAGS="--coverage -O0" \
		-DCMAKE_EXE_LINKER_FLAGS="--coverage" \
		-DCMAKE_SHARED_LINKER_FLAGS="--coverage"

build/coverage/bin/ns_simtest: build/coverage/bin/unit_tests
build/coverage/bin/unit_tests: build/coverage/CMakeCache.txt
	cmake --build build/coverage -j12

build/coverage/.stamp: build/coverage/bin/unit_tests build/coverage/bin/ns_simtest
	find build/coverage -name "*.gcda" -delete
	./build/coverage/bin/unit_tests 123142
	./build/coverage/bin/ns_simtest --duration 5
	touch build/coverage/.stamp

build/coverage/html/index.html: build/coverage/.stamp
	rm -rf build/coverage/html 
	mkdir -p build/coverage/html
	gcovr -r . build/coverage \
		--html-details build/coverage/html/index.html \
		--exclude numstore/apps \
		--exclude build \
		--print-summary


#############################################
### Housekeeping

format:
	scripts/format.sh

# bindings/javascript has no formatter, so it only shows up here.
clean:
	scripts/clean_all.sh
