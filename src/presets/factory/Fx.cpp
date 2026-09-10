#include "presets/FactoryBuilders.h"

namespace am::FactoryContent
{

void registerFx (PresetManager& manager)
{
//==========================================================================
// FX — sound design, not notes
//==========================================================================

manager.addFactory ({ "Fractured Voice", "FX", { "fx", "vocal", "spectral", "fracture" }, [] (PatchState& s)
{
    gesture (s, 3 /* BREATH */, 0.72f, 0.45f, 0.26f, 0.55f, 0.35f, 0.48f);
    amp (s, 0.15f, 1.0f, 0.75f, 1.20f, 0.5f);
    shape (s, 0.58f, 0.14f, 0.44f, 0.58f, 0.74f, 0.30f);
    material (s, MaterialType::Organic, MaterialType::Membrane, 0.45f);
    topology (s, 2 /* CLUSTERS */, 0.50f, 0.52f, 601);
    matter (s, 0.94f, 0.40f, 0.30f, 0.75f);
    evolve (s, 0.0f, 0.18f, 0.34f, 0.24f, 0.48f, 0.26f, 0.0f, 0.32f, 0.35f);
    set (s, Param::evolveScatterSeed, 1483);
    fracture (s, 0 /* SPECTRAL */, 0.62f, 0.70f, 0.62f, 0.45f, 0.40f, 0.55f, 0.62f, 0.48f, 0.25f,
              1 /* 16 */, 3 /* 1/8 */, 8, 0.15f, 2 /* PINGPONG */, 0.90f, 0.35f, 887,
              FractureShape { 16, 0.12f, 0.72f, 0.25f, 0.60f, 0.50f, 0.80f, 0.30f, 0.95f,
                              1.0f, 1.0f, 0.75f, 0.95f, kMinorTerrace, "XLHXoLHX", nullptr });
    space (s, SpacePresets::Dream, 0.48f, 0.68f, 0.58f, 0.40f);

    lfo (s, 1, 0.28f, 5 /* SMOOTH RANDOM */, 1.0f, true);
    chaos (s, 1, 4 /* TARGETS */, 0.80f, 0.55f, 0.55f, 0.5f, 1543);
    env (s, 2, 0.60f, 2.0f, 0.55f, 2.0f);
    macros (s, 0.45f, 0.45f, 0.45f, 0.55f);

    Routings r;
    r.bi  (ModSource::LFO1,   Param::gesturePosition, 0.180f)
     .bi  (ModSource::Chaos1, Param::fracturePitch,  0.070f)
     .uni (ModSource::Env2,   Param::fractureSpread, 0.250f)
     .uni (ModSource::Velocity, Param::gesturePressure, 0.250f)
     .bi  (ModSource::NoteRandom, Param::fractureSequence, 0.150f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.350f)
     .uni (ModSource::Macro1, Param::fractureEvolve, 0.300f)
     .uni (ModSource::Macro2, Param::fractureTone,   0.300f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.280f)
     .uni (ModSource::Macro4, Param::fractureAmount, 0.300f)
     .uni (ModSource::Macro4, Param::fractureFeedback, 0.200f);
    sharedMacros (r, Param::fractureDecay, Param::fractureRandom);
    r.commit (s);
}});

manager.addFactory ({ "Reactor Fault", "FX", { "fx", "noise", "unstable", "machine" }, [] (PatchState& s)
{
    gesture (s, 5 /* ELECTRICAL */, 0.72f, 0.78f, 0.72f, 0.25f, 0.62f, 0.70f);
    amp (s, 0.05f, 0.80f, 0.70f, 0.90f, 0.45f);
    shape (s, 0.72f, 0.94f, 0.28f, 0.72f, 0.46f, 0.62f);
    material (s, MaterialType::Metal, MaterialType::Void, 0.55f);
    topology (s, 4 /* RANDOM */, 0.72f, 0.48f, 653);
    matter (s, 0.86f, 0.78f, 0.42f, 0.85f);
    evolve (s, 0.0f, 0.0f, 0.46f, 0.0f, 0.44f, 0.62f, 0.30f, 0.70f, 0.60f);
    set (s, Param::evolveScatterSeed, 1601);
    fracture (s, 1 /* RHYTHMIC */, 0.58f, 0.60f, 0.70f, 0.62f, 0.45f, 0.45f, 0.55f, 0.60f, 0.40f,
              2 /* 32 */, 4 /* 1/16 */, 12, 0.20f, 3 /* RANDOM */, 0.75f, 0.55f, 1699,
              FractureShape { 32, 0.04f, 0.55f, 0.30f, 0.62f, 0.35f, 0.70f, 0.35f, 1.0f,
                              1.0f, 1.0f, 0.85f, 0.85f, kFallingTerrace, "XoH.XLo.", nullptr });
    space (s, SpacePresets::Machine, 0.40f, 0.45f, 0.55f, 0.45f);

    lfo (s, 1, 1.40f, 4 /* RANDOM */, 1.0f, true);
    chaos (s, 1, 2 /* LOGISTIC */, 6.20f, 0.65f, 0.40f, 0.5f, 1777);
    chaos (s, 2, 3 /* LORENZ */, 0.90f, 0.55f, 0.55f, 0.5f, 1811);
    macros (s, 0.60f, 0.50f, 0.35f, 0.55f);

    Routings r;
    r.bi  (ModSource::LFO1,   Param::gestureRoughness, 0.200f)
     .bi  (ModSource::Chaos1, Param::evolveScatter,  0.180f)
     .bi  (ModSource::Chaos2, Param::fractureSpread, 0.200f)
     .uni (ModSource::Velocity, Param::evolveCrush,  0.200f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.400f)
     .uni (ModSource::Macro1, Param::fractureRandom, 0.300f)
     .uni (ModSource::Macro2, Param::fractureTone,   0.300f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.300f)
     .uni (ModSource::Macro4, Param::evolveTear,     0.300f)
     .uni (ModSource::Macro4, Param::evolveCrush,    0.250f);
    sharedMacros (r, Param::fractureDecay, Param::fractureRandom);
    r.commit (s);
}});

manager.addFactory ({ "Signal Decay", "FX", { "fx", "granular", "collapse", "sample" }, [] (PatchState& s)
{
    sample (s, BuiltInSamples::Kind::NoiseBurst, 3 /* GRANULAR */, 0.0f, 0.70f, 0.52f, 0.75f);
    amp (s, 0.02f, 1.60f, 0.45f, 2.20f, 0.4f);
    shape (s, 0.64f, 0.78f, 0.34f, 0.52f, 0.62f, 0.40f);
    material (s, MaterialType::Void, MaterialType::Crystal, 0.50f);
    topology (s, 3 /* LATTICE */, 0.54f, 0.48f, 719);
    matter (s, 0.90f, 0.70f, 0.35f, 0.88f);
    evolve (s, 0.0f, 0.40f, 0.0f, 0.18f, 0.34f, 0.34f, 0.24f, 0.28f, 0.45f);
    set (s, Param::evolveScatterSeed, 1873);
    fracture (s, 3 /* EVOLVE */, 0.55f, 0.62f, 0.55f, 0.55f, 0.42f, 0.62f, 0.70f, 0.42f, 0.65f,
              1 /* 16 */, 2 /* 1/4 */, 8, 0.0f, 1 /* BACKWARD */, 0.95f, 0.40f, 1907,
              FractureShape { 16, 0.20f, 0.85f, 0.30f, 0.65f, 0.55f, 0.85f, 0.35f, 1.0f,
                              1.0f, 0.9f, 0.7f, 1.0f, kFallingTerrace, "XXLLHHoo", nullptr });
    space (s, SpacePresets::Dust, 0.52f, 0.68f, 0.45f, 0.45f);

    lfo (s, 1, 0.24f, 2 /* SAW */, 1.0f, true);
    env (s, 2, 0.05f, 3.0f, 0.20f, 3.0f, 0.3f);
    chaos (s, 1, 1 /* BROWNIAN */, 0.45f, 0.60f, 0.70f, 0.5f, 1949);
    macros (s, 0.50f, 0.35f, 0.45f, 0.60f);

    Routings r;
    r.bi  (ModSource::LFO1,   Param::sampleStart,    0.150f)
     .uni (ModSource::Env2,   Param::evolveMelt,     0.250f)
     .bi  (ModSource::Chaos1, Param::fracturePitch,  0.080f)
     .uni (ModSource::Velocity, Param::sampleGrain,  0.250f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.400f)
     .uni (ModSource::Macro1, Param::fractureEvolve, 0.300f)
     .uni (ModSource::Macro2, Param::fractureTone,   0.300f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.280f)
     .uni (ModSource::Macro4, Param::evolveMelt,     0.300f)
     .uni (ModSource::Macro4, Param::evolveCrush,    0.250f);
    sharedMacros (r, Param::fractureDecay, Param::sampleSpread);
    r.commit (s);
}});

manager.addFactory ({ "Ascension Wire", "FX", { "metallic", "bright", "evolving", "wide", "transition" }, [] (PatchState& s)
{
    gesture (s, 0 /* BOW */, 0.58f, 0.52f, 0.22f, 0.34f, 0.42f, 0.55f);
    amp (s, 0.30f, 2.50f, 0.88f, 0.90f, 0.62f);
    shape (s, 0.52f, 0.18f, 0.30f, 0.72f, 0.68f, 0.28f);
    material (s, MaterialType::String, MaterialType::Metal, 0.45f);
    topology (s, 0 /* CHAIN */, 0.38f, 0.55f, 3001);
    matter (s, 0.95f, 0.62f, 0.18f, 0.72f);
    evolve (s, 0.34f, 0.0f, 0.0f, 0.22f, 0.40f, 0.14f, 0.0f, 0.45f, 0.42f);
    set (s, Param::evolveBendPivot, 0.30f);
    set (s, Param::evolveBendRange, 0.62f);
    space (s, SpacePresets::Shimmer, 0.48f, 0.72f, 0.62f, 0.45f);

    env (s, 2, 2.20f, 1.50f, 1.0f, 0.60f, 0.65f);      // the climb: two seconds of lift
    lfo (s, 1, 0.22f, 0 /* SINE */, 1.0f, true, 0.8f);
    macros (s, 0.45f, 0.50f, 0.45f, 0.50f);

    Routings r;
    r.uni (ModSource::Env2,   Param::shapePitch,     0.240f)
     .uni (ModSource::Env2,   Param::gestureSpeed,   0.300f)
     .uni (ModSource::Env2,   Param::spaceMix,       0.180f)
     .uni (ModSource::Env2,   Param::evolveBend,     0.200f)
     .bi  (ModSource::LFO1,   Param::gesturePosition, 0.160f)
     .uni (ModSource::Velocity, Param::gesturePressure, 0.280f)
     .bi  (ModSource::KeyTrack, Param::shapeDecay,  -0.150f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.350f)
     .uni (ModSource::Macro2, Param::gestureBandwidth, 0.300f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.280f)
     .uni (ModSource::Macro4, Param::evolveBend,     0.300f)
     .uni (ModSource::Macro4, Param::shapeTension,   0.200f);
    sharedMacros (r, Param::ampDecay, Param::evolveMotion);
    r.commit (s);
}});

manager.addFactory ({ "Terminal Descent", "FX", { "dark", "morphing", "huge", "low", "transition" }, [] (PatchState& s)
{
    wave (s, 5 /* SPECTRAL */, 0.44f, 0.36f, 0.12f, 3, 0.20f, 0.62f, -1, 0.80f);
    impact (s, 6 /* MEMBRANE HIT */, 0.34f, 0.30f, 0.30f, 0.90f, 0.40f, 0.15f, 0.0f, 0.55f);
    set (s, Param::sourceSelected, 0 /* WAVE carries the fall */);
    set (s, Param::sourceMode, 1 /* LAYER */);
    set (s, Param::dustLevel, 0.0f);
    set (s, Param::sampleLevel, 0.0f);
    set (s, Param::gestureLevel, 0.0f);
    amp (s, 0.004f, 3.00f, 0.55f, 1.40f, 0.40f);
    set (s, Param::masterGain, -4.5f);
    shape (s, 0.46f, 0.62f, 0.74f, 0.34f, 0.72f, 0.24f);
    material (s, MaterialType::Void, MaterialType::Wood, 0.42f);
    topology (s, 5 /* STAR */, 0.30f, 0.45f, 3011);
    matter (s, 0.88f, 0.55f, 0.48f, 0.62f);
    evolve (s, 0.0f, 0.30f, 0.0f, 0.0f, 0.80f, 0.20f, 0.0f, 0.28f, 0.30f);
    set (s, Param::evolveScatterSeed, 3013);
    space (s, SpacePresets::Void, 0.52f, 0.86f, 0.34f, 0.50f);

    env (s, 2, 2.20f, 0.50f, 1.0f, 2.00f, 0.45f);      // a two second glide down, and it keeps going
    lfo (s, 1, 0.16f, 5 /* SMOOTH RANDOM */, 1.0f, true);
    macros (s, 0.40f, 0.35f, 0.55f, 0.50f);

    Routings r;
    r.uni (ModSource::Env2,   Param::shapePitch,    -0.240f)
     .uni (ModSource::Env2,   Param::evolveMelt,     0.250f)
     .uni (ModSource::Env2,   Param::spaceSize,      0.150f)
     .bi  (ModSource::LFO1,   Param::waveScan,       0.180f)
     .uni (ModSource::Velocity, Param::impactHardness, 0.300f)
     .bi  (ModSource::NoteRandom, Param::wavePosition, 0.100f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.350f)
     .uni (ModSource::Macro2, Param::waveMorph,      0.300f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.300f)
     .uni (ModSource::Macro4, Param::evolveGravity,  0.180f)
     .uni (ModSource::Macro4, Param::shapeMass,      0.250f);
    sharedMacros (r, Param::ampDecay, Param::evolveMelt);
    r.commit (s);
}});

manager.addFactory ({ "Concrete Impact", "FX", { "dark", "wooden", "impact", "low", "roomy" }, [] (PatchState& s)
{
    impact (s, 6 /* MEMBRANE HIT */, 0.30f, 0.22f, 0.30f, 0.95f, 0.35f, 0.12f, 0.0f, 0.95f);
    amp (s, 0.001f, 1.20f, 0.0f, 0.90f, 0.25f);
    set (s, Param::masterGain, -5.5f);   // a slab landing is all transient: keep the headroom
    shape (s, 0.30f, 0.50f, 0.86f, 0.24f, 0.55f, 0.18f);
    material (s, MaterialType::Wood, MaterialType::Membrane, 0.55f);
    topology (s, 2 /* CLUSTERS */, 0.28f, 0.42f, 3019);
    matter (s, 1.0f, 0.50f, 0.78f, 0.50f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.20f, 0.68f, 0.10f, 0.0f, 0.20f, 0.12f);
    set (s, Param::evolveMagnetTarget, 0 /* OCTAVE */);
    space (s, SpacePresets::Chamber, 0.44f, 0.58f, 0.36f, 0.32f);
    set (s, Param::spaceEqLow, 4.0f);

    env (s, 1, 0.001f, 0.30f, 0.0f, 0.25f, 0.25f);
    lfo (s, 1, 0.30f, 1 /* TRIANGLE */, 1.0f, true);
    macros (s, 0.20f, 0.35f, 0.50f, 0.55f);

    Routings r;
    r.uni (ModSource::Env1,   Param::shapeSurface,   0.220f)
     .uni (ModSource::Env1,   Param::evolveGravity,  0.120f)
     .bi  (ModSource::LFO1,   Param::spaceTone,      0.120f)
     .uni (ModSource::Velocity, Param::impactHardness, 0.400f)
     .uni (ModSource::Velocity, Param::shapeStrike,  0.250f)
     .bi  (ModSource::KeyTrack, Param::shapeDecay,  -0.250f)
     .bi  (ModSource::NoteRandom, Param::impactRandom, 0.100f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.300f)
     .uni (ModSource::Macro2, Param::impactBrightness, 0.350f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.280f)
     .uni (ModSource::Macro4, Param::shapeMass,      0.300f)
     .uni (ModSource::Macro4, Param::spaceSize,      0.220f);
    sharedMacros (r, Param::ampDecay, Param::impactRandom);
    r.commit (s);
}});

manager.addFactory ({ "Glass Detonation", "FX", { "glassy", "bright", "chaotic", "impact", "wide" }, [] (PatchState& s)
{
    impact (s, 4 /* METAL STRIKE */, 0.88f, 0.90f, 0.12f, 0.90f, 0.30f, 0.35f, 0.0f, 0.85f);
    amp (s, 0.001f, 1.60f, 0.06f, 1.40f, 0.30f);
    set (s, Param::masterGain, -3.0f);
    shape (s, 0.72f, 0.78f, 0.20f, 0.80f, 0.62f, 0.42f);
    material (s, MaterialType::Crystal, MaterialType::Metal, 0.35f);
    topology (s, 3 /* LATTICE */, 0.55f, 0.60f, 3023);
    matter (s, 0.95f, 0.80f, 0.85f, 0.90f);
    evolve (s, 0.0f, 0.0f, 0.30f, 0.15f, 0.44f, 0.50f, 0.0f, 0.55f, 0.45f);
    set (s, Param::evolveScatterSeed, 3027);
    fracture (s, 2 /* TRANSIENT */, 0.55f, 0.60f, 0.80f, 0.40f, 0.35f, 0.35f, 0.30f, 0.68f, 0.25f,
              2 /* 32 */, 5 /* 1/32 */, 16, 0.0f, 3 /* RANDOM */, 0.55f, 0.50f, 3029,
              FractureShape { 32, 0.02f, 0.50f, 0.20f, 0.50f, 0.25f, 0.55f, 0.50f, 1.0f,
                              1.0f, 1.0f, 0.95f, 0.60f, kOctaveTerrace, "XHoXHoXH", nullptr });
    space (s, SpacePresets::Orbit, 0.40f, 0.46f, 0.68f, 0.45f);

    env (s, 1, 0.001f, 0.45f, 0.0f, 0.35f, 0.25f);
    chaos (s, 1, 0 /* WALK */, 4.50f, 0.55f, 0.35f, 0.5f, 3031);
    macros (s, 0.50f, 0.55f, 0.40f, 0.60f);

    Routings r;
    r.uni (ModSource::Env1,   Param::fractureAmount, 0.200f)
     .bi  (ModSource::Chaos1, Param::fractureSpread, 0.250f)
     .bi  (ModSource::Chaos1, Param::evolveScatter,  0.150f)
     .uni (ModSource::Velocity, Param::impactBrightness, 0.350f)
     .uni (ModSource::Velocity, Param::fractureAmount, 0.150f)
     .bi  (ModSource::NoteRandom, Param::fractureProbability, -0.150f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.350f)
     .uni (ModSource::Macro1, Param::fractureRandom, 0.250f)
     .uni (ModSource::Macro2, Param::fractureTone,   0.300f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.300f)
     .uni (ModSource::Macro4, Param::evolveTear,     0.300f)
     .uni (ModSource::Macro4, Param::fractureSpread, 0.250f);
    sharedMacros (r, Param::fractureDecay, Param::fractureProbability);
    r.commit (s);
}});

manager.addFactory ({ "Air Slice", "FX", { "cold", "noisy", "wide", "air", "transition" }, [] (PatchState& s)
{
    dust (s, 4 /* FILTERED */, 0.85f, 0.55f, 0.24f, 0.35f, 0.90f, 0.95f, 3037);
    amp (s, 0.16f, 0.90f, 0.42f, 0.50f, 0.68f);
    set (s, Param::masterGain, 2.5f);
    shape (s, 0.34f, 0.90f, 0.25f, 0.50f, 0.30f, 0.32f);
    material (s, MaterialType::Void, MaterialType::Liquid, 0.45f);
    topology (s, 4 /* RANDOM */, 0.24f, 0.62f, 3041);
    matter (s, 0.45f, 0.60f, 0.20f, 0.95f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.38f, 0.35f, 0.0f, 0.70f, 0.55f);
    set (s, Param::evolveScatterSeed, 3043);
    space (s, SpacePresets::Nebula, 0.50f, 0.62f, 0.62f, 0.35f);

    env (s, 2, 0.35f, 0.70f, 0.0f, 0.45f, 0.70f);      // the blade: one pass of the filter
    lfo (s, 1, 0.85f, 5 /* SMOOTH RANDOM */, 1.0f, true);
    macros (s, 0.55f, 0.50f, 0.50f, 0.45f);

    Routings r;
    r.uni (ModSource::Env2,   Param::dustColor,      0.450f)
     .uni (ModSource::Env2,   Param::dustDensity,    0.200f)
     .uni (ModSource::Env2,   Param::spaceMix,       0.150f)
     .bi  (ModSource::LFO1,   Param::dustStereo,     0.250f)
     .uni (ModSource::Velocity, Param::dustGrain,    0.300f)
     .bi  (ModSource::KeyTrack, Param::dustColor,    0.200f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.400f)
     .uni (ModSource::Macro2, Param::dustColor,      0.300f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.280f)
     .uni (ModSource::Macro4, Param::dustJitter,     0.300f)
     .uni (ModSource::Macro4, Param::shapeSurface,   0.220f);
    sharedMacros (r, Param::ampDecay, Param::dustJitter);
    r.commit (s);
}});

manager.addFactory ({ "Doppler Wake", "FX", { "metallic", "dark", "morphing", "wide", "transition" }, [] (PatchState& s)
{
    wave (s, 4 /* METALLIC */, 0.50f, 0.40f, 0.18f, 4, 0.30f, 0.80f, 0, 0.72f);
    dust (s, 2 /* BROWN */, 0.62f, 0.34f, 0.40f, 0.30f, 0.80f, 0.85f, 3049, 0.50f);
    set (s, Param::sourceSelected, 0 /* WAVE is the object, DUST is the air */);
    set (s, Param::sourceMode, 1 /* LAYER */);
    set (s, Param::impactLevel, 0.0f);
    set (s, Param::sampleLevel, 0.0f);
    set (s, Param::gestureLevel, 0.0f);
    amp (s, 0.02f, 1.80f, 0.42f, 1.20f, 0.45f);
    shape (s, 0.55f, 0.55f, 0.45f, 0.60f, 0.50f, 0.35f);
    material (s, MaterialType::Metal, MaterialType::Void, 0.45f);
    topology (s, 1 /* RING */, 0.45f, 0.50f, 3053);
    matter (s, 0.78f, 0.65f, 0.32f, 0.95f);
    evolve (s, 0.22f, 0.0f, 0.0f, 0.0f, 0.58f, 0.26f, 0.0f, 0.50f, 0.50f);
    set (s, Param::evolveScatterSeed, 3057);
    space (s, SpacePresets::Nebula, 0.54f, 0.70f, 0.50f, 0.50f);

    env (s, 2, 0.02f, 2.00f, 0.0f, 1.00f, 0.35f);      // arrives high, leaves low
    lfo (s, 1, 0.40f, 0 /* SINE */, 1.0f, true);
    macros (s, 0.50f, 0.45f, 0.55f, 0.50f);

    Routings r;
    r.uni (ModSource::Env2,   Param::shapePitch,     0.130f)
     .uni (ModSource::Env2,   Param::waveDetune,     0.250f)
     .uni (ModSource::Env2,   Param::dustDensity,    0.200f)
     .bi  (ModSource::LFO1,   Param::dustStereo,     0.300f)
     .uni (ModSource::Velocity, Param::waveMorph,    0.300f)
     .bi  (ModSource::KeyTrack, Param::spaceMix,    -0.150f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.350f)
     .uni (ModSource::Macro2, Param::wavePosition,   0.300f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.300f)
     .uni (ModSource::Macro4, Param::evolveBend,     0.280f)
     .uni (ModSource::Macro4, Param::waveSpread,     0.220f);
    sharedMacros (r, Param::ampDecay, Param::waveScan);
    r.commit (s);
}});

manager.addFactory ({ "Siren Protocol", "FX", { "harsh", "metallic", "pulsing", "mid", "transition" }, [] (PatchState& s)
{
    wave (s, 0 /* BASIC */, 0.30f, 0.15f, 0.0f, 2, 0.08f, 0.35f);
    amp (s, 0.02f, 0.50f, 0.90f, 0.35f, 0.40f);
    shape (s, 0.40f, 0.25f, 0.35f, 0.66f, 0.45f, 0.20f);
    material (s, MaterialType::Metal, MaterialType::Custom, 0.35f);
    topology (s, 1 /* RING */, 0.40f, 0.50f, 3061);
    matter (s, 0.76f, 0.70f, 0.10f, 0.50f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.55f, 0.50f, 0.10f, 0.0f, 0.50f, 0.30f);
    set (s, Param::evolveMagnetTarget, 1 /* FIFTH */);
    space (s, SpacePresets::Machine, 0.34f, 0.42f, 0.50f, 0.42f);

    lfo (s, 1, 2.60f, 3 /* SQUARE */, 1.0f, true);     // the two tones
    lfo (s, 2, 0.18f, 1 /* TRIANGLE */, 1.0f, false);
    env (s, 2, 0.30f, 1.20f, 0.70f, 0.50f, 0.55f);
    macros (s, 0.55f, 0.50f, 0.35f, 0.55f);

    Routings r;
    r.bi  (ModSource::LFO1,   Param::shapePitch,     0.075f)
     .bi  (ModSource::LFO1,   Param::waveFine,       0.120f)
     .bi  (ModSource::LFO2,   Param::spaceMix,       0.150f)
     .uni (ModSource::Env2,   Param::shapeTension,   0.180f)
     .uni (ModSource::Velocity, Param::waveMorph,    0.300f)
     .bi  (ModSource::KeyTrack, Param::shapeSurface, 0.180f)
     .uni (ModSource::Macro1, Param::lfo1Rate,       0.150f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.250f)
     .uni (ModSource::Macro2, Param::wavePosition,   0.320f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.300f)
     .uni (ModSource::Macro4, Param::evolveMagnet,   0.300f)
     .uni (ModSource::Macro4, Param::shapeSurface,   0.250f);
    sharedMacros (r, Param::ampDecay, Param::lfo1Rate);
    r.commit (s);
}});

manager.addFactory ({ "Klaxon Rust", "FX", { "dirty", "metallic", "harsh", "rhythmic", "mid" }, [] (PatchState& s)
{
    impact (s, 0 /* IMPULSE */, 0.55f, 0.45f, 0.50f, 0.85f, 0.50f, 0.30f, 0.55f, 0.70f);
    gesture (s, 5 /* ELECTRICAL */, 0.60f, 0.35f, 0.55f, 0.40f, 0.30f, 0.45f, 0.55f);
    set (s, Param::sourceSelected, 2 /* IMPACT blows the horn */);
    set (s, Param::sourceMode, 1 /* LAYER */);
    set (s, Param::waveLevel, 0.0f);
    set (s, Param::dustLevel, 0.0f);
    set (s, Param::sampleLevel, 0.0f);
    amp (s, 0.01f, 1.20f, 0.72f, 0.60f, 0.45f);
    set (s, Param::masterGain, -2.0f);
    shape (s, 0.60f, 0.42f, 0.58f, 0.46f, 0.50f, 0.50f);
    material (s, MaterialType::Metal, MaterialType::Organic, 0.50f);
    topology (s, 0 /* CHAIN */, 0.50f, 0.45f, 3067);
    matter (s, 0.85f, 0.75f, 0.55f, 0.55f);
    evolve (s, 0.0f, 0.15f, 0.20f, 0.0f, 0.60f, 0.30f, 0.42f, 0.40f, 0.35f);
    set (s, Param::evolveScatterSeed, 3069);
    fracture (s, 1 /* RHYTHMIC */, 0.50f, 0.55f, 0.50f, 0.85f, 0.30f, 0.30f, 0.35f, 0.40f, 0.30f,
              1 /* 16 */, 3 /* 1/8 */, 8, 0.20f, 0 /* FORWARD */, 0.80f, 0.35f, 3071,
              FractureShape { 16, 0.03f, 0.35f, 0.20f, 0.50f, 0.35f, 0.60f, 0.30f, 0.85f,
                              1.0f, 1.0f, 0.70f, 0.85f, kFallingTerrace, "XX.XoX..", nullptr });
    space (s, SpacePresets::Machine, 0.38f, 0.50f, 0.42f, 0.45f);

    env (s, 2, 1.10f, 1.50f, 0.60f, 0.50f, 0.45f);     // the horn breaks up halfway through the blast
    lfo (s, 1, 0.70f, 4 /* RANDOM */, 1.0f, true);
    chaos (s, 1, 2 /* LOGISTIC */, 3.20f, 0.50f, 0.45f, 0.5f, 3079);
    macros (s, 0.50f, 0.40f, 0.40f, 0.60f);

    Routings r;
    r.uni (ModSource::Env2,   Param::evolveCrush,    0.300f)
     .uni (ModSource::Env2,   Param::fractureAmount, 0.250f)
     .bi  (ModSource::LFO1,   Param::gestureRoughness, 0.220f)
     .bi  (ModSource::Chaos1, Param::impactRandom,   0.180f)
     .uni (ModSource::Velocity, Param::impactHardness, 0.300f)
     .bi  (ModSource::NoteRandom, Param::impactRate,  0.150f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.350f)
     .uni (ModSource::Macro1, Param::fractureEvolve, 0.250f)
     .uni (ModSource::Macro2, Param::fractureTone,   0.300f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.300f)
     .uni (ModSource::Macro4, Param::evolveCrush,    0.280f)
     .uni (ModSource::Macro4, Param::evolveTear,     0.220f);
    sharedMacros (r, Param::fractureDecay, Param::evolveCrush);
    r.commit (s);
}});
}

} // namespace am::FactoryContent
