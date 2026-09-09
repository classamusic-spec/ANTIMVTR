#!/usr/bin/env bash
# Runs pluginval against the VST3 (Linux). Usage: scripts/validate.sh [strictness 1-10]
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/env.sh"
STRICT="${1:-5}"
"$ANTIMATR_ROOT/scripts/build.sh" AntiMatr_VST3 >/dev/null
VST3="$ANTIMATR_BUILD_DIR/AntiMatr_artefacts/$ANTIMATR_BUILD_TYPE/VST3/ANTI-MATR.vst3"
PLUGINVAL="${PLUGINVAL:-/home/user/deps/pluginval}"
if [ -n "${DISPLAY:-}" ]; then
  "$PLUGINVAL" --strictness-level "$STRICT" --validate-in-process --timeout-ms 600000 --validate "$VST3"
else
  xvfb-run -a "$PLUGINVAL" --strictness-level "$STRICT" --validate-in-process --timeout-ms 600000 --validate "$VST3"
fi
