#!/usr/bin/env bash
#
# src/scripts/coverage.sh
#
# Runs the full test suite with coverage instrumentation for both the
# Rust code (LLVM source-based, via cargo-llvm-cov) and the C code
# (gcov, via the --coverage flag build.rs adds when NS_COVERAGE=1),
# then merges everything into a single lcov file: coverage.info
#
# Requires:
#   - lcov / genhtml on PATH        (apt-get install lcov)
#   - cargo-llvm-cov                (cargo install cargo-llvm-cov --locked)
#   - llvm-tools rustup component   (rustup component add llvm-tools-preview)
#
# Usage:
#   src/scripts/coverage.sh              # produce coverage.info
#   GEN_HTML=1 src/scripts/coverage.sh   # also produce target/coverage_html
#
# Outputs:
#   coverage.info        merged C + Rust lcov file (upload this to codecov)
#   target/coverage_html HTML report (only if GEN_HTML=1)

set -euo pipefail

# Repo root, regardless of where this script is invoked from.
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT_DIR"

FEATURES="unit_tests,irwr_tests,cgd_tests"
COVERAGE_INFO="coverage.info"

# Tells build.rs to compile the C sources with --coverage (gcov).
# Rust-side instrumentation is handled by cargo-llvm-cov itself --
# do NOT set RUSTFLAGS / LLVM_PROFILE_FILE manually here.
export NS_COVERAGE=1

echo "==> Cleaning previous coverage data"
cargo llvm-cov clean --workspace
rm -f "$COVERAGE_INFO" rust.info c.info

echo "==> Running all tests (instrumented)"
cargo llvm-cov --no-report test --features "$FEATURES"

echo "==> Extracting Rust coverage (lcov)"
cargo llvm-cov report --lcov --output-path rust.info

echo "==> Extracting C coverage (lcov)"
# The C objects (and their .gcno/.gcda) live under cargo-llvm-cov's
# target dir (target/llvm-cov-target/**/build/<crate>-*/out), which is
# still under target/, so one capture over target/ finds them all.
lcov --capture --directory target \
    --output-file c.info \
    --ignore-errors mismatch

# Keep only our own sources in the C report (drops /usr/include etc.,
# and any generated code under target/ like the unit_tests.c template).
# NOTE: use a relative "*/src/*" pattern, NOT "$ROOT_DIR/src/*" -- on
# case-insensitive filesystems (macOS) pwd's casing can differ from the
# paths the compiler recorded in .gcno files, and lcov matches patterns
# case-sensitively, which would silently exclude everything.
lcov --extract c.info "*/src/*" \
    --output-file c.info \
    --ignore-errors unused
# The generated unit_tests.c lives under target/.../out/, so it is
# already dropped by the extract above.

echo "==> Merging C + Rust coverage"
lcov -a rust.info -a c.info -o "$COVERAGE_INFO" --ignore-errors mismatch

if [[ "${GEN_HTML:-0}" == "1" ]]; then
    echo "==> Generating HTML report"
    genhtml "$COVERAGE_INFO" -o target/coverage_html
    echo "HTML report: target/coverage_html/index.html"
fi

echo
echo "Done. lcov file: $COVERAGE_INFO"
