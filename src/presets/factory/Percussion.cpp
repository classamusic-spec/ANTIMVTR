#include "presets/FactoryBuilders.h"

namespace am::FactoryContent
{

void registerPercussion (PresetManager& manager)
{
//==========================================================================
// PERCUSSION — struck objects, tuned across the keyboard
//==========================================================================

manager.addFactory ({ "Bone Marimba", "PERCUSSION", { "percussion", "mallet", "wood", "dry" }, [] (PatchState& s)
{
    impact (s, 6 /* MEMBRANE HIT */, 0.45f, 0.40f, 0.10f, 0.85f, 0.45f, 0.14f, 0.0f, 0.85f);
    amp (s, 0.001f, 0.45f, 0.0f, 0.35f, 0.25f);
    set (s, Param::masterGain, -3.0f);   // struck material has a hot transient: keep headroom at the bottom of the keyboard
    shape (s, 0.28f, 0.20f, 0.42f, 0.48f, 0.42f, 0.22f);
    material (s, MaterialType::Wood, MaterialType::Membrane, 0.30f);
    topology (s, 0 /* CHAIN */, 0.30f, 0.40f, 311);
    matter (s, 1.0f, 0.52f, 0.55f, 0.45f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.34f, 0.52f, 0.10f, 0.0f, 0.25f, 0.12f);
    set (s, Param::evolveMagnetTarget, 6 /* CUSTOM (harmonic) */);
    space (s, SpacePresets::Chamber, 0.26f, 0.28f, 0.50f, 0.20f);

    env (s, 1, 0.001f, 0.16f, 0.0f, 0.14f, 0.25f);
    macros (s, 0.15f, 0.40f, 0.28f, 0.40f);

    Routings r;
    r.uni (ModSource::Env1,   Param::shapeSurface,   0.200f)
     .uni (ModSource::Velocity, Param::impactHardness, 0.350f)
     .uni (ModSource::Velocity, Param::shapeStrike,  0.200f)
     .bi  (ModSource::KeyTrack, Param::shapeDecay,  -0.220f)
     .bi  (ModSource::NoteRandom, Param::impactRandom, 0.080f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.250f)
     .uni (ModSource::Macro2, Param::impactBrightness, 0.350f)
     .uni (ModSource::Macro2, Param::shapeExcite,    0.220f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.280f)
     .uni (ModSource::Macro4, Param::shapeMass,      0.300f)
     .uni (ModSource::Macro4, Param::shapeDecay,     0.200f);
    sharedMacros (r, Param::ampDecay, Param::impactRandom);
    r.commit (s);
}});

manager.addFactory ({ "Steel Rain", "PERCUSSION", { "percussion", "metal", "scatter", "bright" }, [] (PatchState& s)
{
    impact (s, 3 /* NOISE STRIKE */, 0.80f, 0.82f, 0.10f, 0.85f, 0.35f, 0.30f, 0.28f);
    amp (s, 0.001f, 0.60f, 0.10f, 0.45f, 0.25f);
    shape (s, 0.56f, 0.58f, 0.26f, 0.66f, 0.50f, 0.30f);
    material (s, MaterialType::Metal, MaterialType::Crystal, 0.35f);
    topology (s, 4 /* RANDOM */, 0.50f, 0.52f, 359);
    matter (s, 1.0f, 0.72f, 0.82f, 0.80f);
    evolve (s, 0.0f, 0.0f, 0.18f, 0.26f, 0.42f, 0.46f, 0.0f, 0.60f, 0.35f);
    set (s, Param::evolveScatterSeed, 953);
    fracture (s, 2 /* TRANSIENT */, 0.45f, 0.45f, 0.55f, 0.50f, 0.25f, 0.30f, 0.45f, 0.60f, 0.20f,
              2 /* 32 */, 5 /* 1/32 */, 16, 0.0f, 3 /* RANDOM */, 0.80f, 0.45f, 619,
              FractureShape { 32, 0.02f, 0.22f, 0.10f, 0.35f, 0.30f, 0.55f, 0.30f, 0.95f,
                              1.0f, 1.0f, 0.75f, 0.9f, kOctaveTerrace, "XHXoXHXo", nullptr });
    space (s, SpacePresets::Orbit, 0.34f, 0.38f, 0.60f, 0.42f);

    env (s, 1, 0.001f, 0.22f, 0.0f, 0.20f, 0.25f);
    chaos (s, 1, 4 /* TARGETS */, 5.50f, 0.50f, 0.45f, 0.5f, 1013);
    macros (s, 0.40f, 0.50f, 0.32f, 0.45f);

    Routings r;
    r.uni (ModSource::Env1,   Param::fractureAmount, 0.220f)
     .bi  (ModSource::Chaos1, Param::evolveScatter,  0.180f)
     .uni (ModSource::Velocity, Param::impactBrightness, 0.300f)
     .uni (ModSource::Velocity, Param::impactRate,    0.220f)
     .bi  (ModSource::NoteRandom, Param::fracturePitch, 0.060f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.400f)
     .uni (ModSource::Macro2, Param::impactBrightness, 0.300f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.300f)
     .uni (ModSource::Macro4, Param::fractureAmount, 0.300f)
     .uni (ModSource::Macro4, Param::impactRate,     0.300f);
    sharedMacros (r, Param::fractureDecay, Param::impactRandom);
    r.commit (s);
}});

manager.addFactory ({ "Membrane Engine", "PERCUSSION", { "percussion", "drum", "membrane", "body" }, [] (PatchState& s)
{
    impact (s, 6 /* MEMBRANE HIT */, 0.30f, 0.32f, 0.22f, 0.90f, 0.60f, 0.18f, 0.0f, 0.85f);
    amp (s, 0.001f, 0.70f, 0.0f, 0.50f, 0.3f);
    set (s, Param::masterGain, -2.0f);   // struck material has a hot transient: keep headroom at the bottom of the keyboard
    shape (s, 0.34f, 0.34f, 0.66f, 0.38f, 0.44f, 0.40f);
    material (s, MaterialType::Membrane, MaterialType::Wood, 0.40f);
    topology (s, 3 /* LATTICE */, 0.46f, 0.35f, 383);
    matter (s, 1.0f, 0.42f, 0.56f, 0.40f);
    evolve (s, 0.0f, 0.22f, 0.0f, 0.0f, 0.60f, 0.12f, 0.0f, 0.30f, 0.16f);
    space (s, SpacePresets::Void, 0.24f, 0.55f, 0.38f, 0.28f);

    env (s, 1, 0.001f, 0.30f, 0.0f, 0.25f, 0.25f);
    lfo (s, 1, 0.80f, 1 /* TRIANGLE */, 1.0f, true);
    macros (s, 0.20f, 0.35f, 0.30f, 0.50f);

    Routings r;
    r.uni (ModSource::Env1,   Param::evolveMelt,     0.220f)
     .bi  (ModSource::LFO1,   Param::shapeTension,   0.035f)
     .uni (ModSource::Velocity, Param::impactHardness, 0.320f)
     .uni (ModSource::Velocity, Param::shapeStrike,  0.220f)
     .bi  (ModSource::KeyTrack, Param::shapeMass,   -0.200f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.300f)
     .uni (ModSource::Macro2, Param::impactBrightness, 0.320f)
     .uni (ModSource::Macro2, Param::shapeExcite,    0.250f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.280f)
     .uni (ModSource::Macro4, Param::shapeMass,      0.280f)
     .uni (ModSource::Macro4, Param::evolveMelt,     0.250f);
    sharedMacros (r, Param::ampDecay, Param::impactRandom);
    r.commit (s);
}});
}

} // namespace am::FactoryContent
