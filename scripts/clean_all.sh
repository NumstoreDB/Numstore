#!/usr/bin/env bash
#
# Remove build artifacts. The top level dist/ is left alone: it holds release
# tarballs and wheels that take a long time to reproduce.

set -eu
cd "$(dirname "$0")/.."

rm -rf build build_coverage docs/build docs/api

find . -type d \( -name __pycache__ -o -name '*.egg-info' -o -name .pytest_cache \
                  -o -name .ruff_cache \) -prune -exec rm -rf {} +

find . -type f \( -name '*.o' -o -name '*.d' -o -name '*.gcda' -o -name '*.gcno' \
                  -o -name '*.gcov' \) -delete

find . -type f \( -name '*.db' -o -name '*.nsdb' -o -name '*.wal' -o -name 'testdb*' \
                  -o -name 'vgcore.*' -o -name gmon.out \) -delete
