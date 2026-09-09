#!/usr/bin/env bash
# Render the editor to PNG under a virtual X server.
# Usage: scripts/snapshot.sh --out renders/ui.png [--width 1600 --height 1000] [--page 0] [--wait 800]
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/env.sh"
"$ANTIMATR_ROOT/scripts/build.sh" AntiMatrSnapshot >/dev/null
mkdir -p "$ANTIMATR_ROOT/renders"
BIN="$ANTIMATR_BUILD_DIR/AntiMatrSnapshot_artefacts/$ANTIMATR_BUILD_TYPE/AntiMatrSnapshot"
if [ -n "${DISPLAY:-}" ]; then
  "$BIN" "$@"
else
  xvfb-run -a -s "-screen 0 2200x1400x24" "$BIN" "$@"
fi
