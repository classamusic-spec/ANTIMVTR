#pragma once

/**
    ANTI-MATR PARAMETER LIST — THE PERMANENT PARAMETER CONTRACT

    Every host-visible parameter is declared exactly once here through the
    X-macro ANTIMATR_PARAMETER_LIST. The enum `am::Param`, the descriptor
    table, the host parameter layout, the preset format and the UI all derive
    from this list.

    RULES
      * Parameter IDs (the dotted strings) are permanent once released.
        Never rename or reuse them. Deprecate by leaving the ID in place.
      * Append new parameters at the END of their section. Order defines the
        enum value and the index of every runtime array, so inserting in the
        middle is allowed only before the first public release.
      * Keep the customer-facing name short and free of engineering jargon.

    COLUMNS
      X(enumName, "id", "Name", group, kind, min, max, default, skew, "unit",
        "choiceA|choiceB", modulatable, mutationCategory, smoothing)

    Helper macros keep the table readable. `skew` follows the JUCE
    NormalisableRange convention: 1 = linear, < 1 = more resolution at the
    low end (use 0.3 for times, 0.5 for frequencies/ratios).
*/

#define AM_FLOAT(X, e, id, name, grp, mn, mx, df, skew, unit, mod, mut, smooth) \
    X (e, id, name, grp, Float, mn, mx, df, skew, unit, "", mod, mut, smooth)

#define AM_BOOL(X, e, id, name, grp, df, mut) \
    X (e, id, name, grp, Bool, 0.0f, 1.0f, df, 1.0f, "", "", false, mut, None)

#define AM_CHOICE(X, e, id, name, grp, choices, df, mut) \
    X (e, id, name, grp, Choice, 0.0f, 0.0f, df, 1.0f, "", choices, false, mut, None)

#define AM_INT(X, e, id, name, grp, mn, mx, df, unit, mod, mut) \
    X (e, id, name, grp, Int, mn, mx, df, 1.0f, unit, "", mod, mut, None)

// A unipolar 0..1 macro-style control (the most common kind in ANTI-MATR).
#define AM_UNI(X, e, id, name, grp, df, mut) \
    AM_FLOAT (X, e, id, name, grp, 0.0f, 1.0f, df, 1.0f, "", true, mut, Medium)

//------------------------------------------------------------------------------
// Per-slot generators for LFOs, envelopes, chaos generators and macros.

#define AM_LFO(X, n) \
    AM_FLOAT  (X, lfo##n##Rate,     "mod.lfo" #n ".rate",     "LFO " #n " Rate",     Mod, 0.01f, 40.0f, 1.0f, 0.3f, "Hz", true, Movement, Medium) \
    AM_CHOICE (X, lfo##n##Shape,    "mod.lfo" #n ".shape",    "LFO " #n " Shape",    Mod, "SINE|TRIANGLE|SAW|SQUARE|RANDOM|SMOOTH RANDOM|WARP", 0.0f, Movement) \
    AM_BOOL   (X, lfo##n##Sync,     "mod.lfo" #n ".sync",     "LFO " #n " Sync",     Mod, 0.0f, Movement) \
    AM_CHOICE (X, lfo##n##Division, "mod.lfo" #n ".division", "LFO " #n " Division", Mod, "8/1|4/1|2/1|1/1|1/2|1/4|1/8|1/16|1/32|1/4T|1/8T|1/16T|1/4D|1/8D|1/16D", 5.0f, Movement) \
    AM_FLOAT  (X, lfo##n##Phase,    "mod.lfo" #n ".phase",    "LFO " #n " Phase",    Mod, 0.0f, 1.0f, 0.0f, 1.0f, "", false, Movement, None) \
    AM_FLOAT  (X, lfo##n##Symmetry, "mod.lfo" #n ".symmetry", "LFO " #n " Symmetry", Mod, 0.0f, 1.0f, 0.5f, 1.0f, "", true, Movement, Medium) \
    AM_FLOAT  (X, lfo##n##Depth,    "mod.lfo" #n ".depth",    "LFO " #n " Depth",    Mod, 0.0f, 1.0f, 1.0f, 1.0f, "", true, Movement, Medium) \
    AM_BOOL   (X, lfo##n##Retrig,   "mod.lfo" #n ".retrig",   "LFO " #n " Retrigger",Mod, 1.0f, Movement) \
    AM_FLOAT  (X, lfo##n##Fade,     "mod.lfo" #n ".fade",     "LFO " #n " Fade In",  Mod, 0.0f, 10.0f, 0.0f, 0.4f, "s", false, Movement, None)

#define AM_ENV(X, n) \
    AM_FLOAT (X, env##n##Attack,  "mod.env" #n ".attack",  "Env " #n " Attack",  Mod, 0.0f, 10.0f, 0.01f, 0.3f, "s", true, Movement, None) \
    AM_FLOAT (X, env##n##Decay,   "mod.env" #n ".decay",   "Env " #n " Decay",   Mod, 0.0f, 10.0f, 0.3f,  0.3f, "s", true, Movement, None) \
    AM_FLOAT (X, env##n##Sustain, "mod.env" #n ".sustain", "Env " #n " Sustain", Mod, 0.0f, 1.0f,  0.5f,  1.0f, "",  true, Movement, None) \
    AM_FLOAT (X, env##n##Release, "mod.env" #n ".release", "Env " #n " Release", Mod, 0.0f, 20.0f, 0.5f,  0.3f, "s", true, Movement, None) \
    AM_FLOAT (X, env##n##Curve,   "mod.env" #n ".curve",   "Env " #n " Curve",   Mod, 0.0f, 1.0f,  0.5f,  1.0f, "",  false, Movement, None) \
    AM_BOOL  (X, env##n##Loop,    "mod.env" #n ".loop",    "Env " #n " Loop",    Mod, 0.0f, Movement)

#define AM_CHAOS(X, n) \
    AM_CHOICE (X, chaos##n##Type,      "mod.chaos" #n ".type",      "Chaos " #n " Type",      Mod, "WALK|BROWNIAN|LOGISTIC|LORENZ|TARGETS", 0.0f, Chaos) \
    AM_FLOAT  (X, chaos##n##Rate,      "mod.chaos" #n ".rate",      "Chaos " #n " Rate",      Mod, 0.01f, 40.0f, 0.5f, 0.3f, "Hz", true, Chaos, Medium) \
    AM_FLOAT  (X, chaos##n##Depth,     "mod.chaos" #n ".depth",     "Chaos " #n " Depth",     Mod, 0.0f, 1.0f, 0.5f, 1.0f, "", true, Chaos, Medium) \
    AM_FLOAT  (X, chaos##n##Stability, "mod.chaos" #n ".stability", "Chaos " #n " Stability", Mod, 0.0f, 1.0f, 0.5f, 1.0f, "", true, Chaos, Medium) \
    AM_FLOAT  (X, chaos##n##Symmetry,  "mod.chaos" #n ".symmetry",  "Chaos " #n " Symmetry",  Mod, 0.0f, 1.0f, 0.5f, 1.0f, "", true, Chaos, Medium) \
    AM_INT    (X, chaos##n##Seed,      "mod.chaos" #n ".seed",      "Chaos " #n " Seed",      Mod, 0.0f, 9999.0f, (float) (n * 17), "", false, Chaos)

#define AM_MACRO(X, n) \
    AM_FLOAT (X, macro##n, "macro." #n, "Macro " #n, Macro, 0.0f, 1.0f, 0.0f, 1.0f, "", false, None, Medium)

//------------------------------------------------------------------------------
#define ANTIMATR_PARAMETER_LIST(X) \
    /* ---------------------------------------------------------------- MASTER */ \
    AM_FLOAT  (X, masterGain,      "master.gain",      "Gain",         Master, -60.0f, 12.0f, 0.0f, 1.0f, "dB", true, None, Medium) \
    AM_CHOICE (X, masterVoices,    "master.voices",    "Voices",       Master, "8|16|32|64", 1.0f, None) \
    AM_CHOICE (X, masterQuality,   "master.quality",   "Quality",      Master, "ECO|NORMAL|HIGH|ULTRA", 1.0f, None) \
    AM_CHOICE (X, masterMode,      "master.mode",      "Voice Mode",   Master, "POLY|MONO|LEGATO", 0.0f, None) \
    AM_FLOAT  (X, masterGlide,     "master.glide",     "Glide",        Master, 0.0f, 2.0f, 0.0f, 0.4f, "s", false, Pitch, None) \
    AM_INT    (X, masterBendRange, "master.bendRange", "Bend Range",   Master, 0.0f, 24.0f, 2.0f, "st", false, None) \
    AM_INT    (X, masterTranspose, "master.transpose", "Transpose",    Master, -24.0f, 24.0f, 0.0f, "st", false, Pitch) \
    AM_FLOAT  (X, masterFine,      "master.fine",      "Fine Tune",    Master, -100.0f, 100.0f, 0.0f, 1.0f, "ct", true, Pitch, Medium) \
    /* ------------------------------------------------------------------- AMP */ \
    AM_FLOAT  (X, ampAttack,   "amp.attack",   "Attack",   Amp, 0.0f, 10.0f, 0.005f, 0.3f, "s", true, Movement, None) \
    AM_FLOAT  (X, ampDecay,    "amp.decay",    "Decay",    Amp, 0.0f, 10.0f, 0.25f,  0.3f, "s", true, Movement, None) \
    AM_FLOAT  (X, ampSustain,  "amp.sustain",  "Sustain",  Amp, 0.0f, 1.0f,  1.0f,   1.0f, "",  true, Movement, None) \
    AM_FLOAT  (X, ampRelease,  "amp.release",  "Release",  Amp, 0.0f, 20.0f, 0.4f,   0.3f, "s", true, Movement, None) \
    AM_FLOAT  (X, ampCurve,    "amp.curve",    "Curve",    Amp, 0.0f, 1.0f,  0.5f,   1.0f, "",  false, Movement, None) \
    AM_FLOAT  (X, ampVelocity, "amp.velocity", "Velocity", Amp, 0.0f, 1.0f,  0.6f,   1.0f, "",  false, None, None) \
    /* ---------------------------------------------------------------- SOURCE */ \
    AM_CHOICE (X, sourceSelected, "source.selected", "Source",      Source, "WAVE|DUST|IMPACT|SAMPLE|GESTURE", 0.0f, Source) \
    AM_CHOICE (X, sourceMode,     "source.mode",     "Source Mode", Source, "SINGLE|LAYER", 0.0f, Source) \
    /* WAVE */ \
    AM_UNI    (X, waveLevel,    "source.wave.level",    "Wave Level", Wave, 1.0f, Source) \
    AM_CHOICE (X, waveTable,    "source.wave.table",    "Wavetable",  Wave, "BASIC|HARMONIC|FORMANT|FOLDED|METALLIC|SPECTRAL|FRACTURED|NOISE", 0.0f, Source) \
    AM_UNI    (X, wavePosition, "source.wave.position", "Position",   Wave, 0.2f, Source) \
    AM_UNI    (X, waveScan,     "source.wave.scan",     "Scan",       Wave, 0.0f, Source) \
    AM_UNI    (X, waveMorph,    "source.wave.morph",    "Morph",      Wave, 0.0f, Source) \
    AM_FLOAT  (X, wavePhase,    "source.wave.phase",    "Phase",      Wave, 0.0f, 1.0f, 0.0f, 1.0f, "", false, Source, None) \
    AM_FLOAT  (X, wavePhaseRandom, "source.wave.phaseRandom", "Phase Random", Wave, 0.0f, 1.0f, 1.0f, 1.0f, "", false, Source, None) \
    AM_INT    (X, waveUnison,   "source.wave.unison",   "Unison",     Wave, 1.0f, 8.0f, 1.0f, "", false, Source) \
    AM_UNI    (X, waveDetune,   "source.wave.detune",   "Detune",     Wave, 0.15f, Source) \
    AM_UNI    (X, waveSpread,   "source.wave.spread",   "Spread",     Wave, 0.5f, Source) \
    AM_INT    (X, waveOctave,   "source.wave.octave",   "Octave",     Wave, -3.0f, 3.0f, 0.0f, "oct", false, Pitch) \
    AM_INT    (X, waveSemi,     "source.wave.semi",     "Semitone",   Wave, -12.0f, 12.0f, 0.0f, "st", false, Pitch) \
    AM_FLOAT  (X, waveFine,     "source.wave.fine",     "Fine",       Wave, -100.0f, 100.0f, 0.0f, 1.0f, "ct", true, Pitch, Medium) \
    AM_UNI    (X, waveFM,       "source.wave.fm",       "FM",         Wave, 0.0f, Source) \
    AM_UNI    (X, wavePM,       "source.wave.pm",       "PM",         Wave, 0.0f, Source) \
    AM_UNI    (X, waveAM,       "source.wave.am",       "AM",         Wave, 0.0f, Source) \
    AM_UNI    (X, waveRing,     "source.wave.ring",     "Ring",       Wave, 0.0f, Source) \
    AM_UNI    (X, waveSync,     "source.wave.sync",     "Sync",       Wave, 0.0f, Source) \
    AM_FLOAT  (X, waveModRatio, "source.wave.modRatio", "Mod Ratio",  Wave, 0.25f, 16.0f, 2.0f, 0.5f, "", true, Source, Medium) \
    /* DUST */ \
    AM_UNI    (X, dustLevel,    "source.dust.level",    "Dust Level", Dust, 1.0f, Source) \
    AM_CHOICE (X, dustMode,     "source.dust.mode",     "Dust Mode",  Dust, "WHITE|PINK|BROWN|BLUE|FILTERED|CRACKLE|IMPULSE|CLOUD|FROZEN", 0.0f, Source) \
    AM_UNI    (X, dustDensity,  "source.dust.density",  "Density",    Dust, 0.5f, Source) \
    AM_UNI    (X, dustColor,    "source.dust.color",    "Color",      Dust, 0.5f, Source) \
    AM_UNI    (X, dustGrain,    "source.dust.grain",    "Grain",      Dust, 0.3f, Source) \
    AM_UNI    (X, dustJitter,   "source.dust.jitter",   "Jitter",     Dust, 0.2f, Source) \
    AM_FLOAT  (X, dustPitch,    "source.dust.pitch",    "Pitch",      Dust, -24.0f, 24.0f, 0.0f, 1.0f, "st", true, Pitch, Medium) \
    AM_UNI    (X, dustPosition, "source.dust.position", "Position",   Dust, 0.0f, Source) \
    AM_UNI    (X, dustSpread,   "source.dust.spread",   "Spread",     Dust, 0.5f, Source) \
    AM_UNI    (X, dustStereo,   "source.dust.stereo",   "Stereo",     Dust, 0.5f, Source) \
    AM_INT    (X, dustSeed,     "source.dust.seed",     "Seed",       Dust, 0.0f, 9999.0f, 1.0f, "", false, Chaos) \
    /* IMPACT */ \
    AM_UNI    (X, impactLevel,      "source.impact.level",      "Impact Level", Impact, 1.0f, Source) \
    AM_CHOICE (X, impactMode,       "source.impact.mode",       "Impact Mode",  Impact, "IMPULSE|CLICK|PLUCK|NOISE STRIKE|METAL STRIKE|DAMPED SINE|MEMBRANE HIT", 2.0f, Source) \
    AM_UNI    (X, impactHardness,   "source.impact.hardness",   "Hardness",     Impact, 0.5f, Source) \
    AM_UNI    (X, impactBrightness, "source.impact.brightness", "Brightness",   Impact, 0.5f, Source) \
    AM_UNI    (X, impactLength,     "source.impact.length",     "Length",       Impact, 0.3f, Source) \
    AM_UNI    (X, impactVelocity,   "source.impact.velocity",   "Velocity",     Impact, 0.7f, Source) \
    AM_UNI    (X, impactCurve,      "source.impact.curve",      "Curve",        Impact, 0.5f, Source) \
    AM_UNI    (X, impactRandom,     "source.impact.random",     "Random",       Impact, 0.1f, Chaos) \
    AM_UNI    (X, impactRate,       "source.impact.rate",       "Repeat",       Impact, 0.0f, Source) \
    /* SAMPLE */ \
    AM_UNI    (X, sampleLevel,    "source.sample.level",    "Sample Level", Sample, 1.0f, Source) \
    AM_CHOICE (X, sampleMode,     "source.sample.mode",     "Sample Mode",  Sample, "ONE SHOT|LOOP|REVERSE|GRANULAR", 0.0f, Source) \
    AM_INT    (X, sampleRoot,     "source.sample.root",     "Root Note",    Sample, 0.0f, 127.0f, 60.0f, "", false, None) \
    AM_UNI    (X, sampleStart,    "source.sample.start",    "Start",        Sample, 0.0f, Source) \
    AM_UNI    (X, sampleEnd,      "source.sample.end",      "End",          Sample, 1.0f, Source) \
    AM_FLOAT  (X, samplePitch,    "source.sample.pitch",    "Pitch",        Sample, -24.0f, 24.0f, 0.0f, 1.0f, "st", true, Pitch, Medium) \
    AM_UNI    (X, sampleGrain,    "source.sample.grain",    "Grain",        Sample, 0.3f, Source) \
    AM_UNI    (X, sampleSpread,   "source.sample.spread",   "Spread",       Sample, 0.3f, Source) \
    AM_BOOL   (X, sampleKeytrack, "source.sample.keytrack", "Keytrack",     Sample, 1.0f, None) \
    /* GESTURE */ \
    AM_UNI    (X, gestureLevel,     "source.gesture.level",     "Gesture Level", Gesture, 1.0f, Source) \
    AM_CHOICE (X, gestureMode,      "source.gesture.mode",      "Gesture Mode",  Gesture, "BOW|SCRAPE|RUB|BREATH|FRICTION|ELECTRICAL", 0.0f, Source) \
    AM_UNI    (X, gesturePressure,  "source.gesture.pressure",  "Pressure",      Gesture, 0.5f, Source) \
    AM_UNI    (X, gestureSpeed,     "source.gesture.speed",     "Speed",         Gesture, 0.5f, Source) \
    AM_UNI    (X, gestureRoughness, "source.gesture.roughness", "Roughness",     Gesture, 0.3f, Source) \
    AM_UNI    (X, gesturePosition,  "source.gesture.position",  "Position",      Gesture, 0.3f, Source) \
    AM_UNI    (X, gestureMotion,    "source.gesture.motion",    "Motion",        Gesture, 0.2f, Movement) \
    AM_UNI    (X, gestureBandwidth, "source.gesture.bandwidth", "Bandwidth",     Gesture, 0.5f, Source) \
    /* ----------------------------------------------------------------- SHAPE */ \
    AM_UNI    (X, shapeDensity,   "shape.density",   "Density", Shape, 0.5f, Shape) \
    AM_UNI    (X, shapeForm,      "shape.form",      "Form",    Shape, 0.3f, Shape) \
    AM_UNI    (X, shapeMass,      "shape.mass",      "Mass",    Shape, 0.4f, Shape) \
    AM_UNI    (X, shapeTension,   "shape.tension",   "Tension", Shape, 0.5f, Shape) \
    AM_UNI    (X, shapeDecay,     "shape.decay",     "Decay",   Shape, 0.5f, Shape) \
    AM_UNI    (X, shapeSurface,   "shape.surface",   "Surface", Shape, 0.2f, Shape) \
    AM_CHOICE (X, shapeMaterialA, "shape.materialA", "Material A", Shape, "CRYSTAL|METAL|ORGANIC|LIQUID|MEMBRANE|STRING|WOOD|VOID|CUSTOM", 0.0f, Shape) \
    AM_CHOICE (X, shapeMaterialB, "shape.materialB", "Material B", Shape, "CRYSTAL|METAL|ORGANIC|LIQUID|MEMBRANE|STRING|WOOD|VOID|CUSTOM", 1.0f, Shape) \
    AM_UNI    (X, shapeBlend,     "shape.blend",     "Blend",   Shape, 0.0f, Shape) \
    AM_CHOICE (X, shapeTopology,  "shape.topology",  "Topology", Shape, "CHAIN|RING|CLUSTERS|LATTICE|RANDOM|STAR", 2.0f, Shape) \
    AM_UNI    (X, shapeCoupling,  "shape.coupling",  "Coupling", Shape, 0.3f, Shape) \
    AM_UNI    (X, shapeDistribution, "shape.distribution", "Distribution", Shape, 0.5f, Shape) \
    AM_UNI    (X, shapeMix,       "shape.mix",       "Matter Mix", Shape, 1.0f, Shape) \
    AM_UNI    (X, shapeKeytrack,  "shape.keytrack",  "Keytrack", Shape, 1.0f, Pitch) \
    AM_FLOAT  (X, shapePitch,     "shape.pitch",     "Matter Pitch", Shape, -24.0f, 24.0f, 0.0f, 1.0f, "st", true, Pitch, Medium) \
    AM_UNI    (X, shapeStereo,    "shape.stereo",    "Stereo",  Shape, 0.6f, Shape) \
    AM_INT    (X, shapeSeed,      "shape.seed",      "Topology Seed", Shape, 0.0f, 9999.0f, 7.0f, "", false, Chaos) \
    AM_UNI    (X, shapeExcite,    "shape.excite",    "Excite",  Shape, 0.6f, Shape) \
    AM_UNI    (X, shapeStrike,    "shape.strike",    "Strike",  Shape, 0.5f, Shape) \
    /* ---------------------------------------------------------------- EVOLVE */ \
    AM_UNI    (X, evolveBend,     "evolve.bend",     "Bend",    Evolve, 0.0f, Evolve) \
    AM_UNI    (X, evolveMelt,     "evolve.melt",     "Melt",    Evolve, 0.0f, Evolve) \
    AM_UNI    (X, evolveTear,     "evolve.tear",     "Tear",    Evolve, 0.0f, Evolve) \
    AM_UNI    (X, evolveMagnet,   "evolve.magnet",   "Magnet",  Evolve, 0.0f, Evolve) \
    AM_UNI    (X, evolveGravity,  "evolve.gravity",  "Gravity", Evolve, 0.5f, Evolve) \
    AM_UNI    (X, evolveScatter,  "evolve.scatter",  "Scatter", Evolve, 0.0f, Chaos) \
    AM_BOOL   (X, evolveFreeze,   "evolve.freeze",   "Freeze",  Evolve, 0.0f, None) \
    AM_UNI    (X, evolveCrush,    "evolve.crush",    "Crush",   Evolve, 0.0f, Evolve) \
    AM_UNI    (X, evolveSpeed,    "evolve.speed",    "Speed",   Evolve, 0.3f, Movement) \
    AM_UNI    (X, evolveMotion,   "evolve.motion",   "Motion",  Evolve, 0.25f, Movement) \
    AM_CHOICE (X, evolveSelected, "evolve.selected", "Selected Operator", Evolve, "BEND|MELT|TEAR|MAGNET", 0.0f, None) \
    AM_UNI    (X, evolveBendPivot, "evolve.bendPivot", "Bend Pivot", Evolve, 0.5f, Evolve) \
    AM_UNI    (X, evolveBendRange, "evolve.bendRange", "Bend Range", Evolve, 0.5f, Evolve) \
    AM_UNI    (X, evolveBendCurve, "evolve.bendCurve", "Bend Curve", Evolve, 0.5f, Evolve) \
    AM_CHOICE (X, evolveMagnetTarget, "evolve.magnetTarget", "Magnet Target", Evolve, "OCTAVE|FIFTH|MAJOR|MINOR|CHROMATIC|SCALE|CUSTOM", 1.0f, Evolve) \
    AM_INT    (X, evolveScatterSeed, "evolve.scatterSeed", "Scatter Seed", Evolve, 0.0f, 9999.0f, 3.0f, "", false, Chaos) \
    /* -------------------------------------------------------------- FRACTURE */ \
    AM_BOOL   (X, fractureOn,        "fracture.on",        "Fracture On", Fracture, 0.0f, Fracture) \
    AM_CHOICE (X, fractureMode,      "fracture.mode",      "Mode",        Fracture, "SPECTRAL|RHYTHMIC|TRANSIENT|EVOLVE", 0.0f, Fracture) \
    AM_UNI    (X, fractureAmount,    "fracture.amount",    "Amount",      Fracture, 0.5f, Fracture) \
    AM_UNI    (X, fractureSpread,    "fracture.spread",    "Spread",      Fracture, 0.3f, Fracture) \
    AM_UNI    (X, fractureSequence,  "fracture.sequence",  "Sequence",    Fracture, 0.5f, Fracture) \
    AM_UNI    (X, fractureRandom,    "fracture.random",    "Random",      Fracture, 0.2f, Chaos) \
    AM_CHOICE (X, fractureFragments, "fracture.fragments", "Fragments",   Fracture, "8|16|32", 1.0f, Fracture) \
    AM_UNI    (X, fractureFeedback,  "fracture.feedback",  "Feedback",    Fracture, 0.2f, Fracture) \
    AM_FLOAT  (X, fracturePitch,     "fracture.pitch",     "Pitch",       Fracture, -24.0f, 24.0f, 0.0f, 1.0f, "st", true, Pitch, Medium) \
    AM_UNI    (X, fractureDelay,     "fracture.delay",     "Delay",       Fracture, 0.3f, Fracture) \
    AM_UNI    (X, fractureDecay,     "fracture.decay",     "Decay",       Fracture, 0.5f, Fracture) \
    AM_UNI    (X, fractureMix,       "fracture.mix",       "Mix",         Fracture, 1.0f, Fracture) \
    AM_UNI    (X, fractureTone,      "fracture.tone",      "Tone",        Fracture, 0.5f, Fracture) \
    AM_FLOAT  (X, fractureRate,      "fracture.rate",      "Rate",        Fracture, 0.05f, 40.0f, 2.0f, 0.3f, "Hz", true, Movement, Medium) \
    AM_BOOL   (X, fractureSync,      "fracture.sync",      "Sync",        Fracture, 1.0f, None) \
    AM_CHOICE (X, fractureDivision,  "fracture.division",  "Division",    Fracture, "1/1|1/2|1/4|1/8|1/16|1/32|1/4T|1/8T|1/16T|1/4D|1/8D|1/16D", 3.0f, Movement) \
    AM_INT    (X, fractureSteps,     "fracture.steps",     "Steps",       Fracture, 1.0f, 32.0f, 8.0f, "", false, Fracture) \
    AM_UNI    (X, fractureSwing,     "fracture.swing",     "Swing",       Fracture, 0.0f, Movement) \
    AM_CHOICE (X, fractureDirection, "fracture.direction", "Direction",   Fracture, "FORWARD|BACKWARD|PINGPONG|RANDOM", 0.0f, Movement) \
    AM_UNI    (X, fractureProbability, "fracture.probability", "Probability", Fracture, 1.0f, Chaos) \
    AM_INT    (X, fractureSeed,      "fracture.seed",      "Seed",        Fracture, 0.0f, 9999.0f, 11.0f, "", false, Chaos) \
    AM_UNI    (X, fractureEvolve,    "fracture.evolve",    "Evolve",      Fracture, 0.0f, Fracture) \
    AM_BOOL   (X, fractureRetrig,    "fracture.retrig",    "Retrigger",   Fracture, 1.0f, None) \
    /* ----------------------------------------------------------------- SPACE */ \
    AM_CHOICE (X, spaceType,     "space.type",     "Space",    Space, "NEBULA|VOID|CHAMBER|ORBIT|DREAM|MACHINE|SHIMMER|DUST", 0.0f, Space) \
    AM_UNI    (X, spaceMix,      "space.mix",      "Mix",      Space, 0.35f, Space) \
    AM_UNI    (X, spaceSize,     "space.size",     "Size",     Space, 0.5f, Space) \
    AM_UNI    (X, spaceTone,     "space.tone",     "Tone",     Space, 0.5f, Space) \
    AM_UNI    (X, spaceFeedback, "space.feedback", "Feedback", Space, 0.3f, Space) \
    AM_BOOL   (X, spaceDistOn,     "space.dist.on",     "Distortion On", Space, 0.0f, Space) \
    AM_CHOICE (X, spaceDistMode,   "space.dist.mode",   "Distortion Mode", Space, "SOFT|TUBE|FOLD|CRUSH|TAPE", 0.0f, Space) \
    AM_UNI    (X, spaceDistDrive,  "space.dist.drive",  "Drive",         Space, 0.3f, Space) \
    AM_UNI    (X, spaceDistMix,    "space.dist.mix",    "Distortion Mix", Space, 1.0f, Space) \
    AM_BOOL   (X, spaceChorusOn,   "space.chorus.on",   "Chorus On",     Space, 0.0f, Space) \
    AM_FLOAT  (X, spaceChorusRate, "space.chorus.rate", "Chorus Rate",   Space, 0.05f, 10.0f, 0.4f, 0.4f, "Hz", true, Space, Medium) \
    AM_UNI    (X, spaceChorusDepth,"space.chorus.depth","Chorus Depth",  Space, 0.4f, Space) \
    AM_UNI    (X, spaceChorusMix,  "space.chorus.mix",  "Chorus Mix",    Space, 0.5f, Space) \
    AM_BOOL   (X, spaceDelayOn,    "space.delay.on",    "Delay On",      Space, 0.0f, Space) \
    AM_UNI    (X, spaceDelayTime,  "space.delay.time",  "Delay Time",    Space, 0.4f, Space) \
    AM_BOOL   (X, spaceDelaySync,  "space.delay.sync",  "Delay Sync",    Space, 1.0f, None) \
    AM_UNI    (X, spaceDelayFeedback, "space.delay.feedback", "Delay Feedback", Space, 0.4f, Space) \
    AM_UNI    (X, spaceDelayTone,  "space.delay.tone",  "Delay Tone",    Space, 0.5f, Space) \
    AM_UNI    (X, spaceDelayMix,   "space.delay.mix",   "Delay Mix",     Space, 0.3f, Space) \
    AM_BOOL   (X, spaceGrainOn,    "space.grain.on",    "Granular On",   Space, 0.0f, Space) \
    AM_UNI    (X, spaceGrainSize,  "space.grain.size",  "Grain Size",    Space, 0.4f, Space) \
    AM_UNI    (X, spaceGrainDensity, "space.grain.density", "Grain Density", Space, 0.5f, Space) \
    AM_FLOAT  (X, spaceGrainPitch, "space.grain.pitch", "Grain Pitch",   Space, -24.0f, 24.0f, 0.0f, 1.0f, "st", true, Pitch, Medium) \
    AM_UNI    (X, spaceGrainMix,   "space.grain.mix",   "Granular Mix",  Space, 0.3f, Space) \
    AM_BOOL   (X, spaceShiftOn,    "space.shift.on",    "Shift On",      Space, 0.0f, Space) \
    AM_FLOAT  (X, spaceShiftAmount,"space.shift.amount","Shift",         Space, -1.0f, 1.0f, 0.0f, 1.0f, "", true, Space, Medium) \
    AM_UNI    (X, spaceShiftMix,   "space.shift.mix",   "Shift Mix",     Space, 0.3f, Space) \
    AM_BOOL   (X, spaceDiffuseOn,  "space.diffuse.on",  "Diffusion On",  Space, 0.0f, Space) \
    AM_UNI    (X, spaceDiffuseAmount, "space.diffuse.amount", "Diffusion", Space, 0.5f, Space) \
    AM_BOOL   (X, spaceReverbOn,   "space.reverb.on",   "Reverb On",     Space, 1.0f, Space) \
    AM_UNI    (X, spaceReverbSize, "space.reverb.size", "Reverb Size",   Space, 0.6f, Space) \
    AM_UNI    (X, spaceReverbDecay,"space.reverb.decay","Reverb Decay",  Space, 0.5f, Space) \
    AM_UNI    (X, spaceReverbDamp, "space.reverb.damp", "Reverb Damping",Space, 0.4f, Space) \
    AM_FLOAT  (X, spaceReverbPredelay, "space.reverb.predelay", "Pre-delay", Space, 0.0f, 250.0f, 10.0f, 0.5f, "ms", false, Space, None) \
    AM_UNI    (X, spaceReverbMod,  "space.reverb.mod",  "Reverb Mod",    Space, 0.2f, Space) \
    AM_UNI    (X, spaceReverbMix,  "space.reverb.mix",  "Reverb Mix",    Space, 0.35f, Space) \
    AM_FLOAT  (X, spaceEqLow,      "space.eq.low",      "EQ Low",        Space, -12.0f, 12.0f, 0.0f, 1.0f, "dB", true, Space, Medium) \
    AM_FLOAT  (X, spaceEqMid,      "space.eq.mid",      "EQ Mid",        Space, -12.0f, 12.0f, 0.0f, 1.0f, "dB", true, Space, Medium) \
    AM_FLOAT  (X, spaceEqHigh,     "space.eq.high",     "EQ High",       Space, -12.0f, 12.0f, 0.0f, 1.0f, "dB", true, Space, Medium) \
    AM_BOOL   (X, spaceCompOn,     "space.comp.on",     "Compressor On", Space, 0.0f, Space) \
    AM_UNI    (X, spaceCompAmount, "space.comp.amount", "Compression",   Space, 0.3f, Space) \
    AM_BOOL   (X, spaceLimiterOn,  "space.limiter.on",  "Limiter On",    Space, 1.0f, None) \
    /* ------------------------------------------------------------------- MOD */ \
    AM_LFO (X, 1) AM_LFO (X, 2) AM_LFO (X, 3) AM_LFO (X, 4) \
    AM_ENV (X, 1) AM_ENV (X, 2) AM_ENV (X, 3) AM_ENV (X, 4) \
    AM_CHAOS (X, 1) AM_CHAOS (X, 2) AM_CHAOS (X, 3) AM_CHAOS (X, 4) \
    AM_MACRO (X, 1) AM_MACRO (X, 2) AM_MACRO (X, 3) AM_MACRO (X, 4) \
    AM_MACRO (X, 5) AM_MACRO (X, 6) AM_MACRO (X, 7) AM_MACRO (X, 8)
