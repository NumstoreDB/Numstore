#!/bin/sh
# Builds the libnumstore test double used by tests/Numstore.Tests.
set -e
cd "$(dirname "$0")"
${CC:-cc} -O2 -Wall -shared -fPIC numstore_stub.c -o libnumstore.so
