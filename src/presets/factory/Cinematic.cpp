#include "presets/FactoryBuilders.h"

namespace am::FactoryContent
{

void registerCinematic (PresetManager& manager)
{
//==========================================================================
// CINEMATIC — scoring material: one note tells a story
//==========================================================================

manager.addFactory ({ "Broken Choir", "CINEMATIC", { "cinematic", "vocal", "haunting", "wide" }, [] (PatchState& s)
{
    wave (s, 2 /* FORMANT */, 0.30f, 0.45f, 0.08f, 5, 0.24f, 0.90f);
    amp (s, 0.70f, 2.20f, 0.80f, 3.0f, 0.55f);
    shape (s, 0.64f, 0.22f, 0.46f, 0.54f, 0.70f, 0.26f);
    material (s, MaterialType::Organic, MaterialType::Membrane, 0.40f);
    topology (s, 2 /* CLUSTERS */, 0.48f, 0.55f, 1103);
    matter (s, 0.80f, 0.62f, 0.20f, 0.88f);
    evolve (s, 0.0f, 0.22f, 0.26f, 0.30f, 0.46f, 0.22f, 0.0f, 0.18f, 0.40f);
    set (s, Param::evolveMagnetTarget, 3 /* MINOR */);
    fracture (s, 0 /* SPECTRAL */, 0.40f, 0.45f, 0.55f, 0.40f, 0.32f, 0.50f, 0.60f, 0.52f, 0.30f,
              1 /* 16 */, 2 /* 1/4 */, 8, 0.0f, 0 /* FORWARD */, 1.0f, 0.20f, 2521,
              FractureShape { 16, 0.15f, 0.65f, 0.20f, 0.50f, 0.50f, 0.78f, 0.30f, 0.90f,
                              1.0f, 0.85f, 0.70f, 1.0f, kMinorTerrace, "XXLLHHXX", nullptr });
    space (s, SpacePresets::Dream, 0.52f, 0.75f, 0.58f, 0.40f);

    lfo (s, 1, 0.12f, 0 /* SINE */, 1.0f, false, 2.0f);
    lfo (s, 2, 0.08f, 5 /* SMOOTH RANDOM */, 1.0f, false);
    env (s, 2, 2.80f, 6.0f, 0.70f, 5.0f, 0.6f);
    macros (s, 0.45f, 0.40f, 0.50f, 0.45f);

    Routings r;
    r.bi  (ModSource::LFO1,   Param::waveMorph,      0.120f)
     .bi  (ModSource::LFO2,   Param::wavePosition,   0.100f)
     .uni (ModSource::Env2,   Param::evolveTear,     0.220f)
     .uni (ModSource::Velocity, Param::waveScan,     0.180f)
     .bi  (ModSource::KeyTrack, Param::shapeForm,   -0.080f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.350f)
     .uni (ModSource::Macro2, Param::wavePosition,   0.280f)
     .uni (ModSource::Macro2, Param::spaceTone,      0.200f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.280f)
     .uni (ModSource::Macro4, Param::fractureAmount, 0.300f)
     .uni (ModSource::Macro4, Param::evolveTear,     0.250f);
    sharedMacros (r, Param::fractureDecay, Param::waveScan);
    r.commit (s);
}});

manager.addFactory ({ "Event Horizon", "CINEMATIC", { "cinematic", "riser", "granular", "huge" }, [] (PatchState& s)
{
    sample (s, BuiltInSamples::Kind::StoneDrop, 3 /* GRANULAR */, 0.0f, 0.90f, 0.45f, 0.85f);
    amp (s, 1.80f, 3.0f, 0.90f, 3.50f, 0.7f);
    shape (s, 0.70f, 0.32f, 0.62f, 0.48f, 0.78f, 0.30f);
    material (s, MaterialType::Void, MaterialType::Membrane, 0.38f);
    topology (s, 2 /* CLUSTERS */, 0.58f, 0.45f, 1181);
    matter (s, 0.90f, 0.58f, 0.18f, 0.92f);
    evolve (s, 0.34f, 0.18f, 0.0f, 0.24f, 0.40f, 0.24f, 0.0f, 0.14f, 0.48f);
    set (s, Param::evolveBendPivot, 0.30f);
    set (s, Param::evolveBendRange, 0.60f);
    space (s, SpacePresets::Void, 0.56f, 0.92f, 0.42f, 0.50f);

    env (s, 2, 5.0f, 8.0f, 0.85f, 6.0f, 0.75f);
    lfo (s, 1, 0.05f, 2 /* SAW */, 1.0f, true, 2.0f);
    lfo (s, 2, 0.11f, 0 /* SINE */, 1.0f, false, 3.0f);
    chaos (s, 1, 1 /* BROWNIAN */, 0.09f, 0.50f, 0.80f, 0.5f, 2617);
    macros (s, 0.55f, 0.35f, 0.55f, 0.50f);

    Routings r;
    r.uni (ModSource::Env2,   Param::evolveBend,     0.300f)
     .uni (ModSource::Env2,   Param::shapeDensity,   0.180f)
     .bi  (ModSource::LFO1,   Param::sampleGrain,    0.140f)
     .bi  (ModSource::LFO2,   Param::shapeMass,      0.070f)
     .bi  (ModSource::Chaos1, Param::sampleSpread,   0.150f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.400f)
     .uni (ModSource::Macro1, Param::evolveSpeed,    0.200f)
     .uni (ModSource::Macro2, Param::shapeExcite,    0.300f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.250f)
     .uni (ModSource::Macro3, Param::spaceSize,      0.150f)
     .uni (ModSource::Macro4, Param::evolveBend,     0.300f)
     .uni (ModSource::Macro4, Param::sampleGrain,    0.250f);
    sharedMacros (r, Param::spaceSize, Param::sampleSpread);
    r.commit (s);
}});

manager.addFactory ({ "Iron Lullaby", "CINEMATIC", { "cinematic", "music box", "fragile", "metal" }, [] (PatchState& s)
{
    sample (s, BuiltInSamples::Kind::MetalPing, 0 /* ONE SHOT */, 0.0f, 0.55f, 0.25f, 0.35f);
    amp (s, 0.004f, 1.80f, 0.15f, 1.60f, 0.35f);
    shape (s, 0.40f, 0.88f, 0.26f, 0.70f, 0.74f, 0.16f);
    material (s, MaterialType::Crystal, MaterialType::Metal, 0.32f);
    topology (s, 5 /* STAR */, 0.30f, 0.60f, 1259);
    matter (s, 0.94f, 0.60f, 0.66f, 0.72f);
    evolve (s, 0.0f, 0.14f, 0.0f, 0.62f, 0.44f, 0.12f, 0.0f, 0.20f, 0.24f);
    set (s, Param::evolveMagnetTarget, 3 /* MINOR */);
    space (s, SpacePresets::Dream, 0.48f, 0.68f, 0.60f, 0.35f);

    env (s, 1, 0.002f, 1.0f, 0.0f, 0.90f, 0.3f);
    env (s, 2, 1.20f, 4.0f, 0.50f, 4.0f);
    lfo (s, 1, 0.17f, 0 /* SINE */, 1.0f, true, 1.0f);
    macros (s, 0.30f, 0.40f, 0.48f, 0.45f);

    Routings r;
    r.uni (ModSource::Env1,   Param::shapeSurface,   0.180f)
     .uni (ModSource::Env2,   Param::evolveMelt,     0.180f)
     .bi  (ModSource::LFO1,   Param::shapeTension,   0.035f)
     .uni (ModSource::Velocity, Param::sampleStart,  0.150f)
     .bi  (ModSource::KeyTrack, Param::shapeDecay,  -0.180f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.300f)
     .uni (ModSource::Macro2, Param::shapeExcite,    0.300f)
     .uni (ModSource::Macro2, Param::samplePitch,    0.100f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.300f)
     .uni (ModSource::Macro4, Param::evolveMagnet,   0.300f)
     .uni (ModSource::Macro4, Param::shapeTension,   0.180f);
    sharedMacros (r, Param::spaceSize, Param::evolveMelt);
    r.commit (s);
}});
}

} // namespace am::FactoryContent
