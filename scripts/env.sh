#!/usr/bin/env bash
# Shared environment for ANTI-MATR build scripts.
# Sourced by the other scripts; safe to source in your shell too.
export ANTIMATR_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
export ANTIMATR_BUILD_DIR="${ANTIMATR_BUILD_DIR:-$ANTIMATR_ROOT/build}"
export ANTIMATR_BUILD_TYPE="${ANTIMATR_BUILD_TYPE:-Release}"
# A shared JUCE checkout avoids re-downloading per worktree; falls back to FetchContent.
export ANTIMATR_JUCE_PATH="${ANTIMATR_JUCE_PATH:-/home/user/deps/JUCE}"
export CCACHE_CONFIGPATH="${CCACHE_CONFIGPATH:-/home/user/ccache.conf}"
export ANTIMATR_JOBS="${ANTIMATR_JOBS:-$(nproc)}"
export ANTIMATR_PY="${ANTIMATR_PY:-/home/user/pyenv/bin/python}"
