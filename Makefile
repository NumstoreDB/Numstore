.PHONY: all build comprehensive \
        test run-tests run-tests-no-asan \
        comprehensive-tests \
        package package-deb package-rpm package-tar package-zip \
        package-python package-java package-all \
        install clean format tidy

NPROC ?= $(shell nproc 2>/dev/null || sysctl -n hw.logicalcpu 2>/dev/null || echo 1)
target ?= debug

all: build compile_commands.json

######################################################## Main Build Target

compile_commands.json: 
	/usr/bin/cmake . -B build/debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
	cp build/debug/compile_commands.json .

build:
	/usr/bin/cmake --preset $(target)
	/usr/bin/cmake --build --preset $(target) -j$(NPROC)

comprehensive:
	$(MAKE) build target=debug
	$(MAKE) build target=debug-asan
	$(MAKE) build target=debug-leakcheck
	$(MAKE) build target=debug-coverage
	$(MAKE) build target=release
	$(MAKE) build target=release-tests
	$(MAKE) build target=release-asan
	$(MAKE) build target=release-leakcheck
	$(MAKE) build target=package-release

######################################################## Packaging

package:
	$(MAKE) build target=package-release
	cd build/package-release && cpack

package-deb:
	$(MAKE) build target=package-release
	cd build/package-release && cpack -G DEB

package-rpm:
	$(MAKE) build target=package-release
	cd build/package-release && cpack -G RPM

package-tar:
	$(MAKE) build target=package-release
	cd build/package-release && cpack -G TGZ

package-zip:
	$(MAKE) build target=package-release
	cd build/package-release && cpack -G ZIP

package-python:
	@mkdir -p build/pysmartfiles
	python3 -m build --outdir build/pysmartfiles
	@echo "Python wheel ready in build/pysmartfiles/"

package-java:
	$(MAKE) build target=release
	cmake --build build/release --target jsmartfiles-jar
	@mkdir -p build/jsmartfiles
	@cp bindings/java/jsmartfiles/build/libs/jsmartfiles-*.jar build/jsmartfiles/
	@echo "Java jar ready in build/jsmartfiles/"

package-all: package package-python package-java
	cd build/package-release && cpack -G DEB
	cd build/package-release && cpack -G RPM
	cd build/package-release && cpack -G TGZ
	cd build/package-release && cpack -G ZIP

install:
	$(MAKE) build target=package-release
	cmake --install build/package-release --prefix /usr/local

######################################################## Maintenance

clean:
	rm -rf build
	find . -name '*.db' -o -name '*.wal' -o -name '*.nsdb' | xargs rm -f
	rm -f compile_commands.json

format:
	python3 scripts/add_copywrite.ppiy
	find apps src include bindings \
		-name '.git' -prune -o \
		-name 'build' -prune -o \
		\( -name '*.c' -o -name '*.h' \) -print0 \
		| xargs -0 -P $(shell nproc) -n 20 ~/.venv/bin/clang-format -i

tidy:
	@if [ ! -f compile_commands.json ]; then \
		echo "Error: compile_commands.json not found. Running build first..."; \
		$(MAKE) build target=$(target); \
	fi
	/opt/homebrew/opt/llvm/bin/clang-tidy -p . $(shell find lib -name '*.c' -o -name '*.h')
