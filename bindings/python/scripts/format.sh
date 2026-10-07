#!/usr/bin/env bash
#
# Format the python bindings.

set -eu
cd "$(dirname "$0")/.."

files=$(
  find . -type d \( -name build -o -name dist -o -name wheelhouse \
                    -o -name _numstore \) -prune -o \
       -type f -name '*.py' -print
)

if [ -z "$files" ]; then
  echo "== formatting python: nothing to do"
  exit 0
fi

echo "== formatting python ($(printf '%s\n' "$files" | wc -l | tr -d ' ') files)"
printf '%s\n' "$files" | tr '\n' ' ' | fold -s -w 96 \
  | awk '{ sub(/^ +/, ""); sub(/ +$/, ""); print "   " $0 }'
printf '%s\n' "$files" | tr '\n' '\0' | xargs -0 ruff format
