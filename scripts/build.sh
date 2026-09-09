#!/usr/bin/env bash
# Configure (if needed) and build. Usage: scripts/build.sh [target ...]
# Targets default to everything. Examples:
#   scripts/build.sh                      # plugin + tests + tools
#   scripts/build.sh AntiMatrTests        # just the tests
#   ANTIMATR_BUILD_TYPE=Debug scripts/build.sh
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/env.sh"

JUCE_ARG=""
if [ -d "$ANTIMATR_JUCE_PATH" ]; then JUCE_ARG="-DANTIMATR_JUCE_PATH=$ANTIMATR_JUCE_PATH"; fi

if [ ! -f "$ANTIMATR_BUILD_DIR/build.ninja" ]; then
  cmake -S "$ANTIMATR_ROOT" -B "$ANTIMATR_BUILD_DIR" -G Ninja \
        -DCMAKE_BUILD_TYPE="$ANTIMATR_BUILD_TYPE" $JUCE_ARG "$@"
  set --
fi

TARGETS=("$@")
if [ ${#TARGETS[@]} -eq 0 ]; then
  cmake --build "$ANTIMATR_BUILD_DIR" -j "$ANTIMATR_JOBS"
else
  cmake --build "$ANTIMATR_BUILD_DIR" -j "$ANTIMATR_JOBS" --target "${TARGETS[@]}"
fi
