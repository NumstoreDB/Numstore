#############################################
# A thin wrapper around cmake
#
# Don't use this for production builds - just
# eliminates keystrokes while developing

.PHONY: all debug release release-package coverage coverage-xml coverage-check \
        ci ci-debug ci-simtest ci-release ci-coverage ci-format ci-copyright \
        ci-wheels clean copyright format

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
# gcc's gcov emits "branch N taken -1" on this tree (GCC bug 68080), which
# gcovr treats as fatal unless told to warn instead. Apple clang's gcov does
# not, so this only bites in CI.
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

# Measured 88.0% line / 54.9% branch locally, so these sit below that with
# room for the gcc/clang difference. Raise them as coverage climbs.
coverage-check: build/coverage/.stamp
	gcovr -r . build/coverage \
		--gcov-ignore-parse-errors negative_hits.warn_once_per_file \
		--exclude numstore/apps \
		--exclude build \
		--fail-under-line 80 \
		--fail-under-branch 45


#############################################
### CI
#
# One target per workflow step. .github/workflows calls these so the build and
# test commands live here instead of being duplicated in yaml.
#
# Windows builds unit_tests.exe / ns_simtest.exe, and MSVC drops a matching
# .pdb beside each, so the suffix is tested for rather than globbed -- a
# unit_tests* glob would pass the .pdb along as an argument.

############ Debug build and unit tests
ci-debug: debug
	if [ -f build/debug/bin/unit_tests.exe ]; \
		then ./build/debug/bin/unit_tests.exe 123142; \
		else ./build/debug/bin/unit_tests 123142; fi


############ Simulation test
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


############ Release package
ci-release: release-package
	find build/release/package -type f | sort


############ Code coverage
ci-coverage: coverage coverage-xml coverage-check


############ Formatting
# Formats in place, then fails if that changed anything -- i.e. if the commit
# was made without running `make format`. Expects a clean tree.
ci-format: format
	git --no-pager diff --exit-code


############ Copyright headers
# Same idea: adds any missing header, then fails if that changed anything.
ci-copyright: copyright
	git --no-pager diff --exit-code


############ Wheels
ci-wheels:
	$(MAKE) -C bindings/python ci-wheels


############ Everything
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
