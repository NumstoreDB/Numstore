#!/usr/bin/env bash
#
# Remove the node bindings' build artifacts.
#
# node_modules is left alone: it is a dependency cache, not a build artifact,
# and `npm install` takes a while to reproduce.

set -eu
cd "$(dirname "$0")/.."

echo "== removing build/ (node-gyp)"
rm -rf build
