#!/usr/bin/env bash
# Offline render + analysis. Usage: scripts/render.sh --out renders/x.wav [AntiMatrRender options]
# Then: scripts/analyze.py renders/x.wav  (spectrogram PNG + metrics)
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/env.sh"
"$ANTIMATR_ROOT/scripts/build.sh" AntiMatrRender >/dev/null
mkdir -p "$ANTIMATR_ROOT/renders"
"$ANTIMATR_BUILD_DIR/AntiMatrRender_artefacts/$ANTIMATR_BUILD_TYPE/AntiMatrRender" "$@"
