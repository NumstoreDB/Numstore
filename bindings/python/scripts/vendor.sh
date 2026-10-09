#!/usr/bin/env bash
#
# Copy the numstore C library into bindings/python/_numstore so an sdist is
# self-contained. A git checkout builds against ../../numstore; an sdist has
# no parent directory, so CMakeLists.txt looks for _numstore/ first.
#
# Run from anywhere (the Makefile's vendor and sdist targets do):
#
#   scripts/vendor.sh
#
# _numstore/ is gitignored and removed by scripts/clean.sh.

set -eu
cd "$(dirname "$0")/.."

src=../../numstore
dst=_numstore

if [ ! -f "$src/CMakeLists.txt" ]; then
  echo "error: $src/CMakeLists.txt not found -- run from a git checkout" >&2
  exit 1
fi

echo "== vendoring $src -> $dst"
rm -rf "$dst"
mkdir -p "$dst"

# Only what CMakeLists.txt needs; skip any build trees a developer left inside
( cd "$src" && find . -type d -name build -prune -o -type f -print ) \
  | while read -r f; do
      mkdir -p "$dst/$(dirname "$f")"
      cp "$src/$f" "$dst/$f"
    done

echo "   $(find "$dst" -type f | wc -l | tr -d ' ') files"
