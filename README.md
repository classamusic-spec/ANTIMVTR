# ANTI-MATR

**SOUND BEYOND MATTER** — a software synthesizer about creating an imaginary
substance and then destabilizing it.

```
ENERGY → MATTER → TRANSFORMATION → FRACTURE → SPACE
SOURCE   SHAPE    EVOLVE           FRACTURE   SPACE
```

Built in C++20 with JUCE. Formats: VST3, AU (macOS), Standalone. Everything —
DSP, UI, icons, visualizers, wavetables, materials, presets — is generated
from code. No bitmaps, no external assets.

## Build

Requirements: CMake ≥ 3.22, a C++20 compiler, Ninja (recommended). JUCE is
fetched automatically (pinned tag in `cmake/JUCE.cmake`) or taken from
`-DANTIMATR_JUCE_PATH=/path/to/JUCE`.

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build          # unit tests
```

Linux needs the usual JUCE packages (X11, freetype, fontconfig, ALSA, GL,
curl, jack — see `docs/AGENT_GUIDE.md`).

Artefacts land in `build/AntiMatr_artefacts/<Config>/{VST3,Standalone,AU}`.

## Repository map

```
src/plugin      AudioProcessor / editor glue (host-facing only)
src/core        real-time utilities (RNG, lock-free queues, smoothing, math)
src/state       ParameterList (permanent IDs), registry, state/JSON, mutation
src/dsp         SynthEngine, voices, source/, matter/, evolve/, fracture/, mod/, fx/
src/presets     code-generated factory content + user presets
src/ui          theme, look-and-feel, components, views, visualizers
src/dev         diagnostics (engine side) and DSP LAB (developer workspace)
tools           AntiMatrRender (offline MIDI→WAV+analysis), AntiMatrSnapshot (editor→PNG)
tests           JUCE UnitTest suites
docs            SPEC.md (roadmap), ARCHITECTURE.md, AGENT_GUIDE.md
scripts         build / test / render / snapshot / validate helpers
```

## Status

Phase 0 complete: build system, parameter contract, voice system (poly / mono
/ legato, glide, sustain, bend, pressure, stealing), sine WAVE source, ADSR,
master safety stage, JSON state with migration, factory preset skeleton,
mutation engine, diagnostics architecture, DSP LAB shell, Main page UI shell
with the procedural ANTI-MATTER object, offline render + snapshot tools, and
tests across 44.1–96 kHz and 32–1024 sample blocks. pluginval strictness 5
passes on the VST3.

See `docs/SPEC.md` §94 for the phase roadmap.
