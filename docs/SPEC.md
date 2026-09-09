# ANTI-MATR — MASTER AUTONOMOUS BUILD SPECIFICATION

Pronunciation: ANTI-MATR = "Anti-Matter".
Product line: SOUND BEYOND MATTER.

This file is the canonical roadmap for every engineer and agent working on the
project. Read it before touching code. `docs/ARCHITECTURE.md` describes how the
spec is realised in this repository; `docs/AGENT_GUIDE.md` describes the working
rules (build, test, render, screenshot, file ownership).

---

## 0. ROLE

You are the principal engineer building a complete commercial software
synthesizer named ANTI-MATR. You act simultaneously as senior C++ engineer,
audio DSP engineer, synthesis researcher, JUCE plugin engineer, UI/UX engineer,
procedural graphics engineer, preset-system engineer, sound-design systems
engineer, performance engineer, QA engineer and build/release engineer.

Do not build a mockup. Do not build a static UI. Do not create a conventional
wavetable synthesizer with unusual graphics. Build a real instrument. The final
product must have a recognizable sonic identity derived from its synthesis
architecture.

## 1. PRODUCT VISION

ANTI-MATR is a synthesizer about creating an imaginary substance and then
destabilizing it.

Traditional synthesis: OSCILLATOR → FILTER → AMP → FX.
ANTI-MATR: ENERGY → MATTER → TRANSFORMATION → FRACTURE → SPACE.

The musician should feel: "I created something, gave it physical
characteristics, destabilized it, broke it apart, and placed it into another
environment."

Primary goal: SIMPLE TO OPERATE. DEEP UNDERNEATH. DIFFICULT TO MAKE BORING
SOUNDS WITH.

## 2. UX PHILOSOPHY

Expose only five main conceptual stages, using these terms consistently in the
customer-facing UI:

- SOURCE — Choose the energy.
- SHAPE — Define the matter.
- EVOLVE — Destabilize/change it.
- FRACTURE — Break it apart.
- SPACE — Place it somewhere.

Avoid exposing engineering terminology unnecessarily. Internally the engine can
remain sophisticated. Externally it must feel understandable.

## 3. MAIN WORKFLOW

The Main page allows roughly 80% of sound design without changing pages.
Signal concept: SOURCE → MATTER ENGINE → EVOLVE ENGINE → FRACTURE → SPACE → OUTPUT.
Advanced controls live on dedicated pages.

## 4. TECHNOLOGY

C++20, JUCE, CMake, Git. Build VST3, AU, Standalone. Primary systems: macOS
Apple Silicon, Windows x64, Intel macOS where practical. No Electron. No
HTML/CSS for the plugin UI. No flattened screenshot backgrounds.

## 5. PURE-CODE REQUIREMENT

The core instrument must be capable of being built entirely from code: DSP, UI,
controls, icons, visualizers, animations, factory wavetables, procedural noise,
material definitions, sequencer patterns, preset definitions, modulation,
effects, preset browser, mutation/evolution, developer diagnostics. The product
must NOT require bitmap UI skins. Reference images are visual direction only.
Reconstruct the interface through reusable code components.

## 6. CORE ARCHITECTURE

Six distinct architectural domains, never collapsed into one shared mutable
system: Audio Engine, Control Engine, State Engine, Visualization Engine,
UI Engine, Developer Diagnostics Engine.

High-level flow:
MIDI → VOICE MANAGER → SOURCE / EXCITER → MATTER GRAPH → EVOLVE TRANSFORMATIONS
→ VOICE MIXER → FRACTURE → SPACE / FX → MASTER → OUTPUT.

The Control Engine drives parameters. The UI reflects state. The Developer
Diagnostics Engine observes and inspects systems safely.

## 7. REAL-TIME SAFETY

Inside the audio callback: NO disk access, NO JSON parsing, NO preset scanning,
NO blocking mutexes, NO UI interaction, NO large dynamic allocation, NO sample
loading, NO uncontrolled logging, NO expensive object construction.

Preallocate DSP memory. Use parameter smoothing. Use lock-free communication
where appropriate. Use atomics for small shared state. Use double-buffered
snapshots for visualization and diagnostics.

Protect against: NaN, infinity, denormals, DC buildup, feedback explosions,
extreme output.

## 8. VOICE SYSTEM

Support 8 / 16 / 32 / 64 voices. Default 16.
Implement: polyphonic, monophonic, legato, glide, velocity, sustain,
aftertouch, poly pressure, pitch bend, voice stealing, MPE-ready architecture.
Each voice contains: SourceEngine, MatterEngine, per-voice modulation,
amplitude envelope, voice state. Do not place expensive global FFT processing
inside every voice.

## 9. SOURCE

SOURCE means: CHOOSE YOUR ENERGY. Main page exposes four primary sources:
WAVE, DUST, IMPACT, SAMPLE. An advanced Source page may additionally expose
GESTURE and routing.

## 10. WAVE

Production-quality wavetable/VA oscillator. Capabilities: wavetable position,
scan, morph, phase, unison, detune, spread, octave, semitone, fine tune, FM,
PM, AM, ring modulation, hard sync where appropriate.

Generate initial tables procedurally: sine, triangle, saw, square, pulse, odd
harmonic, even harmonic, formant, folded, metallic, spectral, fractured,
noise-derived. Use appropriate antialiasing.

## 11. DUST

Stochastic excitation. Modes: white, pink-like, brown-like, blue-like, filtered
noise, crackle, impulse dust, granular cloud, frozen cloud.
Controls: density, color, grain size, jitter, pitch, position, spread, stereo,
seed. All random behavior must become deterministic when seed is fixed.

## 12. IMPACT

Transient energy. Modes: impulse, click, pluck, noise strike, metal strike,
damped sine, membrane hit. Controls: hardness, brightness, length, velocity,
curve, randomness. IMPACT is especially important for exciting Matter.

## 13. SAMPLE

WAV loading, drag/drop, one shot, loop, reverse, root pitch, start, end, pitch,
granular mode. Do not load/decode files on the audio thread.

## 14. GESTURE — ADVANCED SOURCE (later)

Continuous excitation. Modes: BOW, SCRAPE, RUB, BREATH, FRICTION, ELECTRICAL.
Controls: pressure, speed, roughness, position, motion, bandwidth. Gesture
continuously injects energy into Matter. Lives on the advanced Source page.

## 15. MATTER ENGINE

The technological centerpiece. Do NOT expose the complexity directly to
beginners. Internally, Matter is a dynamic graph of resonating nodes.
Normal mode: 64 nodes per voice. Quality: ECO 32, NORMAL 64, HIGH 96/128,
ULTRA determined through profiling.

Each node: frequency, target frequency, energy, amplitude weight, damping,
phase/filter state, stereo position, nonlinearity, cluster ID, excitation
weighting. Each edge: source, destination, coupling strength, coupling type.
Keep coupling constrained and stable.

## 16. MATTER RESONATORS

Efficient stable modal resonators providing: accurate target frequency,
controllable decay, energy input, stable state, frequency modulation within
safe bounds. Automated tests for: frequency accuracy, decay, sample-rate
changes, extreme parameters, NaN, infinity.

## 17–23. SIMPLIFIED SHAPE CONTROLS

Main page exposes DENSITY, FORM, MASS, TENSION, DECAY, SURFACE.

- DENSITY: active node weighting, node spacing, spectral complexity, cluster
  occupancy. Low = few resonances. High = dense material. Smooth transitions.
- FORM (replaces RIGIDITY): structural/harmonic organization. Morph among
  harmonic, stretched harmonic, membrane, metallic, inharmonic, crystalline,
  stochastic. Must create major audible timbral changes.
- MASS: perceived weight, size, response speed, low/high energy balance,
  frequency-dependent damping. Higher Mass feels larger/heavier. Do not simply
  transpose pitch downward.
- TENSION: stretches/compresses modal relationships (strings, membranes,
  stretched materials, tight metallic surfaces, loose organic materials).
- DECAY (replaces Damping): frequency-dependent damping; how long the material
  retains energy. Separate from the amplitude envelope.
- SURFACE: microstructure and nonlinear complexity (mode saturation, weak
  coupling, micro detune, nonlinear damping, wavefold behavior, excitation
  interaction). Low = smooth. High = rough/complex. Oversample where needed.

## 24. MATERIAL MODELS

Programmatic materials: CRYSTAL, METAL, ORGANIC, LIQUID, MEMBRANE, STRING, WOOD,
VOID, CUSTOM. Each defines: modal distribution, damping curve, node weighting,
coupling topology, nonlinearity, stereo tendencies, excitation response.
Mathematical definitions, not samples.

## 25. MATERIAL BLENDING

Continuous Material A → Material B morphing of the underlying structure (NOT a
crossfade of rendered audio). Interpolate frequencies, weights, decays,
coupling, surface characteristics, excitation behavior. Safe interpolation.

## 26–31. EVOLVE

Public-facing controls (Main): BEND, MELT, TEAR, MAGNET. Advanced Evolve page
additionally: GRAVITY, SCATTER, FREEZE, CRUSH.

- BEND: warp modal frequency relationships. Internal: amount, pivot, range,
  curve. Main exposes Amount.
- MELT: organized material → diffuse structure. Frequency certainty decreases,
  resonances widen, structure less ordered, noise energy may increase, coupling
  less predictable. High values feel dissolved. NOT ordinary distortion.
- TEAR: split Matter clusters (frequency grouping, stereo, coupling, timing,
  decay). One object becomes multiple structures.
- MAGNET: attract nodes toward musical frequency relationships (octave, fifth,
  major, minor, chromatic, current scale, custom intervals). Attraction, not
  only hard quantization.
- GRAVITY: moves spectral center of mass. SCATTER: controlled deterministic
  instability. FREEZE: captures current Matter/spectral state. CRUSH: pulls
  nearby resonances into clusters.

## 32. PARTIAL-DOMAIN PROCESSING

Perform Evolve transformations directly on Matter nodes (frequency, energy,
damping, stereo, cluster relationships, coupling) rather than per-voice FFT.

## 33–36. FRACTURE

FRACTURE means: BREAK INTO NEW REALITIES. Operates after voices are combined.
Normal: 16 fragments (support 8/16/32). Implementation: STFT, filterbank or
hybrid. Each fragment: pitch, delay, pan, decay, probability, feedback, spread,
gain. Main exposes AMOUNT, SPREAD, SEQUENCE, RANDOM and ON/OFF; deeper page
exposes fragment editing. Modes: SPECTRAL, RHYTHMIC, TRANSIENT, EVOLVE.
Sequencer: 1–32 steps, tempo sync, free rate, swing, direction, probability,
random, seed, evolve, retrig. Each step can influence fragment selection,
pitch, pan, gain, Evolve amount, Shape macro.

## 37–38. SPACE / FX ENGINE

Environments: NEBULA, VOID, CHAMBER, ORBIT, DREAM, MACHINE, SHIMMER, DUST. Each
Space is a curated macro FX configuration. Main controls: MIX, SIZE, TONE,
FEEDBACK. Advanced Space page exposes the rack: distortion, saturation, chorus,
delay, granular delay, frequency shift, spectral diffusion, algorithmic reverb,
EQ, compressor, limiter.

## 39–43. MODULATION

4 LFOs, 4 envelopes/MSEGs, 4 random/chaos generators, velocity, keytracking,
aftertouch, poly pressure, mod wheel, note age, audio follower, sequencer,
8 macros. Dedicated MOD page. Workflow: drag source → drop on parameter.
Parameter displays base value, modulation range, current modulated position.
Multiple assignments. Never write modulation into the base host parameter.

ControlGraph: effective = base + automation + modulation with correct ownership
and host behavior; smoothing; safe clamping; poly and mono modulation.
Only selected paths support audio-rate modulation (Wave FM/PM/AM, selected
Surface behavior, selected Matter coupling).
Chaos: deterministic (random walk, Brownian, logistic, Lorenz-inspired,
interpolated random targets) with rate, depth, stability, symmetry, seed.

## 44–46. MUTATION, A/B

MUTATE in the top bar must be useful immediately. Not randomize-all. Every
preset has DNA categories: SOURCE, SHAPE, EVOLVE, FRACTURE, MOVEMENT, SPACE,
PITCH, CHAOS. Each parameter has mutation category, weight, safe min/max,
preferred region, distribution, constraints. Strengths: SUBTLE (close
relatives), EVOLVE (meaningful variations), EXTREME (major structural mutation,
still safe). A/B: two snapshots with continuous morphing of source, Matter,
Evolve, Fracture, modulation, Space using safe interpolation.

## 47. ANALYZE → MATTER (after core synthesis is stable)

Import audio → analyze spectral peaks, partial relationships, amplitudes,
centroid, transients, approximate decay → create a playable Matter structure.
The imported audio becomes a synthetic material, not sample playback.

## 48–50. PRESETS AND FACTORY CONTENT

Store: schema version, plugin version, name, author, category, tags, base
parameters, Source state, Matter topology, Material A/B, Evolve state,
Fracture, modulation, Space, A/B states, DNA, random seeds, sample references.
JSON or ValueTree-backed serialization with migration.

The synth must work without external audio assets. Generate in code:
wavetables, noise, excitation, material structures, patterns, Spaces, presets.
Target 150+ presets eventually; initial target 30 reference patches, including:
Void Bloom, Liquid Teeth, Carbon Bass, Crystal Ghost, Broken Choir, Titanium
Skin, Gravity Drone, Glass Creature, Membrane Sky, Fractured Voice, Impossible
String, Metal Bloom, Frozen Machine, Electric Organism, Dust Piano, Nebula Pad,
Torn Bass, Magnet Bells, Organic Circuit, Anti-String.

## 51–53. BRAND AND VISUAL SYSTEM

Name ANTI-MATR, tagline SOUND BEYOND MATTER. Personality: experimental,
premium, mysterious, scientific, minimal, cinematic, future-facing. Avoid
cartoon sci-fi, gamer aesthetics, cyberpunk clutter.

Dark near-black background, graphite panels, subtle glass, thin borders, large
negative space. Luminous accents: electric blue, soft cyan, violet, magenta,
occasional warm ivory/amber. Glow communicates energy, selection, modulation,
focus, activity. Do not make every border neon.

Logical design 1600 × 1000, resizable, minimum ≈ 1100 × 690, responsive
layout, never a scaled bitmap.

## 54–62. MAIN SCREEN AND PANELS

TOP: branding, preset selector, Browse, Random, Mutate, Settings.
CENTER LEFT: SOURCE. CENTER: ANTI-MATTER OBJECT. CENTER RIGHT: SHAPE.
LOWER LEFT: EVOLVE. LOWER CENTER: FRACTURE. LOWER RIGHT: SPACE.
BOTTOM: navigation (MAIN, SOURCE, SHAPE, EVOLVE, FRACTURE, SPACE, MOD,
optionally BROWSER). Main should always feel like home.

SOURCE panel: four selectors (WAVE, DUST, IMPACT, SAMPLE); the selected source
shows a waveform/display; primary controls POSITION, SCAN, DETUNE, SPREAD.

Central ANTI-MATTER OBJECT: the product's visual signature. Dark matter,
liquid glass, energy, gravity distortion, iridescent plasma, floating
fragments. Procedurally rendered; reacts to actual engine state:
Density → complexity, Form → geometric organization, Mass → inertia,
Tension → stretch, Decay → motion persistence, Surface → detail,
Bend → deformation, Melt → diffusion, Tear → separation, Magnet → alignment,
Fracture → shards, Space → environmental halo, Audio amplitude → pulse,
Pitch → subtle structural change.
Phase 1: JUCE Graphics (Bezier geometry, polar meshes, procedural noise,
translucent layers, particles, gradient fields, spectral displacement).
Phase 2: OpenGL/shader rendering if profiling permits.

SHAPE panel: SIMPLE / ADVANCED toggle. SIMPLE: DENSITY, FORM, MASS, TENSION,
DECAY, SURFACE. ADVANCED: material selection/blending, topology, coupling,
node distribution.

EVOLVE panel (Main): BEND, MELT, TEAR, MAGNET, AMOUNT, SPEED. Advanced page:
CRUSH, GRAVITY, SCATTER, FREEZE plus detail.

FRACTURE panel (Main): large spectral visualization, mode, amount, spread,
sequence, random, on/off. Dedicated page: fragment editor, sequencer.

SPACE panel (Main): Space preset, MIX, SIZE, TONE, FEEDBACK. Dedicated page:
individual FX modules.

## 63–65. COMPONENT SYSTEM, KNOBS, LAYOUT

Reusable classes: AntiMatrLookAndFeel, AntiMatrTheme, AMPanel, AMKnob,
AMSlider, AMButton, AMToggle, AMTab, AMSourceSelector, AMWaveView,
AMSpectrumView, AMXYPad, AMModRing, AMStepEditor, AMPresetCard, AMTooltip,
AntiMatterVisualizer. Never duplicate large drawing implementations.

Knobs: custom vector rendering (dark base, thin value arc, indicator, small
controlled glow, label). Hover shows precise value. Double-click resets. Shift
drag = fine. Right-click = context menu. Large invisible hit target.

Layout: no hardcoded pixel coordinates for the entire interface; use Grid,
FlexBox, relative bounds, custom constraints, reference-scale helpers. Central
visualizer retains aspect ratio. Smaller sizes reduce decoration first, then
spacing; controls never unusably small.

## 66–67. VISUALIZATION STATE AND UI PERFORMANCE

VisualStateSnapshot published by DSP: RMS, peak, spectral bands, active
voices, active Matter nodes, cluster count, Shape values, Evolve values,
Fracture activity, Space activity. UI reads snapshots; never interrogates voice
internals directly. Target 60 FPS, adaptive 30 FPS under load; suspend when
hidden. Audio always has priority.

## 68–90. DSP LAB — INTERNAL DEVELOPER MODE

An internal engineering environment available only in development/internal
builds or via a hidden developer flag. It answers: what is the engine doing,
which nodes are active, which subsystem uses CPU, where instability occurs,
which transformation changed the sound, what is clipping, what produces
NaN/infinity, how the Matter topology changes, how a parameter maps
internally, and dry-vs-processed comparison.

Layout: LEFT engine inspector, CENTER signal visualizers, RIGHT profiling /
diagnostics, BOTTOM A/B and stress tools. Tabs: SOURCE, MATTER, EVOLVE,
FRACTURE, MOD, SPACE, PERFORMANCE, SAFETY.

Views: Matter Inspector (per-node table/plots), Topology View (graph with
before/after snapshots), Resonance Distribution (target vs actual
frequencies, ratios, harmonicity deviation, histogram, material profiles),
Excitation View (energy per exciter/frequency/cluster/node/time; solo;
SOURCE ONLY / MATTER ONLY / FULL PATH), Evolve View (input/output
distributions, node movement, cluster/energy changes, CPU, per-operator
bypass, RAW vs EVOLVED), Fracture View (FFT size, hop, window, latency,
fragment mapping and activity, pre/post spectra, reconstruction error),
Modulation View (trace one parameter end-to-end), Performance Profiler
(per-subsystem average/peak/moving average/percentage, voice count, sample
rate, buffer, quality), Memory Diagnostics (buffers, allocations during
playback → zero unexpected), Safety Monitor (NaN, infinity, denormal, hard
clipping, limiter, DC, feedback clamp, invalid frequency/coefficient,
resonator reset — each identifying subsystem/voice/time), Raw Audio Views
(waveform, spectrum, spectrogram, RMS, peak, crest, DC, correlation, phase
scope at SOURCE / POST-MATTER / POST-EVOLVE / POST-FRACTURE / POST-SPACE /
MASTER), DRY MODE (disable Space, master character FX; SOURCE ONLY / MATTER
ONLY / MATTER + EVOLVE / FULL SYNTH), A/B engine testing (level-matched),
Stress Tests (sustained note, 16-note chord, max polyphony, note flood,
extreme velocity, bend sweep, automation sweep, macro motion, preset
switching loop, sample-rate switch, long feedback), Parameter Sweep Tool
(measure peak, RMS, CPU, NaN, clipping, centroid, active nodes), Preset
Validator, Engine Event Log (lock-free from audio thread), Diagnostic Report
export (no private audio).

DSP LAB style: clarity, precision, technical density. Architectural rule: DSP
LAB observes production DSP through safe diagnostic interfaces; production
engine works without DSP LAB.

## 91–93. PROJECT STRUCTURE, CLASSES, PARAMETERS

Folders: src/plugin, src/core, src/state, src/dsp{,/source,/matter,/evolve,
/fracture,/mod,/fx}, src/ui{,/components,/views,/visualizers}, src/dev{,/dsplab,
/diagnostics}, src/presets, tests, benchmarks. Never build everything in
PluginProcessor.cpp.

Classes (architectural equivalents): AntiMatrProcessor, AntiMatrEditor,
ParameterRegistry, StateManager, PresetManager, SynthEngine, AntiMatrVoice,
SourceEngine, WaveSource, DustSource, ImpactSource, GestureSource,
SampleSource, MatterEngine, MatterNode, MatterCluster, MaterialProfile,
MaterialMorpher, EvolveEngine, FractureEngine, FractureSequencer, ControlGraph,
ModulationEngine, ChaosGenerator, SpaceEngine, FXRack, MutationEngine,
PresetDNA, AudioToMatterAnalyzer, VisualStateSnapshot, AntiMatterVisualizer,
DiagnosticSnapshot, PerformanceProfiler, SafetyMonitor, EngineEventQueue,
DSPLabView, MatterInspector, TopologyView, SignalInspector,
ParameterTraceView, StressTestController, PresetValidator.

Parameter IDs are permanent (e.g. source.wave.level, shape.density,
evolve.melt, fracture.amount, space.mix). Every parameter includes ID, name,
unit, range, default, skew, smoothing, modulatable, mutation category,
mutation range, UI group. Never casually rename released IDs.

## 94. BUILD PHASES (each must compile and pass tests before proceeding)

- PHASE 0: Repository, JUCE, CMake, VST3, AU, Standalone, Processor, Editor.
- PHASE 1: ParameterRegistry, State, Serialization, Preset skeleton.
- PHASE 2: Voice system, MIDI, sine oscillator, ADSR, polyphony.
- PHASE 3: Wave Source.
- PHASE 4: Dust + Impact.
- PHASE 5: Matter MVP (32 resonators; Density, Form, Mass, Tension, Decay).
  Initial DSP LAB shell exposing waveform, spectrum, node list, frequency
  distribution, peak/RMS, NaN counter, CPU timing.
  STOP HERE UNTIL MATTER SOUNDS INTERESTING.
- PHASE 6: 64-node Matter, Surface, clusters, coupling, materials. DSP LAB:
  topology graph, coupling inspection, material distribution comparison.
- PHASE 7: Evolve (Bend, Melt, Tear, Magnet, then Gravity, Scatter, Crush,
  Freeze). DSP LAB before/after transformations.
- PHASE 8: ANTI-MATR UI design system, Main page only.
- PHASE 9: AntiMatterVisualizer.
- PHASE 10: ControlGraph/modulation; Parameter Trace in DSP LAB.
- PHASE 11: Fracture + diagnostics.
- PHASE 12: Space/FX + raw/dry pipeline views.
- PHASE 13: Deep UI pages.
- PHASE 14: Mutation, A/B, DNA.
- PHASE 15: Sample engine, Gesture.
- PHASE 16: Analyze → Matter.
- PHASE 17: Preset browser, factory content, Preset Validator.
- PHASE 18: Optimization, full DSP LAB performance profiling.
- PHASE 19: QA/release; hide DSP LAB per release configuration.

## 95. CRITICAL MATTER GATE

Phase 5/6 is a mandatory quality gate. Do NOT compensate for weak Matter DSP
using reverb, distortion, delay, chorus or huge unison. Use DSP LAB DRY MODE
(SOURCE ONLY, MATTER ONLY, MATTER + EVOLVE). Matter should sound compelling
substantially dry. Test patches: Crystal, Metal, Membrane, Organic, String,
Liquid. If these do not feel meaningfully different, keep improving Matter.

## 96–98. TESTING, HOSTS, PERFORMANCE

Automate: parameter defaults, state roundtrip, preset migration, oscillator
frequency, Matter resonance frequency, Matter decay, sample-rate changes,
modulation limits, deterministic random, mutation safety, NaN/infinity
detection, feedback stability, Fracture bypass, FFT reconstruction.
Sample rates 44.1/48/88.2/96 kHz; buffers 32–1024.

Hosts (eventually): Ableton Live, Logic Pro, FL Studio, REAPER, Bitwig, Cubase.

Performance target: 48 kHz, 128 samples, 16 active voices, 64 Matter
nodes/voice practically playable on modern Apple Silicon / desktop CPUs.
Measure, profile, optimize actual hotspots.

## 99–100. RELEASE QUALITY AND DEFINITION OF DONE

No placeholder buttons, fake meters, fake visualizers, dead pages,
unconnected knobs, debug text, random crashes, unsafe feedback, broken state
recall. UI controls affect actual DSP. Visualizers reflect actual synthesis
state. DSP LAB diagnostics do not leak into customer builds.

Done only when VST3, AU, Standalone, MIDI, polyphony, automation, preset
recall, DAW recall, Source, Shape/Matter, Evolve, Fracture, Space, modulation,
Mutation, A/B, resizing, reactive visualizer, stable audio under animation,
acceptable CPU, recognizable factory sound, dry-engine identity, and DSP LAB
validation all hold.

## 101–104. NORTH STARS

Sonic: impossible physical instruments, dark cinematic pads, organic bass,
metallic creatures, unstable keys, spectral percussion, living drones, alien
bells, fractured vocals, evolving textures, hybrid acoustic/electronic sounds.
Not "another wavetable synth".

UX: SOURCE = what creates energy. SHAPE = what the object is. EVOLVE = how it
changes. FRACTURE = how it breaks. SPACE = where it exists. A dramatically
different sound within ~30 seconds without opening an advanced page.

Engineering: the customer sees simplicity; the developer sees everything.

Product: prefer 5 powerful macros over 20 controls; better Matter over another
oscillator feature; better Evolve over another FX; graphics that explain the
sound over decoration; musical immediacy over technical flexibility; inspect
in DSP LAB instead of hiding problems with post-processing.

## 105. FIRST EXECUTION

Start with PHASE 0: repository structure, CMake, JUCE, VST3/AU/Standalone
targets, AntiMatrProcessor, AntiMatrEditor, ParameterRegistry skeleton,
SynthEngine skeleton, AntiMatrVoice, basic MIDI, sine oscillator, ADSR, master
output, state serialization foundation. Then the first code-rendered Main UI
shell (logo, tagline, preset bar, SOURCE panel, central visualizer
placeholder, SHAPE, EVOLVE, FRACTURE, SPACE panels, bottom navigation), and
the foundational diagnostics architecture (DiagnosticSnapshot, SafetyMonitor,
PerformanceProfiler, EngineEventQueue). Verify: compiles, Standalone launches,
VST3 loads, MIDI creates sound, UI resizes, state survives reload, diagnostic
counters operate safely. Only then advance.

FINAL INSTRUCTION: treat this as an engineering roadmap. Build one validated
subsystem at a time. The identity of ANTI-MATR comes from the relationship
between ENERGY, MATTER, TRANSFORMATION, FRACTURE and SPACE. The final
instrument should feel less like programming a synthesizer and more like
discovering and manipulating a form of matter that does not exist.
