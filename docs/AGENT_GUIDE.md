# ANTI-MATR — Working Guide for Engineers and Agents

Read `docs/SPEC.md` (what) and `docs/ARCHITECTURE.md` (how) first.

## Build, test, listen, look

```bash
scripts/build.sh                 # configure + build everything (Release, ccache, shared JUCE)
scripts/build.sh AntiMatrTests   # one target
scripts/test.sh                  # run all unit tests
scripts/test.sh engine           # one category: core | state | engine | diagnostics | matter | ...
scripts/render.sh --out renders/x.wav --note 60 --hold 1.5 --seconds 4 --set shape.form=0.7 --dry matter
/home/user/pyenv/bin/python scripts/analyze.py renders/x.wav     # metrics + spectrogram PNG
scripts/snapshot.sh --out renders/ui.png --page 0                # editor PNG (page 7 = DSP LAB)
scripts/validate.sh 5                                            # pluginval strictness 5
```

Environment variables: `ANTIMATR_BUILD_DIR` (default `build`),
`ANTIMATR_BUILD_TYPE` (Release), `ANTIMATR_JUCE_PATH`
(`/home/user/deps/JUCE` shared checkout; FetchContent otherwise).

The render tool prints JSON: peak, RMS, DC, non-finite count, spectral
centroid, decay (t20/t60 after note-off), safety counters and CPU per
subsystem. `analyze.py` adds spectral flatness, top partials and a
spectrogram image. **Use them.** Never claim something "sounds good" without
numbers and a spectrogram; never claim a UI change works without a snapshot.

## Rules of engagement

1. **Own your files.** Each task names the directories/files you may create or
   edit. Do not edit files outside that set; if you need an interface change,
   describe it in your final report instead.
2. **Parameter IDs are permanent.** Add parameters only by appending to the
   relevant section of `src/state/ParameterList.h`; never rename or reorder.
   Prefer using the existing IDs — the list already covers the whole spec.
3. **Real-time safety** (SPEC §7): preallocate in `prepare`, no locks, no
   allocation, no I/O, no logging in `process`/`render`. Use `am::Rng` for
   randomness (seeded, deterministic). Guard filters against NaN/inf and report
   through `SafetyMonitor` (`ctx.diagnostics->safety.note (...)`).
4. **Tests are mandatory.** Add a `tests/<Subsystem>Tests.cpp` with a
   `juce::UnitTest` (category = your subsystem). `scripts/test.sh` must pass
   before you finish. Test frequency accuracy, decay, sample rates 44.1/48/
   88.2/96 k, buffer sizes 32–1024, extreme parameters, NaN/inf.
5. **Sound is the product.** DSP tasks must include renders that prove the
   behaviour (dry, via `--dry matter` where relevant) and describe what the
   spectrogram shows. Matter must sound compelling without effects.
6. **Pure code.** No bitmaps, no external assets. Icons, tables, noise,
   materials and presets are generated.
7. **Style.** C++20, `namespace am` (UI: `am::ui`, dev: `am::dev`), JUCE
   naming (`camelCase` functions, `PascalCase` types), 4-space indent, braces
   on their own line, `juce::` qualified. Keep files focused; no giant
   PluginProcessor.
8. **Diagnostics-first.** When a subsystem exposes state (nodes, fragments,
   routings), publish it through `DiagnosticSnapshot`/`VisualStateSnapshot`
   fields and show it in DSP LAB rather than printing.
9. **Commit discipline.** Work in your worktree/branch, commit with clear
   messages, keep the build green at every commit. Do not push to other
   branches. Report: what you built, how you verified it (test output,
   render metrics, snapshot paths), what is left.

## Where things go

| You are building…          | Put it in…                                   |
|----------------------------|----------------------------------------------|
| A source (Dust, Impact, …) | `src/dsp/source/<Name>Source.{h,cpp}`, register in `SourceEngine.cpp` |
| Matter                     | `src/dsp/matter/*` (`MatterEngine`, `MaterialProfile`, `MaterialMorpher`, `ModalResonator`, `MatterTopology`) |
| Evolve                     | `src/dsp/evolve/*`                            |
| Fracture                   | `src/dsp/fracture/*`                          |
| Space / FX                 | `src/dsp/fx/*`                                |
| Modulation                 | `src/dsp/mod/*`                               |
| Presets / factory content  | `src/presets/*`                               |
| Mutation / DNA / A-B       | `src/state/*`                                 |
| UI components              | `src/ui/components/*`                         |
| Pages / panels             | `src/ui/views/*`                              |
| Visualizers                | `src/ui/visualizers/*`                        |
| DSP LAB views              | `src/dev/dsplab/*`                            |
| Tests                      | `tests/*Tests.cpp`                            |

CMake globs `src/**` and `tests/*.cpp`; new files are picked up on the next
configure (`CONFIGURE_DEPENDS`), no CMake edits needed.

## Reviewing UI work

Take snapshots at 1600×1000 and 1100×690 (`--width/--height`) and look at
them. Check: readable labels, no clipped text, glow used for meaning, nothing
neon-everywhere, controls still usable at the minimum size.

## Reviewing DSP work

Render at least: a single note dry (`--dry matter`), a chord, a note with
note-off before the tail ends (decay measurement), and extremes of each
parameter (0 and 1). Confirm `nonFinite == 0`, `safety` all zero (or
explained), `peak <= 1`, and CPU well within budget at 16 voices.

Level targets (velocity 100, one voice): sources ≈ −15 dBFS peak; Matter
notes −12…−2 dBFS peak across Shape extremes; a 16-note chord may touch the
limiter but must never count `Hard clip`. Start notes at 0.2 s
(`--seq 60:0.2:1.7`) when measuring peaks so the gain smoother has settled.
