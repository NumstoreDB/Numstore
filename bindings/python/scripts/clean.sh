#!/usr/bin/env bash
#
# Remove the python bindings' build artifacts.

set -eu
cd "$(dirname "$0")/.."

echo "== removing build dirs"
rm -rf dist build wheelhouse sources.txt

# Vendored by scripts/vendor.sh; reproducible in a second
echo "== removing vendored numstore"
rm -rf _numstore

# In-place builds (pip install -e, a manual setup.py build_ext) drop the
# extension module next to the python sources, where wheel.packages copies it
# into every wheel built afterwards -- including wheels for other interpreters,
# which then fail delocate/auditwheel.
echo "== removing in-place extension modules"
find src -type f \( -name '*.so' -o -name '*.dylib' -o -name '*.pyd' \) -print -delete

echo "== removing caches"
find . -type d \( -name __pycache__ -o -name '*.egg-info' -o -name .pytest_cache \
                  -o -name .ruff_cache \) -prune -exec rm -rf {} +
