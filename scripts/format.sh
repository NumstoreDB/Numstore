#!/usr/bin/env bash
#
# Format the source files the repo itself owns: C/H across numstore and the
# bindings (one shared .clang-format), the cmake files, and the repo's own
# python. Packages with their own toolchain format themselves -- see the
# format target in the root Makefile.

set -eu
cd "$(dirname "$0")/.."

# Echo the file list for a step as a few wrapped lines, then run the formatter
# once over all of them -- invoking the tool (or echoing) per file is much slower.
step() {
  local what=$1 files=$2
  shift 2
  if [ -z "$files" ]; then
    echo "== $what: nothing to do"
    return
  fi
  echo "== $what ($(printf '%s\n' "$files" | wc -l | tr -d ' ') files)"
  printf '%s\n' "$files" | tr '\n' ' ' | fold -s -w 96 \
    | awk '{ sub(/^ +/, ""); sub(/ +$/, ""); print "   " $0 }'
  printf '%s\n' "$files" | tr '\n' '\0' | xargs -0 "$@"
}

step "formatting c/h" "$(
  find numstore bindings -type d \( -name build -o -name dist -o -name _numstore \) -prune -o \
       -type f \( -name '*.c' -o -name '*.h' \) -print
)" clang-format --style=file:.clang-format -i

step "formatting cmake" "$(
  find . -type d \( -name build -o -name dist -o -name target \) -prune -o \
       -type f \( -name CMakeLists.txt -o -name '*.cmake' \) -print
)" gersemi -i

# bindings/python has its own ruff step, and the two crates their own cargo fmt
# -- the root Makefile's format target calls into those. What is left here is
# the python that belongs to the repo itself.
step "formatting python" "$(
  find numstore scripts -type d \( -name build -o -name dist \) -prune -o \
       -type f -name '*.py' -print
)" ruff format
