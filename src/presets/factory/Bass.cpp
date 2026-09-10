#include "presets/FactoryBuilders.h"

namespace am::FactoryContent
{

void registerBass (PresetManager& manager)
{
//==========================================================================
// BASS — weight that still belongs to this instrument
//==========================================================================

manager.addFactory ({ "Carbon Bass", "BASS", { "bass", "organic", "mono", "dark" }, [] (PatchState& s)
{
    wave (s, 3 /* FOLDED */, 0.22f, 0.18f, 0.0f, 2, 0.08f, 0.30f, -1);
    set (s, Param::waveRing, 0.12f);
    set (s, Param::waveModRatio, 1.5f);
    amp (s, 0.003f, 0.45f, 0.68f, 0.20f, 0.35f);
    shape (s, 0.26f, 0.05f, 0.80f, 0.34f, 0.30f, 0.42f);
    material (s, MaterialType::Wood, MaterialType::Organic, 0.35f);
    topology (s, 0 /* CHAIN */, 0.30f, 0.35f, 53);
    matter (s, 0.82f, 0.45f, 0.45f, 0.35f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.30f, 0.62f, 0.0f, 0.0f, 0.30f, 0.10f);
    set (s, Param::evolveMagnetTarget, 0 /* OCTAVE */);
    set (s, Param::masterMode, 1 /* MONO */);
    set (s, Param::masterGlide, 0.06f);
    space (s, SpacePresets::Chamber, 0.16f, 0.30f, 0.45f, 0.20f);

    env (s, 1, 0.001f, 0.28f, 0.0f, 0.20f, 0.3f);
    lfo (s, 1, 4.80f, 0 /* SINE */, 1.0f, true, 0.35f);
    macros (s, 0.20f, 0.35f, 0.20f, 0.40f);

    Routings r;
    r.uni (ModSource::Env1,   Param::shapeSurface,   0.260f)
     .uni (ModSource::Env1,   Param::waveRing,       0.180f)
     .bi  (ModSource::LFO1,   Param::shapePitch,     0.006f)
     .uni (ModSource::Velocity, Param::shapeStrike,  0.300f)
     .bi  (ModSource::KeyTrack, Param::shapeMass,   -0.150f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.300f)
     .uni (ModSource::Macro2, Param::shapeExcite,    0.350f)
     .uni (ModSource::Macro2, Param::waveMorph,      0.250f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.250f)
     .uni (ModSource::Macro4, Param::waveRing,       0.400f)
     .uni (ModSource::Macro4, Param::shapeSurface,   0.200f);
    sharedMacros (r, Param::ampDecay, Param::evolveTear);
    r.commit (s);
}});

manager.addFactory ({ "Torn Bass", "BASS", { "bass", "aggressive", "distorted", "tear" }, [] (PatchState& s)
{
    wave (s, 6 /* FRACTURED */, 0.48f, 0.30f, 0.05f, 3, 0.14f, 0.45f, -1);
    amp (s, 0.002f, 0.35f, 0.72f, 0.24f, 0.3f);
    set (s, Param::masterGain, -3.0f);   // struck material has a hot transient: keep headroom at the bottom of the keyboard
    shape (s, 0.42f, 0.52f, 0.72f, 0.40f, 0.34f, 0.52f);
    material (s, MaterialType::Metal, MaterialType::Wood, 0.55f);
    topology (s, 4 /* RANDOM */, 0.55f, 0.40f, 89);
    matter (s, 0.88f, 0.55f, 0.58f, 0.45f);
    evolve (s, 0.0f, 0.0f, 0.42f, 0.24f, 0.58f, 0.20f, 0.16f, 0.42f, 0.30f);
    set (s, Param::evolveScatterSeed, 617);
    set (s, Param::masterMode, 1 /* MONO */);
    set (s, Param::masterGlide, 0.03f);
    space (s, SpacePresets::Machine, 0.22f, 0.28f, 0.42f, 0.30f);
    set (s, Param::spaceDistDrive, 0.42f);

    env (s, 1, 0.001f, 0.22f, 0.10f, 0.18f, 0.25f);
    lfo (s, 1, 7.20f, 3 /* SQUARE */, 1.0f, true, 0.15f);
    chaos (s, 1, 2 /* LOGISTIC */, 3.40f, 0.45f, 0.80f, 0.5f, 733);
    macros (s, 0.30f, 0.45f, 0.20f, 0.45f);

    Routings r;
    r.uni (ModSource::Env1,   Param::evolveTear,     0.300f)
     .uni (ModSource::Env1,   Param::shapeSurface,   0.220f)
     .bi  (ModSource::LFO1,   Param::evolveCrush,    0.080f)
     .bi  (ModSource::Chaos1, Param::evolveScatter,  0.100f)
     .uni (ModSource::Velocity, Param::spaceDistDrive, 0.220f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.350f)
     .uni (ModSource::Macro2, Param::shapeExcite,    0.300f)
     .uni (ModSource::Macro2, Param::spaceTone,      0.250f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.250f)
     .uni (ModSource::Macro4, Param::evolveTear,     0.400f)
     .uni (ModSource::Macro4, Param::evolveCrush,    0.200f);
    sharedMacros (r, Param::ampDecay, Param::chaos1Depth);
    r.commit (s);
}});

manager.addFactory ({ "Rust Sub", "BASS", { "bass", "sub", "gritty", "gesture" }, [] (PatchState& s)
{
    gesture (s, 4 /* FRICTION */, 0.58f, 0.26f, 0.28f, 0.35f, 0.18f, 0.22f);
    amp (s, 0.02f, 0.60f, 0.80f, 0.35f, 0.4f);
    shape (s, 0.22f, 0.02f, 0.86f, 0.30f, 0.28f, 0.36f);
    material (s, MaterialType::Organic, MaterialType::Membrane, 0.30f);
    topology (s, 0 /* CHAIN */, 0.26f, 0.30f, 167);
    matter (s, 0.92f, 0.30f, 0.25f, 0.30f);
    evolve (s, 0.0f, 0.12f, 0.0f, 0.36f, 0.66f, 0.0f, 0.0f, 0.22f, 0.15f);
    set (s, Param::evolveMagnetTarget, 0 /* OCTAVE */);
    set (s, Param::masterMode, 2 /* LEGATO */);
    set (s, Param::masterGlide, 0.12f);
    space (s, SpacePresets::Void, 0.14f, 0.45f, 0.30f, 0.25f);

    lfo (s, 1, 0.35f, 1 /* TRIANGLE */, 1.0f, true, 0.5f);
    env (s, 1, 0.05f, 0.80f, 0.40f, 0.60f);
    macros (s, 0.25f, 0.30f, 0.20f, 0.50f);

    Routings r;
    r.bi  (ModSource::LFO1,   Param::gesturePressure, 0.120f)
     .uni (ModSource::Env1,   Param::gestureSpeed,    0.250f)
     .uni (ModSource::Velocity, Param::gestureRoughness, 0.300f)
     .bi  (ModSource::KeyTrack, Param::gestureBandwidth, 0.180f)
     .uni (ModSource::Macro1, Param::gestureMotion,   0.400f)
     .uni (ModSource::Macro2, Param::gestureBandwidth, 0.350f)
     .uni (ModSource::Macro2, Param::shapeExcite,     0.250f)
     .uni (ModSource::Macro3, Param::spaceMix,        0.250f)
     .uni (ModSource::Macro4, Param::gestureRoughness, 0.400f)
     .uni (ModSource::Macro4, Param::shapeSurface,    0.250f);
    sharedMacros (r, Param::ampDecay, Param::gestureRoughness);
    r.commit (s);
}});
}

} // namespace am::FactoryContent
