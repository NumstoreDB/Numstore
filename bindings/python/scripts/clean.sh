#!/usr/bin/env bash
#
# Remove the python bindings' build artifacts.

set -eu
cd "$(dirname "$0")/.."

rm -rf dist
rm -rf build
rm -rf wheelhouse
rm -rf sources.txt
rm -rf _numstore

find src -type f \( \
  -name '*.so' \
  -o -name '*.dylib' \
  -o -name '*.pyd' \) \
  -delete

find . -type d \( \
  -name __pycache__ \
  -o -name '*.egg-info' \
  -o -name .pytest_cache \
  -o -name .ruff_cache \) \
  -prune \
  -exec rm -rf {} +
