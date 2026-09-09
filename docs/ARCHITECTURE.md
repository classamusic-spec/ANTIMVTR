# ANTI-MATR — Architecture

This document describes how the repository realises `docs/SPEC.md`. It is the
contract every subsystem (and every agent) builds against. Interfaces named
here are stable; implementations behind them evolve phase by phase.

## Six domains, six directories

| Domain                     | Location                         | Threads            | Depends on |
|----------------------------|----------------------------------|--------------------|------------|
| Audio Engine               | `src/dsp/**`                     | audio              | core, state (read-only descriptors), diagnostics (write-only, wait-free) |
| Control Engine             | `src/dsp/mod/ControlGraph`       | audio              | state |
| State Engine               | `src/state/**`, `src/presets/**` | message            | core |
| Visualization Engine       | `src/ui/visualizers/**`          | message            | diagnostics snapshots only |
| UI Engine                  | `src/ui/**`, `src/plugin/AntiMatrEditor` | message    | plugin (parameters), diagnostics snapshots |
| Developer Diagnostics      | `src/dev/diagnostics/**` (data), `src/dev/dsplab/**` (views) | audio writes / message reads | core |

`src/plugin/` is the only place that knows about the host
(`juce::AudioProcessor`, APVTS). Everything under `src/core`, `src/state`,
`src/dsp`, `src/presets` and `src/dev/diagnostics` compiles without any GUI or
host module — that is what the tests and the offline tools link.

## Signal flow

```
MIDI ─► VoiceManager ─► AntiMatrVoice ×N ─┐
                        │  SourceEngine  │  (WAVE | DUST | IMPACT | SAMPLE | GESTURE)
                        │  EvolveEngine  │  (operates on Matter nodes, before rendering)
                        │  MatterEngine  │  (modal node graph, materials)
                        │  ADSR + velocity
                        └────────────────┘ sum
                                 ▼
                          FractureEngine (post-mix, STFT)
                                 ▼
                          SpaceEngine   (FX rack driven by Space presets)
                                 ▼
                          MasterSection (gain, DC, NaN scrub, limiter)
                                 ▼
                              output
```

`SynthEngine::process()` splits every host block at MIDI event positions
(sample-accurate notes), chunks blocks larger than `kMaxBlockSize`, and pushes
diagnostics taps at SOURCE / POST-MATTER / POST-EVOLVE / POST-FRACTURE /
POST-SPACE / MASTER.

## Parameters

* `src/state/ParameterList.h` — the X-macro list. IDs are permanent.
* `src/state/ParameterRegistry.*` — `enum class Param`, `ParamDesc` (range,
  default, skew, unit, choices, modulatable, mutation category, smoothing),
  lookup by ID, validation.
* `ParamValues` = `std::array<float, kNumParams>` in natural units. The plugin
  snapshots the host's atomics into a `ParamValues` once per block; the engine
  never touches APVTS.
* `ControlGraph` turns base values into *effective* values: smoothing per the
  descriptor's `SmoothingKind`, plus modulation contributions added per block
  and never written back to the host.
* Non-parameter state (fragment tables, sequencer steps, mod routings, sample
  paths, DNA, seeds, A/B) lives in `PatchState` JSON sections and travels to
  the engine through explicit message-thread → audio-thread handoffs (added
  by the subsystems that need them).

## State

`PatchState` ⇄ JSON via `StateManager` (`format: "ANTI-MATR"`, `meta.schema`).
Missing parameters default, unknown ones warn, out-of-range values clamp,
older schemas migrate in `StateManager::migrate`. The plugin's
`get/setStateInformation` embed the same JSON (plus editor size and the other
A/B slot). Presets on disk are `*.antimatr.json`.

`PresetManager` holds code-generated factory presets (builders, no assets)
and user preset I/O (message thread only).

## Voices

`VoiceManager` owns `kMaxVoices` (64) heap-allocated `AntiMatrVoice`s and
handles note on/off, sustain (CC64), pitch bend per channel, channel/poly
pressure, mod wheel, all-notes-off, stealing (releasing → quietest → oldest),
POLY / MONO / LEGATO with a held-note stack and glide. Per-channel bend and
pressure make the structure MPE-ready.

Each voice: `SourceEngine` (all sources, SINGLE/LAYER mixing) →
`EvolveEngine::apply (matter)` → `MatterEngine::process` → `shape.mix` blend →
velocity → `ADSREnvelope`. Voices end when the amp envelope finishes.

## Matter (interface, Phase 5+ implementation)

`MatterEngine::Node` holds frequency / target frequency / ratio / weight /
damping / pan / nonlinearity / excitation / energy / cluster / coupling count
plus resonator state. `noteOn` builds the distribution for the note from the
Shape parameters and materials; `process` renders the response to the
excitation; `fillDiagnostics` copies node state for DSP LAB. Evolve operators
manipulate `Node`s directly (partial-domain processing), never audio FFTs.

## Diagnostics

`Diagnostics` (owned by `SynthEngine`) aggregates:

* `SafetyMonitor` — atomic counters per `SafetyEvent` with subsystem/voice of
  the last occurrence; first occurrences are also logged as events.
* `PerformanceProfiler` — per-`Subsystem` timing with `Scoped` RAII timers;
  publishes avg / peak / moving % of block budget. Compiled out when
  `ANTIMATR_PROFILING` is 0.
* `EngineEventQueue` — SPSC lock-free log (audio → message thread).
* `TripleBuffer<VisualStateSnapshot>` — lightweight state for the consumer UI.
* `TripleBuffer<DiagnosticSnapshot>` — full engine state for DSP LAB
  (node list of the focus voice, stage levels, safety, perf, fracture info).
* `AudioTapRing` per stage — waveform / spectrum sources for displays.
* `DevControls` — atomics written by DSP LAB (dry mode, bypasses, focus voice,
  profiling on/off).

The production engine never depends on DSP LAB; DSP LAB only reads snapshots.

## UI

* `AntiMatrTheme` — colours, typography helpers, reference size (1600×1000).
* `AntiMatrLookAndFeel` — generic JUCE widgets (menus, tooltips, scrollbars).
* Components (`src/ui/components`): `AMPanel`, `AMKnob`, `AMSlider`,
  `AMButton`, `AMIconButton`, `AMSegment`, `AMSourceSelector`, `AMWaveView`,
  `AMSpectrumView`, `AMLogo`, procedural `Icons`, drawing helpers
  (`draw::glowEllipse`, `draw::glowPath`, `draw::panelSurface`, …).
* Views (`src/ui/views`): `TopBar`, `NavBar`, `MainView` (proportional layout
  from the reference design), the five Main panels, `GroupPage` (generic deep
  page from the registry until dedicated pages exist).
* `AntiMatterVisualizer` — the central object; reads `VisualStateSnapshot`.
* Every size (fonts, strokes, glows) derives from component bounds; there are
  no bitmaps and no absolute pixel layouts.

Parameter binding: `BoundKnob` (SliderAttachment) for continuous controls,
`juce::ParameterAttachment` for selectors/toggles.

## Tools

* `AntiMatrTests` — JUCE UnitTest runner (`scripts/test.sh [category]`).
* `AntiMatrRender` — MIDI → WAV + JSON analysis (`scripts/render.sh`), the
  primary way to evaluate sound without a DAW; pair with `scripts/analyze.py`
  for spectrograms.
* `AntiMatrSnapshot` — renders the real editor to PNG (`scripts/snapshot.sh`)
  for visual review; `--page N` selects a page (7 = DSP LAB in dev builds).
* `scripts/validate.sh` — pluginval on the VST3.

## Real-time rules (enforced by review and tests)

No allocation, locks, I/O, logging or JSON on the audio thread after
`prepare`. Every recursive structure guards against NaN/inf/denormals. The
master stage scrubs non-finite samples and limits the output regardless of
what upstream does. Seeded `am::Rng` for all randomness.
