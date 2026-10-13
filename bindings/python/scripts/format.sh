#!/usr/bin/env bash
#
# Format the python bindings.

set -eu
cd "$(dirname "$0")/.."

find . -type d \( \
  -name build \
  -o -name dist \
  -o -name wheelhouse \
  -o -name _numstore \) \
  -prune \
  -o -type f \
  -name '*.py' \
  -exec ruff format {} +
