#!/usr/bin/env bash
# Build and run the unit tests. Usage: scripts/test.sh [category-or-name]
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/env.sh"
"$ANTIMATR_ROOT/scripts/build.sh" AntiMatrTests
"$ANTIMATR_BUILD_DIR/AntiMatrTests_artefacts/$ANTIMATR_BUILD_TYPE/AntiMatrTests" "$@"
