#!/usr/bin/env bash
#
# build_all_libs.sh
#
# Builds the C library release artifact for the native host plus every
# cross platform in the Makefile's PACKAGE_PLATFORMS, and collects the
# resulting tarballs/zips into one directory.
#
# Unlike `make package-release-all-platforms` (which stops at the first
# failure) this attempts every platform and prints a matrix at the end, so one
# broken target does not hide the state of the rest.
#
# Usage:
#   scripts/build_all_libs.sh [options] [platform ...]
#
#   -t TARGET    debug | release            (default: release)
#   -g GOAL      make goal run per platform (default: release-tarball)
#   -o DIR       collect artifacts here     (default: dist/libs)
#   -j N         parallelism passed to make (default: number of host CPUs)
#   -n           skip the native host build
#   -h           this help
#
# With no platform arguments the list comes from `make print-package-platforms`.
#
# Examples:
#   scripts/build_all_libs.sh
#   scripts/build_all_libs.sh -t debug -g all
#   scripts/build_all_libs.sh linux-arm64 windows-static-x64
#
# Exit status: non-zero if any *attempted* platform failed. Platforms skipped
# because this host cannot build them do not count as failures.

set -uo pipefail
# Intentionally no 'set -e': per-platform failures are collected, not fatal.

cd "$(git rev-parse --show-toplevel)" || exit 1

TARGET="release"
GOAL="release-tarball"
OUT_DIR="dist/libs"
JOBS="$( (command -v nproc >/dev/null && nproc) || sysctl -n hw.ncpu 2>/dev/null || echo 4 )"
DO_NATIVE=1

while getopts ":t:g:o:j:nh" opt; do
  case "$opt" in
    t) TARGET="$OPTARG" ;;
    g) GOAL="$OPTARG" ;;
    o) OUT_DIR="$OPTARG" ;;
    j) JOBS="$OPTARG" ;;
    n) DO_NATIVE=0 ;;
    h) sed -n '2,32p' "$0"; exit 0 ;;
    \?) echo "unknown option: -$OPTARG (try -h)" >&2; exit 2 ;;
    :)  echo "-$OPTARG needs a value" >&2; exit 2 ;;
  esac
done
shift $((OPTIND - 1))

PLATFORMS=("$@")
if [ ${#PLATFORMS[@]} -eq 0 ]; then
  # shellcheck disable=SC2207
  PLATFORMS=($(make --no-print-directory print-package-platforms)) || {
    echo "error: could not read PACKAGE_PLATFORMS from the Makefile" >&2
    exit 1
  }
fi

HOST_ARCH="$(uname -m)"
STAMP="$(date +%Y%m%d_%H%M%S)"
LOG_DIR="build/logs/libs_${STAMP}"
mkdir -p "$LOG_DIR" "$OUT_DIR"

if ! command -v docker >/dev/null 2>&1; then
  echo "error: docker is required for the cross builds (install it, or pass -n and a platform list)" >&2
  exit 1
fi
if ! docker info >/dev/null 2>&1; then
  echo "error: docker is installed but not running" >&2
  exit 1
fi
if ! command -v pandoc >/dev/null 2>&1; then
  echo "note: pandoc not found on the host - native packages ship markdown docs only."
fi
echo "note: cross containers have no pandoc, so cross packages always ship markdown docs only."
echo

# A 32-bit x86 dockcross image runs linux32 in its entrypoint to set a
# PER_LINUX32 personality. That syscall is not emulated on arm64 hosts, so the
# image cannot even emit its launcher there. Detect it up front and report a
# skip rather than a failure.
host_cannot_build() {
  case "$HOST_ARCH" in
    arm64|aarch64)
      case "$1" in
        linux-x86|manylinux2014-x86|linux-i686)
          echo "32-bit x86 target needs an x86_64 host (linux32 personality is not emulated on $HOST_ARCH)"
          return 0 ;;
      esac ;;
  esac
  return 1
}

NAMES=() STATES=() NOTES=()
record() { NAMES+=("$1"); STATES+=("$2"); NOTES+=("$3"); }

# ARTIFACT_NAME is built from RELEASE_OS/RELEASE_ARCH only, so platforms that
# differ by toolchain rather than by os/arch collide: linux-x64 and
# manylinux2014-x64 both emit numstore-<ver>-linux-x86_64.tar.gz despite
# targeting different glibc baselines. Qualify the second one with its platform
# instead of letting it overwrite the first.
collect() {
  # $1 = build output dir to harvest from, $2 = platform label for collisions
  local dir="$1" label="$2" found="" base dest
  shopt -s nullglob
  for f in "$dir"/*.tar.gz "$dir"/*.zip; do
    base="$(basename "$f")"
    dest="$OUT_DIR/$base"
    if [ -e "$dest" ] && ! cmp -s "$f" "$dest"; then
      dest="$OUT_DIR/${label}--${base}"
      found="${found:+$found, }${label}--${base} (name clash, qualified)"
    else
      found="${found:+$found, }$base"
    fi
    cp -f "$f" "$dest"
  done
  shopt -u nullglob
  printf '%s' "$found"
}

# ---------------------------------------------------------------- native host
if [ "$DO_NATIVE" -eq 1 ]; then
  log="$LOG_DIR/native.log"
  echo ">>> native ($(uname -s)/$HOST_ARCH): make TARGET=$TARGET $GOAL"
  if make -j"$JOBS" TARGET="$TARGET" "$GOAL" >"$log" 2>&1; then
    art="$(collect "build/$TARGET" "native")"
    record "native ($HOST_ARCH)" OK "${art:-<no archive; goal was '$GOAL'>}"
  else
    record "native ($HOST_ARCH)" FAIL "see $log"
  fi
fi

# ----------------------------------------------------------------- cross
for p in "${PLATFORMS[@]}"; do
  if reason="$(host_cannot_build "$p")"; then
    echo ">>> $p: SKIP - $reason"
    record "$p" SKIP "$reason"
    continue
  fi

  launcher="docker/dockcross-$p"
  # A zero-byte launcher would run as an empty script and "succeed" without
  # building anything, so drop it and let make fetch it again.
  if [ -e "$launcher" ] && [ ! -s "$launcher" ]; then
    echo ">>> $p: removing empty $launcher so it gets regenerated"
    rm -f "$launcher"
  fi

  log="$LOG_DIR/$p.log"
  echo ">>> $p: make cross CROSS_GOAL=\"TARGET=$TARGET $GOAL\""
  if make cross PLATFORM="$p" CROSS_GOAL="TARGET=$TARGET $GOAL -j$JOBS" >"$log" 2>&1; then
    art="$(collect "build/$TARGET-$p" "$p")"
    if [ -n "$art" ]; then
      record "$p" OK "$art"
    else
      # make succeeded but produced nothing - the empty-launcher trap, or a
      # goal that does not package.
      record "$p" WARN "built, but no archive found (goal was '$GOAL')"
    fi
  else
    record "$p" FAIL "see $log"
  fi
done

# ----------------------------------------------------------------- summary
echo
echo "========================================================================"
printf '%-24s %-6s %s\n' "PLATFORM" "STATE" "ARTIFACT / NOTE"
echo "------------------------------------------------------------------------"
fails=0
for i in "${!NAMES[@]}"; do
  printf '%-24s %-6s %s\n' "${NAMES[$i]}" "${STATES[$i]}" "${NOTES[$i]}"
  case "${STATES[$i]}" in FAIL|WARN) fails=$((fails + 1)) ;; esac
done
echo "========================================================================"
echo "artifacts: $OUT_DIR"
echo "logs:      $LOG_DIR"

[ "$fails" -eq 0 ] || echo "$fails platform(s) need attention."
exit $(( fails > 0 ? 1 : 0 ))
