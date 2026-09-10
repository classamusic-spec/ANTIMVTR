#include "presets/FactoryBuilders.h"

namespace am::FactoryContent
{

void registerLead (PresetManager& manager)
{
//==========================================================================
// LEAD — one line, in front
//==========================================================================

manager.addFactory ({ "Liquid Teeth", "LEAD", { "lead", "liquid", "biting", "mono" }, [] (PatchState& s)
{
    wave (s, 4 /* METALLIC */, 0.55f, 0.42f, 0.12f, 2, 0.10f, 0.35f);
    set (s, Param::waveSync, 0.28f);
    set (s, Param::waveModRatio, 3.0f);
    amp (s, 0.006f, 0.30f, 0.90f, 0.45f, 0.4f);
    shape (s, 0.46f, 0.62f, 0.38f, 0.66f, 0.66f, 0.44f);
    material (s, MaterialType::Liquid, MaterialType::Metal, 0.42f);
    topology (s, 1 /* RING */, 0.52f, 0.50f, 199);
    matter (s, 0.70f, 0.55f, 0.40f, 0.45f);
    evolve (s, 0.22f, 0.16f, 0.0f, 0.30f, 0.46f, 0.14f, 0.0f, 0.38f, 0.28f);
    set (s, Param::evolveMagnetTarget, 4 /* CHROMATIC */);
    set (s, Param::masterMode, 2 /* LEGATO */);
    set (s, Param::masterGlide, 0.09f);
    space (s, SpacePresets::Orbit, 0.28f, 0.42f, 0.55f, 0.42f);

    lfo (s, 1, 5.20f, 0 /* SINE */, 1.0f, true, 0.45f);
    lfo (s, 2, 0.28f, 1 /* TRIANGLE */, 1.0f, false);
    env (s, 1, 0.004f, 0.35f, 0.35f, 0.30f, 0.3f);
    macros (s, 0.30f, 0.45f, 0.30f, 0.40f);

    Routings r;
    r.bi  (ModSource::LFO1,   Param::shapePitch,     0.005f)
     .bi  (ModSource::LFO2,   Param::waveMorph,      0.140f)
     .uni (ModSource::Env1,   Param::waveSync,       0.260f)
     .uni (ModSource::Velocity, Param::shapeSurface, 0.240f)
     .uni (ModSource::ModWheel, Param::lfo1Depth,    0.600f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.350f)
     .uni (ModSource::Macro2, Param::wavePosition,   0.300f)
     .uni (ModSource::Macro2, Param::shapeExcite,    0.220f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.280f)
     .uni (ModSource::Macro4, Param::waveSync,       0.400f)
     .uni (ModSource::Macro4, Param::evolveBend,     0.200f);
    sharedMacros (r, Param::ampDecay, Param::evolveTear);
    r.commit (s);
}});

manager.addFactory ({ "Impossible String", "LEAD", { "lead", "bowed", "string", "expressive" }, [] (PatchState& s)
{
    gesture (s, 0 /* BOW */, 0.58f, 0.48f, 0.24f, 0.32f, 0.22f, 0.45f);
    amp (s, 0.12f, 0.50f, 0.88f, 0.45f, 0.5f);
    shape (s, 0.48f, 0.12f, 0.40f, 0.62f, 0.56f, 0.22f);
    material (s, MaterialType::String, MaterialType::Organic, 0.30f);
    topology (s, 0 /* CHAIN */, 0.50f, 0.55f, 61);
    matter (s, 0.95f, 0.52f, 0.25f, 0.50f);
    evolve (s, 0.16f, 0.0f, 0.0f, 0.44f, 0.48f, 0.08f, 0.0f, 0.28f, 0.24f);
    set (s, Param::evolveMagnetTarget, 6 /* CUSTOM (harmonic) */);
    set (s, Param::masterMode, 2 /* LEGATO */);
    set (s, Param::masterGlide, 0.10f);
    space (s, SpacePresets::Chamber, 0.34f, 0.45f, 0.55f, 0.28f);

    lfo (s, 1, 5.60f, 0 /* SINE */, 1.0f, true, 0.60f);
    lfo (s, 2, 0.22f, 5 /* SMOOTH RANDOM */, 1.0f, false);
    env (s, 1, 0.18f, 0.60f, 0.60f, 0.50f);
    macros (s, 0.30f, 0.45f, 0.35f, 0.40f);

    Routings r;
    r.bi  (ModSource::LFO1,   Param::shapePitch,     0.006f)
     .bi  (ModSource::LFO2,   Param::gesturePressure, 0.100f)
     .uni (ModSource::Env1,   Param::gestureSpeed,   0.220f)
     .uni (ModSource::Velocity, Param::gesturePressure, 0.260f)
     .uni (ModSource::Pressure, Param::gestureSpeed, 0.300f)
     .uni (ModSource::ModWheel, Param::lfo1Depth,    0.550f)
     .uni (ModSource::Macro1, Param::gestureMotion,  0.350f)
     .uni (ModSource::Macro2, Param::gestureBandwidth, 0.300f)
     .uni (ModSource::Macro2, Param::shapeExcite,    0.250f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.300f)
     .uni (ModSource::Macro4, Param::gestureRoughness, 0.350f)
     .uni (ModSource::Macro4, Param::shapeSurface,   0.180f);
    sharedMacros (r, Param::ampDecay, Param::gestureRoughness);
    r.commit (s);
}});

manager.addFactory ({ "Solar Filament", "LEAD", { "lead", "bright", "burning", "wide" }, [] (PatchState& s)
{
    wave (s, 2 /* FORMANT */, 0.62f, 0.55f, 0.18f, 2, 0.12f, 0.40f);
    set (s, Param::waveFM, 0.18f);
    set (s, Param::waveModRatio, 2.0f);
    amp (s, 0.02f, 0.45f, 0.85f, 0.55f, 0.5f);
    shape (s, 0.55f, 0.58f, 0.46f, 0.70f, 0.62f, 0.30f);
    material (s, MaterialType::Crystal, MaterialType::Metal, 0.55f);
    topology (s, 5 /* STAR */, 0.44f, 0.60f, 149);
    matter (s, 0.62f, 0.48f, 0.35f, 0.60f);
    evolve (s, 0.26f, 0.0f, 0.0f, 0.38f, 0.36f, 0.18f, 0.0f, 0.40f, 0.30f);
    set (s, Param::evolveBendPivot, 0.65f);
    set (s, Param::evolveMagnetTarget, 2 /* MAJOR */);
    space (s, SpacePresets::Shimmer, 0.36f, 0.55f, 0.62f, 0.35f);

    lfo (s, 1, 6.20f, 0 /* SINE */, 1.0f, true, 0.55f);
    lfo (s, 2, 0.42f, 2 /* SAW */, 1.0f, true);
    env (s, 1, 0.02f, 0.55f, 0.45f, 0.50f);
    macros (s, 0.35f, 0.55f, 0.35f, 0.30f);

    Routings r;
    r.bi  (ModSource::LFO1,   Param::shapePitch,     0.005f)
     .bi  (ModSource::LFO2,   Param::waveScan,       0.120f)
     .uni (ModSource::Env1,   Param::waveFM,         0.200f)
     .uni (ModSource::Velocity, Param::waveFM,       0.150f)
     .bi  (ModSource::KeyTrack, Param::shapeForm,   -0.100f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.400f)
     .uni (ModSource::Macro2, Param::wavePosition,   0.300f)
     .uni (ModSource::Macro2, Param::shapeExcite,    0.250f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.300f)
     .uni (ModSource::Macro4, Param::waveFM,         0.250f)
     .uni (ModSource::Macro4, Param::evolveBend,     0.250f);
    sharedMacros (r, Param::spaceSize, Param::waveScan);
    r.commit (s);
}});

manager.addFactory ({ "Steel Reed", "LEAD", { "metallic", "bowed", "bright", "mid", "melodic" }, [] (PatchState& s)
{
    gesture (s, 0 /* BOW */, 0.72f, 0.56f, 0.34f, 0.26f, 0.12f, 0.38f);
    amp (s, 0.008f, 0.28f, 0.92f, 0.28f, 0.35f);
    shape (s, 0.42f, 0.50f, 0.28f, 0.76f, 0.56f, 0.42f);
    material (s, MaterialType::Metal, MaterialType::Crystal, 0.30f);
    topology (s, 1 /* RING */, 0.46f, 0.42f, 311);
    matter (s, 0.88f, 0.60f, 0.46f, 0.32f);
    evolve (s, 0.10f, 0.0f, 0.0f, 0.32f, 0.44f, 0.06f, 0.0f, 0.30f, 0.18f);
    set (s, Param::evolveMagnetTarget, 4 /* CHROMATIC */);
    set (s, Param::masterMode, 2 /* LEGATO */);
    set (s, Param::masterGlide, 0.05f);
    space (s, SpacePresets::Chamber, 0.20f, 0.30f, 0.62f, 0.22f);

    lfo (s, 1, 5.80f, 0 /* SINE */, 1.0f, true, 0.50f);
    lfo (s, 2, 0.31f, 1 /* TRIANGLE */, 1.0f, false);
    env (s, 1, 0.003f, 0.14f, 0.0f, 0.12f, 0.25f);
    macros (s, 0.25f, 0.45f, 0.25f, 0.45f);

    Routings r;
    r.bi  (ModSource::LFO1,      Param::shapePitch,       0.005f)
     .bi  (ModSource::LFO2,      Param::gesturePosition,  0.110f)
     .uni (ModSource::Env1,      Param::gesturePressure,  0.280f)
     .uni (ModSource::Velocity,  Param::gesturePressure,  0.240f)
     .uni (ModSource::Velocity,  Param::shapeStrike,      0.300f)
     .bi  (ModSource::KeyTrack,  Param::shapeSurface,    -0.140f)
     .uni (ModSource::ModWheel,  Param::lfo1Depth,        0.600f)
     .uni (ModSource::Macro1,    Param::gestureMotion,    0.400f)
     .uni (ModSource::Macro2,    Param::gestureBandwidth, 0.320f)
     .uni (ModSource::Macro2,    Param::shapeExcite,      0.220f)
     .uni (ModSource::Macro3,    Param::spaceMix,         0.300f)
     .uni (ModSource::Macro4,    Param::gestureRoughness, 0.380f);
    sharedMacros (r, Param::ampDecay, Param::gestureRoughness);
    r.commit (s);
}});

manager.addFactory ({ "Brass Lantern", "LEAD", { "warm", "hollow", "blown", "mid", "melodic" }, [] (PatchState& s)
{
    gesture (s, 3 /* BREATH */, 0.56f, 0.40f, 0.20f, 0.46f, 0.26f, 0.52f);
    amp (s, 0.09f, 0.60f, 0.86f, 0.45f, 0.55f);
    shape (s, 0.50f, 0.30f, 0.52f, 0.54f, 0.62f, 0.24f);
    material (s, MaterialType::Metal, MaterialType::Wood, 0.44f);
    topology (s, 0 /* CHAIN */, 0.40f, 0.58f, 907);
    matter (s, 0.92f, 0.60f, 0.30f, 0.42f);
    evolve (s, 0.0f, 0.14f, 0.0f, 0.24f, 0.56f, 0.08f, 0.0f, 0.22f, 0.30f);
    set (s, Param::evolveMagnetTarget, 0 /* OCTAVE */);
    set (s, Param::masterMode, 2 /* LEGATO */);
    set (s, Param::masterGlide, 0.11f);
    space (s, SpacePresets::Nebula, 0.30f, 0.48f, 0.44f, 0.30f);

    lfo (s, 1, 4.60f, 0 /* SINE */, 1.0f, true, 0.80f);
    lfo (s, 2, 0.24f, 5 /* SMOOTH RANDOM */, 1.0f, false);
    env (s, 1, 0.30f, 1.10f, 0.55f, 0.60f, 0.6f);
    macros (s, 0.30f, 0.40f, 0.35f, 0.40f);

    Routings r;
    r.bi  (ModSource::LFO1,      Param::shapePitch,       0.006f)
     .bi  (ModSource::LFO2,      Param::gestureBandwidth, 0.130f)
     .uni (ModSource::Env1,      Param::gestureSpeed,     0.220f)
     .uni (ModSource::Velocity,  Param::gesturePressure,  0.280f)
     .uni (ModSource::Pressure,  Param::gestureSpeed,     0.320f)
     .bi  (ModSource::KeyTrack,  Param::gestureBandwidth, 0.120f)
     .uni (ModSource::ModWheel,  Param::lfo1Depth,        0.550f)
     .uni (ModSource::Macro1,    Param::gestureMotion,    0.380f)
     .uni (ModSource::Macro2,    Param::shapeTension,     0.240f)
     .uni (ModSource::Macro2,    Param::shapeExcite,      0.260f)
     .uni (ModSource::Macro3,    Param::spaceMix,         0.320f)
     .uni (ModSource::Macro4,    Param::shapeBlend,       0.350f);
    sharedMacros (r, Param::spaceSize, Param::gestureRoughness);
    r.commit (s);
}});

manager.addFactory ({ "Sung Wire", "LEAD", { "vocal", "formant", "organic", "mid", "melodic" }, [] (PatchState& s)
{
    wave (s, 2 /* FORMANT */, 0.34f, 0.48f, 0.22f, 2, 0.07f, 0.28f);
    set (s, Param::wavePM, 0.14f);
    set (s, Param::waveModRatio, 1.0f);
    amp (s, 0.04f, 0.42f, 0.88f, 0.38f, 0.5f);
    shape (s, 0.54f, 0.40f, 0.44f, 0.58f, 0.60f, 0.30f);
    material (s, MaterialType::Organic, MaterialType::Membrane, 0.38f);
    topology (s, 2 /* CLUSTERS */, 0.50f, 0.52f, 523);
    matter (s, 0.78f, 0.58f, 0.34f, 0.46f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.40f, 0.50f, 0.10f, 0.0f, 0.26f, 0.26f);
    set (s, Param::evolveMagnetTarget, 6 /* CUSTOM (harmonic) */);
    set (s, Param::masterMode, 2 /* LEGATO */);
    set (s, Param::masterGlide, 0.13f);
    space (s, SpacePresets::Dream, 0.30f, 0.44f, 0.50f, 0.30f);

    lfo (s, 1, 5.10f, 0 /* SINE */, 1.0f, true, 0.55f);
    lfo (s, 2, 0.18f, 1 /* TRIANGLE */, 1.0f, false, 0.60f);
    env (s, 2, 0.45f, 1.60f, 0.70f, 0.70f, 0.6f);
    macros (s, 0.30f, 0.45f, 0.30f, 0.50f);

    Routings r;
    r.bi  (ModSource::LFO1,      Param::shapePitch,     0.005f)
     .bi  (ModSource::LFO2,      Param::wavePosition,   0.120f)
     .uni (ModSource::Env2,      Param::waveMorph,      0.320f)
     .uni (ModSource::Env2,      Param::shapeForm,      0.160f)
     .uni (ModSource::Velocity,  Param::waveScan,       0.220f)
     .bi  (ModSource::NoteRandom, Param::wavePosition,  0.060f)
     .uni (ModSource::ModWheel,  Param::wavePosition,   0.400f)
     .uni (ModSource::Macro1,    Param::lfo2Depth,      0.500f)
     .uni (ModSource::Macro2,    Param::waveScan,       0.300f)
     .uni (ModSource::Macro2,    Param::shapeExcite,    0.200f)
     .uni (ModSource::Macro3,    Param::spaceMix,       0.300f)
     .uni (ModSource::Macro4,    Param::waveMorph,      0.400f);
    sharedMacros (r, Param::ampRelease, Param::wavePM);
    r.commit (s);
}});

manager.addFactory ({ "Feedback Halo", "LEAD", { "metallic", "resonant", "evolving", "high", "melodic" }, [] (PatchState& s)
{
    wave (s, 1 /* HARMONIC */, 0.28f, 0.35f, 0.10f, 1, 0.04f, 0.20f);
    amp (s, 0.012f, 0.35f, 0.82f, 0.55f, 0.45f);
    shape (s, 0.46f, 0.56f, 0.34f, 0.72f, 0.64f, 0.26f);
    material (s, MaterialType::Metal, MaterialType::Crystal, 0.52f);
    topology (s, 5 /* STAR */, 0.42f, 0.56f, 641);
    matter (s, 0.72f, 0.52f, 0.36f, 0.50f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.46f, 0.42f, 0.10f, 0.0f, 0.34f, 0.22f);
    set (s, Param::evolveMagnetTarget, 0 /* OCTAVE */);
    set (s, Param::masterMode, 2 /* LEGATO */);
    set (s, Param::masterGlide, 0.07f);
    fracture (s, 0 /* SPECTRAL */, 0.44f, 0.38f, 0.55f, 0.35f, 0.58f, 0.22f, 0.66f, 0.62f, 0.30f,
              2 /* 32 */, 6 /* 1/4T */, 4, 0.0f, 0 /* FORWARD */, 1.0f, 0.10f, 613,
              FractureShape { 32, 0.04f, 0.30f, 0.30f, 0.62f, 0.50f, 0.78f, 0.20f, 0.80f,
                              1.0f, 0.85f, 0.55f, 1.0f, kOctaveTerrace, "XXLX", nullptr });
    space (s, SpacePresets::Shimmer, 0.32f, 0.52f, 0.60f, 0.34f);

    lfo (s, 1, 6.40f, 0 /* SINE */, 1.0f, true, 0.60f);
    env (s, 2, 0.90f, 2.40f, 0.75f, 0.90f, 0.6f);
    env (s, 1, 0.006f, 0.20f, 0.0f, 0.20f, 0.3f);
    macros (s, 0.35f, 0.50f, 0.35f, 0.45f);

    Routings r;
    r.bi  (ModSource::LFO1,      Param::shapePitch,        0.004f)
     .uni (ModSource::Env2,      Param::fractureFeedback,  0.220f)
     .uni (ModSource::Env2,      Param::fractureMix,       0.240f)
     .uni (ModSource::Env1,      Param::shapeExcite,       0.220f)
     .uni (ModSource::Velocity,  Param::fractureAmount,    0.260f)
     .bi  (ModSource::KeyTrack,  Param::fractureTone,     -0.180f)
     .uni (ModSource::ModWheel,  Param::lfo1Depth,         0.550f)
     .uni (ModSource::Macro1,    Param::fractureEvolve,    0.400f)
     .uni (ModSource::Macro2,    Param::fractureTone,      0.300f)
     .uni (ModSource::Macro2,    Param::wavePosition,      0.250f)
     .uni (ModSource::Macro3,    Param::spaceMix,          0.300f)
     .uni (ModSource::Macro4,    Param::fractureFeedback,  0.250f);
    sharedMacros (r, Param::fractureDecay, Param::fractureRandom);
    r.commit (s);
}});

manager.addFactory ({ "Bowed Girder", "LEAD", { "dark", "low", "bowed", "metallic", "melodic" }, [] (PatchState& s)
{
    gesture (s, 0 /* BOW */, 0.80f, 0.30f, 0.44f, 0.62f, 0.16f, 0.30f);
    amp (s, 0.05f, 0.55f, 0.82f, 0.50f, 0.5f);
    shape (s, 0.38f, 0.22f, 0.72f, 0.44f, 0.70f, 0.34f);
    material (s, MaterialType::String, MaterialType::Metal, 0.54f);
    topology (s, 3 /* LATTICE */, 0.64f, 0.38f, 419);
    matter (s, 0.86f, 0.54f, 0.30f, 0.38f);
    set (s, Param::shapePitch, -12.0f);
    evolve (s, 0.14f, 0.0f, 0.0f, 0.0f, 0.62f, 0.08f, 0.0f, 0.20f, 0.22f);
    set (s, Param::evolveBendPivot, 0.30f);
    set (s, Param::masterMode, 2 /* LEGATO */);
    set (s, Param::masterGlide, 0.14f);
    space (s, SpacePresets::Void, 0.26f, 0.58f, 0.34f, 0.30f);

    lfo (s, 1, 4.90f, 0 /* SINE */, 1.0f, true, 0.70f);
    lfo (s, 2, 0.14f, 5 /* SMOOTH RANDOM */, 1.0f, false);
    env (s, 1, 0.12f, 0.80f, 0.40f, 0.50f, 0.5f);
    macros (s, 0.25f, 0.40f, 0.30f, 0.45f);

    Routings r;
    r.bi  (ModSource::LFO1,      Param::shapePitch,       0.004f)
     .bi  (ModSource::LFO2,      Param::gesturePressure,  0.100f)
     .uni (ModSource::Env1,      Param::evolveBend,       0.200f)
     .uni (ModSource::Velocity,  Param::gesturePressure,  0.300f)
     .bi  (ModSource::KeyTrack,  Param::shapeMass,       -0.220f)
     .uni (ModSource::Pressure,  Param::gestureSpeed,     0.280f)
     .uni (ModSource::ModWheel,  Param::lfo1Depth,        0.500f)
     .uni (ModSource::Macro1,    Param::gestureMotion,    0.350f)
     .uni (ModSource::Macro2,    Param::gestureBandwidth, 0.340f)
     .uni (ModSource::Macro2,    Param::shapeSurface,     0.180f)
     .uni (ModSource::Macro3,    Param::spaceMix,         0.300f)
     .uni (ModSource::Macro4,    Param::shapeCoupling,    0.300f);
    sharedMacros (r, Param::ampRelease, Param::gestureRoughness);
    r.commit (s);
}});

manager.addFactory ({ "Rolling Iron", "LEAD", { "metallic", "struck", "rhythmic", "mid", "melodic" }, [] (PatchState& s)
{
    impact (s, 4 /* METAL STRIKE */, 0.70f, 0.60f, 0.22f, 0.70f, 0.45f, 0.14f, 0.46f);
    amp (s, 0.003f, 0.50f, 0.76f, 0.40f, 0.35f);
    shape (s, 0.44f, 0.60f, 0.36f, 0.68f, 0.58f, 0.38f);
    material (s, MaterialType::Metal, MaterialType::Crystal, 0.34f);
    topology (s, 1 /* RING */, 0.54f, 0.48f, 733);
    matter (s, 0.90f, 0.62f, 0.50f, 0.44f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.34f, 0.46f, 0.12f, 0.0f, 0.36f, 0.20f);
    set (s, Param::evolveMagnetTarget, 0 /* OCTAVE */);
    set (s, Param::masterMode, 1 /* MONO */);
    fracture (s, 1 /* RHYTHMIC */, 0.34f, 0.30f, 0.40f, 0.55f, 0.24f, 0.28f, 0.44f, 0.55f, 0.20f,
              1 /* 16 */, 4 /* 1/16 */, 8, 0.12f, 0 /* FORWARD */, 0.90f, 0.20f, 251,
              FractureShape { 16, 0.03f, 0.26f, 0.10f, 0.34f, 0.35f, 0.58f, 0.20f, 0.75f,
                              1.0f, 0.90f, 0.55f, 0.90f, kFifthTerrace, "XoXoXXoX", nullptr });
    space (s, SpacePresets::Machine, 0.24f, 0.36f, 0.56f, 0.28f);

    lfo (s, 1, 0.42f, 1 /* TRIANGLE */, 1.0f, true);
    env (s, 1, 0.004f, 0.30f, 0.20f, 0.25f, 0.3f);
    macros (s, 0.35f, 0.45f, 0.25f, 0.50f);

    Routings r;
    r.bi  (ModSource::LFO1,       Param::impactRate,       0.120f)
     .uni (ModSource::Env1,       Param::impactBrightness, 0.240f)
     .uni (ModSource::Velocity,   Param::impactHardness,   0.300f)
     .uni (ModSource::Velocity,   Param::impactVelocity,   0.200f)
     .bi  (ModSource::NoteRandom, Param::impactRandom,     0.150f)
     .bi  (ModSource::KeyTrack,   Param::impactLength,    -0.200f)
     .uni (ModSource::ModWheel,   Param::impactRate,       0.400f)
     .uni (ModSource::Macro1,     Param::fractureSequence, 0.380f)
     .uni (ModSource::Macro2,     Param::impactBrightness, 0.320f)
     .uni (ModSource::Macro2,     Param::shapeExcite,      0.200f)
     .uni (ModSource::Macro3,     Param::spaceMix,         0.300f)
     .uni (ModSource::Macro4,     Param::fractureAmount,   0.340f);
    sharedMacros (r, Param::fractureDecay, Param::impactRandom);
    r.commit (s);
}});

manager.addFactory ({ "Ceramic Whistle", "LEAD", { "glassy", "blown", "air", "high", "melodic" }, [] (PatchState& s)
{
    wave (s, 0 /* BASIC */, 0.16f, 0.22f, 0.06f, 1, 0.03f, 0.18f, 0, 0.72f);
    dust (s, 4 /* FILTERED */, 0.62f, 0.72f, 0.28f, 0.26f, 0.45f, 0.55f, 137, 0.34f);
    set (s, Param::sourceMode, 1 /* LAYER */);
    set (s, Param::impactLevel, 0.0f);
    set (s, Param::sampleLevel, 0.0f);
    set (s, Param::gestureLevel, 0.0f);
    amp (s, 0.06f, 0.40f, 0.90f, 0.35f, 0.5f);
    shape (s, 0.34f, 0.66f, 0.22f, 0.70f, 0.54f, 0.20f);
    material (s, MaterialType::Crystal, MaterialType::Void, 0.34f);
    topology (s, 5 /* STAR */, 0.30f, 0.62f, 877);
    matter (s, 0.80f, 0.56f, 0.28f, 0.52f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.30f, 0.38f, 0.14f, 0.0f, 0.28f, 0.24f);
    set (s, Param::evolveMagnetTarget, 1 /* FIFTH */);
    set (s, Param::masterMode, 2 /* LEGATO */);
    set (s, Param::masterGlide, 0.08f);
    space (s, SpacePresets::Dust, 0.30f, 0.46f, 0.66f, 0.30f);

    lfo (s, 1, 5.40f, 0 /* SINE */, 1.0f, true, 0.65f);
    lfo (s, 2, 0.36f, 5 /* SMOOTH RANDOM */, 1.0f, false);
    env (s, 1, 0.05f, 0.45f, 0.30f, 0.30f, 0.5f);
    macros (s, 0.30f, 0.50f, 0.35f, 0.40f);

    Routings r;
    r.bi  (ModSource::LFO1,      Param::shapePitch,    0.005f)
     .bi  (ModSource::LFO2,      Param::dustColor,     0.140f)
     .uni (ModSource::Env1,      Param::dustDensity,   0.260f)
     .uni (ModSource::Velocity,  Param::dustLevel,     0.180f)
     .uni (ModSource::Velocity,  Param::shapeExcite,   0.220f)
     .bi  (ModSource::KeyTrack,  Param::dustColor,    -0.200f)
     .uni (ModSource::ModWheel,  Param::lfo1Depth,     0.560f)
     .uni (ModSource::Macro1,    Param::lfo2Depth,     0.450f)
     .uni (ModSource::Macro2,    Param::wavePosition,  0.280f)
     .uni (ModSource::Macro2,    Param::dustColor,     0.240f)
     .uni (ModSource::Macro3,    Param::spaceMix,      0.320f)
     .uni (ModSource::Macro4,    Param::dustLevel,     0.300f);
    sharedMacros (r, Param::ampRelease, Param::dustJitter);
    r.commit (s);
}});

manager.addFactory ({ "Salt Tongue", "LEAD", { "dirty", "harsh", "synthetic", "mid", "melodic" }, [] (PatchState& s)
{
    wave (s, 3 /* FOLDED */, 0.58f, 0.52f, 0.16f, 3, 0.18f, 0.42f);
    set (s, Param::waveRing, 0.22f);
    set (s, Param::waveModRatio, 1.5f);
    amp (s, 0.006f, 0.32f, 0.88f, 0.30f, 0.4f);
    shape (s, 0.58f, 0.72f, 0.40f, 0.62f, 0.50f, 0.52f);
    material (s, MaterialType::Metal, MaterialType::Void, 0.36f);
    topology (s, 4 /* RANDOM */, 0.52f, 0.44f, 1097);
    matter (s, 0.66f, 0.62f, 0.42f, 0.40f);
    evolve (s, 0.0f, 0.0f, 0.18f, 0.38f, 0.48f, 0.16f, 0.44f, 0.50f, 0.28f);
    set (s, Param::evolveMagnetTarget, 4 /* CHROMATIC */);
    set (s, Param::masterMode, 1 /* MONO */);
    set (s, Param::masterGlide, 0.04f);
    space (s, SpacePresets::Machine, 0.26f, 0.34f, 0.48f, 0.32f);

    lfo (s, 1, 5.90f, 0 /* SINE */, 1.0f, true, 0.45f);
    chaos (s, 1, 2 /* LOGISTIC */, 3.20f, 0.40f, 0.70f, 0.5f, 331);
    env (s, 1, 0.005f, 0.25f, 0.15f, 0.20f, 0.3f);
    macros (s, 0.35f, 0.45f, 0.25f, 0.55f);

    Routings r;
    r.bi  (ModSource::LFO1,      Param::shapePitch,   0.005f)
     .bi  (ModSource::Chaos1,    Param::evolveCrush,  0.120f)
     .uni (ModSource::Env1,      Param::waveRing,     0.240f)
     .uni (ModSource::Velocity,  Param::evolveCrush,  0.200f)
     .uni (ModSource::Velocity,  Param::waveMorph,    0.240f)
     .bi  (ModSource::KeyTrack,  Param::waveDetune,  -0.150f)
     .uni (ModSource::ModWheel,  Param::evolveTear,   0.350f)
     .uni (ModSource::Macro1,    Param::evolveMotion, 0.400f)
     .uni (ModSource::Macro2,    Param::wavePosition, 0.320f)
     .uni (ModSource::Macro2,    Param::shapeExcite,  0.220f)
     .uni (ModSource::Macro3,    Param::spaceMix,     0.300f)
     .uni (ModSource::Macro4,    Param::evolveCrush,  0.380f);
    sharedMacros (r, Param::ampDecay, Param::evolveTear);
    r.commit (s);
}});

manager.addFactory ({ "Amber Horn", "LEAD", { "warm", "wooden", "soft", "mid", "melodic" }, [] (PatchState& s)
{
    wave (s, 1 /* HARMONIC */, 0.30f, 0.40f, 0.14f, 2, 0.09f, 0.32f, 0, 0.52f);
    set (s, Param::waveAM, 0.16f);
    set (s, Param::waveModRatio, 2.0f);
    amp (s, 0.12f, 0.70f, 0.76f, 0.55f, 0.6f);
    shape (s, 0.48f, 0.24f, 0.58f, 0.50f, 0.66f, 0.18f);
    material (s, MaterialType::Wood, MaterialType::Organic, 0.42f);
    topology (s, 0 /* CHAIN */, 0.44f, 0.60f, 149);
    matter (s, 0.48f, 0.40f, 0.22f, 0.40f);
    evolve (s, 0.0f, 0.10f, 0.0f, 0.28f, 0.52f, 0.06f, 0.0f, 0.18f, 0.24f);
    set (s, Param::evolveMagnetTarget, 6 /* CUSTOM (harmonic) */);
    set (s, Param::masterMode, 2 /* LEGATO */);
    set (s, Param::masterGlide, 0.16f);
    space (s, SpacePresets::Orbit, 0.28f, 0.40f, 0.42f, 0.36f);

    lfo (s, 1, 4.30f, 0 /* SINE */, 1.0f, true, 0.90f);
    lfo (s, 2, 0.20f, 1 /* TRIANGLE */, 1.0f, false, 0.80f);
    env (s, 2, 0.60f, 1.80f, 0.60f, 0.80f, 0.6f);
    macros (s, 0.30f, 0.40f, 0.30f, 0.45f);

    Routings r;
    r.bi  (ModSource::LFO1,      Param::shapePitch,   0.006f)
     .bi  (ModSource::LFO2,      Param::waveMorph,    0.110f)
     .uni (ModSource::Env2,      Param::wavePosition, 0.240f)
     .uni (ModSource::Env2,      Param::shapeTension, 0.150f)
     .uni (ModSource::Velocity,  Param::shapeStrike,  0.320f)
     .bi  (ModSource::KeyTrack,  Param::shapeDecay,  -0.180f)
     .uni (ModSource::ModWheel,  Param::lfo1Depth,    0.520f)
     .uni (ModSource::Macro1,    Param::lfo2Depth,    0.450f)
     .uni (ModSource::Macro2,    Param::waveAM,       0.300f)
     .uni (ModSource::Macro2,    Param::shapeExcite,  0.240f)
     .uni (ModSource::Macro3,    Param::spaceMix,     0.300f)
     .uni (ModSource::Macro4,    Param::shapeBlend,   0.400f);
    sharedMacros (r, Param::ampRelease, Param::waveDetune);
    r.commit (s);
}});

manager.addFactory ({ "Glacier Bow", "LEAD", { "cold", "glassy", "bowed", "wide", "melodic" }, [] (PatchState& s)
{
    gesture (s, 0 /* BOW */, 0.50f, 0.62f, 0.16f, 0.34f, 0.30f, 0.62f);
    amp (s, 0.16f, 0.80f, 0.86f, 0.70f, 0.55f);
    shape (s, 0.52f, 0.74f, 0.26f, 0.66f, 0.68f, 0.16f);
    material (s, MaterialType::Crystal, MaterialType::Liquid, 0.36f);
    topology (s, 5 /* STAR */, 0.38f, 0.66f, 1471);
    matter (s, 0.90f, 0.54f, 0.24f, 0.66f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.58f, 0.40f, 0.12f, 0.0f, 0.24f, 0.34f);
    set (s, Param::evolveMagnetTarget, 1 /* FIFTH */);
    set (s, Param::masterMode, 2 /* LEGATO */);
    set (s, Param::masterGlide, 0.18f);
    space (s, SpacePresets::Nebula, 0.38f, 0.62f, 0.58f, 0.34f);

    lfo (s, 1, 5.00f, 0 /* SINE */, 1.0f, true, 0.85f);
    lfo (s, 2, 0.11f, 5 /* SMOOTH RANDOM */, 1.0f, false);
    env (s, 2, 1.10f, 2.60f, 0.55f, 1.20f, 0.6f);
    macros (s, 0.35f, 0.45f, 0.45f, 0.40f);

    Routings r;
    r.bi  (ModSource::LFO1,      Param::shapePitch,       0.005f)
     .bi  (ModSource::LFO2,      Param::gestureSpeed,     0.140f)
     .uni (ModSource::Env2,      Param::evolveMagnet,     0.260f)
     .uni (ModSource::Env2,      Param::shapeCoupling,    0.180f)
     .uni (ModSource::Velocity,  Param::gesturePressure,  0.260f)
     .bi  (ModSource::NoteRandom, Param::gesturePosition, 0.120f)
     .uni (ModSource::ModWheel,  Param::lfo1Depth,        0.500f)
     .uni (ModSource::Macro1,    Param::gestureMotion,    0.420f)
     .uni (ModSource::Macro2,    Param::gestureBandwidth, 0.320f)
     .uni (ModSource::Macro2,    Param::shapeForm,        0.180f)
     .uni (ModSource::Macro3,    Param::spaceMix,         0.340f)
     .uni (ModSource::Macro4,    Param::evolveMagnet,     0.300f);
    sharedMacros (r, Param::spaceSize, Param::evolveScatter);
    r.commit (s);
}});

manager.addFactory ({ "Locust Voice", "LEAD", { "harsh", "noisy", "rhythmic", "mid", "melodic" }, [] (PatchState& s)
{
    gesture (s, 4 /* FRICTION */, 0.70f, 0.70f, 0.58f, 0.36f, 0.28f, 0.34f);
    amp (s, 0.010f, 0.35f, 0.86f, 0.26f, 0.4f);
    shape (s, 0.56f, 0.58f, 0.32f, 0.70f, 0.48f, 0.46f);
    material (s, MaterialType::Custom, MaterialType::Metal, 0.40f);
    topology (s, 3 /* LATTICE */, 0.58f, 0.40f, 1789);
    matter (s, 0.94f, 0.76f, 0.44f, 0.36f);
    set (s, Param::masterGain, 1.5f);
    evolve (s, 0.0f, 0.0f, 0.16f, 0.36f, 0.46f, 0.20f, 0.0f, 0.60f, 0.26f);
    set (s, Param::evolveMagnetTarget, 4 /* CHROMATIC */);
    set (s, Param::masterMode, 1 /* MONO */);
    set (s, Param::masterGlide, 0.03f);
    fracture (s, 1 /* RHYTHMIC */, 0.40f, 0.34f, 0.36f, 0.62f, 0.20f, 0.18f, 0.38f, 0.60f, 0.25f,
              1 /* 16 */, 5 /* 1/32 */, 8, 0.0f, 2 /* PINGPONG */, 0.80f, 0.30f, 1231,
              FractureShape { 16, 0.02f, 0.18f, 0.08f, 0.30f, 0.30f, 0.52f, 0.25f, 0.80f,
                              1.0f, 0.85f, 0.60f, 0.85f, nullptr, "XoXoXoXo", nullptr });
    space (s, SpacePresets::Machine, 0.22f, 0.32f, 0.54f, 0.26f);

    lfo (s, 1, 7.20f, 3 /* SQUARE */, 1.0f, true, 0.30f);
    chaos (s, 1, 0 /* WALK */, 4.50f, 0.35f, 0.72f, 0.5f, 811);
    env (s, 1, 0.006f, 0.22f, 0.25f, 0.18f, 0.3f);
    macros (s, 0.40f, 0.45f, 0.25f, 0.50f);

    Routings r;
    r.bi  (ModSource::LFO1,      Param::gestureSpeed,     0.130f)
     .bi  (ModSource::Chaos1,    Param::gestureRoughness, 0.160f)
     .uni (ModSource::Env1,      Param::gesturePressure,  0.260f)
     .uni (ModSource::Velocity,  Param::gestureSpeed,     0.280f)
     .bi  (ModSource::NoteRandom, Param::gesturePosition, 0.140f)
     .bi  (ModSource::KeyTrack,  Param::gestureBandwidth, 0.200f)
     .uni (ModSource::ModWheel,  Param::fractureAmount,   0.400f)
     .uni (ModSource::Macro1,    Param::fractureSequence, 0.380f)
     .uni (ModSource::Macro2,    Param::gestureBandwidth, 0.300f)
     .uni (ModSource::Macro2,    Param::fractureTone,     0.240f)
     .uni (ModSource::Macro3,    Param::spaceMix,         0.280f)
     .uni (ModSource::Macro4,    Param::gestureRoughness, 0.400f);
    sharedMacros (r, Param::fractureDecay, Param::fractureRandom);
    r.commit (s);
}});

manager.addFactory ({ "Anvil Song", "LEAD", { "metallic", "struck", "bright", "close", "melodic" }, [] (PatchState& s)
{
    impact (s, 2 /* PLUCK */, 0.78f, 0.66f, 0.16f, 0.70f, 0.40f, 0.10f, 0.0f, 0.72f);
    wave (s, 4 /* METALLIC */, 0.44f, 0.36f, 0.10f, 2, 0.06f, 0.26f, 0, 0.50f);
    set (s, Param::sourceMode, 1 /* LAYER */);
    set (s, Param::dustLevel, 0.0f);
    set (s, Param::sampleLevel, 0.0f);
    set (s, Param::gestureLevel, 0.0f);
    amp (s, 0.002f, 0.45f, 0.72f, 0.35f, 0.35f);
    shape (s, 0.40f, 0.54f, 0.34f, 0.72f, 0.62f, 0.34f);
    material (s, MaterialType::Metal, MaterialType::Liquid, 0.28f);
    topology (s, 1 /* RING */, 0.50f, 0.46f, 2003);
    matter (s, 0.76f, 0.58f, 0.52f, 0.38f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.42f, 0.44f, 0.10f, 0.0f, 0.30f, 0.18f);
    set (s, Param::evolveMagnetTarget, 0 /* OCTAVE */);
    set (s, Param::masterMode, 1 /* MONO */);
    set (s, Param::masterGlide, 0.02f);
    space (s, SpacePresets::Chamber, 0.22f, 0.28f, 0.60f, 0.20f);

    lfo (s, 1, 5.60f, 0 /* SINE */, 1.0f, true, 0.55f);
    env (s, 1, 0.002f, 0.18f, 0.0f, 0.15f, 0.25f);
    env (s, 2, 0.35f, 1.40f, 0.60f, 0.60f, 0.55f);
    macros (s, 0.25f, 0.50f, 0.25f, 0.45f);

    Routings r;
    r.bi  (ModSource::LFO1,      Param::shapePitch,       0.004f)
     .uni (ModSource::Env1,      Param::impactBrightness, 0.260f)
     .uni (ModSource::Env2,      Param::waveMorph,        0.220f)
     .uni (ModSource::Velocity,  Param::impactHardness,   0.320f)
     .uni (ModSource::Velocity,  Param::waveLevel,        0.180f)
     .bi  (ModSource::KeyTrack,  Param::impactLength,    -0.220f)
     .uni (ModSource::ModWheel,  Param::waveLevel,        0.300f)
     .uni (ModSource::Macro1,    Param::evolveMotion,     0.360f)
     .uni (ModSource::Macro2,    Param::wavePosition,     0.300f)
     .uni (ModSource::Macro2,    Param::shapeExcite,      0.220f)
     .uni (ModSource::Macro3,    Param::spaceMix,         0.300f)
     .uni (ModSource::Macro4,    Param::impactHardness,   0.340f);
    sharedMacros (r, Param::ampDecay, Param::impactRandom);
    r.commit (s);
}});

manager.addFactory ({ "Wolf Note", "LEAD", { "organic", "unstable", "bowed", "mid", "melodic" }, [] (PatchState& s)
{
    gesture (s, 0 /* BOW */, 0.54f, 0.44f, 0.40f, 0.52f, 0.34f, 0.44f, 0.80f);
    amp (s, 0.07f, 0.55f, 0.78f, 0.42f, 0.5f);
    shape (s, 0.44f, 0.34f, 0.46f, 0.56f, 0.60f, 0.44f);
    material (s, MaterialType::String, MaterialType::Wood, 0.44f);
    topology (s, 4 /* RANDOM */, 0.56f, 0.48f, 2311);
    matter (s, 0.50f, 0.42f, 0.24f, 0.44f);
    evolve (s, 0.0f, 0.0f, 0.36f, 0.22f, 0.50f, 0.22f, 0.0f, 0.42f, 0.40f);
    set (s, Param::evolveScatterSeed, 1553);
    set (s, Param::masterMode, 2 /* LEGATO */);
    set (s, Param::masterGlide, 0.12f);
    space (s, SpacePresets::Chamber, 0.30f, 0.46f, 0.48f, 0.30f);

    lfo (s, 1, 5.30f, 0 /* SINE */, 1.0f, true, 0.60f);
    chaos (s, 1, 3 /* LORENZ */, 1.60f, 0.45f, 0.55f, 0.5f, 1699);
    chaos (s, 2, 1 /* BROWNIAN */, 0.40f, 0.35f, 0.70f, 0.5f, 1811);
    env (s, 1, 0.09f, 0.60f, 0.35f, 0.40f, 0.5f);
    macros (s, 0.40f, 0.40f, 0.30f, 0.50f);

    Routings r;
    r.bi  (ModSource::LFO1,      Param::shapePitch,       0.005f)
     .bi  (ModSource::Chaos1,    Param::gesturePressure,  0.170f)
     .bi  (ModSource::Chaos2,    Param::evolveTear,       0.140f)
     .uni (ModSource::Env1,      Param::gestureSpeed,     0.220f)
     .uni (ModSource::Velocity,  Param::gesturePressure,  0.280f)
     .bi  (ModSource::NoteRandom, Param::shapeDistribution, 0.180f)
     .uni (ModSource::ModWheel,  Param::evolveTear,       0.320f)
     .uni (ModSource::Macro1,    Param::evolveMotion,     0.400f)
     .uni (ModSource::Macro2,    Param::gestureBandwidth, 0.300f)
     .uni (ModSource::Macro2,    Param::shapeSurface,     0.200f)
     .uni (ModSource::Macro3,    Param::spaceMix,         0.300f)
     .uni (ModSource::Macro4,    Param::evolveTear,       0.360f);
    sharedMacros (r, Param::ampRelease, Param::chaos1Depth);
    r.commit (s);
}});

manager.addFactory ({ "Neon Thread", "LEAD", { "synthetic", "cold", "bright", "high", "melodic" }, [] (PatchState& s)
{
    gesture (s, 5 /* ELECTRICAL */, 0.54f, 0.74f, 0.30f, 0.24f, 0.20f, 0.30f);
    amp (s, 0.005f, 0.30f, 0.90f, 0.24f, 0.35f);
    shape (s, 0.38f, 0.78f, 0.22f, 0.78f, 0.52f, 0.28f);
    material (s, MaterialType::Crystal, MaterialType::Metal, 0.48f);
    topology (s, 3 /* LATTICE */, 0.36f, 0.64f, 2687);
    matter (s, 0.92f, 0.70f, 0.40f, 0.48f);
    evolve (s, 0.18f, 0.0f, 0.0f, 0.44f, 0.34f, 0.14f, 0.0f, 0.44f, 0.20f);
    set (s, Param::evolveBendPivot, 0.70f);
    set (s, Param::evolveMagnetTarget, 4 /* CHROMATIC */);
    set (s, Param::masterMode, 2 /* LEGATO */);
    set (s, Param::masterGlide, 0.06f);
    fracture (s, 0 /* SPECTRAL */, 0.36f, 0.30f, 0.62f, 0.30f, 0.34f, 0.16f, 0.48f, 0.66f, 0.22f,
              2 /* 32 */, 7 /* 1/8T */, 6, 0.0f, 0 /* FORWARD */, 1.0f, 0.12f, 3001,
              FractureShape { 32, 0.03f, 0.22f, 0.16f, 0.40f, 0.36f, 0.60f, 0.30f, 0.90f,
                              1.0f, 0.80f, 0.60f, 1.0f, kFifthTerrace, "XHXLXH", nullptr });
    space (s, SpacePresets::Orbit, 0.26f, 0.36f, 0.64f, 0.38f);

    lfo (s, 1, 6.80f, 0 /* SINE */, 1.0f, true, 0.40f);
    lfo (s, 2, 0.52f, 2 /* SAW */, 1.0f, true);
    env (s, 1, 0.004f, 0.26f, 0.20f, 0.18f, 0.3f);
    macros (s, 0.35f, 0.55f, 0.30f, 0.45f);

    Routings r;
    r.bi  (ModSource::LFO1,      Param::shapePitch,       0.004f)
     .bi  (ModSource::LFO2,      Param::gestureBandwidth, 0.150f)
     .uni (ModSource::Env1,      Param::gestureSpeed,     0.240f)
     .uni (ModSource::Velocity,  Param::gesturePressure,  0.300f)
     .bi  (ModSource::KeyTrack,  Param::fractureTone,    -0.200f)
     .bi  (ModSource::NoteRandom, Param::fractureSeed,    0.060f)
     .uni (ModSource::ModWheel,  Param::evolveBend,       0.300f)
     .uni (ModSource::Macro1,    Param::fractureEvolve,   0.380f)
     .uni (ModSource::Macro2,    Param::gestureBandwidth, 0.320f)
     .uni (ModSource::Macro2,    Param::fractureTone,     0.240f)
     .uni (ModSource::Macro3,    Param::spaceMix,         0.300f)
     .uni (ModSource::Macro4,    Param::gestureRoughness, 0.360f);
    sharedMacros (r, Param::fractureDecay, Param::fractureSpread);
    r.commit (s);
}});

manager.addFactory ({ "Tar Reed", "LEAD", { "dark", "morphing", "hollow", "low", "melodic" }, [] (PatchState& s)
{
    wave (s, 5 /* SPECTRAL */, 0.42f, 0.56f, 0.20f, 2, 0.11f, 0.30f, 0, 0.80f);
    set (s, Param::waveFM, 0.12f);
    set (s, Param::waveModRatio, 0.5f);
    amp (s, 0.03f, 0.50f, 0.88f, 0.45f, 0.5f);
    shape (s, 0.60f, 0.38f, 0.66f, 0.46f, 0.64f, 0.36f);
    material (s, MaterialType::Organic, MaterialType::Membrane, 0.46f);
    topology (s, 4 /* RANDOM */, 0.48f, 0.42f, 3167);
    matter (s, 0.64f, 0.50f, 0.28f, 0.42f);
    evolve (s, 0.0f, 0.46f, 0.0f, 0.20f, 0.58f, 0.12f, 0.0f, 0.28f, 0.36f);
    set (s, Param::evolveMagnetTarget, 3 /* MINOR */);
    set (s, Param::masterMode, 2 /* LEGATO */);
    set (s, Param::masterGlide, 0.15f);
    space (s, SpacePresets::Void, 0.28f, 0.54f, 0.30f, 0.32f);

    lfo (s, 1, 4.70f, 0 /* SINE */, 1.0f, true, 0.75f);
    lfo (s, 2, 0.16f, 1 /* TRIANGLE */, 1.0f, false);
    env (s, 2, 0.70f, 2.20f, 0.80f, 0.90f, 0.65f);
    macros (s, 0.35f, 0.35f, 0.30f, 0.50f);

    Routings r;
    r.bi  (ModSource::LFO1,      Param::shapePitch,   0.006f)
     .bi  (ModSource::LFO2,      Param::waveScan,     0.120f)
     .uni (ModSource::Env2,      Param::evolveMelt,   0.260f)
     .uni (ModSource::Env2,      Param::waveMorph,    0.200f)
     .uni (ModSource::Velocity,  Param::waveFM,       0.180f)
     .bi  (ModSource::KeyTrack,  Param::shapeMass,   -0.240f)
     .uni (ModSource::ModWheel,  Param::evolveMelt,   0.340f)
     .uni (ModSource::Macro1,    Param::evolveMotion, 0.400f)
     .uni (ModSource::Macro2,    Param::wavePosition, 0.300f)
     .uni (ModSource::Macro2,    Param::shapeExcite,  0.220f)
     .uni (ModSource::Macro3,    Param::spaceMix,     0.300f)
     .uni (ModSource::Macro4,    Param::evolveMelt,   0.380f);
    sharedMacros (r, Param::ampRelease, Param::evolveGravity);
    r.commit (s);
}});

manager.addFactory ({ "Ice Pick", "LEAD", { "cold", "harsh", "glassy", "air", "melodic" }, [] (PatchState& s)
{
    wave (s, 4 /* METALLIC */, 0.72f, 0.30f, 0.24f, 1, 0.05f, 0.16f);
    set (s, Param::waveSync, 0.34f);
    set (s, Param::waveModRatio, 4.0f);
    amp (s, 0.004f, 0.26f, 0.84f, 0.20f, 0.3f);
    shape (s, 0.30f, 0.82f, 0.18f, 0.80f, 0.44f, 0.22f);
    material (s, MaterialType::Crystal, MaterialType::Void, 0.30f);
    topology (s, 5 /* STAR */, 0.28f, 0.70f, 3559);
    matter (s, 0.76f, 0.62f, 0.48f, 0.30f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.30f, 0.30f, 0.10f, 0.0f, 0.50f, 0.16f);
    set (s, Param::evolveMagnetTarget, 4 /* CHROMATIC */);
    set (s, Param::masterMode, 1 /* MONO */);
    fracture (s, 2 /* TRANSIENT */, 0.30f, 0.26f, 0.50f, 0.24f, 0.18f, 0.14f, 0.34f, 0.72f, 0.14f,
              0 /* 8 */, 4 /* 1/16 */, 4, 0.0f, 0 /* FORWARD */, 0.75f, 0.20f, 4127,
              FractureShape { 8, 0.02f, 0.14f, 0.06f, 0.24f, 0.28f, 0.46f, 0.30f, 0.85f,
                              1.0f, 0.70f, 0.55f, 0.75f, kOctaveTerrace, "XHXH", nullptr });
    space (s, SpacePresets::Chamber, 0.18f, 0.26f, 0.72f, 0.18f);

    lfo (s, 1, 7.80f, 1 /* TRIANGLE */, 1.0f, true, 0.25f);
    lfo (s, 2, 6.10f, 0 /* SINE */, 1.0f, true, 0.35f);
    env (s, 1, 0.003f, 0.16f, 0.10f, 0.14f, 0.25f);
    macros (s, 0.30f, 0.55f, 0.20f, 0.45f);

    Routings r;
    r.bi  (ModSource::LFO2,      Param::shapePitch,     0.004f)
     .uni (ModSource::LFO1,      Param::shapeExcite,    0.160f)
     .uni (ModSource::Env1,      Param::waveSync,       0.280f)
     .uni (ModSource::Velocity,  Param::waveSync,       0.220f)
     .uni (ModSource::Velocity,  Param::shapeStrike,    0.280f)
     .bi  (ModSource::KeyTrack,  Param::shapeSurface,  -0.180f)
     .uni (ModSource::ModWheel,  Param::lfo1Depth,      0.500f)
     .uni (ModSource::Macro1,    Param::lfo1Depth,      0.400f)
     .uni (ModSource::Macro2,    Param::wavePosition,   0.300f)
     .uni (ModSource::Macro2,    Param::fractureTone,   0.240f)
     .uni (ModSource::Macro3,    Param::spaceMix,       0.280f)
     .uni (ModSource::Macro4,    Param::waveSync,       0.400f);
    sharedMacros (r, Param::fractureDecay, Param::fractureRandom);
    r.commit (s);
}});

manager.addFactory ({ "Paper Trumpet", "LEAD", { "organic", "blown", "granular", "close", "melodic" }, [] (PatchState& s)
{
    sample (s, BuiltInSamples::Kind::Breath, 3 /* GRANULAR */, 0.06f, 0.78f, 0.46f, 0.54f, 60);
    amp (s, 0.05f, 0.45f, 0.95f, 0.35f, 0.5f);
    shape (s, 0.52f, 0.28f, 0.48f, 0.58f, 0.58f, 0.26f);
    material (s, MaterialType::Membrane, MaterialType::Wood, 0.42f);
    topology (s, 2 /* CLUSTERS */, 0.46f, 0.54f, 4231);
    matter (s, 0.96f, 0.86f, 0.34f, 0.40f);
    set (s, Param::masterGain, 1.2f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.40f, 0.48f, 0.14f, 0.0f, 0.32f, 0.28f);
    set (s, Param::evolveMagnetTarget, 6 /* CUSTOM (harmonic) */);
    set (s, Param::masterMode, 2 /* LEGATO */);
    set (s, Param::masterGlide, 0.10f);
    space (s, SpacePresets::Dust, 0.24f, 0.34f, 0.52f, 0.26f);

    lfo (s, 1, 4.80f, 0 /* SINE */, 1.0f, true, 0.70f);
    lfo (s, 2, 0.42f, 5 /* SMOOTH RANDOM */, 1.0f, true);
    env (s, 1, 0.04f, 0.50f, 0.40f, 0.35f, 0.5f);
    macros (s, 0.30f, 0.45f, 0.30f, 0.45f);

    Routings r;
    r.bi  (ModSource::LFO1,       Param::shapePitch,   0.006f)
     .bi  (ModSource::LFO2,       Param::sampleStart,  0.090f)
     .uni (ModSource::Env1,       Param::sampleGrain,  0.240f)
     .uni (ModSource::Velocity,   Param::sampleLevel,  0.200f)
     .uni (ModSource::Velocity,   Param::shapeExcite,  0.240f)
     .bi  (ModSource::KeyTrack,   Param::sampleSpread, 0.180f)
     .bi  (ModSource::NoteRandom, Param::sampleStart,  0.070f)
     .uni (ModSource::ModWheel,   Param::lfo1Depth,    0.520f)
     .uni (ModSource::Macro1,     Param::lfo2Depth,    0.420f)
     .uni (ModSource::Macro2,     Param::shapeTension, 0.260f)
     .uni (ModSource::Macro3,     Param::spaceMix,     0.300f)
     .uni (ModSource::Macro4,     Param::sampleGrain,  0.380f);
    sharedMacros (r, Param::ampRelease, Param::sampleSpread);
    r.commit (s);
}});

manager.addFactory ({ "Bell Tongue", "LEAD", { "metallic", "struck", "resonant", "roomy", "melodic" }, [] (PatchState& s)
{
    impact (s, 5 /* DAMPED SINE */, 0.52f, 0.44f, 0.34f, 0.82f, 0.50f, 0.08f, 0.0f, 0.78f);
    sample (s, BuiltInSamples::Kind::MetalPing, 0 /* ONE SHOT */, 0.0f, 0.58f, 0.22f, 0.28f, 60, 0.46f);
    set (s, Param::sourceMode, 1 /* LAYER */);
    set (s, Param::waveLevel, 0.0f);
    set (s, Param::dustLevel, 0.0f);
    set (s, Param::gestureLevel, 0.0f);
    amp (s, 0.004f, 0.60f, 0.78f, 0.60f, 0.4f);
    shape (s, 0.42f, 0.62f, 0.40f, 0.66f, 0.70f, 0.24f);
    material (s, MaterialType::Metal, MaterialType::Crystal, 0.44f);
    topology (s, 1 /* RING */, 0.52f, 0.50f, 4691);
    matter (s, 0.86f, 0.58f, 0.56f, 0.52f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.50f, 0.46f, 0.12f, 0.0f, 0.26f, 0.22f);
    set (s, Param::evolveMagnetTarget, 0 /* OCTAVE */);
    set (s, Param::masterMode, 1 /* MONO */);
    set (s, Param::masterGlide, 0.03f);
    space (s, SpacePresets::Dream, 0.34f, 0.52f, 0.54f, 0.34f);

    lfo (s, 1, 5.20f, 0 /* SINE */, 1.0f, true, 0.60f);
    env (s, 1, 0.003f, 0.24f, 0.0f, 0.20f, 0.3f);
    env (s, 2, 0.50f, 1.80f, 0.55f, 0.80f, 0.6f);
    macros (s, 0.30f, 0.45f, 0.40f, 0.45f);

    Routings r;
    r.bi  (ModSource::LFO1,       Param::shapePitch,       0.004f)
     .uni (ModSource::Env1,       Param::impactBrightness, 0.240f)
     .uni (ModSource::Env2,       Param::evolveMagnet,     0.200f)
     .uni (ModSource::Velocity,   Param::impactHardness,   0.300f)
     .uni (ModSource::Velocity,   Param::sampleLevel,      0.200f)
     .bi  (ModSource::KeyTrack,   Param::sampleSpread,    -0.160f)
     .bi  (ModSource::NoteRandom, Param::impactRandom,     0.120f)
     .uni (ModSource::ModWheel,   Param::sampleLevel,      0.280f)
     .uni (ModSource::Macro1,     Param::evolveMotion,     0.360f)
     .uni (ModSource::Macro2,     Param::impactBrightness, 0.320f)
     .uni (ModSource::Macro3,     Param::spaceMix,         0.320f)
     .uni (ModSource::Macro4,     Param::shapeBlend,       0.380f);
    sharedMacros (r, Param::ampDecay, Param::impactRandom);
    r.commit (s);
}});

manager.addFactory ({ "Rising Iron", "LEAD", { "metallic", "evolving", "bright", "mid", "melodic" }, [] (PatchState& s)
{
    wave (s, 4 /* METALLIC */, 0.38f, 0.44f, 0.18f, 2, 0.08f, 0.30f);
    set (s, Param::waveFM, 0.10f);
    set (s, Param::waveModRatio, 3.0f);
    amp (s, 0.010f, 0.40f, 0.88f, 0.40f, 0.45f);
    shape (s, 0.50f, 0.60f, 0.42f, 0.64f, 0.62f, 0.30f);
    material (s, MaterialType::Metal, MaterialType::Crystal, 0.40f);
    topology (s, 3 /* LATTICE */, 0.50f, 0.52f, 5273);
    matter (s, 0.88f, 0.60f, 0.38f, 0.46f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.34f, 0.66f, 0.10f, 0.0f, 0.30f, 0.24f);
    set (s, Param::evolveMagnetTarget, 2 /* MAJOR */);
    set (s, Param::masterMode, 2 /* LEGATO */);
    set (s, Param::masterGlide, 0.08f);
    space (s, SpacePresets::Dream, 0.30f, 0.48f, 0.56f, 0.32f);

    lfo (s, 1, 5.70f, 0 /* SINE */, 1.0f, true, 0.55f);
    lfo (s, 2, 0.26f, 1 /* TRIANGLE */, 1.0f, false);
    env (s, 2, 0.80f, 2.40f, 0.85f, 1.00f, 0.6f);
    macros (s, 0.35f, 0.50f, 0.30f, 0.50f);

    Routings r;
    r.bi  (ModSource::LFO1,      Param::shapePitch,     0.005f)
     .bi  (ModSource::LFO2,      Param::waveScan,       0.110f)
     .uni (ModSource::Env2,      Param::evolveGravity, -0.320f)
     .uni (ModSource::Env2,      Param::shapeForm,      0.140f)
     .uni (ModSource::Velocity,  Param::waveFM,         0.200f)
     .bi  (ModSource::KeyTrack,  Param::evolveGravity, -0.120f)
     .uni (ModSource::ModWheel,  Param::lfo1Depth,      0.500f)
     .uni (ModSource::Macro1,    Param::evolveMotion,   0.400f)
     .uni (ModSource::Macro2,    Param::wavePosition,   0.320f)
     .uni (ModSource::Macro2,    Param::shapeExcite,    0.220f)
     .uni (ModSource::Macro3,    Param::spaceMix,       0.300f)
     .uni (ModSource::Macro4,    Param::evolveGravity, -0.300f);
    sharedMacros (r, Param::ampRelease, Param::evolveScatter);
    r.commit (s);
}});

manager.addFactory ({ "Hollow Reed", "LEAD", { "wooden", "hollow", "blown", "dry", "melodic" }, [] (PatchState& s)
{
    gesture (s, 3 /* BREATH */, 0.48f, 0.34f, 0.30f, 0.58f, 0.22f, 0.40f);
    amp (s, 0.06f, 0.48f, 0.88f, 0.30f, 0.5f);
    shape (s, 0.36f, 0.18f, 0.50f, 0.48f, 0.56f, 0.30f);
    material (s, MaterialType::Wood, MaterialType::Void, 0.34f);
    topology (s, 0 /* CHAIN */, 0.38f, 0.46f, 5807);
    matter (s, 0.96f, 0.62f, 0.28f, 0.30f);
    evolve (s, 0.0f, 0.12f, 0.0f, 0.30f, 0.52f, 0.10f, 0.0f, 0.24f, 0.26f);
    set (s, Param::evolveMagnetTarget, 6 /* CUSTOM (harmonic) */);
    set (s, Param::masterMode, 2 /* LEGATO */);
    set (s, Param::masterGlide, 0.09f);
    space (s, SpacePresets::Chamber, 0.16f, 0.24f, 0.44f, 0.18f);

    lfo (s, 1, 4.40f, 0 /* SINE */, 1.0f, true, 0.85f);
    lfo (s, 2, 0.30f, 5 /* SMOOTH RANDOM */, 1.0f, false);
    env (s, 1, 0.05f, 0.40f, 0.30f, 0.30f, 0.5f);
    macros (s, 0.25f, 0.40f, 0.25f, 0.45f);

    Routings r;
    r.bi  (ModSource::LFO1,       Param::shapePitch,       0.007f)
     .bi  (ModSource::LFO2,       Param::gestureRoughness, 0.140f)
     .uni (ModSource::Env1,       Param::gesturePressure,  0.240f)
     .uni (ModSource::Velocity,   Param::gestureSpeed,     0.260f)
     .bi  (ModSource::NoteRandom, Param::gesturePosition,  0.120f)
     .bi  (ModSource::KeyTrack,   Param::gestureBandwidth, 0.180f)
     .uni (ModSource::Pressure,   Param::gesturePressure,  0.300f)
     .uni (ModSource::ModWheel,   Param::lfo1Depth,        0.560f)
     .uni (ModSource::Macro1,     Param::gestureMotion,    0.380f)
     .uni (ModSource::Macro2,     Param::gestureBandwidth, 0.320f)
     .uni (ModSource::Macro3,     Param::spaceMix,         0.340f)
     .uni (ModSource::Macro4,     Param::shapeForm,        0.300f);
    sharedMacros (r, Param::ampRelease, Param::gestureRoughness);
    r.commit (s);
}});

manager.addFactory ({ "Copper Kite", "LEAD", { "bright", "noisy", "bowed", "wide", "melodic" }, [] (PatchState& s)
{
    gesture (s, 0 /* BOW */, 0.58f, 0.50f, 0.26f, 0.40f, 0.24f, 0.50f, 0.86f);
    dust (s, 3 /* BLUE */, 0.44f, 0.66f, 0.24f, 0.34f, 0.62f, 0.70f, 3319, 0.22f);
    set (s, Param::sourceMode, 1 /* LAYER */);
    set (s, Param::waveLevel, 0.0f);
    set (s, Param::impactLevel, 0.0f);
    set (s, Param::sampleLevel, 0.0f);
    amp (s, 0.04f, 0.55f, 0.86f, 0.50f, 0.5f);
    shape (s, 0.46f, 0.56f, 0.36f, 0.60f, 0.60f, 0.32f);
    material (s, MaterialType::Metal, MaterialType::Liquid, 0.46f);
    topology (s, 2 /* CLUSTERS */, 0.44f, 0.58f, 6203);
    matter (s, 0.84f, 0.60f, 0.30f, 0.68f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.36f, 0.44f, 0.16f, 0.0f, 0.34f, 0.30f);
    set (s, Param::evolveMagnetTarget, 1 /* FIFTH */);
    set (s, Param::masterMode, 2 /* LEGATO */);
    set (s, Param::masterGlide, 0.10f);
    fracture (s, 0 /* SPECTRAL */, 0.32f, 0.28f, 0.66f, 0.28f, 0.30f, 0.30f, 0.52f, 0.58f, 0.26f,
              1 /* 16 */, 6 /* 1/4T */, 4, 0.0f, 3 /* RANDOM */, 0.85f, 0.24f, 6673,
              FractureShape { 16, 0.05f, 0.34f, 0.14f, 0.38f, 0.40f, 0.62f, 0.35f, 0.95f,
                              1.0f, 0.75f, 0.70f, 0.85f, kFifthTerrace, "XLXH", nullptr });
    space (s, SpacePresets::Nebula, 0.34f, 0.58f, 0.56f, 0.32f);

    lfo (s, 1, 5.10f, 0 /* SINE */, 1.0f, true, 0.65f);
    lfo (s, 2, 0.23f, 5 /* SMOOTH RANDOM */, 1.0f, false);
    env (s, 1, 0.03f, 0.50f, 0.35f, 0.40f, 0.5f);
    macros (s, 0.35f, 0.45f, 0.40f, 0.45f);

    Routings r;
    r.bi  (ModSource::LFO1,      Param::shapePitch,       0.005f)
     .bi  (ModSource::LFO2,      Param::dustColor,        0.150f)
     .uni (ModSource::Env1,      Param::dustDensity,      0.220f)
     .uni (ModSource::Velocity,  Param::gesturePressure,  0.280f)
     .uni (ModSource::Velocity,  Param::dustLevel,        0.150f)
     .bi  (ModSource::KeyTrack,  Param::dustColor,       -0.180f)
     .uni (ModSource::ModWheel,  Param::lfo1Depth,        0.520f)
     .uni (ModSource::Macro1,    Param::gestureMotion,    0.400f)
     .uni (ModSource::Macro2,    Param::gestureBandwidth, 0.300f)
     .uni (ModSource::Macro2,    Param::fractureTone,     0.220f)
     .uni (ModSource::Macro3,    Param::spaceMix,         0.320f)
     .uni (ModSource::Macro4,    Param::fractureAmount,   0.340f);
    sharedMacros (r, Param::fractureDecay, Param::dustJitter);
    r.commit (s);
}});

manager.addFactory ({ "Bent Spire", "LEAD", { "glassy", "evolving", "cold", "high", "melodic" }, [] (PatchState& s)
{
    wave (s, 6 /* FRACTURED */, 0.46f, 0.38f, 0.30f, 2, 0.07f, 0.26f);
    amp (s, 0.008f, 0.36f, 0.86f, 0.36f, 0.4f);
    shape (s, 0.44f, 0.70f, 0.28f, 0.74f, 0.58f, 0.24f);
    material (s, MaterialType::Crystal, MaterialType::Metal, 0.38f);
    topology (s, 5 /* STAR */, 0.34f, 0.60f, 7127);
    matter (s, 0.76f, 0.56f, 0.36f, 0.54f);
    evolve (s, 0.24f, 0.0f, 0.0f, 0.30f, 0.46f, 0.12f, 0.0f, 0.36f, 0.20f);
    set (s, Param::evolveBendPivot, 0.42f);
    set (s, Param::evolveBendRange, 0.34f);
    set (s, Param::evolveMagnetTarget, 5 /* SCALE */);
    set (s, Param::masterMode, 2 /* LEGATO */);
    set (s, Param::masterGlide, 0.07f);
    space (s, SpacePresets::Shimmer, 0.30f, 0.50f, 0.62f, 0.30f);

    lfo (s, 1, 6.00f, 0 /* SINE */, 1.0f, true, 0.50f);
    lfo (s, 2, 0.34f, 1 /* TRIANGLE */, 1.0f, true);
    env (s, 2, 0.55f, 2.00f, 0.80f, 0.85f, 0.6f);
    macros (s, 0.35f, 0.50f, 0.35f, 0.50f);

    Routings r;
    r.bi  (ModSource::LFO1,      Param::shapePitch,     0.004f)
     .bi  (ModSource::LFO2,      Param::waveMorph,      0.130f)
     .uni (ModSource::Env2,      Param::evolveBend,     0.300f)
     .uni (ModSource::Env2,      Param::evolveBendPivot, 0.180f)
     .uni (ModSource::Velocity,  Param::waveScan,       0.240f)
     .bi  (ModSource::KeyTrack,  Param::evolveBendPivot, 0.200f)
     .uni (ModSource::ModWheel,  Param::evolveBend,     0.320f)
     .uni (ModSource::Macro1,    Param::evolveMotion,   0.400f)
     .uni (ModSource::Macro2,    Param::wavePosition,   0.300f)
     .uni (ModSource::Macro2,    Param::shapeExcite,    0.220f)
     .uni (ModSource::Macro3,    Param::spaceMix,       0.300f)
     .uni (ModSource::Macro4,    Param::evolveBendRange, 0.360f);
    sharedMacros (r, Param::ampRelease, Param::evolveBendCurve);
    r.commit (s);
}});
}

} // namespace am::FactoryContent
