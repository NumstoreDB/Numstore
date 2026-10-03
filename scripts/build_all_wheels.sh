#!/usr/bin/env bash
#
# build_all_wheels.sh
#
# Builds every pynumstore wheel this host is capable of producing, via
# cibuildwheel, and collects them in one directory.
#
# What a given host can do:
#   Linux wheels   any host with Docker (manylinux + musllinux). Non-native
#                  architectures go through emulation, which is slow but works.
#   macOS wheels   macOS hosts only (arm64 natively, x86_64 cross-compiled).
#   Windows wheels Windows hosts only - they need MSVC, which cannot be
#                  emulated. Use CI (.github/workflows/build-wheels.yml, or
#                  `gh workflow run build-wheels.yml`) for those.
#
# Which CPython versions, which architectures are skipped, and the post-build
# smoke test all come from [tool.cibuildwheel] in bindings/python/pyproject.toml
# so that this script and CI stay in agreement.
#
# Usage:
#   scripts/build_all_wheels.sh [options]
#
#   -o DIR       collect wheels here           (default: dist/wheels)
#   -a "A B"     Linux architectures to build  (default: native + x86_64 if
#                                               emulation is available)
#   -b PATTERN   CIBW_BUILD selector, e.g. 'cp312-*'   (default: all)
#   -L           skip Linux wheels
#   -M           skip macOS wheels
#   -h           this help
#
# Examples:
#   scripts/build_all_wheels.sh
#   scripts/build_all_wheels.sh -b 'cp312-*'
#   scripts/build_all_wheels.sh -a aarch64 -M
#
# Exit status: non-zero if any attempted platform failed. Platforms skipped
# because this host cannot build them do not count as failures.

set -uo pipefail
# Intentionally no 'set -e': per-platform failures are collected, not fatal.

# cibuildwheel copies its invocation directory into the build environment, and
# setup.py compiles the C library from ../../src - so this must run from the
# repository root with bindings/python as the package.
cd "$(git rev-parse --show-toplevel)" || exit 1
PACKAGE="bindings/python"

OUT_DIR="dist/wheels"
LINUX_ARCHS=""
BUILD_SELECTOR=""
DO_LINUX=1
DO_MACOS=1

while getopts ":o:a:b:LMh" opt; do
  case "$opt" in
    o) OUT_DIR="$OPTARG" ;;
    a) LINUX_ARCHS="$OPTARG" ;;
    b) BUILD_SELECTOR="$OPTARG" ;;
    L) DO_LINUX=0 ;;
    M) DO_MACOS=0 ;;
    h) sed -n '2,40p' "$0"; exit 0 ;;
    \?) echo "unknown option: -$OPTARG (try -h)" >&2; exit 2 ;;
    :)  echo "-$OPTARG needs a value" >&2; exit 2 ;;
  esac
done

PYTHON="${PYTHON:-python3}"

# Probe by import rather than by running the CLI: cibuildwheel has no
# --version flag, so a CLI probe reports "not installed" even when it is.
if ! "$PYTHON" -c "import cibuildwheel" >/dev/null 2>&1; then
  echo "error: cibuildwheel is not installed for $PYTHON" >&2
  echo "       $PYTHON -m pip install cibuildwheel" >&2
  exit 1
fi

HOST_OS="$(uname -s)"
HOST_ARCH="$(uname -m)"
STAMP="$(date +%Y%m%d_%H%M%S)"
LOG_DIR="build/logs/wheels_${STAMP}"
mkdir -p "$LOG_DIR" "$OUT_DIR"

NAMES=() STATES=() NOTES=()
record() { NAMES+=("$1"); STATES+=("$2"); NOTES+=("$3"); }

# Count wheels before/after so each platform reports its own contribution.
wheel_count() { ls -1 "$OUT_DIR"/*.whl 2>/dev/null | wc -l | tr -d ' '; }

run_cibw() {
  # $1 = label, $2 = cibuildwheel --platform value, rest = extra env assignments
  local label="$1" platform="$2"; shift 2
  local log="$LOG_DIR/$label.log"
  local before after

  before="$(wheel_count)"
  echo ">>> $label: cibuildwheel --platform $platform"

  if env "$@" "$PYTHON" -m cibuildwheel \
        --platform "$platform" --output-dir "$OUT_DIR" "$PACKAGE" \
        >"$log" 2>&1; then
    after="$(wheel_count)"
    record "$label" OK "$(( after - before )) wheel(s)"
  else
    record "$label" FAIL "see $log"
  fi
}

# ------------------------------------------------------------------- Linux
if [ "$DO_LINUX" -eq 1 ]; then
  if ! command -v docker >/dev/null 2>&1 || ! docker info >/dev/null 2>&1; then
    record "linux" SKIP "Docker not available"
  else
    if [ -z "$LINUX_ARCHS" ]; then
      LINUX_ARCHS="auto"
      # "auto" is the host architecture only. Add x86_64 when the host is not
      # already x86_64 and the daemon can actually run amd64 images, so a
      # single run still covers the most common deployment target.
      case "$HOST_ARCH" in
        x86_64|amd64) ;;
        *)
          echo "    probing amd64 emulation..."
          if docker run --rm --platform linux/amd64 alpine true >/dev/null 2>&1; then
            LINUX_ARCHS="auto x86_64"
            echo "    amd64 emulation works - building x86_64 too (slow)"
          else
            echo "    no amd64 emulation - native architecture only"
          fi ;;
      esac
    fi
    envs=("CIBW_ARCHS_LINUX=$LINUX_ARCHS")
    [ -n "$BUILD_SELECTOR" ] && envs+=("CIBW_BUILD=$BUILD_SELECTOR")
    run_cibw "linux" linux "${envs[@]}"
  fi
fi

# ------------------------------------------------------------------- macOS
if [ "$DO_MACOS" -eq 1 ]; then
  if [ "$HOST_OS" != "Darwin" ]; then
    record "macos" SKIP "needs a macOS host (this is $HOST_OS)"
  else
    envs=()
    [ -n "$BUILD_SELECTOR" ] && envs+=("CIBW_BUILD=$BUILD_SELECTOR")
    # archs come from [tool.cibuildwheel.macos]; cibuildwheel skips the smoke
    # test on the cross-compiled slice, which it cannot execute here.
    run_cibw "macos" macos "${envs[@]:-PATH=$PATH}"
  fi
fi

# ----------------------------------------------------------------- Windows
case "$HOST_OS" in
  MINGW*|MSYS*|CYGWIN*)
    envs=()
    [ -n "$BUILD_SELECTOR" ] && envs+=("CIBW_BUILD=$BUILD_SELECTOR")
    run_cibw "windows" windows "${envs[@]:-PATH=$PATH}" ;;
  *)
    record "windows" SKIP "needs a Windows host with MSVC - use CI (gh workflow run build-wheels.yml)" ;;
esac

# ----------------------------------------------------------------- summary
echo
echo "========================================================================"
printf '%-10s %-6s %s\n' "PLATFORM" "STATE" "RESULT / NOTE"
echo "------------------------------------------------------------------------"
fails=0
for i in "${!NAMES[@]}"; do
  printf '%-10s %-6s %s\n' "${NAMES[$i]}" "${STATES[$i]}" "${NOTES[$i]}"
  [ "${STATES[$i]}" = "FAIL" ] && fails=$((fails + 1))
done
echo "------------------------------------------------------------------------"
echo "total wheels in $OUT_DIR: $(wheel_count)"
echo "logs: $LOG_DIR"
echo "========================================================================"

if [ "$(wheel_count)" -gt 0 ]; then
  echo
  echo "Upload with:"
  echo "  $PYTHON -m twine upload $OUT_DIR/*.whl"
fi

exit $(( fails > 0 ? 1 : 0 ))
