#include "presets/FactoryBuilders.h"

namespace am::FactoryContent
{

void registerSequence (PresetManager& manager)
{
//==========================================================================
// SEQUENCE — hold a note, the patch plays the pattern
//==========================================================================

manager.addFactory ({ "Frozen Machine", "SEQUENCE", { "sequence", "rhythmic", "metal", "fracture" }, [] (PatchState& s)
{
    dust (s, 6 /* IMPULSE */, 0.45f, 0.30f, 0.50f, 0.16f, 0.35f, 0.45f, 761);
    amp (s, 0.001f, 0.40f, 0.92f, 0.50f, 0.3f);
    shape (s, 0.44f, 0.56f, 0.32f, 0.62f, 0.50f, 0.24f);
    material (s, MaterialType::Metal, MaterialType::Crystal, 0.40f);
    topology (s, 1 /* RING */, 0.42f, 0.55f, 761);
    matter (s, 0.55f, 0.55f, 0.0f, 0.68f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.44f, 0.46f, 0.16f, 0.0f, 0.35f, 0.20f);
    set (s, Param::evolveMagnetTarget, 5 /* SCALE */);
    fracture (s, 1 /* RHYTHMIC */, 0.85f, 1.0f, 0.45f, 1.0f, 0.35f, 0.40f, 0.55f, 0.30f, 0.20f,
              1 /* 16 */, 4 /* 1/16 */, 8, 0.12f, 0 /* FORWARD */, 1.0f, 0.15f, 2003,
              FractureShape { 16, 0.03f, 0.30f, 0.20f, 0.45f, 0.35f, 0.60f, 0.25f, 0.85f,
                              2.0f, 2.0f, 0.70f, 1.0f, kOctaveTerrace, "XLoHXLoH", kOctaveTerrace });
    space (s, SpacePresets::Machine, 0.30f, 0.40f, 0.55f, 0.40f);
    set (s, Param::spaceEqHigh, -8.0f);

    env (s, 1, 0.001f, 0.30f, 0.0f, 0.25f, 0.25f);
    lfo (s, 1, 0.50f, 3 /* SQUARE */, 1.0f, true);
    macros (s, 0.35f, 0.45f, 0.35f, 0.55f);

    Routings r;
    r.uni (ModSource::Env1,   Param::fractureAmount, 0.200f)
     .bi  (ModSource::LFO1,   Param::fractureSequence, 0.150f)
     .uni (ModSource::Velocity, Param::dustColor,    0.300f)
     .uni (ModSource::Velocity, Param::fractureAmount, 0.150f)
     .bi  (ModSource::KeyTrack, Param::fractureTone,  0.150f)
     .uni (ModSource::Macro1, Param::fractureEvolve, 0.350f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.250f)
     .uni (ModSource::Macro2, Param::fractureTone,   0.300f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.300f)
     .uni (ModSource::Macro4, Param::fractureSpread, 0.350f)
     .uni (ModSource::Macro4, Param::fractureFeedback, 0.200f);
    sharedMacros (r, Param::fractureDecay, Param::fractureRandom);
    r.commit (s);
}});

manager.addFactory ({ "Pulse Lattice", "SEQUENCE", { "sequence", "pulsing", "bright", "wave" }, [] (PatchState& s)
{
    wave (s, 1 /* HARMONIC */, 0.38f, 0.30f, 0.15f, 2, 0.12f, 0.50f);
    amp (s, 0.004f, 0.50f, 0.88f, 0.35f, 0.35f);
    shape (s, 0.48f, 0.44f, 0.34f, 0.60f, 0.50f, 0.26f);
    material (s, MaterialType::Crystal, MaterialType::Metal, 0.35f);
    topology (s, 3 /* LATTICE */, 0.48f, 0.52f, 811);
    matter (s, 0.82f, 0.60f, 0.04f, 0.70f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.36f, 0.46f, 0.18f, 0.0f, 0.48f, 0.25f);
    set (s, Param::evolveMagnetTarget, 5 /* SCALE */);
    fracture (s, 1 /* RHYTHMIC */, 0.62f, 1.0f, 0.38f, 1.0f, 0.28f, 0.28f, 0.42f, 0.62f, 0.15f,
              0 /* 8 */, 4 /* 1/16 */, 8, 0.18f, 2 /* PINGPONG */, 1.0f, 0.10f, 2087,
              FractureShape { 8, 0.02f, 0.25f, 0.15f, 0.35f, 0.30f, 0.50f, 0.20f, 0.80f,
                              2.0f, 2.0f, 0.60f, 1.0f, kFifthTerrace, "XoLoHoXo", kFifthTerrace });
    space (s, SpacePresets::Orbit, 0.34f, 0.40f, 0.60f, 0.50f);

    env (s, 1, 0.002f, 0.25f, 0.20f, 0.20f, 0.3f);
    lfo (s, 1, 2.40f, 3 /* SQUARE */, 1.0f, true);
    lfo (s, 2, 0.31f, 1 /* TRIANGLE */, 1.0f, false);
    macros (s, 0.40f, 0.50f, 0.35f, 0.50f);

    Routings r;
    r.bi  (ModSource::LFO1,   Param::wavePosition,   0.120f)
     .bi  (ModSource::LFO2,   Param::fractureSequence, 0.180f)
     .uni (ModSource::Env1,   Param::shapeExcite,    0.200f)
     .uni (ModSource::Velocity, Param::waveMorph,    0.250f)
     .bi  (ModSource::KeyTrack, Param::fractureTone,  0.150f)
     .uni (ModSource::Macro1, Param::fractureEvolve, 0.300f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.250f)
     .uni (ModSource::Macro2, Param::wavePosition,   0.300f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.300f)
     .uni (ModSource::Macro4, Param::fractureSpread, 0.300f)
     .uni (ModSource::Macro4, Param::fractureSwing,  0.250f);
    sharedMacros (r, Param::fractureDecay, Param::fractureSwing);
    r.commit (s);
}});

manager.addFactory ({ "Ghost Arpeggio", "SEQUENCE", { "sequence", "arp", "soft", "spectral" }, [] (PatchState& s)
{
    wave (s, 5 /* SPECTRAL */, 0.30f, 0.22f, 0.04f, 2, 0.10f, 0.55f);
    amp (s, 0.02f, 0.60f, 0.88f, 1.20f, 0.4f);
    shape (s, 0.50f, 0.82f, 0.28f, 0.64f, 0.62f, 0.18f);
    material (s, MaterialType::Crystal, MaterialType::Void, 0.42f);
    topology (s, 5 /* STAR */, 0.34f, 0.58f, 863);
    matter (s, 0.80f, 0.52f, 0.08f, 0.75f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.58f, 0.42f, 0.14f, 0.0f, 0.32f, 0.22f);
    set (s, Param::evolveMagnetTarget, 5 /* SCALE */);
    fracture (s, 1 /* RHYTHMIC */, 0.75f, 1.0f, 0.55f, 0.96f, 0.42f, 0.50f, 0.62f, 0.55f, 0.25f,
              1 /* 16 */, 3 /* 1/8 */, 8, 0.22f, 2 /* PINGPONG */, 0.92f, 0.20f, 2131,
              FractureShape { 16, 0.06f, 0.45f, 0.25f, 0.55f, 0.45f, 0.72f, 0.25f, 0.90f,
                              1.8f, 1.8f, 0.70f, 0.95f, kMinorTerrace, "XLHoXLHo", kMinorTerrace });
    space (s, SpacePresets::Dream, 0.42f, 0.62f, 0.58f, 0.38f);

    env (s, 1, 0.001f, 0.50f, 0.0f, 0.40f, 0.3f);
    lfo (s, 1, 0.38f, 1 /* TRIANGLE */, 1.0f, true);
    chaos (s, 1, 4 /* TARGETS */, 1.60f, 0.40f, 0.60f, 0.5f, 2213);
    macros (s, 0.35f, 0.45f, 0.42f, 0.50f);

    Routings r;
    r.uni (ModSource::Env1,   Param::shapeExcite,    0.180f)
     .bi  (ModSource::LFO1,   Param::fractureSequence, 0.120f)
     .bi  (ModSource::Chaos1, Param::fracturePitch,  0.050f)
     .uni (ModSource::Velocity, Param::wavePosition, 0.280f)
     .bi  (ModSource::KeyTrack, Param::shapeDecay,  -0.150f)
     .uni (ModSource::Macro1, Param::fractureEvolve, 0.300f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.250f)
     .uni (ModSource::Macro2, Param::waveMorph,      0.300f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.300f)
     .uni (ModSource::Macro4, Param::fractureProbability, -0.300f)
     .uni (ModSource::Macro4, Param::fractureSpread, 0.300f);
    sharedMacros (r, Param::fractureDecay, Param::fractureRandom);
    r.commit (s);
}});
}

} // namespace am::FactoryContent
