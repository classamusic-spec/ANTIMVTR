#include "presets/FactoryBuilders.h"

namespace am::FactoryContent
{

void registerEvolving (PresetManager& manager)
{
//==========================================================================
// EVOLVING — the patch is not the same at the end of the note
//==========================================================================

manager.addFactory ({ "Metal Bloom", "EVOLVING", { "evolving", "metal", "blooming", "long" }, [] (PatchState& s)
{
    impact (s, 4 /* METAL STRIKE */, 0.60f, 0.45f, 0.22f, 0.78f, 0.50f, 0.10f);
    amp (s, 0.002f, 2.40f, 0.55f, 2.60f, 0.45f);
    shape (s, 0.62f, 0.50f, 0.42f, 0.58f, 0.76f, 0.28f);
    material (s, MaterialType::Metal, MaterialType::Organic, 0.35f);
    topology (s, 2 /* CLUSTERS */, 0.52f, 0.52f, 907);
    matter (s, 0.98f, 0.55f, 0.58f, 0.78f);
    evolve (s, 0.30f, 0.24f, 0.0f, 0.30f, 0.42f, 0.22f, 0.0f, 0.16f, 0.55f);
    set (s, Param::evolveBendPivot, 0.45f);
    set (s, Param::evolveBendRange, 0.55f);
    set (s, Param::evolveMagnetTarget, 1 /* FIFTH */);
    space (s, SpacePresets::Shimmer, 0.46f, 0.70f, 0.60f, 0.40f);

    env (s, 1, 0.001f, 1.20f, 0.0f, 1.0f, 0.35f);
    env (s, 2, 2.20f, 5.0f, 0.75f, 5.0f, 0.6f);
    lfo (s, 1, 0.14f, 0 /* SINE */, 1.0f, true, 1.2f);
    chaos (s, 1, 0 /* WALK */, 0.20f, 0.50f, 0.75f, 0.5f, 2281);
    macros (s, 0.50f, 0.45f, 0.45f, 0.50f);

    Routings r;
    r.uni (ModSource::Env2,   Param::evolveBend,     0.320f)
     .uni (ModSource::Env2,   Param::evolveMelt,     0.200f)
     .bi  (ModSource::LFO1,   Param::shapeForm,      0.060f)
     .bi  (ModSource::Chaos1, Param::evolveScatter,  0.120f)
     .uni (ModSource::Velocity, Param::impactHardness, 0.280f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.400f)
     .uni (ModSource::Macro1, Param::evolveSpeed,    0.250f)
     .uni (ModSource::Macro2, Param::impactBrightness, 0.300f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.300f)
     .uni (ModSource::Macro4, Param::evolveBend,     0.300f)
     .uni (ModSource::Macro4, Param::evolveMagnet,   0.250f);
    sharedMacros (r, Param::spaceSize, Param::evolveTear);
    r.commit (s);
}});

manager.addFactory ({ "Slow Collapse", "EVOLVING", { "evolving", "melting", "descending", "dark" }, [] (PatchState& s)
{
    gesture (s, 2 /* RUB */, 0.60f, 0.30f, 0.42f, 0.48f, 0.28f, 0.45f);
    amp (s, 0.60f, 2.60f, 0.80f, 3.0f, 0.55f);
    shape (s, 0.66f, 0.40f, 0.58f, 0.44f, 0.72f, 0.36f);
    material (s, MaterialType::Membrane, MaterialType::Void, 0.48f);
    topology (s, 0 /* CHAIN */, 0.58f, 0.42f, 977);
    matter (s, 0.94f, 0.52f, 0.20f, 0.72f);
    evolve (s, 0.0f, 0.30f, 0.0f, 0.0f, 0.62f, 0.20f, 0.0f, 0.12f, 0.50f);
    space (s, SpacePresets::Void, 0.50f, 0.80f, 0.42f, 0.42f);

    env (s, 2, 1.0f, 9.0f, 0.10f, 6.0f, 0.35f);
    env (s, 3, 4.0f, 8.0f, 0.60f, 8.0f);
    lfo (s, 1, 0.06f, 2 /* SAW */, 1.0f, true, 1.0f);
    chaos (s, 1, 1 /* BROWNIAN */, 0.10f, 0.45f, 0.85f, 0.5f, 2377);
    macros (s, 0.50f, 0.30f, 0.50f, 0.60f);

    Routings r;
    r.uni (ModSource::Env2,   Param::evolveMelt,     0.400f)
     .uni (ModSource::Env3,   Param::evolveGravity,  0.220f)
     .bi  (ModSource::LFO1,   Param::shapeMass,      0.080f)
     .bi  (ModSource::Chaos1, Param::gesturePressure, 0.120f)
     .uni (ModSource::Velocity, Param::gestureSpeed, 0.200f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.400f)
     .uni (ModSource::Macro1, Param::gestureMotion,  0.300f)
     .uni (ModSource::Macro2, Param::gestureBandwidth, 0.300f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.250f)
     .uni (ModSource::Macro4, Param::evolveMelt,     0.350f)
     .uni (ModSource::Macro4, Param::shapeMass,      0.200f);
    sharedMacros (r, Param::spaceSize, Param::gestureRoughness);
    r.commit (s);
}});

manager.addFactory ({ "Living Alloy", "EVOLVING", { "evolving", "sample", "morphing", "metal" }, [] (PatchState& s)
{
    sample (s, BuiltInSamples::Kind::MetalPing, 1 /* LOOP */, 0.06f, 0.88f, 0.30f, 0.42f);
    amp (s, 0.30f, 1.80f, 0.88f, 2.0f, 0.5f);
    shape (s, 0.60f, 0.60f, 0.40f, 0.60f, 0.80f, 0.34f);
    material (s, MaterialType::Metal, MaterialType::Liquid, 0.48f);
    topology (s, 1 /* RING */, 0.60f, 0.50f, 1049);
    matter (s, 0.88f, 0.62f, 0.32f, 0.80f);
    evolve (s, 0.24f, 0.20f, 0.22f, 0.32f, 0.48f, 0.26f, 0.0f, 0.34f, 0.52f);
    set (s, Param::evolveBendPivot, 0.55f);
    set (s, Param::evolveMagnetTarget, 1 /* FIFTH */);
    space (s, SpacePresets::Nebula, 0.44f, 0.65f, 0.56f, 0.38f);

    env (s, 2, 1.50f, 6.0f, 0.55f, 5.0f);
    lfo (s, 1, 0.21f, 5 /* SMOOTH RANDOM */, 1.0f, true);
    lfo (s, 2, 0.13f, 0 /* SINE */, 1.0f, false, 1.5f);
    chaos (s, 1, 3 /* LORENZ */, 0.28f, 0.55f, 0.65f, 0.5f, 2447);
    macros (s, 0.50f, 0.45f, 0.42f, 0.50f);

    Routings r;
    r.uni (ModSource::Env2,   Param::evolveTear,     0.280f)
     .bi  (ModSource::LFO1,   Param::sampleStart,    0.120f)
     .bi  (ModSource::LFO2,   Param::shapeForm,      0.060f)
     .bi  (ModSource::Chaos1, Param::evolveScatter,  0.140f)
     .uni (ModSource::Velocity, Param::sampleGrain,  0.220f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.400f)
     .uni (ModSource::Macro1, Param::evolveSpeed,    0.250f)
     .uni (ModSource::Macro2, Param::shapeExcite,    0.300f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.280f)
     .uni (ModSource::Macro4, Param::evolveBend,     0.300f)
     .uni (ModSource::Macro4, Param::evolveTear,     0.250f);
    sharedMacros (r, Param::spaceSize, Param::sampleSpread);
    r.commit (s);
}});
}

} // namespace am::FactoryContent
