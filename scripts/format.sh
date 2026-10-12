#!/usr/bin/env bash
#
# Format the source the repo itself owns. bindings/python formats itself --
# see the format target in the root Makefile.

set -eu
cd "$(dirname "$0")/.."

find numstore bindings -type d \( -name build -o -name dist -o -name _numstore \) -prune -o \
     -type f \( -name '*.c' -o -name '*.h' \) -exec clang-format -i {} +

find . -type d \( -name build -o -name dist -o -name target \) -prune -o \
     -type f \( -name CMakeLists.txt -o -name '*.cmake' \) -exec gersemi -i {} +

find numstore scripts -type d \( -name build -o -name dist \) -prune -o \
     -type f -name '*.py' -exec ruff format {} +
