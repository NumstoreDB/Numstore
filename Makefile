
all: build/debug
	cmake --build --preset debug

build/debug:
	cmake --preset debug 

format:
	python3 src/scripts/format_code.py
	python3 src/scripts/add_copywrite.py

clean:
	rm -rf build
