#!/usr/bin/env bash
#
# Remove the nsserver crate's build artifacts.

set -eu
cd "$(dirname "$0")/.."

echo "== removing target/ (cargo clean)"
cargo clean
