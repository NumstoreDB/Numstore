#!/usr/bin/env bash
#
# Copy the numstore C library into _numstore so an sdist is self-contained.
# A git checkout builds against ../../numstore; an sdist has no parent
# directory, so CMakeLists.txt looks for _numstore first.

set -eu
cd "$(dirname "$0")/.."

rm -rf _numstore
cp -R ../../numstore _numstore
find _numstore -type d -name build -prune -exec rm -rf {} +
