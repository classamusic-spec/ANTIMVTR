#include "presets/FactoryBuilders.h"

namespace am::FactoryContent
{

void registerKeys (PresetManager& manager)
{
//==========================================================================
// KEYS — struck and played, an object under the fingers
//==========================================================================

manager.addFactory ({ "Crystal Ghost", "KEYS", { "keys", "bells", "crystal", "bright" }, [] (PatchState& s)
{
    impact (s, 4 /* METAL STRIKE */, 0.72f, 0.68f, 0.18f, 0.80f, 0.45f, 0.12f);
    amp (s, 0.001f, 1.60f, 0.0f, 1.40f, 0.35f);
    shape (s, 0.46f, 0.84f, 0.24f, 0.68f, 0.82f, 0.10f);
    material (s, MaterialType::Crystal, MaterialType::Void, 0.30f);
    topology (s, 5 /* STAR */, 0.28f, 0.62f, 3);
    matter (s, 0.98f, 0.62f, 0.72f, 0.70f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.52f, 0.44f, 0.08f, 0.0f, 0.25f, 0.12f);
    set (s, Param::evolveMagnetTarget, 5 /* SCALE */);
    space (s, SpacePresets::Shimmer, 0.40f, 0.60f, 0.62f, 0.30f);

    env (s, 1, 0.001f, 0.90f, 0.0f, 0.90f, 0.3f);
    lfo (s, 1, 0.24f, 0 /* SINE */, 1.0f, true, 0.6f);
    macros (s, 0.25f, 0.45f, 0.40f, 0.35f);

    Routings r;
    r.uni (ModSource::Env1,   Param::shapeSurface,   0.200f)
     .bi  (ModSource::LFO1,   Param::shapeTension,   0.030f)
     .uni (ModSource::Velocity, Param::impactBrightness, 0.320f)
     .uni (ModSource::Velocity, Param::shapeStrike,  0.200f)
     .bi  (ModSource::KeyTrack, Param::shapeDecay,  -0.180f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.300f)
     .uni (ModSource::Macro2, Param::impactBrightness, 0.350f)
     .uni (ModSource::Macro2, Param::shapeExcite,    0.250f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.320f)
     .uni (ModSource::Macro4, Param::evolveMagnet,   0.350f)
     .uni (ModSource::Macro4, Param::shapeTension,   0.200f);
    sharedMacros (r, Param::spaceSize, Param::impactRandom);
    r.commit (s);
}});

manager.addFactory ({ "Dust Piano", "KEYS", { "keys", "piano", "noisy", "intimate" }, [] (PatchState& s)
{
    dust (s, 5 /* CRACKLE */, 0.68f, 0.55f, 0.22f, 0.45f, 0.35f, 0.55f, 17);
    amp (s, 0.002f, 1.10f, 0.14f, 0.90f, 0.35f);
    shape (s, 0.52f, 0.14f, 0.44f, 0.56f, 0.60f, 0.20f);
    material (s, MaterialType::String, MaterialType::Wood, 0.42f);
    topology (s, 0 /* CHAIN */, 0.42f, 0.52f, 41);
    matter (s, 0.96f, 0.58f, 0.45f, 0.55f);
    evolve (s, 0.0f, 0.10f, 0.0f, 0.30f, 0.50f, 0.14f, 0.0f, 0.20f, 0.18f);
    set (s, Param::evolveMagnetTarget, 6 /* CUSTOM (harmonic) */);
    space (s, SpacePresets::Chamber, 0.30f, 0.35f, 0.52f, 0.22f);

    env (s, 1, 0.001f, 0.60f, 0.0f, 0.50f, 0.3f);
    env (s, 2, 0.30f, 2.50f, 0.30f, 2.0f);
    chaos (s, 1, 0 /* WALK */, 0.60f, 0.35f, 0.85f, 0.5f, 59);
    macros (s, 0.20f, 0.40f, 0.30f, 0.45f);

    Routings r;
    r.uni (ModSource::Env1,   Param::dustDensity,    0.300f)
     .uni (ModSource::Env2,   Param::shapeSurface,   0.150f)
     .bi  (ModSource::Chaos1, Param::dustColor,      0.100f)
     .uni (ModSource::Velocity, Param::dustGrain,    0.260f)
     .uni (ModSource::Velocity, Param::shapeStrike,  0.250f)
     .bi  (ModSource::KeyTrack, Param::shapeDecay,  -0.200f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.300f)
     .uni (ModSource::Macro2, Param::dustColor,      0.320f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.280f)
     .uni (ModSource::Macro4, Param::dustDensity,    0.350f)
     .uni (ModSource::Macro4, Param::dustGrain,      0.250f);
    sharedMacros (r, Param::ampDecay, Param::dustJitter);
    r.commit (s);
}});

manager.addFactory ({ "Ash Keys", "KEYS", { "keys", "electric", "soft", "vintage" }, [] (PatchState& s)
{
    impact (s, 2 /* PLUCK */, 0.38f, 0.42f, 0.26f, 0.72f, 0.55f, 0.08f, 0.0f, 0.85f);
    amp (s, 0.002f, 1.30f, 0.22f, 0.80f, 0.45f);
    shape (s, 0.34f, 0.16f, 0.48f, 0.50f, 0.58f, 0.30f);
    material (s, MaterialType::Metal, MaterialType::Wood, 0.60f);
    topology (s, 2 /* CLUSTERS */, 0.36f, 0.48f, 71);
    matter (s, 0.94f, 0.48f, 0.50f, 0.50f);
    evolve (s, 0.10f, 0.0f, 0.0f, 0.40f, 0.52f, 0.06f, 0.0f, 0.18f, 0.14f);
    set (s, Param::evolveBendPivot, 0.35f);
    set (s, Param::evolveMagnetTarget, 6 /* CUSTOM (harmonic) */);
    space (s, SpacePresets::Chamber, 0.28f, 0.40f, 0.50f, 0.25f);
    set (s, Param::spaceChorusOn, 1.0f);
    set (s, Param::spaceChorusRate, 0.55f);
    set (s, Param::spaceChorusDepth, 0.35f);
    set (s, Param::spaceChorusMix, 0.35f);

    env (s, 1, 0.001f, 0.70f, 0.0f, 0.60f, 0.35f);
    lfo (s, 1, 4.20f, 0 /* SINE */, 1.0f, true, 0.8f);
    macros (s, 0.20f, 0.40f, 0.30f, 0.35f);

    Routings r;
    r.uni (ModSource::Env1,   Param::shapeExcite,    0.220f)
     .bi  (ModSource::LFO1,   Param::shapePitch,     0.004f)
     .uni (ModSource::Velocity, Param::impactHardness, 0.300f)
     .uni (ModSource::Velocity, Param::shapeSurface, 0.180f)
     .bi  (ModSource::KeyTrack, Param::shapeMass,   -0.140f)
     .uni (ModSource::Macro1, Param::lfo1Depth,      0.350f)
     .uni (ModSource::Macro2, Param::impactBrightness, 0.350f)
     .uni (ModSource::Macro2, Param::shapeTension,   0.150f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.300f)
     .uni (ModSource::Macro4, Param::evolveBend,     0.250f)
     .uni (ModSource::Macro4, Param::shapeSurface,   0.200f);
    sharedMacros (r, Param::ampDecay, Param::impactRandom);
    r.commit (s);
}});
}

} // namespace am::FactoryContent
