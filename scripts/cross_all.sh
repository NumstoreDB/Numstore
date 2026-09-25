#!/usr/bin/env bash
#
# run_dockcross_all.sh
#
# Runs a given command against a set of dockcross platform images.
# Resilient: if a platform's command fails, it logs the failure and
# moves on to the next platform instead of aborting the whole run.

set -uo pipefail
# NOTE: intentionally NOT using 'set -e' — we want failures to be
# caught and logged, not to kill the script.

# ---------------------------------------------------------------------------
# CONFIG — edit this section for your use case
# ---------------------------------------------------------------------------

# 20 platforms chosen to MAXIMIZE OS/architecture/ISA diversity.
# Covers Linux (8 distinct ISAs across 10 bitness/variant combos),
# Windows (x64, x86, and ARM), Android (both arches x both bitnesses),
# and WebAssembly as a non-native bonus target.
PLATFORMS=(
  "linux-x64"           # Linux 64-bit x86
  "linux-x86"           # Linux 32-bit x86
  "linux-arm64"         # Linux 64-bit ARM
  "linux-armv7"         # Linux 32-bit ARM (modern baseline)
  "linux-armv5"         # Linux 32-bit ARM (older baseline)
  "linux-mips"          # Linux MIPS, big-endian
  "linux-mipsel-lts"    # Linux MIPS, little-endian (distinct from above)
  "linux-ppc64le"       # Linux PowerPC (distinct ISA)
  "linux-s390x"         # Linux IBM Z (distinct ISA)
  "linux-riscv64"       # Linux RISC-V 64-bit
  "linux-riscv32"       # Linux RISC-V 32-bit (distinct bitness)
  "linux-m68k-uclibc"   # Linux Motorola 68k (legacy ISA)
  "windows-static-x64"  # Windows 64-bit
  "windows-static-x86"  # Windows 32-bit
  "windows-armv7"       # Windows on ARM
  "android-arm64"       # Android 64-bit ARM
  "android-arm"         # Android 32-bit ARM
  "android-x86_64"      # Android 64-bit x86
  "android-x86"         # Android 32-bit x86
  "web-wasm32"          # WebAssembly (non-native, browser target)
)

# The command to run for each platform. Use ${PLATFORM} as a placeholder
# for the current platform name.
COMMAND_TEMPLATE='make cross PLATFORM=${PLATFORM} CROSS_GOAL="release-package -j10"'

# Where to put logs
OUTPUT_DIR="./dockcross_run_$(date +%Y%m%d_%H%M%S)"
SUCCESS_DIR="${OUTPUT_DIR}/success"
FAILURE_DIR="${OUTPUT_DIR}/failure"
SUMMARY_FILE="${OUTPUT_DIR}/summary.log"

# ---------------------------------------------------------------------------
# SCRIPT LOGIC — shouldn't need to edit below here
# ---------------------------------------------------------------------------

mkdir -p "${SUCCESS_DIR}" "${FAILURE_DIR}"

success_count=0
fail_count=0
failed_platforms=()

echo "Starting dockcross run across ${#PLATFORMS[@]} platforms" | tee "${SUMMARY_FILE}"
echo "Logs directory: ${OUTPUT_DIR}" | tee -a "${SUMMARY_FILE}"
echo "----------------------------------------" | tee -a "${SUMMARY_FILE}"

for PLATFORM in "${PLATFORMS[@]}"; do
  # Write to a temp file first, then move it into success/ or failure/
  # once we know the outcome.
  TMP_LOG="${OUTPUT_DIR}/.tmp_${PLATFORM}.log"

  # Substitute ${PLATFORM} into the command template
  CMD="${COMMAND_TEMPLATE//\$\{PLATFORM\}/${PLATFORM}}"

  echo ""
  echo ">>> [${PLATFORM}] Running: ${CMD}"
  echo ">>> [${PLATFORM}] Running: ${CMD}" >> "${SUMMARY_FILE}"

  # Run the command, streaming stdout+stderr to the terminal live
  # while also capturing it to the temp log via tee.
  if eval "${CMD}" 2>&1 | tee "${TMP_LOG}"; then
    FINAL_LOG="${SUCCESS_DIR}/${PLATFORM}.txt"
    mv "${TMP_LOG}" "${FINAL_LOG}"
    echo "    [${PLATFORM}] SUCCESS — ${FINAL_LOG}"
    echo "    [${PLATFORM}] SUCCESS — ${FINAL_LOG}" >> "${SUMMARY_FILE}"
    success_count=$((success_count + 1))
  else
    exit_code=$?
    FINAL_LOG="${FAILURE_DIR}/${PLATFORM}.txt"
    mv "${TMP_LOG}" "${FINAL_LOG}"
    echo "    [${PLATFORM}] FAILED (exit code ${exit_code}) — ${FINAL_LOG}"
    echo "    [${PLATFORM}] FAILED (exit code ${exit_code}) — ${FINAL_LOG}" >> "${SUMMARY_FILE}"
    fail_count=$((fail_count + 1))
    failed_platforms+=("${PLATFORM}")
  fi
done

echo ""
echo "========================================" | tee -a "${SUMMARY_FILE}"
echo "Run complete: ${success_count} succeeded, ${fail_count} failed" | tee -a "${SUMMARY_FILE}"

if [ "${fail_count}" -gt 0 ]; then
  echo "Failed platforms:" | tee -a "${SUMMARY_FILE}"
  for p in "${failed_platforms[@]}"; do
    echo "  - ${p}" | tee -a "${SUMMARY_FILE}"
  done
fi

echo ""
echo "Success logs: ${SUCCESS_DIR}"
echo "Failure logs: ${FAILURE_DIR}"

# Exit 0 regardless of per-platform failures — the script itself succeeded
# at doing its job (attempting all platforms). Change this if you want
# the script to exit non-zero when any platform fails.
exit 0
