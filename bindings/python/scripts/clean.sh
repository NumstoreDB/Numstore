#!/usr/bin/env bash
#
# Remove the python bindings' build artifacts.

set -eu
cd "$(dirname "$0")/.."

rm -rf dist build wheelhouse sources.txt _numstore

# In-place builds drop the extension module next to the python sources, where
# wheel.packages then copies it into every wheel built afterwards.
find src -type f \( -name '*.so' -o -name '*.dylib' -o -name '*.pyd' \) -delete

find . -type d \( -name __pycache__ -o -name '*.egg-info' -o -name .pytest_cache \
                  -o -name .ruff_cache \) -prune -exec rm -rf {} +
