#!/usr/bin/env bash
#
# Format every source file in the tree.

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
  find numstore bindings -type d \( -name build -o -name dist \) -prune -o \
       -type f \( -name '*.c' -o -name '*.h' \) -print
)" clang-format --style=file:.clang-format -i

step "formatting cmake" "$(
  find . -type d \( -name build -o -name dist -o -name target \) -prune -o \
       -type f \( -name CMakeLists.txt -o -name '*.cmake' \) -print
)" gersemi -i

step "formatting python" "$(
  find bindings/python numstore scripts -type d \( -name build -o -name dist \) -prune -o \
       -type f -name '*.py' -print
)" ruff format

echo "== formatting rust (nsserver via cargo fmt)"
cd nsserver && cargo fmt
