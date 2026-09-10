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
}

} // namespace am::FactoryContent
