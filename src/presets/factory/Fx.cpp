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
}

} // namespace am::FactoryContent
