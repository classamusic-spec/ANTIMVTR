#include "presets/FactoryBuilders.h"

namespace am::FactoryContent
{

void registerDrone (PresetManager& manager)
{
//==========================================================================
// DRONE — one note, played for a minute
//==========================================================================

manager.addFactory ({ "Gravity Drone", "DRONE", { "drone", "heavy", "friction", "slow" }, [] (PatchState& s)
{
    gesture (s, 4 /* FRICTION */, 0.68f, 0.22f, 0.38f, 0.45f, 0.30f, 0.42f);
    amp (s, 1.60f, 3.0f, 0.92f, 4.0f, 0.6f);
    shape (s, 0.58f, 0.26f, 0.74f, 0.40f, 0.78f, 0.34f);
    material (s, MaterialType::Void, MaterialType::Membrane, 0.42f);
    topology (s, 2 /* CLUSTERS */, 0.56f, 0.38f, 439);
    matter (s, 0.96f, 0.48f, 0.18f, 0.70f);
    evolve (s, 0.0f, 0.32f, 0.0f, 0.28f, 0.76f, 0.18f, 0.0f, 0.10f, 0.42f);
    set (s, Param::evolveMagnetTarget, 0 /* OCTAVE */);
    space (s, SpacePresets::Void, 0.52f, 0.85f, 0.38f, 0.42f);

    lfo (s, 1, 0.05f, 0 /* SINE */, 1.0f, false, 3.0f);
    lfo (s, 2, 0.09f, 1 /* TRIANGLE */, 1.0f, false, 2.0f);
    env (s, 2, 6.0f, 8.0f, 0.80f, 8.0f, 0.7f);
    chaos (s, 1, 1 /* BROWNIAN */, 0.07f, 0.50f, 0.85f, 0.5f, 1117);
    macros (s, 0.45f, 0.30f, 0.55f, 0.50f);

    Routings r;
    r.bi  (ModSource::LFO1,   Param::shapeMass,      0.070f)
     .bi  (ModSource::LFO2,   Param::gesturePressure, 0.120f)
     .uni (ModSource::Env2,   Param::evolveMelt,     0.240f)
     .bi  (ModSource::Chaos1, Param::shapeForm,      0.060f)
     .uni (ModSource::Velocity, Param::gesturePressure, 0.200f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.450f)
     .uni (ModSource::Macro1, Param::gestureMotion,  0.350f)
     .uni (ModSource::Macro2, Param::gestureBandwidth, 0.300f)
     .uni (ModSource::Macro2, Param::shapeExcite,    0.250f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.250f)
     .uni (ModSource::Macro4, Param::evolveMelt,     0.300f)
     .uni (ModSource::Macro4, Param::shapeDecay,     0.150f);
    sharedMacros (r, Param::spaceSize, Param::gestureRoughness);
    r.commit (s);
}});

manager.addFactory ({ "Electric Organism", "DRONE", { "drone", "alive", "electric", "unstable" }, [] (PatchState& s)
{
    gesture (s, 2 /* RUB */, 0.64f, 0.42f, 0.44f, 0.38f, 0.55f, 0.42f);
    amp (s, 0.80f, 2.20f, 0.88f, 2.80f, 0.55f);
    shape (s, 0.70f, 0.64f, 0.46f, 0.54f, 0.72f, 0.44f);
    material (s, MaterialType::Organic, MaterialType::Liquid, 0.50f);
    topology (s, 1 /* RING */, 0.66f, 0.50f, 491);
    matter (s, 0.94f, 0.50f, 0.20f, 0.88f);
    evolve (s, 0.0f, 0.26f, 0.28f, 0.0f, 0.50f, 0.38f, 0.0f, 0.30f, 0.55f);
    set (s, Param::evolveScatterSeed, 1229);
    space (s, SpacePresets::Nebula, 0.48f, 0.70f, 0.52f, 0.45f);

    lfo (s, 1, 0.16f, 5 /* SMOOTH RANDOM */, 1.0f, false);
    lfo (s, 2, 0.31f, 0 /* SINE */, 1.0f, false, 1.5f);
    chaos (s, 1, 3 /* LORENZ */, 0.34f, 0.62f, 0.60f, 0.5f, 1301);
    chaos (s, 2, 0 /* WALK */, 0.12f, 0.45f, 0.80f, 0.5f, 1373);
    env (s, 2, 4.0f, 6.0f, 0.70f, 6.0f);
    macros (s, 0.55f, 0.45f, 0.45f, 0.50f);

    Routings r;
    r.bi  (ModSource::LFO1,   Param::gestureSpeed,   0.160f)
     .bi  (ModSource::LFO2,   Param::shapeTension,   0.050f)
     .bi  (ModSource::Chaos1, Param::evolveScatter,  0.150f)
     .bi  (ModSource::Chaos2, Param::shapeForm,      0.070f)
     .uni (ModSource::Env2,   Param::evolveTear,     0.200f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.450f)
     .uni (ModSource::Macro1, Param::gestureMotion,  0.300f)
     .uni (ModSource::Macro2, Param::gestureBandwidth, 0.300f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.280f)
     .uni (ModSource::Macro4, Param::evolveTear,     0.300f)
     .uni (ModSource::Macro4, Param::gestureRoughness, 0.300f);
    sharedMacros (r, Param::spaceSize, Param::chaos1Depth);
    r.commit (s);
}});

manager.addFactory ({ "Deep Field", "DRONE", { "drone", "vast", "dark", "sub" }, [] (PatchState& s)
{
    wave (s, 5 /* SPECTRAL */, 0.18f, 0.15f, 0.03f, 6, 0.30f, 1.0f, -2);
    amp (s, 2.50f, 4.0f, 0.90f, 5.0f, 0.65f);
    shape (s, 0.50f, 0.04f, 0.78f, 0.36f, 0.80f, 0.18f);
    material (s, MaterialType::Void, MaterialType::Organic, 0.28f);
    topology (s, 2 /* CLUSTERS */, 0.40f, 0.42f, 547);
    matter (s, 0.70f, 0.42f, 0.12f, 0.92f);
    evolve (s, 0.0f, 0.16f, 0.0f, 0.42f, 0.68f, 0.10f, 0.0f, 0.08f, 0.30f);
    set (s, Param::evolveMagnetTarget, 0 /* OCTAVE */);
    space (s, SpacePresets::Void, 0.58f, 0.95f, 0.32f, 0.48f);

    lfo (s, 1, 0.04f, 0 /* SINE */, 1.0f, false, 4.0f);
    lfo (s, 2, 0.06f, 5 /* SMOOTH RANDOM */, 1.0f, false, 2.0f);
    env (s, 2, 8.0f, 10.0f, 0.85f, 10.0f, 0.75f);
    macros (s, 0.40f, 0.25f, 0.60f, 0.35f);

    Routings r;
    r.bi  (ModSource::LFO1,   Param::wavePosition,   0.090f)
     .bi  (ModSource::LFO2,   Param::shapeMass,      0.060f)
     .uni (ModSource::Env2,   Param::shapeDensity,   0.200f)
     .bi  (ModSource::KeyTrack, Param::shapeDecay,  -0.100f)
     .uni (ModSource::Velocity, Param::shapeExcite,  0.150f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.400f)
     .uni (ModSource::Macro2, Param::shapeExcite,    0.300f)
     .uni (ModSource::Macro2, Param::spaceTone,      0.250f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.250f)
     .uni (ModSource::Macro3, Param::spaceSize,      0.150f)
     .uni (ModSource::Macro4, Param::waveDetune,     0.300f)
     .uni (ModSource::Macro4, Param::shapeDensity,   0.200f);
    sharedMacros (r, Param::spaceSize, Param::waveDetune);
    r.commit (s);
}});
}

} // namespace am::FactoryContent
