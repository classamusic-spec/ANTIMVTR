#include "presets/FactoryBuilders.h"

namespace am::FactoryContent
{

void registerPluck (PresetManager& manager)
{
//==========================================================================
// PLUCK — short, precise, physical
//==========================================================================

manager.addFactory ({ "Magnet Bells", "PLUCK", { "pluck", "bells", "tuned", "magnet" }, [] (PatchState& s)
{
    impact (s, 4 /* METAL STRIKE */, 0.82f, 0.60f, 0.12f, 0.85f, 0.40f, 0.10f);
    amp (s, 0.001f, 0.85f, 0.0f, 0.70f, 0.3f);
    shape (s, 0.38f, 0.66f, 0.30f, 0.62f, 0.66f, 0.14f);
    material (s, MaterialType::Metal, MaterialType::Crystal, 0.45f);
    topology (s, 1 /* RING */, 0.34f, 0.58f, 13);
    matter (s, 1.0f, 0.60f, 0.80f, 0.65f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.72f, 0.40f, 0.10f, 0.0f, 0.30f, 0.18f);
    set (s, Param::evolveMagnetTarget, 5 /* SCALE */);
    space (s, SpacePresets::Orbit, 0.34f, 0.45f, 0.55f, 0.40f);

    env (s, 1, 0.001f, 0.45f, 0.0f, 0.40f, 0.3f);
    lfo (s, 1, 0.60f, 5 /* SMOOTH RANDOM */, 1.0f, true);
    macros (s, 0.25f, 0.40f, 0.35f, 0.55f);

    Routings r;
    r.uni (ModSource::Env1,   Param::evolveScatter,  0.120f)
     .bi  (ModSource::LFO1,   Param::shapeTension,   0.040f)
     .uni (ModSource::Velocity, Param::impactHardness, 0.300f)
     .uni (ModSource::Velocity, Param::shapeStrike,  0.220f)
     .bi  (ModSource::NoteRandom, Param::shapePitch, 0.004f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.350f)
     .uni (ModSource::Macro2, Param::impactBrightness, 0.320f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.300f)
     .uni (ModSource::Macro3, Param::spaceFeedback,  0.200f)
     .uni (ModSource::Macro4, Param::evolveMagnet,   0.280f)
     .uni (ModSource::Macro4, Param::shapeForm,      0.150f);
    sharedMacros (r, Param::spaceFeedback, Param::impactRandom);
    r.commit (s);
}});

manager.addFactory ({ "Anti-String", "PLUCK", { "pluck", "string", "impossible", "inharmonic" }, [] (PatchState& s)
{
    impact (s, 2 /* PLUCK */, 0.62f, 0.55f, 0.16f, 0.78f, 0.42f, 0.06f);
    amp (s, 0.001f, 1.40f, 0.0f, 1.10f, 0.3f);
    shape (s, 0.44f, 0.18f, 0.36f, 0.74f, 0.70f, 0.24f);
    material (s, MaterialType::String, MaterialType::Void, 0.38f);
    topology (s, 0 /* CHAIN */, 0.48f, 0.55f, 29);
    matter (s, 1.0f, 0.56f, 0.68f, 0.60f);
    evolve (s, 0.32f, 0.0f, 0.0f, 0.0f, 0.46f, 0.16f, 0.0f, 0.22f, 0.20f);
    set (s, Param::evolveBendPivot, 0.28f);
    set (s, Param::evolveBendRange, 0.42f);
    set (s, Param::evolveBendCurve, 0.62f);
    space (s, SpacePresets::Dream, 0.32f, 0.55f, 0.55f, 0.28f);

    env (s, 1, 0.001f, 1.20f, 0.0f, 1.0f, 0.35f);
    lfo (s, 1, 0.32f, 1 /* TRIANGLE */, 1.0f, true, 0.4f);
    macros (s, 0.30f, 0.40f, 0.30f, 0.45f);

    Routings r;
    r.uni (ModSource::Env1,   Param::evolveBend,     0.180f)
     .bi  (ModSource::LFO1,   Param::shapeSurface,   0.050f)
     .uni (ModSource::Velocity, Param::impactBrightness, 0.280f)
     .uni (ModSource::Velocity, Param::shapeExcite,  0.200f)
     .bi  (ModSource::KeyTrack, Param::evolveBendPivot, 0.150f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.320f)
     .uni (ModSource::Macro2, Param::shapeExcite,    0.300f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.300f)
     .uni (ModSource::Macro4, Param::evolveBend,     0.350f)
     .uni (ModSource::Macro4, Param::shapeTension,   0.180f);
    sharedMacros (r, Param::ampDecay, Param::evolveTear);
    r.commit (s);
}});

manager.addFactory ({ "Copper Thorn", "PLUCK", { "pluck", "metal", "short", "percussive" }, [] (PatchState& s)
{
    impact (s, 1 /* CLICK */, 0.88f, 0.78f, 0.08f, 0.85f, 0.30f, 0.14f);
    amp (s, 0.001f, 0.40f, 0.0f, 0.30f, 0.25f);
    shape (s, 0.30f, 0.50f, 0.28f, 0.58f, 0.48f, 0.34f);
    material (s, MaterialType::Metal, MaterialType::Wood, 0.30f);
    topology (s, 4 /* RANDOM */, 0.60f, 0.42f, 97);
    matter (s, 1.0f, 0.66f, 0.70f, 0.55f);
    evolve (s, 0.0f, 0.0f, 0.20f, 0.34f, 0.50f, 0.24f, 0.22f, 0.45f, 0.20f);
    set (s, Param::evolveScatterSeed, 331);
    set (s, Param::evolveMagnetTarget, 3 /* MINOR */);
    space (s, SpacePresets::Orbit, 0.30f, 0.35f, 0.50f, 0.45f);

    env (s, 1, 0.001f, 0.20f, 0.0f, 0.18f, 0.25f);
    chaos (s, 1, 4 /* TARGETS */, 2.20f, 0.40f, 0.60f, 0.5f, 811);
    macros (s, 0.30f, 0.50f, 0.30f, 0.35f);

    Routings r;
    r.uni (ModSource::Env1,   Param::evolveCrush,    0.150f)
     .bi  (ModSource::Chaos1, Param::evolveScatter,  0.120f)
     .uni (ModSource::Velocity, Param::impactHardness, 0.320f)
     .uni (ModSource::Velocity, Param::shapeStrike,  0.250f)
     .bi  (ModSource::NoteRandom, Param::shapeSurface, 0.060f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.400f)
     .uni (ModSource::Macro2, Param::impactBrightness, 0.300f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.300f)
     .uni (ModSource::Macro4, Param::evolveTear,     0.300f)
     .uni (ModSource::Macro4, Param::evolveCrush,    0.200f);
    sharedMacros (r, Param::spaceFeedback, Param::impactRandom);
    r.commit (s);
}});
}

} // namespace am::FactoryContent
