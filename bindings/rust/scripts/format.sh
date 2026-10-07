#!/usr/bin/env bash
#
# Format the rust bindings crate.

set -eu
cd "$(dirname "$0")/.."

echo "== formatting rust (cargo fmt)"
cargo fmt
