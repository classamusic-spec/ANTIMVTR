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

Integrated so far (all on the branch, tests green, pluginval strictness 5 passes):

* **Sources** — WAVE (8 procedural banks × 16 frames, mip-mapped, BLEP/BLAMP
  sync, unison), DUST (stochastic particle exciter), IMPACT (strike models).
  SAMPLE and GESTURE are in progress.
* **Matter** — SIMD coupled-form modal node graph (up to 64 nodes per voice),
  9 materials with morphing, 7 FORM anchors, topologies, contractive coupling,
  velocity-scaled strike with coherence normalisation and per-note scatter.
  Dry material renders (Crystal / Metal / Membrane / Organic / String /
  Liquid) are audibly distinct — see `renders/gate*/` after
  `scripts/render.sh`.
* **Voice lifetime** — the amplitude envelope shapes the excitation; Matter
  rings out per its Decay after note-off (30 s cap), stolen voices fade in 3 ms.
* **Fracture** — post-mix STFT fragment engine (1024/2048, 75 % overlap,
  mel-spaced fragments, step sequencer, dynamic latency reporting).
* **Space** — curated FX racks per Space type (distortion, chorus, delay,
  granular delay, frequency shifter, spectral diffusion, pitch shifter,
  reverb, EQ, compressor, limiter) recalled on type change, macros live.
* **Master** — gain, DC guard, NaN scrub, instant-attack limiter, safety
  counters; polyphony headroom (N^-0.3).
* **UI** — full design system (pure code, no bitmaps), Main page with the
  procedural ANTI-MATTER object, Source / Shape / Evolve / Fracture / Space /
  Mod pages, preset browser, resizable from 1100×690 to 1600×1000+.
* **DSP LAB** — hidden developer workspace (page 8 in dev builds): signal
  inspector, Matter node/topology views, performance, safety, presets, events.
* **Tools** — `AntiMatrRender` (MIDI → WAV + JSON metrics), `AntiMatrSnapshot`
  (editor → PNG), `scripts/analyze.py` (spectrograms).

In progress: Evolve operators, modulation engine and mod matrix, Sample +
Gesture sources, factory content and mutation/DNA refinement.

See `docs/SPEC.md` §94 for the phase roadmap.
