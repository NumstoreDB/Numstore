#############################################
# A thin wrapper around cmake
#
# Don't use this for production builds - just
# eliminates keystrokes while developing

.PHONY: all debug release release-package coverage coverage-xml coverage-check \
        ci ci-debug ci-simtest ci-release ci-coverage ci-format ci-copyright \
        ci-wheels valgrind valgrind-unit valgrind-simtest leaks leaks-unit \
        leaks-simtest lldb lldb-simtest clean copyright format

all: debug


############ Debug
debug: build/debug/CMakeCache.txt 
	cmake --build build/debug 

build/debug/CMakeCache.txt:
	cmake -S numstore -B build/debug \
		-DCMAKE_BUILD_TYPE=Debug \
		-DCMAKE_C_FLAGS="-Wall -Wextra -Werror" \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON


############ Release
release-package: release
	rm -rf build/release/package
	cmake --install build/release --prefix build/release/package

release: build/release/CMakeCache.txt
	cmake --build build/release -j12

build/release/CMakeCache.txt:
	cmake -S numstore -B build/release -DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_C_FLAGS="-Wall -Wextra -Werror"


############ Code Coverage
coverage: build/coverage/html/index.html

build/coverage/CMakeCache.txt:
	cmake -S numstore -B build/coverage \
		-DCMAKE_BUILD_TYPE=Debug \
		-DCMAKE_C_FLAGS="--coverage -O0 -Wall -Wextra -Werror" \
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
		--gcov-ignore-parse-errors negative_hits.warn_once_per_file \
		--exclude numstore/apps \
		--exclude build \
		--print-summary

coverage-xml: build/coverage/report/coverage.xml

build/coverage/report/coverage.xml: build/coverage/.stamp
	mkdir -p build/coverage/report
	gcovr -r . build/coverage \
		--cobertura build/coverage/report/coverage.xml \
		--cobertura-pretty \
		--txt build/coverage/report/coverage.txt \
		--gcov-ignore-parse-errors negative_hits.warn_once_per_file \
		--exclude numstore/apps \
		--exclude build \
		--print-summary

coverage-check: build/coverage/.stamp
	gcovr -r . build/coverage \
		--gcov-ignore-parse-errors negative_hits.warn_once_per_file \
		--exclude numstore/apps \
		--exclude build \
		--fail-under-line 80 \
		--fail-under-branch 45


############ Leak checking

build/memcheck/CMakeCache.txt:
	cmake -S numstore -B build/memcheck \
		-DCMAKE_BUILD_TYPE=Debug \
		-DCMAKE_C_FLAGS="-g3 -O0 -fno-inline -fno-omit-frame-pointer -Wall -Wextra -Werror"

build/memcheck/bin/ns_simtest: build/memcheck/bin/unit_tests
build/memcheck/bin/unit_tests: build/memcheck/CMakeCache.txt
	cmake --build build/memcheck -j12


############ Valgrind (linux)

VALGRIND = valgrind \
	--tool=memcheck \
	--error-exitcode=1 \
	--leak-check=full \
	--show-leak-kinds=all \
	--errors-for-leak-kinds=all \
	--track-origins=yes \
	--track-fds=yes \
	--trace-children=yes \
	--undef-value-errors=yes \
	--expensive-definedness-checks=yes \
	--partial-loads-ok=no \
	--malloc-fill=0xab \
	--free-fill=0xcd \
	--keep-debuginfo=yes \
	--read-var-info=yes \
	--num-callers=50 \
	--fair-sched=yes \
	--gen-suppressions=all

valgrind: valgrind-unit valgrind-simtest

valgrind-unit: build/memcheck/bin/unit_tests
	$(VALGRIND) ./build/memcheck/bin/unit_tests 123142

valgrind-simtest: build/memcheck/bin/ns_simtest
	$(VALGRIND) ./build/memcheck/bin/ns_simtest --dbname simtest.db --duration 5 --seed 123142


############ Leaks (macos)

LEAKS = MallocStackLogging=1 \
	MallocStackLoggingNoCompact=1 \
	MallocScribble=1 \
	MallocPreScribble=1 \
	MallocGuardEdges=1 \
	leaks --atExit --list --fullStacks --

leaks: leaks-unit leaks-simtest

leaks-unit: build/memcheck/bin/unit_tests
	$(LEAKS) ./build/memcheck/bin/unit_tests 123142

leaks-simtest: build/memcheck/bin/ns_simtest
	$(LEAKS) ./build/memcheck/bin/ns_simtest --dbname simtest.db --duration 5 --seed 123142


#############################################
### CI

# Run the debug unit tests
ci-debug: debug
	if [ -f build/debug/bin/unit_tests.exe ]; \
		then ./build/debug/bin/unit_tests.exe 123142; \
		else ./build/debug/bin/unit_tests 123142; fi


# Run the debug simulation tests
ci-simtest: debug
	if [ -f build/debug/bin/ns_simtest.exe ]; \
		then exe=./build/debug/bin/ns_simtest.exe; \
		else exe=./build/debug/bin/ns_simtest; fi; \
	$$exe \
		--dbname simtest.db \
		--duration 5 \
		--seed 123142 \
		--commit-hash $$(git rev-parse HEAD) \
		--seqid 123142

# Build the release package
ci-release: release-package
	find build/release/package -type f | sort

# Run code coverage
ci-coverage: coverage coverage-xml coverage-check

# Format - if anything is different - fail
ci-format: format
	git --no-pager diff --exit-code

# Copyright - if anything is different - fail
ci-copyright: copyright
	git --no-pager diff --exit-code

# Generate python wheels
ci-wheels:
	$(MAKE) -C bindings/python ci-wheels

# All ci
ci: ci-debug ci-simtest ci-coverage ci-release


#############################################
### Housekeeping

copyright:
	scripts/add_copywrite.py

format:
	scripts/format.sh
	$(MAKE) -C bindings/python format

# bindings/javascript has no formatter, so it only shows up here.
clean:
	scripts/clean_all.sh
	$(MAKE) -C bindings/python clean
