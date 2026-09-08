#!/usr/bin/env bash
set -euo pipefail

# TruthRaw v0.4 final Adobe DNG SDK validation gate.
# Usage:
#   ./RUN_ADOBE_DNG_VALIDATE_V0_4.sh /path/to/dng_validate /path/to/v0_4_directory
#
# The script is fail-closed: it first verifies the exact candidate SHA-256 values,
# then executes dng_validate on each DNG and saves the logs.

VALIDATOR="${1:?First argument must be path to dng_validate}"
DIR="${2:?Second argument must be directory containing the two v0.4 DNG files}"

A="IMG_BNC_TRUTHRAW20260907_094414_423__TRUTHRAW_DERIVED_LINEAR_RAW_v0_4.dng"
B="IMG_BNC_TRUTHRAW20260907_094449_565__TRUTHRAW_DERIVED_LINEAR_RAW_v0_4.dng"

EXPECTED_A="13e32e0967dd9726f31dfbec33a1461c4f4c2f98e4f67789eed1cc5f7766b9fc"
EXPECTED_B="347cd68ade21f99607028b23ed7cdc0c6b498885d236e9c6443624228747766f"

check_sha() {
  local file="$1" expected="$2"
  local got
  got="$(sha256sum "$file" | awk '{print $1}')"
  if [[ "$got" != "$expected" ]]; then
    echo "FAIL SHA-256: $file"
    echo "expected=$expected"
    echo "got=$got"
    exit 2
  fi
  echo "SHA PASS: $file"
}

check_sha "$DIR/$A" "$EXPECTED_A"
check_sha "$DIR/$B" "$EXPECTED_B"

mkdir -p "$DIR/dng_validate_logs"

run_one() {
  local file="$1"
  local log="$DIR/dng_validate_logs/${file%.dng}.dng_validate.log"
  echo "Running dng_validate: $file"
  set +e
  "$VALIDATOR" "$DIR/$file" >"$log" 2>&1
  rc=$?
  set -e
  cat "$log"
  if [[ $rc -ne 0 ]]; then
    echo "DNG_VALIDATE_FAIL rc=$rc: $file"
    exit "$rc"
  fi
  echo "DNG_VALIDATE_PROCESS_EXIT_PASS: $file"
}

run_one "$A"
run_one "$B"

echo "Both exact TruthRaw v0.4 candidates completed dng_validate with exit code 0."
echo "Review logs for SDK-reported validation errors/warnings before promotion."
