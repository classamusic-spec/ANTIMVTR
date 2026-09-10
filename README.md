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

Integrated on the branch, tests green (2.68 M assertions), pluginval strictness 5
passes on the VST3:

* **Sources** — WAVE (8 procedural banks x 16 frames, mip-mapped, BLEP/BLAMP
  sync, unison), DUST (stochastic particle exciter), IMPACT (strike models),
  SAMPLE (built-in procedural samples, one-shot / loop / reverse / granular,
  ANALYZE to Matter) and GESTURE (bow, scrape, rub, breath, friction,
  electrical).
* **Matter** — SIMD coupled-form modal node graph (up to 64 nodes per voice),
  9 materials with morphing, 7 FORM anchors, topologies, contractive coupling,
  velocity-scaled strike with coherence normalisation and per-note scatter.
  Dry material renders (Crystal / Metal / Membrane / Organic / String /
  Liquid) are audibly distinct.
* **Evolve** — the eight operators (BEND, MELT, TEAR, MAGNET, GRAVITY,
  SCATTER, FREEZE, CRUSH) as partial-domain transformations of the node graph,
  re-applied every block so they never accumulate.
* **Modulation** — 4 LFOs, 4 envelopes, 4 chaos generators, 8 macros and the
  MIDI sources, routed through a mod matrix that reaches every modulatable
  parameter, per voice. 64 voices with 64 routings run at 71 % of realtime.
* **Fracture** — post-mix STFT fragment engine (1024/2048, 75 % overlap,
  mel-spaced fragments, step sequencer, dynamic latency reporting).
* **Space** — curated FX racks per Space type (distortion, chorus, delay,
  granular delay, frequency shifter, spectral diffusion, pitch shifter,
  reverb, EQ, compressor, limiter) recalled on type change, macros live.
* **Voice lifetime** — the amplitude envelope shapes the excitation; Matter
  rings out per its Decay after note-off (30 s cap), stolen voices fade in 3 ms.
* **Master** — gain, DC guard, NaN scrub, instant-attack limiter, safety
  counters; polyphony headroom (N^-0.3).
* **State** — JSON patches with migration, A/B slots with continuous safe
  morphing, DNA-aware mutation, code-generated factory presets.
* **UI** — full design system (pure code, no bitmaps), Main page with the
  procedural ANTI-MATTER object, Source / Shape / Evolve / Fracture / Space /
  Mod pages, mod rings on every knob, preset browser, resizable from 1100x690
  to 1600x1000+.
* **DSP LAB** — hidden developer workspace (page 8 in dev builds): signal
  inspector, Matter node/topology views, per-fragment Fracture activity,
  performance, safety, preset validator, events.
* **Tools** — `AntiMatrRender` (MIDI to WAV + JSON metrics, with `--mod`,
  `--sample`, `--dry`), `AntiMatrSnapshot` (editor to PNG), `scripts/analyze.py`
  (spectrograms).

See `docs/SPEC.md` §94 for the phase roadmap.
