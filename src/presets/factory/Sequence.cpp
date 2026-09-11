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

manager.addFactory ({ "Iron Metronome", "SEQUENCE", { "metallic", "dry", "pulsing", "struck", "mid" }, [] (PatchState& s)
{
    wave (s, 4 /* METALLIC */, 0.34f, 0.22f, 0.06f, 2, 0.10f, 0.40f);
    amp (s, 0.003f, 0.60f, 0.90f, 0.30f, 0.35f);
    set (s, Param::masterGain, 1.0f);
    shape (s, 0.42f, 0.48f, 0.40f, 0.58f, 0.44f, 0.26f);
    material (s, MaterialType::Metal, MaterialType::Wood, 0.35f);
    topology (s, 0 /* CHAIN */, 0.36f, 0.45f, 4001);
    matter (s, 0.85f, 0.62f, 0.30f, 0.55f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.30f, 0.50f, 0.10f, 0.0f, 0.30f, 0.15f);
    set (s, Param::evolveMagnetTarget, 0 /* OCTAVE */);
    fracture (s, 1 /* RHYTHMIC */, 0.88f, 1.0f, 0.30f, 1.0f, 0.25f, 0.25f, 0.22f, 0.45f, 0.12f,
              1 /* 16 */, 3 /* 1/8 */, 8, 0.0f, 0 /* FORWARD */, 1.0f, 0.05f, 4003,
              FractureShape { 16, 0.02f, 0.20f, 0.15f, 0.35f, 0.30f, 0.50f, 0.20f, 0.70f,
                              1.8f, 1.8f, 0.45f, 1.0f, nullptr, "X.X.X.X.", nullptr });
    space (s, SpacePresets::Chamber, 0.24f, 0.30f, 0.50f, 0.25f);

    env (s, 1, 0.001f, 0.20f, 0.0f, 0.18f, 0.25f);
    lfo (s, 1, 0.25f, 1 /* TRIANGLE */, 1.0f, true);
    macros (s, 0.30f, 0.45f, 0.30f, 0.50f);

    Routings r;
    r.uni (ModSource::Env1,   Param::shapeExcite,    0.220f)
     .bi  (ModSource::LFO1,   Param::wavePosition,   0.120f)
     .uni (ModSource::Velocity, Param::waveMorph,    0.300f)
     .uni (ModSource::Velocity, Param::fractureAmount, 0.120f)
     .bi  (ModSource::KeyTrack, Param::fractureTone, 0.180f)
     .bi  (ModSource::NoteRandom, Param::waveFine,   0.060f)
     .uni (ModSource::Macro1, Param::fractureEvolve, 0.320f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.250f)
     .uni (ModSource::Macro2, Param::fractureTone,   0.300f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.300f)
     .uni (ModSource::Macro4, Param::fractureSwing,  0.300f)
     .uni (ModSource::Macro4, Param::shapeSurface,   0.220f);
    sharedMacros (r, Param::fractureDecay, Param::fractureRandom);
    r.commit (s);
}});

manager.addFactory ({ "Offbeat Thicket", "SEQUENCE", { "wooden", "dry", "rhythmic", "close", "mid" }, [] (PatchState& s)
{
    dust (s, 5 /* CRACKLE */, 0.82f, 0.42f, 0.44f, 0.30f, 0.55f, 0.60f, 4007);
    amp (s, 0.002f, 0.50f, 0.90f, 0.35f, 0.30f);
    set (s, Param::masterGain, 0.5f);
    shape (s, 0.40f, 0.30f, 0.45f, 0.50f, 0.42f, 0.30f);
    material (s, MaterialType::Wood, MaterialType::Organic, 0.40f);
    topology (s, 2 /* CLUSTERS */, 0.38f, 0.50f, 4009);
    matter (s, 0.82f, 0.65f, 0.35f, 0.65f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.25f, 0.55f, 0.15f, 0.0f, 0.35f, 0.20f);
    set (s, Param::evolveMagnetTarget, 5 /* SCALE */);
    fracture (s, 1 /* RHYTHMIC */, 1.0f, 1.0f, 0.35f, 1.0f, 0.28f, 0.22f, 0.20f, 0.42f, 0.18f,
              1 /* 16 */, 4 /* 1/16 */, 8, 0.32f, 0 /* FORWARD */, 1.0f, 0.12f, 4013,
              FractureShape { 16, 0.02f, 0.22f, 0.18f, 0.40f, 0.30f, 0.52f, 0.25f, 0.75f,
                              2.0f, 2.0f, 0.55f, 1.0f, nullptr, ".XX.X..X", nullptr });
    space (s, SpacePresets::Chamber, 0.28f, 0.34f, 0.48f, 0.30f);

    env (s, 1, 0.001f, 0.18f, 0.0f, 0.15f, 0.25f);
    lfo (s, 1, 0.33f, 5 /* SMOOTH RANDOM */, 1.0f, true);
    macros (s, 0.35f, 0.45f, 0.30f, 0.55f);

    Routings r;
    r.uni (ModSource::Env1,   Param::shapeStrike,    0.200f)
     .bi  (ModSource::LFO1,   Param::dustGrain,      0.180f)
     .uni (ModSource::Velocity, Param::dustColor,    0.320f)
     .uni (ModSource::Velocity, Param::shapeExcite,  0.200f)
     .bi  (ModSource::KeyTrack, Param::dustDensity, -0.180f)
     .bi  (ModSource::NoteRandom, Param::dustJitter, 0.150f)
     .uni (ModSource::Macro1, Param::fractureEvolve, 0.300f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.250f)
     .uni (ModSource::Macro2, Param::dustColor,      0.300f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.300f)
     .uni (ModSource::Macro4, Param::fractureSwing,  0.320f)
     .uni (ModSource::Macro4, Param::fractureProbability, -0.250f);
    sharedMacros (r, Param::fractureDecay, Param::fractureRandom);
    r.commit (s);
}});

manager.addFactory ({ "Cross Weave", "SEQUENCE", { "glassy", "rhythmic", "evolving", "melodic", "mid" }, [] (PatchState& s)
{
    wave (s, 3 /* FOLDED */, 0.45f, 0.35f, 0.10f, 2, 0.14f, 0.50f);
    amp (s, 0.004f, 0.55f, 0.90f, 0.40f, 0.35f);
    set (s, Param::masterGain, 6.0f);
    shape (s, 0.50f, 0.40f, 0.35f, 0.62f, 0.48f, 0.28f);
    material (s, MaterialType::Crystal, MaterialType::String, 0.45f);
    topology (s, 3 /* LATTICE */, 0.44f, 0.52f, 4019);
    matter (s, 0.82f, 0.65f, 0.10f, 0.70f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.40f, 0.50f, 0.12f, 0.0f, 0.45f, 0.25f);
    set (s, Param::evolveMagnetTarget, 1 /* FIFTH */);
    // Three steps of an eighth against a four-four bar: the pattern turns over every
    // three eighths, so the accent walks around the beat and only comes home on bar four.
    fracture (s, 1 /* RHYTHMIC */, 1.0f, 1.0f, 0.36f, 1.0f, 0.18f, 0.16f, 0.22f, 0.50f, 0.20f,
              1 /* 16 */, 3 /* 1/8 */, 3, 0.0f, 0 /* FORWARD */, 1.0f, 0.10f, 4021,
              FractureShape { 16, 0.01f, 0.16f, 0.08f, 0.26f, 0.32f, 0.55f, 0.25f, 0.80f,
                              2.0f, 2.0f, 0.60f, 1.0f, kFifthTerrace, "XLo", kFifthTerrace });
    space (s, SpacePresets::Orbit, 0.32f, 0.42f, 0.58f, 0.45f);

    env (s, 1, 0.002f, 0.24f, 0.10f, 0.20f, 0.30f);
    lfo (s, 1, 0.20f, 1 /* TRIANGLE */, 1.0f, false);
    set (s, Param::lfo1Sync, 1.0f);
    set (s, Param::lfo1Division, 2.0f /* 2/1 — two bars */);
    macros (s, 0.40f, 0.50f, 0.35f, 0.50f);

    Routings r;
    r.uni (ModSource::Env1,   Param::shapeExcite,    0.200f)
     .bi  (ModSource::LFO1,   Param::fractureSequence, 0.160f)
     .bi  (ModSource::LFO1,   Param::waveMorph,      0.150f)
     .uni (ModSource::Velocity, Param::wavePosition, 0.280f)
     .bi  (ModSource::KeyTrack, Param::fractureTone, 0.180f)
     .bi  (ModSource::NoteRandom, Param::wavePosition, 0.120f)
     .uni (ModSource::Macro1, Param::fractureEvolve, 0.300f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.250f)
     .uni (ModSource::Macro2, Param::waveMorph,      0.300f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.300f)
     .uni (ModSource::Macro4, Param::evolveMagnet,   0.300f)
     .uni (ModSource::Macro4, Param::fractureSpread, 0.250f);
    sharedMacros (r, Param::fractureDecay, Param::fractureRandom);
    r.commit (s);
}});

manager.addFactory ({ "Bell Phase", "SEQUENCE", { "glassy", "bright", "rhythmic", "wide", "melodic" }, [] (PatchState& s)
{
    sample (s, BuiltInSamples::Kind::MetalPing, 3 /* GRANULAR */, 0.0f, 0.60f, 0.45f, 0.55f, 60, 0.90f);
    amp (s, 0.010f, 0.80f, 0.90f, 0.60f, 0.40f);
    set (s, Param::masterGain, 7.5f);
    shape (s, 0.45f, 0.55f, 0.30f, 0.60f, 0.55f, 0.22f);
    material (s, MaterialType::String, MaterialType::Crystal, 0.40f);
    topology (s, 5 /* STAR */, 0.35f, 0.55f, 4027);
    matter (s, 0.88f, 0.72f, 0.12f, 0.75f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.45f, 0.48f, 0.12f, 0.0f, 0.40f, 0.30f);
    set (s, Param::evolveMagnetTarget, 2 /* MAJOR */);
    // Five triplet eighths per cycle: five thirds of a beat against four, so the bells
    // only land on the downbeat once every five bars.
    fracture (s, 1 /* RHYTHMIC */, 0.78f, 1.0f, 0.50f, 1.0f, 0.35f, 0.32f, 0.45f, 0.55f, 0.25f,
              1 /* 16 */, 7 /* 1/8T */, 5, 0.0f, 0 /* FORWARD */, 0.95f, 0.15f, 4029,
              FractureShape { 16, 0.03f, 0.35f, 0.20f, 0.48f, 0.35f, 0.62f, 0.25f, 0.85f,
                              2.0f, 2.0f, 0.70f, 0.95f, kFifthTerrace, "XLHoX", kFifthTerrace });
    space (s, SpacePresets::Dream, 0.40f, 0.55f, 0.60f, 0.40f);

    env (s, 1, 0.002f, 0.30f, 0.15f, 0.25f, 0.30f);
    lfo (s, 1, 0.28f, 0 /* SINE */, 1.0f, false);
    macros (s, 0.40f, 0.50f, 0.45f, 0.50f);

    Routings r;
    r.uni (ModSource::Env1,   Param::sampleGrain,    0.200f)
     .bi  (ModSource::LFO1,   Param::sampleStart,    0.120f)
     .uni (ModSource::Velocity, Param::sampleGrain,  0.280f)
     .uni (ModSource::Velocity, Param::fractureAmount, 0.120f)
     .bi  (ModSource::KeyTrack, Param::shapeDecay,  -0.180f)
     .bi  (ModSource::NoteRandom, Param::samplePitch, 0.050f)
     .uni (ModSource::Macro1, Param::fractureEvolve, 0.300f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.250f)
     .uni (ModSource::Macro2, Param::fractureTone,   0.300f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.300f)
     .uni (ModSource::Macro4, Param::sampleSpread,   0.300f)
     .uni (ModSource::Macro4, Param::fractureSpread, 0.250f);
    sharedMacros (r, Param::fractureDecay, Param::fractureProbability);
    r.commit (s);
}});

manager.addFactory ({ "Rust Cycle", "SEQUENCE", { "metallic", "dirty", "evolving", "rhythmic", "mid" }, [] (PatchState& s)
{
    wave (s, 7 /* NOISE */, 0.50f, 0.40f, 0.20f, 2, 0.18f, 0.60f, 0, 0.55f);
    dust (s, 2 /* BROWN */, 0.50f, 0.30f, 0.45f, 0.25f, 0.60f, 0.60f, 4031, 0.60f);
    set (s, Param::sourceSelected, 0 /* WAVE is the metal, DUST is the corrosion */);
    set (s, Param::sourceMode, 1 /* LAYER */);
    set (s, Param::impactLevel, 0.0f);
    set (s, Param::sampleLevel, 0.0f);
    set (s, Param::gestureLevel, 0.0f);
    amp (s, 0.004f, 0.70f, 0.88f, 0.50f, 0.35f);
    set (s, Param::masterGain, 3.0f);
    shape (s, 0.58f, 0.50f, 0.45f, 0.50f, 0.50f, 0.42f);
    material (s, MaterialType::Metal, MaterialType::Organic, 0.50f);
    topology (s, 4 /* RANDOM */, 0.50f, 0.55f, 4033);
    matter (s, 0.85f, 0.68f, 0.30f, 0.70f);
    evolve (s, 0.0f, 0.35f, 0.0f, 0.0f, 0.58f, 0.30f, 0.20f, 0.40f, 0.35f);
    set (s, Param::evolveScatterSeed, 4035);
    // Thirty-two sixteenths: two bars of pattern before anything repeats, and a
    // four-bar LFO underneath so the second pass through is not the first one.
    fracture (s, 1 /* RHYTHMIC */, 0.98f, 1.0f, 0.50f, 1.0f, 0.35f, 0.30f, 0.30f, 0.45f, 0.35f,
              2 /* 32 */, 4 /* 1/16 */, 32, 0.12f, 0 /* FORWARD */, 0.90f, 0.20f, 4037,
              FractureShape { 32, 0.02f, 0.30f, 0.20f, 0.45f, 0.30f, 0.55f, 0.30f, 0.85f,
                              1.6f, 1.6f, 0.65f, 0.92f, kFallingTerrace,
                              "XL.oX.HLX..oXH.LX.oXL..HX.LoX.H.", nullptr });
    space (s, SpacePresets::Machine, 0.34f, 0.45f, 0.42f, 0.42f);

    lfo (s, 1, 0.10f, 1 /* TRIANGLE */, 1.0f, false);
    set (s, Param::lfo1Sync, 1.0f);
    set (s, Param::lfo1Division, 1.0f /* 4/1 — four bars */);
    env (s, 1, 0.002f, 0.26f, 0.10f, 0.20f, 0.30f);
    macros (s, 0.50f, 0.45f, 0.35f, 0.55f);

    Routings r;
    r.bi  (ModSource::LFO1,   Param::fractureTone,   0.250f)
     .bi  (ModSource::LFO1,   Param::fractureProbability, -0.180f)
     .uni (ModSource::Env1,   Param::shapeExcite,    0.200f)
     .uni (ModSource::Velocity, Param::waveScan,     0.280f)
     .bi  (ModSource::KeyTrack, Param::dustColor,    0.200f)
     .bi  (ModSource::NoteRandom, Param::wavePosition, 0.150f)
     .uni (ModSource::Macro1, Param::fractureEvolve, 0.320f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.250f)
     .uni (ModSource::Macro2, Param::fractureTone,   0.300f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.300f)
     .uni (ModSource::Macro4, Param::evolveMelt,     0.300f)
     .uni (ModSource::Macro4, Param::evolveCrush,    0.220f);
    sharedMacros (r, Param::fractureDecay, Param::fractureRandom);
    r.commit (s);
}});

manager.addFactory ({ "Gated Cathedral", "SEQUENCE", { "bowed", "cold", "pulsing", "huge", "chords" }, [] (PatchState& s)
{
    gesture (s, 0 /* BOW */, 0.60f, 0.45f, 0.20f, 0.40f, 0.30f, 0.50f);
    amp (s, 0.15f, 1.20f, 0.92f, 1.20f, 0.50f);
    shape (s, 0.50f, 0.35f, 0.40f, 0.60f, 0.70f, 0.20f);
    material (s, MaterialType::Membrane, MaterialType::Crystal, 0.45f);
    topology (s, 5 /* STAR */, 0.40f, 0.55f, 4041);
    matter (s, 0.90f, 0.60f, 0.10f, 0.85f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.30f, 0.45f, 0.10f, 0.0f, 0.30f, 0.25f);
    set (s, Param::evolveMagnetTarget, 5 /* SCALE */);
    // A long step decay: this is a gate across a held chord, not a plucked pattern.
    fracture (s, 1 /* RHYTHMIC */, 0.80f, 1.0f, 0.45f, 1.0f, 0.40f, 0.35f, 0.82f, 0.55f, 0.20f,
              1 /* 16 */, 4 /* 1/16 */, 8, 0.0f, 0 /* FORWARD */, 1.0f, 0.08f, 4043,
              FractureShape { 16, 0.03f, 0.30f, 0.25f, 0.50f, 0.55f, 0.80f, 0.25f, 0.85f,
                              1.5f, 1.5f, 0.75f, 1.0f, nullptr, "XX..XX..", nullptr });
    space (s, SpacePresets::Void, 0.55f, 0.88f, 0.45f, 0.50f);

    env (s, 2, 0.60f, 2.00f, 0.80f, 1.20f, 0.55f);
    lfo (s, 1, 0.13f, 0 /* SINE */, 1.0f, false);
    macros (s, 0.35f, 0.45f, 0.60f, 0.50f);

    Routings r;
    r.uni (ModSource::Env2,   Param::gesturePressure, 0.220f)
     .uni (ModSource::Env2,   Param::spaceMix,       0.150f)
     .bi  (ModSource::LFO1,   Param::gesturePosition, 0.180f)
     .uni (ModSource::Velocity, Param::gestureSpeed, 0.280f)
     .bi  (ModSource::KeyTrack, Param::fractureTone, 0.180f)
     .bi  (ModSource::NoteRandom, Param::gesturePosition, 0.120f)
     .uni (ModSource::Macro1, Param::fractureEvolve, 0.300f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.250f)
     .uni (ModSource::Macro2, Param::gestureBandwidth, 0.300f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.300f)
     .uni (ModSource::Macro4, Param::fractureDecay, -0.300f)
     .uni (ModSource::Macro4, Param::fractureSpread, 0.220f);
    sharedMacros (r, Param::fractureDecay, Param::fractureRandom);
    r.commit (s);
}});

manager.addFactory ({ "Breath Pump", "SEQUENCE", { "organic", "breathing", "soft", "roomy", "mid" }, [] (PatchState& s)
{
    gesture (s, 3 /* BREATH */, 0.55f, 0.40f, 0.30f, 0.50f, 0.35f, 0.55f);
    amp (s, 0.08f, 1.00f, 0.90f, 0.80f, 0.50f);
    set (s, Param::masterGain, 5.5f);
    shape (s, 0.50f, 0.28f, 0.35f, 0.50f, 0.55f, 0.30f);
    material (s, MaterialType::Organic, MaterialType::Membrane, 0.50f);
    topology (s, 2 /* CLUSTERS */, 0.38f, 0.50f, 4049);
    matter (s, 0.86f, 0.68f, 0.12f, 0.70f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.20f, 0.50f, 0.18f, 0.0f, 0.35f, 0.40f);
    set (s, Param::evolveMagnetTarget, 3 /* MINOR */);
    // Dotted eighths: eight of them make six beats, so the lope crosses the bar line
    // and lands somewhere new every bar and a half.
    fracture (s, 1 /* RHYTHMIC */, 0.86f, 1.0f, 0.50f, 1.0f, 0.35f, 0.35f, 0.45f, 0.50f, 0.30f,
              1 /* 16 */, 10 /* 1/8D */, 8, 0.15f, 2 /* PINGPONG */, 0.95f, 0.20f, 4051,
              FractureShape { 16, 0.04f, 0.38f, 0.22f, 0.48f, 0.40f, 0.65f, 0.25f, 0.80f,
                              1.5f, 1.5f, 0.65f, 0.95f, kMinorTerrace, "XoLoXoHo", nullptr });
    space (s, SpacePresets::Dream, 0.42f, 0.60f, 0.55f, 0.40f);

    env (s, 2, 0.30f, 1.60f, 0.70f, 0.80f, 0.55f);
    lfo (s, 1, 0.17f, 0 /* SINE */, 1.0f, true, 0.5f);
    macros (s, 0.40f, 0.45f, 0.45f, 0.50f);

    Routings r;
    r.uni (ModSource::Env2,   Param::gestureSpeed,   0.250f)
     .bi  (ModSource::LFO1,   Param::gesturePressure, 0.200f)
     .uni (ModSource::Velocity, Param::gesturePressure, 0.300f)
     .uni (ModSource::Velocity, Param::fractureAmount, 0.120f)
     .bi  (ModSource::KeyTrack, Param::gestureBandwidth, 0.200f)
     .bi  (ModSource::NoteRandom, Param::gestureRoughness, 0.150f)
     .uni (ModSource::Macro1, Param::fractureEvolve, 0.300f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.250f)
     .uni (ModSource::Macro2, Param::gestureBandwidth, 0.300f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.300f)
     .uni (ModSource::Macro4, Param::fractureSwing,  0.300f)
     .uni (ModSource::Macro4, Param::gestureRoughness, 0.250f);
    sharedMacros (r, Param::fractureDecay, Param::fractureProbability);
    r.commit (s);
}});

manager.addFactory ({ "Terrace Climb", "SEQUENCE", { "glassy", "bright", "melodic", "pulsing", "high" }, [] (PatchState& s)
{
    // Eight steps that climb a minor eleventh and drop back: the pattern is the tune.
    static constexpr float kClimb[8] = { 0.0f, 3.0f, 7.0f, 10.0f, 12.0f, 15.0f, 19.0f, 24.0f };

    wave (s, 0 /* BASIC */, 0.25f, 0.20f, 0.05f, 3, 0.12f, 0.55f);
    amp (s, 0.003f, 0.50f, 0.88f, 0.35f, 0.30f);
    set (s, Param::masterGain, 3.5f);
    shape (s, 0.45f, 0.35f, 0.30f, 0.60f, 0.45f, 0.24f);
    material (s, MaterialType::Crystal, MaterialType::Membrane, 0.40f);
    topology (s, 1 /* RING */, 0.40f, 0.50f, 4057);
    matter (s, 0.85f, 0.65f, 0.08f, 0.70f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.35f, 0.45f, 0.10f, 0.0f, 0.40f, 0.20f);
    set (s, Param::evolveMagnetTarget, 5 /* SCALE */);
    fracture (s, 1 /* RHYTHMIC */, 0.90f, 1.0f, 0.40f, 1.0f, 0.30f, 0.25f, 0.16f, 0.60f, 0.20f,
              1 /* 16 */, 4 /* 1/16 */, 8, 0.0f, 0 /* FORWARD */, 1.0f, 0.08f, 4059,
              FractureShape { 16, 0.02f, 0.25f, 0.18f, 0.40f, 0.30f, 0.52f, 0.20f, 0.75f,
                              1.8f, 1.8f, 0.60f, 1.0f, nullptr, "XXXXXXXX", kClimb });
    space (s, SpacePresets::Orbit, 0.34f, 0.42f, 0.62f, 0.45f);

    env (s, 1, 0.002f, 0.20f, 0.10f, 0.18f, 0.30f);
    lfo (s, 1, 0.24f, 1 /* TRIANGLE */, 1.0f, false);
    macros (s, 0.35f, 0.50f, 0.35f, 0.50f);

    Routings r;
    r.uni (ModSource::Env1,   Param::shapeExcite,    0.200f)
     .bi  (ModSource::LFO1,   Param::fractureSequence, 0.150f)
     .uni (ModSource::Velocity, Param::waveMorph,    0.300f)
     .uni (ModSource::Velocity, Param::shapeExcite,  0.180f)
     .bi  (ModSource::KeyTrack, Param::shapeDecay,  -0.200f)
     .bi  (ModSource::NoteRandom, Param::waveDetune, 0.120f)
     .uni (ModSource::Macro1, Param::fractureEvolve, 0.300f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.250f)
     .uni (ModSource::Macro2, Param::wavePosition,   0.300f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.300f)
     .uni (ModSource::Macro4, Param::fracturePitch,  0.150f)
     .uni (ModSource::Macro4, Param::fractureSpread, 0.250f);
    sharedMacros (r, Param::fractureDecay, Param::fractureRandom);
    r.commit (s);
}});

manager.addFactory ({ "Falling Stair", "SEQUENCE", { "wooden", "dark", "melodic", "rhythmic", "low" }, [] (PatchState& s)
{
    wave (s, 1 /* HARMONIC */, 0.30f, 0.25f, 0.08f, 1, 0.08f, 0.35f, -1);
    amp (s, 0.004f, 0.55f, 0.88f, 0.40f, 0.30f);
    set (s, Param::masterGain, 1.5f);
    shape (s, 0.44f, 0.30f, 0.55f, 0.45f, 0.48f, 0.26f);
    material (s, MaterialType::String, MaterialType::Wood, 0.45f);
    topology (s, 0 /* CHAIN */, 0.34f, 0.45f, 4061);
    matter (s, 0.86f, 0.62f, 0.20f, 0.60f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.35f, 0.58f, 0.10f, 0.0f, 0.35f, 0.20f);
    set (s, Param::evolveMagnetTarget, 3 /* MINOR */);
    // The step pitches walk down a two-octave stair; the fragments carry the same fall,
    // so the whole object drops rather than just the top of it.
    fracture (s, 1 /* RHYTHMIC */, 0.88f, 1.0f, 0.38f, 1.0f, 0.30f, 0.26f, 0.20f, 0.42f, 0.20f,
              1 /* 16 */, 4 /* 1/16 */, 8, 0.0f, 0 /* FORWARD */, 1.0f, 0.08f, 4063,
              FractureShape { 16, 0.02f, 0.24f, 0.18f, 0.40f, 0.30f, 0.50f, 0.20f, 0.72f,
                              1.9f, 1.9f, 0.55f, 1.0f, kFallingTerrace, "XXXXXXXX", kFallingTerrace });
    space (s, SpacePresets::Chamber, 0.28f, 0.38f, 0.42f, 0.30f);

    env (s, 1, 0.002f, 0.22f, 0.10f, 0.18f, 0.30f);
    lfo (s, 1, 0.22f, 1 /* TRIANGLE */, 1.0f, false);
    macros (s, 0.35f, 0.45f, 0.35f, 0.50f);

    Routings r;
    r.uni (ModSource::Env1,   Param::shapeExcite,    0.200f)
     .bi  (ModSource::LFO1,   Param::fractureSequence, 0.150f)
     .uni (ModSource::Velocity, Param::waveMorph,    0.300f)
     .uni (ModSource::Velocity, Param::shapeStrike,  0.180f)
     .bi  (ModSource::KeyTrack, Param::shapeDecay,  -0.200f)
     .bi  (ModSource::NoteRandom, Param::waveFine,   0.080f)
     .uni (ModSource::Macro1, Param::fractureEvolve, 0.300f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.250f)
     .uni (ModSource::Macro2, Param::wavePosition,   0.300f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.300f)
     .uni (ModSource::Macro4, Param::fracturePitch, -0.150f)
     .uni (ModSource::Macro4, Param::shapeMass,      0.250f);
    sharedMacros (r, Param::fractureDecay, Param::fractureRandom);
    r.commit (s);
}});

manager.addFactory ({ "Cold Ladder", "SEQUENCE", { "glassy", "cold", "close", "struck", "melodic" }, [] (PatchState& s)
{
    impact (s, 5 /* DAMPED SINE */, 0.55f, 0.60f, 0.18f, 0.85f, 0.40f, 0.15f, 0.75f, 0.75f);
    amp (s, 0.002f, 0.60f, 0.90f, 0.35f, 0.30f);
    set (s, Param::masterGain, 0.0f);
    shape (s, 0.46f, 0.42f, 0.30f, 0.62f, 0.42f, 0.24f);
    material (s, MaterialType::Crystal, MaterialType::Liquid, 0.40f);
    topology (s, 3 /* LATTICE */, 0.42f, 0.50f, 4067);
    matter (s, 0.85f, 0.66f, 0.45f, 0.65f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.40f, 0.48f, 0.12f, 0.0f, 0.40f, 0.22f);
    set (s, Param::evolveMagnetTarget, 3 /* MINOR */);
    // A fast free-running strike stream under a locked gate: the roll is the material,
    // the sixteenths are the music.
    fracture (s, 1 /* RHYTHMIC */, 0.98f, 1.0f, 0.40f, 1.0f, 0.28f, 0.24f, 0.22f, 0.58f, 0.20f,
              1 /* 16 */, 4 /* 1/16 */, 8, 0.0f, 0 /* FORWARD */, 1.0f, 0.10f, 4069,
              FractureShape { 16, 0.02f, 0.26f, 0.15f, 0.38f, 0.28f, 0.48f, 0.20f, 0.70f,
                              1.8f, 1.8f, 0.50f, 1.0f, kMinorTerrace, "XLXHXLXH", kMinorTerrace });
    space (s, SpacePresets::Chamber, 0.22f, 0.28f, 0.58f, 0.25f);

    env (s, 1, 0.001f, 0.16f, 0.0f, 0.14f, 0.25f);
    lfo (s, 1, 0.30f, 5 /* SMOOTH RANDOM */, 1.0f, true);
    macros (s, 0.35f, 0.50f, 0.25f, 0.55f);

    Routings r;
    r.uni (ModSource::Env1,   Param::shapeStrike,    0.200f)
     .bi  (ModSource::LFO1,   Param::impactRate,     0.120f)
     .uni (ModSource::Velocity, Param::impactBrightness, 0.320f)
     .uni (ModSource::Velocity, Param::impactHardness, 0.200f)
     .bi  (ModSource::KeyTrack, Param::impactRate,   0.150f)
     .bi  (ModSource::NoteRandom, Param::impactRandom, 0.150f)
     .uni (ModSource::Macro1, Param::fractureEvolve, 0.300f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.250f)
     .uni (ModSource::Macro2, Param::impactBrightness, 0.300f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.300f)
     .uni (ModSource::Macro4, Param::impactRate,     0.250f)
     .uni (ModSource::Macro4, Param::fractureSpread, 0.220f);
    sharedMacros (r, Param::fractureDecay, Param::impactRandom);
    r.commit (s);
}});

manager.addFactory ({ "Stutter Relay", "SEQUENCE", { "synthetic", "unstable", "chaotic", "high", "close" }, [] (PatchState& s)
{
    dust (s, 8 /* FROZEN */, 0.62f, 0.55f, 0.48f, 0.40f, 0.60f, 0.70f, 4073);
    amp (s, 0.002f, 0.60f, 0.90f, 0.30f, 0.30f);
    set (s, Param::masterGain, 1.5f);
    shape (s, 0.60f, 0.62f, 0.26f, 0.62f, 0.38f, 0.42f);
    material (s, MaterialType::Void, MaterialType::Crystal, 0.45f);
    topology (s, 4 /* RANDOM */, 0.48f, 0.55f, 4075);
    matter (s, 0.78f, 0.70f, 0.35f, 0.80f);
    evolve (s, 0.0f, 0.0f, 0.20f, 0.0f, 0.50f, 0.40f, 0.15f, 0.65f, 0.50f);
    set (s, Param::evolveScatterSeed, 4077);
    // Locked to the thirty-second grid, but the step randomiser rewrites the gate, the
    // gain and which fragments are open every time round: the same loop never repeats.
    fracture (s, 1 /* RHYTHMIC */, 1.0f, 1.0f, 0.35f, 1.0f, 0.14f, 0.10f, 0.16f, 0.55f, 0.30f,
              2 /* 32 */, 4 /* 1/16 */, 16, 0.0f, 3 /* RANDOM */, 0.80f, 0.35f, 4079,
              FractureShape { 32, 0.005f, 0.10f, 0.05f, 0.18f, 0.25f, 0.48f, 0.35f, 0.90f,
                              1.8f, 1.8f, 0.80f, 0.85f, kOctaveTerrace, "XX.oX..H", nullptr });
    space (s, SpacePresets::Machine, 0.20f, 0.28f, 0.55f, 0.30f);

    chaos (s, 1, 2 /* LOGISTIC */, 6.00f, 0.55f, 0.35f, 0.5f, 4081);
    env (s, 1, 0.001f, 0.12f, 0.0f, 0.10f, 0.25f);
    macros (s, 0.55f, 0.50f, 0.30f, 0.60f);

    Routings r;
    r.bi  (ModSource::Chaos1, Param::fractureProbability, 0.200f)
     .bi  (ModSource::Chaos1, Param::dustGrain,      0.200f)
     .uni (ModSource::Env1,   Param::shapeSurface,   0.180f)
     .uni (ModSource::Velocity, Param::dustColor,    0.300f)
     .bi  (ModSource::KeyTrack, Param::fractureTone, 0.200f)
     .bi  (ModSource::NoteRandom, Param::fractureSpread, 0.200f)
     .uni (ModSource::Macro1, Param::fractureEvolve, 0.300f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.280f)
     .uni (ModSource::Macro2, Param::fractureTone,   0.300f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.300f)
     .uni (ModSource::Macro4, Param::fractureRandom, 0.300f)
     .uni (ModSource::Macro4, Param::evolveTear,     0.220f);
    sharedMacros (r, Param::fractureDecay, Param::fractureProbability);
    r.commit (s);
}});

manager.addFactory ({ "Broken Clock", "SEQUENCE", { "hollow", "unstable", "rhythmic", "dry", "mid" }, [] (PatchState& s)
{
    impact (s, 0 /* IMPULSE */, 0.50f, 0.40f, 0.20f, 0.80f, 0.50f, 0.30f, 0.70f, 0.60f);
    dust (s, 0 /* WHITE */, 0.40f, 0.40f, 0.35f, 0.30f, 0.50f, 0.50f, 4083, 0.30f);
    set (s, Param::sourceSelected, 2 /* IMPACT ticks, DUST is the room */);
    set (s, Param::sourceMode, 1 /* LAYER */);
    set (s, Param::waveLevel, 0.0f);
    set (s, Param::sampleLevel, 0.0f);
    set (s, Param::gestureLevel, 0.0f);
    amp (s, 0.002f, 0.70f, 0.88f, 0.40f, 0.30f);
    set (s, Param::masterGain, -1.0f);
    shape (s, 0.48f, 0.55f, 0.42f, 0.48f, 0.45f, 0.35f);
    material (s, MaterialType::Void, MaterialType::Wood, 0.45f);
    topology (s, 2 /* CLUSTERS */, 0.40f, 0.52f, 4085);
    matter (s, 0.80f, 0.65f, 0.40f, 0.68f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.55f, 0.45f, 0.0f, 0.45f, 0.40f);
    set (s, Param::evolveScatterSeed, 4087);
    // Twelve steps at an eighth with a probability of a little over a half: the clock
    // keeps missing beats, and the ones it does hit move around the bar.
    fracture (s, 1 /* RHYTHMIC */, 0.85f, 1.0f, 0.45f, 1.0f, 0.30f, 0.30f, 0.28f, 0.45f, 0.25f,
              1 /* 16 */, 3 /* 1/8 */, 12, 0.10f, 0 /* FORWARD */, 0.58f, 0.25f, 4089,
              FractureShape { 16, 0.02f, 0.30f, 0.18f, 0.42f, 0.30f, 0.52f, 0.25f, 0.78f,
                              1.9f, 1.9f, 0.60f, 0.90f, kOctaveTerrace, "XL.XoH.X.L.o", nullptr });
    space (s, SpacePresets::Chamber, 0.26f, 0.36f, 0.46f, 0.32f);

    chaos (s, 1, 0 /* WALK */, 0.90f, 0.55f, 0.45f, 0.5f, 4091);
    env (s, 1, 0.001f, 0.20f, 0.0f, 0.16f, 0.25f);
    macros (s, 0.45f, 0.45f, 0.30f, 0.60f);

    Routings r;
    r.bi  (ModSource::Chaos1, Param::fractureProbability, 0.250f)
     .bi  (ModSource::Chaos1, Param::evolveScatter,  0.150f)
     .uni (ModSource::Env1,   Param::shapeStrike,    0.200f)
     .uni (ModSource::Velocity, Param::impactHardness, 0.300f)
     .bi  (ModSource::KeyTrack, Param::impactBrightness, 0.200f)
     .bi  (ModSource::NoteRandom, Param::impactRate,  0.180f)
     .uni (ModSource::Macro1, Param::fractureEvolve, 0.300f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.250f)
     .uni (ModSource::Macro2, Param::fractureTone,   0.300f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.300f)
     .uni (ModSource::Macro4, Param::fractureProbability, 0.300f)
     .uni (ModSource::Macro4, Param::evolveScatter,  0.220f);
    sharedMacros (r, Param::fractureDecay, Param::fractureRandom);
    r.commit (s);
}});

manager.addFactory ({ "Hammer Floor", "SEQUENCE", { "dark", "low", "pulsing", "struck", "roomy" }, [] (PatchState& s)
{
    wave (s, 0 /* BASIC */, 0.15f, 0.10f, 0.0f, 1, 0.0f, 0.25f, -1, 0.80f);
    impact (s, 6 /* MEMBRANE HIT */, 0.35f, 0.30f, 0.25f, 0.90f, 0.40f, 0.12f, 0.62f, 0.55f);
    set (s, Param::sourceSelected, 0 /* WAVE is the floor, IMPACT is the hammer */);
    set (s, Param::sourceMode, 1 /* LAYER */);
    set (s, Param::dustLevel, 0.0f);
    set (s, Param::sampleLevel, 0.0f);
    set (s, Param::gestureLevel, 0.0f);
    amp (s, 0.003f, 0.90f, 0.88f, 0.55f, 0.30f);
    set (s, Param::masterGain, -3.5f);
    shape (s, 0.32f, 0.40f, 0.80f, 0.30f, 0.52f, 0.20f);
    material (s, MaterialType::Membrane, MaterialType::Wood, 0.45f);
    topology (s, 0 /* CHAIN */, 0.30f, 0.42f, 4093);
    matter (s, 0.78f, 0.55f, 0.55f, 0.45f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.25f, 0.66f, 0.10f, 0.0f, 0.25f, 0.15f);
    set (s, Param::evolveMagnetTarget, 0 /* OCTAVE */);
    // Four quarter notes: one bar, and the third one drops to the bottom half only.
    fracture (s, 1 /* RHYTHMIC */, 0.90f, 1.0f, 0.30f, 1.0f, 0.28f, 0.28f, 0.26f, 0.35f, 0.15f,
              1 /* 16 */, 2 /* 1/4 */, 4, 0.0f, 0 /* FORWARD */, 1.0f, 0.08f, 4095,
              FractureShape { 16, 0.02f, 0.20f, 0.15f, 0.32f, 0.35f, 0.55f, 0.15f, 0.60f,
                              1.7f, 1.7f, 0.40f, 1.0f, nullptr, "X.XL", nullptr });
    space (s, SpacePresets::Chamber, 0.30f, 0.50f, 0.38f, 0.35f);
    set (s, Param::spaceEqLow, 3.0f);

    env (s, 1, 0.001f, 0.35f, 0.0f, 0.30f, 0.25f);
    lfo (s, 1, 0.18f, 0 /* SINE */, 1.0f, true);
    macros (s, 0.30f, 0.40f, 0.40f, 0.50f);

    Routings r;
    r.uni (ModSource::Env1,   Param::shapeSurface,   0.200f)
     .bi  (ModSource::LFO1,   Param::spaceTone,      0.150f)
     .uni (ModSource::Velocity, Param::impactHardness, 0.350f)
     .uni (ModSource::Velocity, Param::shapeStrike,  0.220f)
     .bi  (ModSource::KeyTrack, Param::shapeDecay,  -0.220f)
     .bi  (ModSource::NoteRandom, Param::impactRandom, 0.120f)
     .uni (ModSource::Macro1, Param::fractureEvolve, 0.300f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.250f)
     .uni (ModSource::Macro2, Param::waveMorph,      0.300f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.300f)
     .uni (ModSource::Macro4, Param::shapeMass,      0.300f)
     .uni (ModSource::Macro4, Param::fractureSwing,  0.250f);
    sharedMacros (r, Param::fractureDecay, Param::fractureRandom);
    r.commit (s);
}});

manager.addFactory ({ "Rubber Steps", "SEQUENCE", { "organic", "soft", "rhythmic", "close", "mid" }, [] (PatchState& s)
{
    gesture (s, 2 /* RUB */, 0.58f, 0.55f, 0.40f, 0.45f, 0.35f, 0.50f);
    amp (s, 0.006f, 0.70f, 0.90f, 0.45f, 0.35f);
    set (s, Param::masterGain, 2.5f);
    shape (s, 0.48f, 0.35f, 0.45f, 0.42f, 0.50f, 0.35f);
    material (s, MaterialType::Liquid, MaterialType::Membrane, 0.45f);
    topology (s, 1 /* RING */, 0.44f, 0.50f, 4099);
    matter (s, 0.85f, 0.66f, 0.25f, 0.70f);
    evolve (s, 0.0f, 0.15f, 0.0f, 0.30f, 0.52f, 0.15f, 0.0f, 0.40f, 0.35f);
    set (s, Param::evolveMagnetTarget, 5 /* SCALE */);
    // Heavy swing on a ping-pong sixteenth: every second step arrives late and the
    // pattern walks back down the way it came.
    fracture (s, 1 /* RHYTHMIC */, 0.84f, 1.0f, 0.45f, 1.0f, 0.32f, 0.30f, 0.30f, 0.48f, 0.25f,
              1 /* 16 */, 4 /* 1/16 */, 8, 0.62f, 2 /* PINGPONG */, 1.0f, 0.12f, 4101,
              FractureShape { 16, 0.03f, 0.30f, 0.20f, 0.44f, 0.32f, 0.55f, 0.25f, 0.80f,
                              1.8f, 1.8f, 0.65f, 1.0f, kMinorTerrace, "XoXLoXHo", nullptr });
    space (s, SpacePresets::Chamber, 0.28f, 0.34f, 0.50f, 0.32f);

    env (s, 1, 0.002f, 0.24f, 0.10f, 0.20f, 0.30f);
    lfo (s, 1, 0.26f, 0 /* SINE */, 1.0f, true);
    macros (s, 0.40f, 0.45f, 0.30f, 0.55f);

    Routings r;
    r.uni (ModSource::Env1,   Param::gesturePressure, 0.200f)
     .bi  (ModSource::LFO1,   Param::gestureSpeed,   0.200f)
     .uni (ModSource::Velocity, Param::gesturePressure, 0.300f)
     .uni (ModSource::Velocity, Param::fractureAmount, 0.100f)
     .bi  (ModSource::KeyTrack, Param::gestureRoughness, 0.180f)
     .bi  (ModSource::NoteRandom, Param::gesturePosition, 0.150f)
     .uni (ModSource::Macro1, Param::fractureEvolve, 0.300f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.250f)
     .uni (ModSource::Macro2, Param::gestureBandwidth, 0.300f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.300f)
     .uni (ModSource::Macro4, Param::fractureSwing, -0.300f)
     .uni (ModSource::Macro4, Param::evolveMelt,     0.220f);
    sharedMacros (r, Param::fractureDecay, Param::fractureRandom);
    r.commit (s);
}});

manager.addFactory ({ "Cicada Grid", "SEQUENCE", { "bright", "granular", "rhythmic", "high", "wide" }, [] (PatchState& s)
{
    dust (s, 7 /* CLOUD */, 0.85f, 0.72f, 0.25f, 0.45f, 0.90f, 0.85f, 4111);
    amp (s, 0.004f, 0.70f, 0.90f, 0.40f, 0.35f);
    set (s, Param::masterGain, 8.5f);
    shape (s, 0.66f, 0.60f, 0.22f, 0.66f, 0.40f, 0.38f);
    material (s, MaterialType::Crystal, MaterialType::Organic, 0.40f);
    topology (s, 3 /* LATTICE */, 0.46f, 0.58f, 4113);
    matter (s, 0.82f, 0.70f, 0.25f, 0.88f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.30f, 0.42f, 0.30f, 0.0f, 0.60f, 0.45f);
    set (s, Param::evolveMagnetTarget, 5 /* SCALE */);
    set (s, Param::evolveScatterSeed, 4115);
    // Thirty-seconds: dense enough that the grid reads as a texture with a pulse in it
    // rather than as separate notes.
    fracture (s, 1 /* RHYTHMIC */, 1.0f, 1.0f, 0.35f, 1.0f, 0.12f, 0.09f, 0.16f, 0.62f, 0.25f,
              2 /* 32 */, 5 /* 1/32 */, 16, 0.0f, 0 /* FORWARD */, 0.95f, 0.15f, 4117,
              FractureShape { 32, 0.004f, 0.09f, 0.04f, 0.16f, 0.25f, 0.45f, 0.35f, 0.95f,
                              1.7f, 1.7f, 0.85f, 0.95f, kFifthTerrace, "Xo.oXH.o", nullptr });
    space (s, SpacePresets::Dust, 0.20f, 0.40f, 0.62f, 0.35f);

    lfo (s, 1, 0.18f, 5 /* SMOOTH RANDOM */, 1.0f, false);
    env (s, 1, 0.001f, 0.14f, 0.10f, 0.12f, 0.25f);
    macros (s, 0.50f, 0.55f, 0.40f, 0.50f);

    Routings r;
    r.bi  (ModSource::LFO1,   Param::dustJitter,     0.220f)
     .bi  (ModSource::LFO1,   Param::fractureSpread, 0.150f)
     .uni (ModSource::Env1,   Param::shapeExcite,    0.180f)
     .uni (ModSource::Velocity, Param::dustDensity,  0.280f)
     .bi  (ModSource::KeyTrack, Param::dustColor,   -0.200f)
     .bi  (ModSource::NoteRandom, Param::dustGrain,  0.180f)
     .uni (ModSource::Macro1, Param::fractureEvolve, 0.300f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.280f)
     .uni (ModSource::Macro2, Param::dustColor,      0.300f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.300f)
     .uni (ModSource::Macro4, Param::fractureSpread, 0.300f)
     .uni (ModSource::Macro4, Param::dustJitter,     0.250f);
    sharedMacros (r, Param::fractureDecay, Param::fractureProbability);
    r.commit (s);
}});

manager.addFactory ({ "Tape Shuffle", "SEQUENCE", { "wooden", "warm", "rhythmic", "roomy", "mid" }, [] (PatchState& s)
{
    sample (s, BuiltInSamples::Kind::WoodKnock, 1 /* LOOP */, 0.05f, 0.85f, 0.40f, 0.50f, 60, 0.90f);
    amp (s, 0.006f, 0.80f, 0.90f, 0.50f, 0.35f);
    set (s, Param::masterGain, 2.0f);
    shape (s, 0.44f, 0.32f, 0.50f, 0.44f, 0.50f, 0.32f);
    material (s, MaterialType::Wood, MaterialType::Liquid, 0.40f);
    topology (s, 4 /* RANDOM */, 0.40f, 0.48f, 4119);
    matter (s, 0.84f, 0.66f, 0.35f, 0.62f);
    evolve (s, 0.0f, 0.22f, 0.0f, 0.25f, 0.55f, 0.20f, 0.15f, 0.35f, 0.35f);
    set (s, Param::evolveMagnetTarget, 5 /* SCALE */);
    // A hard shuffle on the eighths, with the loop drifting under it.
    fracture (s, 1 /* RHYTHMIC */, 0.85f, 1.0f, 0.42f, 1.0f, 0.32f, 0.32f, 0.30f, 0.42f, 0.25f,
              1 /* 16 */, 3 /* 1/8 */, 8, 0.58f, 0 /* FORWARD */, 0.95f, 0.18f, 4121,
              FractureShape { 16, 0.03f, 0.32f, 0.20f, 0.45f, 0.32f, 0.55f, 0.25f, 0.78f,
                              1.8f, 1.8f, 0.60f, 0.95f, kOctaveTerrace, "XoLXoH.o", nullptr });
    space (s, SpacePresets::Dust, 0.34f, 0.45f, 0.42f, 0.38f);

    lfo (s, 1, 0.80f, 0 /* SINE */, 1.0f, false);
    env (s, 1, 0.002f, 0.26f, 0.10f, 0.22f, 0.30f);
    macros (s, 0.40f, 0.40f, 0.40f, 0.55f);

    Routings r;
    r.bi  (ModSource::LFO1,   Param::samplePitch,    0.025f)
     .uni (ModSource::Env1,   Param::sampleGrain,    0.180f)
     .uni (ModSource::Velocity, Param::sampleGrain,  0.280f)
     .uni (ModSource::Velocity, Param::fractureAmount, 0.100f)
     .bi  (ModSource::KeyTrack, Param::sampleStart,  0.150f)
     .bi  (ModSource::NoteRandom, Param::sampleStart, 0.120f)
     .uni (ModSource::Macro1, Param::fractureEvolve, 0.300f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.250f)
     .uni (ModSource::Macro2, Param::spaceTone,      0.300f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.300f)
     .uni (ModSource::Macro4, Param::fractureSwing, -0.320f)
     .uni (ModSource::Macro4, Param::evolveCrush,    0.220f);
    sharedMacros (r, Param::fractureDecay, Param::sampleSpread);
    r.commit (s);
}});

manager.addFactory ({ "Bowed Pump", "SEQUENCE", { "bowed", "organic", "breathing", "roomy", "low" }, [] (PatchState& s)
{
    gesture (s, 0 /* BOW */, 0.66f, 0.38f, 0.28f, 0.32f, 0.30f, 0.45f);
    amp (s, 0.12f, 1.40f, 0.92f, 1.00f, 0.50f);
    set (s, Param::masterGain, -7.0f);
    shape (s, 0.42f, 0.22f, 0.62f, 0.48f, 0.66f, 0.24f);
    material (s, MaterialType::String, MaterialType::Organic, 0.45f);
    topology (s, 0 /* CHAIN */, 0.42f, 0.48f, 4127);
    matter (s, 0.82f, 0.55f, 0.15f, 0.72f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.32f, 0.58f, 0.14f, 0.0f, 0.30f, 0.30f);
    set (s, Param::evolveMagnetTarget, 5 /* SCALE */);
    // Quarter notes with a long step decay: the bow keeps sounding underneath and the
    // pattern leans on it rather than chopping it up.
    fracture (s, 1 /* RHYTHMIC */, 0.94f, 1.0f, 0.40f, 1.0f, 0.22f, 0.30f, 0.70f, 0.45f, 0.22f,
              1 /* 16 */, 2 /* 1/4 */, 8, 0.18f, 0 /* FORWARD */, 1.0f, 0.10f, 4129,
              FractureShape { 16, 0.04f, 0.30f, 0.12f, 0.30f, 0.45f, 0.62f, 0.25f, 0.80f,
                              1.5f, 1.5f, 0.55f, 1.0f, kFifthTerrace, "XLXoXHXL", nullptr });
    space (s, SpacePresets::Chamber, 0.36f, 0.55f, 0.45f, 0.38f);

    env (s, 2, 0.50f, 2.20f, 0.75f, 1.00f, 0.55f);
    lfo (s, 1, 0.15f, 0 /* SINE */, 1.0f, true, 0.6f);
    macros (s, 0.35f, 0.45f, 0.45f, 0.50f);

    Routings r;
    r.uni (ModSource::Env2,   Param::gesturePressure, 0.220f)
     .uni (ModSource::Env2,   Param::gestureSpeed,   0.200f)
     .bi  (ModSource::LFO1,   Param::gesturePosition, 0.180f)
     .uni (ModSource::Velocity, Param::gesturePressure, 0.300f)
     .bi  (ModSource::KeyTrack, Param::shapeDecay,  -0.180f)
     .bi  (ModSource::NoteRandom, Param::gestureRoughness, 0.140f)
     .uni (ModSource::Macro1, Param::fractureEvolve, 0.300f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.250f)
     .uni (ModSource::Macro2, Param::gestureBandwidth, 0.300f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.300f)
     .uni (ModSource::Macro4, Param::fractureDecay, -0.300f)
     .uni (ModSource::Macro4, Param::shapeMass,      0.220f);
    sharedMacros (r, Param::ampDecay, Param::fractureRandom);
    r.commit (s);
}});

manager.addFactory ({ "Split Register", "SEQUENCE", { "cold", "hollow", "rhythmic", "wide", "layer" }, [] (PatchState& s)
{
    wave (s, 5 /* SPECTRAL */, 0.38f, 0.30f, 0.12f, 2, 0.14f, 0.60f, 0, 0.70f);
    dust (s, 3 /* BLUE */, 0.55f, 0.62f, 0.30f, 0.25f, 0.75f, 0.80f, 4133, 0.45f);
    set (s, Param::sourceSelected, 0 /* WAVE holds the bottom, DUST the top */);
    set (s, Param::sourceMode, 1 /* LAYER */);
    set (s, Param::impactLevel, 0.0f);
    set (s, Param::sampleLevel, 0.0f);
    set (s, Param::gestureLevel, 0.0f);
    amp (s, 0.005f, 0.80f, 0.90f, 0.45f, 0.35f);
    set (s, Param::masterGain, 4.0f);
    shape (s, 0.54f, 0.58f, 0.40f, 0.58f, 0.50f, 0.30f);
    material (s, MaterialType::Void, MaterialType::Metal, 0.45f);
    topology (s, 5 /* STAR */, 0.44f, 0.55f, 4135);
    matter (s, 0.84f, 0.66f, 0.18f, 0.90f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.28f, 0.50f, 0.20f, 0.0f, 0.45f, 0.30f);
    set (s, Param::evolveMagnetTarget, 1 /* FIFTH */);
    // The step masks do the work: L opens the bottom half of the spectrum and H the top,
    // so the two halves of the object answer each other across the bar.
    fracture (s, 1 /* RHYTHMIC */, 1.0f, 1.0f, 0.55f, 1.0f, 0.20f, 0.18f, 0.28f, 0.52f, 0.22f,
              2 /* 32 */, 4 /* 1/16 */, 16, 0.0f, 0 /* FORWARD */, 1.0f, 0.10f, 4139,
              FractureShape { 32, 0.01f, 0.18f, 0.10f, 0.28f, 0.30f, 0.55f, 0.30f, 0.90f,
                              1.8f, 1.8f, 0.85f, 1.0f, kOctaveTerrace, "LLHHL.H.LHLHL.H.", nullptr });
    space (s, SpacePresets::Orbit, 0.34f, 0.48f, 0.58f, 0.45f);

    env (s, 1, 0.002f, 0.26f, 0.10f, 0.20f, 0.30f);
    lfo (s, 1, 0.12f, 1 /* TRIANGLE */, 1.0f, false);
    set (s, Param::lfo1Sync, 1.0f);
    set (s, Param::lfo1Division, 2.0f /* 2/1 — two bars */);
    macros (s, 0.45f, 0.50f, 0.40f, 0.50f);

    Routings r;
    r.bi  (ModSource::LFO1,   Param::fractureTone,   0.280f)
     .bi  (ModSource::LFO1,   Param::dustColor,      0.200f)
     .uni (ModSource::Env1,   Param::shapeExcite,    0.180f)
     .uni (ModSource::Velocity, Param::waveMorph,    0.280f)
     .bi  (ModSource::KeyTrack, Param::dustDensity, -0.200f)
     .bi  (ModSource::NoteRandom, Param::wavePosition, 0.150f)
     .uni (ModSource::Macro1, Param::fractureEvolve, 0.300f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.250f)
     .uni (ModSource::Macro2, Param::fractureTone,   0.320f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.300f)
     .uni (ModSource::Macro4, Param::fractureSpread, 0.300f)
     .uni (ModSource::Macro4, Param::waveSpread,     0.220f);
    sharedMacros (r, Param::fractureDecay, Param::fractureRandom);
    r.commit (s);
}});

manager.addFactory ({ "Reverse Cascade", "SEQUENCE", { "glassy", "cold", "morphing", "distant", "melodic" }, [] (PatchState& s)
{
    sample (s, BuiltInSamples::Kind::GlassStrike, 2 /* REVERSE */, 0.05f, 0.80f, 0.38f, 0.60f, 60, 0.90f);
    amp (s, 0.025f, 0.90f, 0.90f, 0.60f, 0.40f);
    set (s, Param::masterGain, 3.5f);
    shape (s, 0.48f, 0.50f, 0.34f, 0.56f, 0.55f, 0.26f);
    material (s, MaterialType::Liquid, MaterialType::String, 0.45f);
    topology (s, 5 /* STAR */, 0.38f, 0.52f, 4147);
    matter (s, 0.86f, 0.62f, 0.20f, 0.80f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.48f, 0.46f, 0.16f, 0.0f, 0.42f, 0.32f);
    set (s, Param::evolveMagnetTarget, 3 /* MINOR */);
    // Played backwards through a falling terrace: the sequencer walks the steps in
    // reverse while the pitches fall, so the figure seems to be sucked inwards.
    fracture (s, 1 /* RHYTHMIC */, 0.98f, 1.0f, 0.48f, 1.0f, 0.20f, 0.26f, 0.44f, 0.52f, 0.25f,
              1 /* 16 */, 4 /* 1/16 */, 8, 0.0f, 1 /* BACKWARD */, 1.0f, 0.12f, 4149,
              FractureShape { 16, 0.02f, 0.24f, 0.14f, 0.36f, 0.32f, 0.55f, 0.25f, 0.82f,
                              1.8f, 1.8f, 0.70f, 1.0f, kFallingTerrace, "XLoXHLoX", kFallingTerrace });
    space (s, SpacePresets::Dream, 0.44f, 0.66f, 0.55f, 0.34f);

    env (s, 1, 0.002f, 0.28f, 0.12f, 0.24f, 0.30f);
    lfo (s, 1, 0.21f, 5 /* SMOOTH RANDOM */, 1.0f, false);
    macros (s, 0.40f, 0.45f, 0.55f, 0.50f);

    Routings r;
    r.uni (ModSource::Env1,   Param::sampleGrain,    0.180f)
     .bi  (ModSource::LFO1,   Param::sampleStart,    0.150f)
     .uni (ModSource::Velocity, Param::sampleGrain,  0.280f)
     .uni (ModSource::Velocity, Param::fractureAmount, 0.100f)
     .bi  (ModSource::KeyTrack, Param::shapeExcite,  0.200f)
     .bi  (ModSource::NoteRandom, Param::samplePitch, 0.050f)
     .uni (ModSource::Macro1, Param::fractureEvolve, 0.300f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.250f)
     .uni (ModSource::Macro2, Param::fractureTone,   0.300f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.300f)
     .uni (ModSource::Macro4, Param::fracturePitch, -0.150f)
     .uni (ModSource::Macro4, Param::sampleSpread,   0.250f);
    sharedMacros (r, Param::fractureDecay, Param::fractureProbability);
    r.commit (s);
}});

manager.addFactory ({ "Bit Ladder", "SEQUENCE", { "synthetic", "harsh", "melodic", "close", "high" }, [] (PatchState& s)
{
    // Four notes up and three back down: a square-wave arpeggio with the corners filed off.
    static constexpr float kBitRungs[8] = { 0.0f, 7.0f, 12.0f, 19.0f, 24.0f, 19.0f, 12.0f, 7.0f };

    wave (s, 0 /* BASIC */, 0.70f, 0.15f, 0.0f, 1, 0.04f, 0.30f);
    amp (s, 0.002f, 0.55f, 0.90f, 0.25f, 0.30f);
    set (s, Param::masterGain, 4.0f);
    shape (s, 0.40f, 0.30f, 0.26f, 0.64f, 0.38f, 0.30f);
    material (s, MaterialType::Custom, MaterialType::Metal, 0.35f);
    topology (s, 3 /* LATTICE */, 0.38f, 0.48f, 4153);
    matter (s, 0.72f, 0.68f, 0.22f, 0.55f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.30f, 0.48f, 0.12f, 0.52f, 0.45f, 0.25f);
    set (s, Param::evolveMagnetTarget, 0 /* OCTAVE */);
    fracture (s, 1 /* RHYTHMIC */, 1.0f, 1.0f, 0.30f, 1.0f, 0.18f, 0.14f, 0.20f, 0.60f, 0.18f,
              0 /* 8 */, 4 /* 1/16 */, 8, 0.0f, 0 /* FORWARD */, 1.0f, 0.06f, 4157,
              FractureShape { 8, 0.01f, 0.14f, 0.08f, 0.24f, 0.28f, 0.48f, 0.15f, 0.60f,
                              1.9f, 1.9f, 0.40f, 1.0f, kOctaveTerrace, "XXXXXXXX", kBitRungs });
    space (s, SpacePresets::Machine, 0.22f, 0.26f, 0.55f, 0.30f);

    env (s, 1, 0.001f, 0.14f, 0.0f, 0.12f, 0.25f);
    lfo (s, 1, 0.19f, 3 /* SQUARE */, 1.0f, false);
    set (s, Param::lfo1Sync, 1.0f);
    set (s, Param::lfo1Division, 3.0f /* 1/1 — one bar */);
    macros (s, 0.40f, 0.50f, 0.25f, 0.60f);

    Routings r;
    r.uni (ModSource::Env1,   Param::shapeExcite,    0.200f)
     .bi  (ModSource::LFO1,   Param::fracturePitch,  0.060f)
     .uni (ModSource::Velocity, Param::evolveCrush, -0.250f)
     .uni (ModSource::Velocity, Param::wavePosition, 0.250f)
     .bi  (ModSource::KeyTrack, Param::fractureTone, 0.200f)
     .bi  (ModSource::NoteRandom, Param::waveFine,   0.060f)
     .uni (ModSource::Macro1, Param::fractureEvolve, 0.300f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.250f)
     .uni (ModSource::Macro2, Param::wavePosition,   0.320f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.300f)
     .uni (ModSource::Macro4, Param::evolveCrush,    0.300f)
     .uni (ModSource::Macro4, Param::fractureSwing,  0.250f);
    sharedMacros (r, Param::fractureDecay, Param::evolveCrush);
    r.commit (s);
}});

manager.addFactory ({ "Pulse Cloud", "SEQUENCE", { "soft", "evolving", "granular", "wide", "drift" }, [] (PatchState& s)
{
    dust (s, 7 /* CLOUD */, 0.70f, 0.50f, 0.40f, 0.35f, 0.85f, 0.80f, 4159, 0.75f);
    gesture (s, 1 /* SCRAPE */, 0.45f, 0.35f, 0.45f, 0.40f, 0.40f, 0.50f, 0.40f);
    set (s, Param::sourceSelected, 1 /* DUST is the cloud, GESTURE the grain in it */);
    set (s, Param::sourceMode, 1 /* LAYER */);
    set (s, Param::waveLevel, 0.0f);
    set (s, Param::impactLevel, 0.0f);
    set (s, Param::sampleLevel, 0.0f);
    amp (s, 0.05f, 1.20f, 0.90f, 0.80f, 0.45f);
    set (s, Param::masterGain, 2.0f);
    shape (s, 0.62f, 0.66f, 0.34f, 0.54f, 0.58f, 0.40f);
    material (s, MaterialType::Liquid, MaterialType::Void, 0.45f);
    topology (s, 2 /* CLUSTERS */, 0.48f, 0.58f, 4161);
    matter (s, 0.86f, 0.64f, 0.20f, 0.88f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.40f, 0.44f, 0.28f, 0.0f, 0.55f, 0.55f);
    set (s, Param::evolveMagnetTarget, 5 /* SCALE */);
    set (s, Param::evolveScatterSeed, 4163);
    // EVOLVE mode: the fragments are given new pitches, delays and pans on every step
    // and take half a step to get there, so the pulse is still a pulse but the cloud
    // inside it is never twice the same.
    fracture (s, 3 /* EVOLVE */, 0.92f, 1.0f, 0.60f, 0.85f, 0.35f, 0.40f, 0.55f, 0.55f, 0.80f,
              2 /* 32 */, 6 /* 1/4T */, 12, 0.0f, 2 /* PINGPONG */, 0.90f, 0.25f, 4167,
              FractureShape { 32, 0.05f, 0.45f, 0.20f, 0.42f, 0.40f, 0.68f, 0.30f, 0.95f,
                              1.6f, 1.6f, 0.85f, 0.92f, kMinorTerrace, "XLoH.XoL.HXo", nullptr });
    space (s, SpacePresets::Dust, 0.44f, 0.62f, 0.55f, 0.45f);

    lfo (s, 1, 0.14f, 5 /* SMOOTH RANDOM */, 1.0f, false);
    chaos (s, 1, 4 /* TARGETS */, 0.80f, 0.45f, 0.55f, 0.5f, 4169);
    env (s, 2, 0.40f, 2.00f, 0.70f, 1.00f, 0.50f);
    macros (s, 0.55f, 0.50f, 0.50f, 0.55f);

    Routings r;
    r.bi  (ModSource::LFO1,   Param::dustGrain,      0.200f)
     .bi  (ModSource::Chaos1, Param::fractureSpread, 0.200f)
     .uni (ModSource::Env2,   Param::dustDensity,    0.220f)
     .uni (ModSource::Velocity, Param::dustColor,    0.280f)
     .bi  (ModSource::KeyTrack, Param::fractureTone, 0.180f)
     .bi  (ModSource::NoteRandom, Param::gesturePosition, 0.180f)
     .uni (ModSource::Macro1, Param::fractureEvolve, 0.320f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.280f)
     .uni (ModSource::Macro2, Param::dustColor,      0.300f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.300f)
     .uni (ModSource::Macro4, Param::fractureSpread, 0.300f)
     .uni (ModSource::Macro4, Param::evolveScatter,  0.220f);
    sharedMacros (r, Param::fractureDecay, Param::fractureProbability);
    r.commit (s);
}});

manager.addFactory ({ "Anvil Waltz", "SEQUENCE", { "metallic", "wooden", "rhythmic", "roomy", "mid" }, [] (PatchState& s)
{
    impact (s, 4 /* METAL STRIKE */, 0.62f, 0.50f, 0.16f, 0.88f, 0.38f, 0.18f, 0.70f, 0.72f);
    amp (s, 0.002f, 0.70f, 0.90f, 0.40f, 0.30f);
    set (s, Param::masterGain, 0.0f);
    shape (s, 0.46f, 0.52f, 0.48f, 0.54f, 0.46f, 0.28f);
    material (s, MaterialType::Metal, MaterialType::Membrane, 0.42f);
    topology (s, 1 /* RING */, 0.40f, 0.50f, 4171);
    matter (s, 0.86f, 0.64f, 0.50f, 0.65f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.34f, 0.54f, 0.12f, 0.0f, 0.35f, 0.22f);
    set (s, Param::evolveMagnetTarget, 2 /* MAJOR */);
    // Six eighths to the cycle: a three-beat lope laid over whatever the host thinks
    // the bar is, with the weight on one and the answer on four.
    fracture (s, 1 /* RHYTHMIC */, 0.98f, 1.0f, 0.38f, 1.0f, 0.26f, 0.24f, 0.26f, 0.48f, 0.20f,
              1 /* 16 */, 3 /* 1/8 */, 6, 0.08f, 0 /* FORWARD */, 1.0f, 0.10f, 4177,
              FractureShape { 16, 0.02f, 0.24f, 0.14f, 0.36f, 0.30f, 0.52f, 0.22f, 0.76f,
                              1.8f, 1.8f, 0.60f, 1.0f, kFifthTerrace, "X..Lo.", nullptr });
    space (s, SpacePresets::Chamber, 0.32f, 0.48f, 0.48f, 0.35f);

    env (s, 1, 0.001f, 0.22f, 0.0f, 0.18f, 0.25f);
    lfo (s, 1, 0.16f, 1 /* TRIANGLE */, 1.0f, true);
    macros (s, 0.35f, 0.45f, 0.40f, 0.55f);

    Routings r;
    r.uni (ModSource::Env1,   Param::shapeStrike,    0.200f)
     .bi  (ModSource::LFO1,   Param::impactRate,     0.120f)
     .uni (ModSource::Velocity, Param::impactHardness, 0.320f)
     .uni (ModSource::Velocity, Param::impactBrightness, 0.220f)
     .bi  (ModSource::KeyTrack, Param::shapeDecay,  -0.200f)
     .bi  (ModSource::NoteRandom, Param::impactRandom, 0.150f)
     .uni (ModSource::Macro1, Param::fractureEvolve, 0.300f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.250f)
     .uni (ModSource::Macro2, Param::impactBrightness, 0.300f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.300f)
     .uni (ModSource::Macro4, Param::fractureSwing,  0.300f)
     .uni (ModSource::Macro4, Param::impactRate,     0.250f);
    sharedMacros (r, Param::fractureDecay, Param::impactRandom);
    r.commit (s);
}});
}

} // namespace am::FactoryContent
