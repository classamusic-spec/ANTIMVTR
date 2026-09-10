#include "presets/FactoryBuilders.h"

namespace am::FactoryContent
{

void registerPad (PresetManager& manager)
{
//==========================================================================
// PAD — held chords that stay interesting for a whole bar
//==========================================================================

manager.addFactory ({ "Void Bloom", "PAD", { "pad", "dark", "evolving", "cinematic" }, [] (PatchState& s)
{
    wave (s, 5 /* SPECTRAL */, 0.34f, 0.28f, 0.06f, 4, 0.20f, 0.85f);
    amp (s, 0.90f, 1.40f, 0.78f, 3.20f, 0.55f);
    shape (s, 0.68f, 0.30f, 0.55f, 0.42f, 0.72f, 0.22f);
    material (s, MaterialType::Void, MaterialType::Organic, 0.35f);
    topology (s, 2 /* CLUSTERS */, 0.38f, 0.55f, 7);
    matter (s, 0.86f, 0.55f, 0.22f, 0.78f);
    evolve (s, 0.0f, 0.26f, 0.0f, 0.34f, 0.44f, 0.12f, 0.0f, 0.18f, 0.40f);
    set (s, Param::evolveMagnetTarget, 1 /* FIFTH */);
    space (s, SpacePresets::Nebula, 0.52f, 0.74f, 0.55f, 0.38f);

    lfo (s, 1, 0.11f, 0 /* SINE */, 1.0f, false, 1.5f);
    lfo (s, 2, 0.07f, 1 /* TRIANGLE */, 1.0f, false, 0.0f, 0.6f);
    env (s, 2, 3.20f, 4.0f, 0.85f, 4.0f, 0.6f);
    chaos (s, 1, 0 /* WALK */, 0.13f, 0.6f, 0.75f, 0.5f, 211);
    macros (s, 0.40f, 0.35f, 0.45f, 0.30f);

    Routings r;
    r.bi  (ModSource::LFO1,   Param::shapeForm,      0.055f)
     .bi  (ModSource::LFO2,   Param::shapeSurface,   0.070f)
     .uni (ModSource::Env2,   Param::evolveMelt,     0.220f)
     .bi  (ModSource::Chaos1, Param::wavePosition,   0.090f)
     .uni (ModSource::Velocity, Param::shapeExcite,  0.180f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.400f)
     .uni (ModSource::Macro1, Param::evolveSpeed,    0.250f)
     .uni (ModSource::Macro2, Param::shapeExcite,    0.300f)
     .uni (ModSource::Macro2, Param::spaceTone,      0.250f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.280f)
     .uni (ModSource::Macro3, Param::spaceSize,      0.200f)
     .uni (ModSource::Macro4, Param::shapeDensity,   0.250f)
     .uni (ModSource::Macro4, Param::evolveMagnet,   0.300f);
    sharedMacros (r, Param::spaceSize, Param::chaos1Depth);
    r.commit (s);
}});

manager.addFactory ({ "Nebula Pad", "PAD", { "pad", "wide", "warm", "chords" }, [] (PatchState& s)
{
    wave (s, 1 /* HARMONIC */, 0.42f, 0.35f, 0.10f, 5, 0.26f, 0.95f);
    amp (s, 0.55f, 1.80f, 0.72f, 2.60f, 0.5f);
    shape (s, 0.60f, 0.08f, 0.55f, 0.52f, 0.74f, 0.30f);
    material (s, MaterialType::Organic, MaterialType::String, 0.40f);
    topology (s, 1 /* RING */, 0.45f, 0.60f, 23);
    matter (s, 0.62f, 0.45f, 0.18f, 0.85f);
    evolve (s, 0.14f, 0.0f, 0.0f, 0.22f, 0.42f, 0.16f, 0.0f, 0.14f, 0.30f);
    set (s, Param::evolveBendPivot, 0.60f);
    set (s, Param::evolveMagnetTarget, 2 /* MAJOR */);
    space (s, SpacePresets::Dream, 0.46f, 0.62f, 0.60f, 0.32f);

    lfo (s, 1, 0.18f, 0 /* SINE */, 1.0f, false, 0.8f);
    lfo (s, 2, 0.09f, 5 /* SMOOTH RANDOM */, 1.0f, false);
    env (s, 2, 1.80f, 3.0f, 0.60f, 3.0f);
    macros (s, 0.35f, 0.50f, 0.40f, 0.25f);

    Routings r;
    r.bi  (ModSource::LFO1,   Param::waveMorph,      0.120f)
     .bi  (ModSource::LFO2,   Param::shapeTension,   0.045f)
     .uni (ModSource::Env2,   Param::shapeDensity,   0.180f)
     .uni (ModSource::Velocity, Param::waveScan,     0.150f)
     .bi  (ModSource::KeyTrack, Param::shapeDecay,  -0.120f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.350f)
     .uni (ModSource::Macro1, Param::lfo1Rate,       0.020f)
     .uni (ModSource::Macro2, Param::wavePosition,   0.320f)
     .uni (ModSource::Macro2, Param::spaceTone,      0.220f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.300f)
     .uni (ModSource::Macro4, Param::waveDetune,     0.260f)
     .uni (ModSource::Macro4, Param::shapeStereo,    0.150f);
    sharedMacros (r, Param::spaceReverbDecay, Param::waveDetune);
    r.commit (s);
}});

manager.addFactory ({ "Membrane Sky", "PAD", { "pad", "airy", "membrane", "breathing" }, [] (PatchState& s)
{
    dust (s, 2 /* BROWN */, 0.88f, 0.20f, 0.30f, 0.22f, 0.70f, 0.80f, 91);
    amp (s, 0.40f, 2.00f, 0.92f, 3.60f, 0.6f);
    shape (s, 0.72f, 0.34f, 0.60f, 0.44f, 0.86f, 0.24f);
    material (s, MaterialType::Membrane, MaterialType::Liquid, 0.45f);
    topology (s, 3 /* LATTICE */, 0.52f, 0.38f, 131);
    matter (s, 0.50f, 0.30f, 0.05f, 0.90f);
    evolve (s, 0.0f, 0.18f, 0.0f, 0.0f, 0.38f, 0.22f, 0.0f, 0.20f, 0.45f);
    space (s, SpacePresets::Shimmer, 0.45f, 0.68f, 0.45f, 0.35f);

    lfo (s, 1, 0.13f, 5 /* SMOOTH RANDOM */, 1.0f, false, 2.0f);
    lfo (s, 2, 0.26f, 0 /* SINE */, 1.0f, false);
    env (s, 2, 2.50f, 5.0f, 0.70f, 5.0f, 0.7f);
    chaos (s, 2, 1 /* BROWNIAN */, 0.22f, 0.55f, 0.70f, 0.5f, 407);
    macros (s, 0.45f, 0.40f, 0.42f, 0.35f);

    Routings r;
    r.bi  (ModSource::LFO1,   Param::dustColor,      0.140f)
     .bi  (ModSource::LFO2,   Param::shapeForm,      0.040f)
     .uni (ModSource::Env2,   Param::shapeDensity,   0.200f)
     .bi  (ModSource::Chaos2, Param::dustDensity,    0.150f)
     .uni (ModSource::Velocity, Param::dustGrain,    0.200f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.400f)
     .uni (ModSource::Macro1, Param::dustJitter,     0.300f)
     .uni (ModSource::Macro2, Param::dustColor,      0.300f)
     .uni (ModSource::Macro2, Param::shapeExcite,    0.200f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.300f)
     .uni (ModSource::Macro4, Param::dustDensity,    0.350f);
    sharedMacros (r, Param::spaceSize, Param::dustJitter);
    r.commit (s);
}});

//--------------------------------------------------------------------------
// A sheet of glass bowed at one edge. The bow takes a moment to catch, then
// the plate answers by opening its upper lattice for as long as the key is
// held — the note is brighter at the end of the bar than at the start.
manager.addFactory ({ "Bowed Pane", "PAD", { "glassy", "bowed", "breathing", "wide", "chords" }, [] (PatchState& s)
{
    gesture (s, 0 /* BOW */, 0.52f, 0.30f, 0.16f, 0.42f, 0.26f, 0.30f);
    amp (s, 0.80f, 2.20f, 0.88f, 3.20f, 0.55f);
    set (s, Param::masterGain, 5.0f);   // a bowed plate feeds Matter gently: make the level up after it
    shape (s, 0.56f, 0.60f, 0.30f, 0.64f, 0.78f, 0.14f);
    material (s, MaterialType::Crystal, MaterialType::Liquid, 0.36f);
    topology (s, 3 /* LATTICE */, 0.46f, 0.62f, 41);
    matter (s, 0.94f, 0.44f, 0.05f, 0.84f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.44f, 0.08f, 0.0f, 0.10f, 0.26f);
    space (s, SpacePresets::Dream, 0.48f, 0.66f, 0.62f, 0.28f);

    lfo (s, 1, 0.09f, 0 /* SINE */, 1.0f, false, 2.0f);
    lfo (s, 2, 0.16f, 5 /* SMOOTH RANDOM */, 1.0f, false, 1.0f);
    env (s, 2, 4.50f, 6.0f, 0.90f, 5.0f, 0.65f);
    macros (s, 0.35f, 0.45f, 0.45f, 0.30f);

    Routings r;
    r.uni (ModSource::Env2,     Param::gestureBandwidth, 0.300f)
     .uni (ModSource::Env2,     Param::shapeDensity,     0.220f)
     .bi  (ModSource::LFO1,     Param::gesturePosition,  0.100f)
     .bi  (ModSource::LFO2,     Param::gestureSpeed,     0.080f)
     .uni (ModSource::Velocity, Param::gesturePressure,  0.240f)
     .bi  (ModSource::KeyTrack, Param::gestureBandwidth, 0.200f)
     .uni (ModSource::Macro1,   Param::gestureMotion,    0.400f)
     .uni (ModSource::Macro1,   Param::evolveMotion,     0.250f)
     .uni (ModSource::Macro2,   Param::gestureBandwidth, 0.350f)
     .uni (ModSource::Macro2,   Param::shapeExcite,      0.220f)
     .uni (ModSource::Macro3,   Param::spaceMix,         0.280f)
     .uni (ModSource::Macro3,   Param::spaceSize,        0.200f)
     .uni (ModSource::Macro4,   Param::gesturePressure,  0.320f)
     .uni (ModSource::Macro4,   Param::shapeSurface,     0.180f);
    sharedMacros (r, Param::spaceReverbDecay, Param::gestureRoughness);
    r.commit (s);
}});

//--------------------------------------------------------------------------
// A rack of tuned metal rods, restruck about once a second by a soft mallet
// and left to ring into each other through a tight coupling ring. FRACTURE
// terraces the strikes into octaves so the rack sounds twice its size.
manager.addFactory ({ "Rod Choir", "PAD", { "metallic", "struck", "resonant", "roomy", "chords" }, [] (PatchState& s)
{
    impact (s, 4 /* METAL STRIKE */, 0.34f, 0.46f, 0.40f, 0.62f, 0.45f, 0.35f, 0.17f);
    amp (s, 0.25f, 1.60f, 0.82f, 3.00f, 0.5f);
    shape (s, 0.62f, 0.52f, 0.42f, 0.58f, 0.80f, 0.18f);
    material (s, MaterialType::Metal, MaterialType::Crystal, 0.42f);
    topology (s, 1 /* RING */, 0.62f, 0.48f, 137);
    matter (s, 0.90f, 0.52f, 0.44f, 0.70f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.26f, 0.46f, 0.14f, 0.0f, 0.16f, 0.30f);
    set (s, Param::evolveMagnetTarget, 5 /* SCALE */);
    space (s, SpacePresets::Chamber, 0.42f, 0.58f, 0.55f, 0.28f);

    FractureShape f;
    f.fragments = 16;
    f.delayLow = 0.10f; f.delayHigh = 0.55f;
    f.feedbackLow = 0.10f; f.feedbackHigh = 0.30f;
    f.decayLow = 0.55f; f.decayHigh = 0.78f;
    f.gainLow = 0.85f; f.gainHigh = 0.45f;
    f.panWidth = 0.70f;
    f.pitchCycle = kOctaveTerrace;
    f.pattern = "XLXHXLXH";
    fracture (s, 0 /* SPECTRAL */, 0.55f, 0.40f, 0.60f, 0.35f, 0.18f, 0.42f, 0.62f, 0.55f, 0.20f,
              1 /* 16 */, 4 /* 1/16 */, 8, 0.0f, 0 /* FORWARD */, 1.0f, 0.15f, 271, f);

    lfo (s, 1, 0.12f, 1 /* TRIANGLE */, 1.0f, false, 1.2f, 0.65f);
    env (s, 2, 2.40f, 4.0f, 0.70f, 3.5f, 0.6f);
    chaos (s, 1, 0 /* WALK */, 0.20f, 0.45f, 0.80f, 0.5f, 313);
    macros (s, 0.35f, 0.45f, 0.40f, 0.35f);

    Routings r;
    r.bi  (ModSource::LFO1,       Param::shapeCoupling,   0.070f)
     .uni (ModSource::Env2,       Param::fractureMix,     0.200f)
     .bi  (ModSource::Chaos1,     Param::impactHardness,  0.120f)
     .uni (ModSource::Velocity,   Param::shapeStrike,     0.260f)
     .uni (ModSource::NoteRandom, Param::impactRandom,    0.220f)
     .bi  (ModSource::KeyTrack,   Param::impactBrightness, 0.180f)
     .uni (ModSource::Macro1,     Param::impactRate,      0.220f)
     .uni (ModSource::Macro1,     Param::evolveMotion,    0.300f)
     .uni (ModSource::Macro2,     Param::impactBrightness, 0.320f)
     .uni (ModSource::Macro2,     Param::spaceTone,       0.220f)
     .uni (ModSource::Macro3,     Param::spaceMix,        0.300f)
     .uni (ModSource::Macro4,     Param::shapeCoupling,   0.260f)
     .uni (ModSource::Macro4,     Param::fractureAmount,  0.250f);
    sharedMacros (r, Param::fractureDecay, Param::fractureRandom);
    r.commit (s);
}});

//--------------------------------------------------------------------------
// A stone chamber with a cloud of dust turning inside it. Holding the key
// does not sustain a chord so much as open a door: a six second envelope
// walks the node count and the room size up while the note is down.
manager.addFactory ({ "Slow Chamber", "PAD", { "hollow", "breathing", "huge", "mid", "chords" }, [] (PatchState& s)
{
    dust (s, 7 /* CLOUD */, 0.42f, 0.34f, 0.52f, 0.30f, 0.65f, 0.72f, 617);
    amp (s, 1.20f, 2.60f, 0.90f, 4.20f, 0.6f);
    shape (s, 0.36f, 0.24f, 0.52f, 0.44f, 0.82f, 0.20f);
    material (s, MaterialType::Void, MaterialType::Membrane, 0.44f);
    topology (s, 2 /* CLUSTERS */, 0.34f, 0.30f, 71);
    matter (s, 0.88f, 0.34f, 0.08f, 0.88f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.38f, 0.16f, 0.0f, 0.14f, 0.34f);
    space (s, SpacePresets::Void, 0.54f, 0.72f, 0.40f, 0.36f);

    lfo (s, 1, 0.07f, 5 /* SMOOTH RANDOM */, 1.0f, false, 2.5f);
    lfo (s, 2, 0.05f, 0 /* SINE */, 1.0f, false, 3.0f);
    env (s, 2, 6.00f, 8.0f, 0.95f, 6.0f, 0.7f);
    env (s, 3, 3.00f, 6.0f, 0.80f, 5.0f, 0.55f);
    macros (s, 0.40f, 0.30f, 0.55f, 0.35f);

    Routings r;
    r.uni (ModSource::Env2,     Param::shapeDensity,   0.380f)
     .uni (ModSource::Env2,     Param::spaceSize,      0.180f)
     .uni (ModSource::Env3,     Param::shapeForm,      0.240f)
     .bi  (ModSource::LFO1,     Param::dustGrain,      0.140f)
     .bi  (ModSource::LFO2,     Param::shapeMass,      0.060f)
     .uni (ModSource::Velocity, Param::dustDensity,    0.200f)
     .uni (ModSource::Macro1,   Param::evolveMotion,   0.380f)
     .uni (ModSource::Macro1,   Param::dustJitter,     0.250f)
     .uni (ModSource::Macro2,   Param::dustColor,      0.320f)
     .uni (ModSource::Macro2,   Param::shapeExcite,    0.240f)
     .uni (ModSource::Macro3,   Param::spaceMix,       0.260f)
     .uni (ModSource::Macro3,   Param::spaceSize,      0.240f)
     .uni (ModSource::Macro4,   Param::shapeDensity,   0.300f);
    sharedMacros (r, Param::spaceReverbDecay, Param::dustJitter);
    r.commit (s);
}});

//--------------------------------------------------------------------------
// Cedar slats woven into a loom and set humming. A chain topology passes the
// energy down the run of wood, and a slow LFO on the coupling makes the weave
// tighten and loosen so the chord never settles into one colour.
manager.addFactory ({ "Cedar Loom", "PAD", { "wooden", "warm", "soft", "roomy", "chords" }, [] (PatchState& s)
{
    wave (s, 1 /* HARMONIC */, 0.28f, 0.22f, 0.08f, 4, 0.18f, 0.72f, 0, 0.90f);
    amp (s, 0.60f, 1.90f, 0.80f, 2.60f, 0.5f);
    shape (s, 0.48f, 0.14f, 0.58f, 0.38f, 0.70f, 0.26f);
    material (s, MaterialType::Wood, MaterialType::Organic, 0.44f);
    topology (s, 0 /* CHAIN */, 0.42f, 0.46f, 19);
    matter (s, 0.82f, 0.48f, 0.16f, 0.66f);
    evolve (s, 0.10f, 0.0f, 0.0f, 0.0f, 0.44f, 0.12f, 0.0f, 0.12f, 0.26f);
    set (s, Param::evolveBendPivot, 0.34f);
    set (s, Param::evolveBendRange, 0.30f);
    space (s, SpacePresets::Chamber, 0.38f, 0.50f, 0.48f, 0.26f);

    lfo (s, 1, 0.10f, 0 /* SINE */, 1.0f, false, 1.5f);
    lfo (s, 2, 0.23f, 1 /* TRIANGLE */, 1.0f, false, 0.0f, 0.35f);
    env (s, 2, 2.20f, 4.5f, 0.65f, 3.0f);
    macros (s, 0.30f, 0.40f, 0.35f, 0.40f);

    Routings r;
    r.bi  (ModSource::LFO1,     Param::shapeCoupling,  0.110f)
     .bi  (ModSource::LFO2,     Param::waveMorph,      0.090f)
     .uni (ModSource::Env2,     Param::shapeTension,   0.140f)
     .bi  (ModSource::KeyTrack, Param::shapeDecay,    -0.160f)
     .uni (ModSource::Velocity, Param::shapeExcite,    0.200f)
     .uni (ModSource::Macro1,   Param::evolveMotion,   0.320f)
     .uni (ModSource::Macro1,   Param::lfo1Rate,       0.015f)
     .uni (ModSource::Macro2,   Param::wavePosition,   0.300f)
     .uni (ModSource::Macro2,   Param::shapeExcite,    0.240f)
     .uni (ModSource::Macro3,   Param::spaceMix,       0.280f)
     .uni (ModSource::Macro4,   Param::shapeCoupling,  0.280f)
     .uni (ModSource::Macro4,   Param::waveDetune,     0.220f);
    sharedMacros (r, Param::spaceReverbDecay, Param::evolveBend);
    r.commit (s);
}});

//--------------------------------------------------------------------------
// Breath pushed through a bundle of stopped reeds. The pipes are joined at a
// single hub, so the air pressure in one is felt in all of them; leaning on
// the key drives the whole star harder rather than louder.
manager.addFactory ({ "Hollow Reeds", "PAD", { "blown", "hollow", "organic", "breathing", "chords" }, [] (PatchState& s)
{
    gesture (s, 3 /* BREATH */, 0.52f, 0.36f, 0.30f, 0.30f, 0.30f, 0.44f);
    amp (s, 0.55f, 1.80f, 0.84f, 2.20f, 0.45f);
    shape (s, 0.44f, 0.36f, 0.46f, 0.42f, 0.66f, 0.24f);
    material (s, MaterialType::Membrane, MaterialType::Wood, 0.40f);
    topology (s, 5 /* STAR */, 0.50f, 0.42f, 89);
    matter (s, 0.86f, 0.54f, 0.08f, 0.62f);
    set (s, Param::masterGain, 6.0f);   // breath through wood is a low-output excitation
    evolve (s, 0.0f, 0.14f, 0.0f, 0.0f, 0.50f, 0.10f, 0.0f, 0.20f, 0.32f);
    space (s, SpacePresets::Chamber, 0.40f, 0.46f, 0.52f, 0.24f);

    lfo (s, 1, 0.28f, 0 /* SINE */, 1.0f, true, 0.8f);
    lfo (s, 2, 0.14f, 5 /* SMOOTH RANDOM */, 1.0f, false, 1.0f);
    env (s, 2, 1.60f, 3.0f, 0.72f, 2.5f, 0.5f);
    macros (s, 0.35f, 0.40f, 0.35f, 0.45f);

    Routings r;
    r.bi  (ModSource::LFO1,     Param::gesturePressure, 0.090f)
     .bi  (ModSource::LFO2,     Param::gesturePosition, 0.130f)
     .uni (ModSource::Env2,     Param::gestureBandwidth, 0.220f)
     .uni (ModSource::Velocity, Param::gesturePressure, 0.280f)
     .uni (ModSource::Velocity, Param::gestureRoughness, 0.180f)
     .bi  (ModSource::KeyTrack, Param::gestureSpeed,    0.200f)
     .uni (ModSource::Macro1,   Param::gestureMotion,   0.400f)
     .uni (ModSource::Macro2,   Param::gestureBandwidth, 0.300f)
     .uni (ModSource::Macro2,   Param::spaceTone,       0.220f)
     .uni (ModSource::Macro3,   Param::spaceMix,        0.300f)
     .uni (ModSource::Macro4,   Param::gestureRoughness, 0.300f)
     .uni (ModSource::Macro4,   Param::shapeSurface,    0.200f);
    sharedMacros (r, Param::ampDecay, Param::gestureMotion);
    r.commit (s);
}});

//--------------------------------------------------------------------------
// A choir heard through salt air: a breath sample read as grains and poured
// into a cluster of organic resonators, which give the grains formants they
// never had. Velocity opens the grain size, so a soft chord is smoother.
manager.addFactory ({ "Salt Choir", "PAD", { "vocal", "formant", "granular", "wide", "chords" }, [] (PatchState& s)
{
    sample (s, BuiltInSamples::Kind::Breath, 3 /* GRANULAR */, 0.06f, 0.92f, 0.58f, 0.55f, 60, 0.95f);
    amp (s, 0.70f, 2.20f, 0.86f, 3.00f, 0.55f);
    shape (s, 0.54f, 0.30f, 0.44f, 0.48f, 0.80f, 0.22f);
    material (s, MaterialType::Organic, MaterialType::Membrane, 0.46f);
    topology (s, 2 /* CLUSTERS */, 0.44f, 0.58f, 233);
    matter (s, 0.86f, 0.58f, 0.14f, 0.86f);
    set (s, Param::masterGain, 5.5f);   // grains leave gaps: the object has to be driven harder to sing
    evolve (s, 0.0f, 0.0f, 0.0f, 0.20f, 0.46f, 0.14f, 0.0f, 0.16f, 0.30f);
    set (s, Param::evolveMagnetTarget, 2 /* MAJOR */);
    space (s, SpacePresets::Nebula, 0.50f, 0.68f, 0.58f, 0.32f);
    set (s, Param::spaceCompOn, 1.0f);
    set (s, Param::spaceCompAmount, 0.40f);   // a choir has to breathe evenly, not in grain-sized gusts

    lfo (s, 1, 0.13f, 5 /* SMOOTH RANDOM */, 1.0f, false, 1.5f);
    lfo (s, 2, 0.21f, 0 /* SINE */, 1.0f, false, 0.5f);
    env (s, 2, 2.80f, 5.0f, 0.78f, 4.0f, 0.6f);
    macros (s, 0.35f, 0.40f, 0.50f, 0.40f);

    Routings r;
    r.bi  (ModSource::LFO1,     Param::sampleStart,   0.070f)
     .bi  (ModSource::LFO2,     Param::shapeForm,     0.060f)
     .uni (ModSource::Env2,     Param::sampleSpread,  0.220f)
     .uni (ModSource::Velocity, Param::sampleGrain,   0.280f)
     .bi  (ModSource::KeyTrack, Param::sampleSpread, -0.180f)
     .uni (ModSource::NoteRandom, Param::sampleStart, 0.120f)
     .uni (ModSource::Macro1,   Param::evolveMotion,  0.360f)
     .uni (ModSource::Macro1,   Param::sampleGrain,   0.240f)
     .uni (ModSource::Macro2,   Param::shapeExcite,   0.320f)
     .uni (ModSource::Macro2,   Param::spaceTone,     0.220f)
     .uni (ModSource::Macro3,   Param::spaceMix,      0.300f)
     .uni (ModSource::Macro4,   Param::shapeDensity,  0.280f)
     .uni (ModSource::Macro4,   Param::evolveMagnet,  0.260f);
    sharedMacros (r, Param::spaceSize, Param::sampleSpread);
    r.commit (s);
}});

//--------------------------------------------------------------------------
// A wide sheet of water held up against gravity. MELT is the patch: the
// partials sag through the note and the high ones fade first, so the chord
// slumps a semitone-ish and darkens without ever being retriggered.
manager.addFactory ({ "Tide Sheet", "PAD", { "soft", "morphing", "wide", "mid", "drift" }, [] (PatchState& s)
{
    wave (s, 5 /* SPECTRAL */, 0.30f, 0.44f, 0.12f, 3, 0.16f, 0.80f, 0, 0.85f);
    amp (s, 1.00f, 2.40f, 0.88f, 3.60f, 0.6f);
    shape (s, 0.58f, 0.42f, 0.50f, 0.46f, 0.76f, 0.28f);
    material (s, MaterialType::Liquid, MaterialType::Membrane, 0.48f);
    topology (s, 4 /* RANDOM */, 0.40f, 0.52f, 401);
    matter (s, 0.88f, 0.44f, 0.10f, 0.80f);
    set (s, Param::masterGain, 4.0f);
    evolve (s, 0.0f, 0.46f, 0.0f, 0.0f, 0.58f, 0.12f, 0.0f, 0.10f, 0.42f);
    space (s, SpacePresets::Dream, 0.52f, 0.70f, 0.44f, 0.34f);

    lfo (s, 1, 0.06f, 0 /* SINE */, 1.0f, false, 2.0f);
    env (s, 2, 3.60f, 6.0f, 0.85f, 4.5f, 0.65f);
    chaos (s, 1, 1 /* BROWNIAN */, 0.11f, 0.50f, 0.72f, 0.5f, 907);
    macros (s, 0.45f, 0.30f, 0.50f, 0.45f);

    Routings r;
    r.uni (ModSource::Env2,     Param::evolveMelt,    0.320f)
     .bi  (ModSource::LFO1,     Param::shapeMass,     0.070f)
     .bi  (ModSource::Chaos1,   Param::waveMorph,     0.110f)
     .uni (ModSource::Velocity, Param::waveScan,      0.180f)
     .bi  (ModSource::KeyTrack, Param::evolveMelt,   -0.140f)
     .uni (ModSource::Macro1,   Param::evolveMotion,  0.400f)
     .uni (ModSource::Macro1,   Param::evolveSpeed,   0.200f)
     .uni (ModSource::Macro2,   Param::wavePosition,  0.300f)
     .uni (ModSource::Macro2,   Param::shapeExcite,   0.220f)
     .uni (ModSource::Macro3,   Param::spaceMix,      0.280f)
     .uni (ModSource::Macro3,   Param::spaceSize,     0.220f)
     .uni (ModSource::Macro4,   Param::evolveMelt,    0.380f)
     .uni (ModSource::Macro4,   Param::evolveGravity, 0.200f);
    sharedMacros (r, Param::spaceReverbDecay, Param::chaos1Depth);
    r.commit (s);
}});

//--------------------------------------------------------------------------
// A band of filtered noise that is not a chord until you wait for it. MAGNET
// hauls the partials onto a major grid over four seconds, so the patch enters
// as weather and resolves into harmony while the key is still down.
manager.addFactory ({ "Magnet Vespers", "PAD", { "cold", "evolving", "resonant", "huge", "chords" }, [] (PatchState& s)
{
    dust (s, 4 /* FILTERED */, 0.60f, 0.44f, 0.30f, 0.34f, 0.60f, 0.70f, 1013);
    amp (s, 0.90f, 2.20f, 0.88f, 3.40f, 0.55f);
    shape (s, 0.70f, 0.68f, 0.40f, 0.62f, 0.78f, 0.16f);
    material (s, MaterialType::Void, MaterialType::Crystal, 0.52f);
    topology (s, 2 /* CLUSTERS */, 0.56f, 0.70f, 547);
    matter (s, 0.92f, 0.46f, 0.06f, 0.82f);
    set (s, Param::masterGain, 5.0f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.30f, 0.44f, 0.18f, 0.0f, 0.12f, 0.30f);
    set (s, Param::evolveMagnetTarget, 2 /* MAJOR */);
    space (s, SpacePresets::Shimmer, 0.48f, 0.74f, 0.56f, 0.34f);

    lfo (s, 1, 0.08f, 1 /* TRIANGLE */, 1.0f, false, 2.0f, 0.7f);
    env (s, 2, 4.00f, 7.0f, 0.92f, 5.0f, 0.6f);
    chaos (s, 2, 4 /* TARGETS */, 0.16f, 0.55f, 0.65f, 0.5f, 199);
    macros (s, 0.40f, 0.40f, 0.50f, 0.50f);

    Routings r;
    r.uni (ModSource::Env2,     Param::evolveMagnet,  0.500f)
     .uni (ModSource::Env2,     Param::shapeDensity,  0.180f)
     .bi  (ModSource::LFO1,     Param::dustColor,     0.120f)
     .bi  (ModSource::Chaos2,   Param::dustDensity,   0.140f)
     .uni (ModSource::Velocity, Param::dustDensity,   0.200f)
     .bi  (ModSource::KeyTrack, Param::dustColor,     0.160f)
     .uni (ModSource::Macro1,   Param::evolveMotion,  0.360f)
     .uni (ModSource::Macro1,   Param::chaos2Rate,    0.020f)
     .uni (ModSource::Macro2,   Param::dustColor,     0.300f)
     .uni (ModSource::Macro2,   Param::spaceTone,     0.240f)
     .uni (ModSource::Macro3,   Param::spaceMix,      0.280f)
     .uni (ModSource::Macro3,   Param::spaceSize,     0.200f)
     .uni (ModSource::Macro4,   Param::evolveMagnet,  0.400f);
    sharedMacros (r, Param::spaceReverbDecay, Param::chaos2Depth);
    r.commit (s);
}});

//--------------------------------------------------------------------------
// LAYER — brown dust blowing across a corrugated iron sheet that something
// keeps knocking, a long way off. The knocks arrive about twice a second and
// the wind never stops, so the pad has a foreground and a background at once.
manager.addFactory ({ "Iron Weather", "PAD", { "metallic", "cold", "noisy", "distant", "drift" }, [] (PatchState& s)
{
    impact (s, 3 /* NOISE STRIKE */, 0.40f, 0.34f, 0.46f, 0.50f, 0.55f, 0.60f, 0.22f, 0.55f);
    dust (s, 2 /* BROWN */, 0.72f, 0.26f, 0.34f, 0.36f, 0.72f, 0.85f, 733, 0.70f);
    set (s, Param::sourceMode, 1 /* LAYER */);
    set (s, Param::waveLevel, 0.0f);
    set (s, Param::sampleLevel, 0.0f);
    set (s, Param::gestureLevel, 0.0f);
    amp (s, 0.65f, 2.40f, 0.85f, 3.60f, 0.55f);
    set (s, Param::masterGain, -1.5f);   // two layered sources sum: give the knocks room
    shape (s, 0.52f, 0.56f, 0.48f, 0.60f, 0.74f, 0.30f);
    material (s, MaterialType::Metal, MaterialType::Void, 0.46f);
    topology (s, 4 /* RANDOM */, 0.44f, 0.36f, 811);
    matter (s, 0.84f, 0.44f, 0.30f, 0.86f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.52f, 0.24f, 0.0f, 0.18f, 0.36f);
    space (s, SpacePresets::Machine, 0.44f, 0.62f, 0.38f, 0.30f);
    set (s, Param::spaceDistDrive, 0.18f);

    lfo (s, 1, 0.09f, 5 /* SMOOTH RANDOM */, 1.0f, false, 2.0f);
    lfo (s, 2, 0.15f, 1 /* TRIANGLE */, 1.0f, false, 1.0f, 0.30f);
    env (s, 2, 3.20f, 5.0f, 0.80f, 4.0f, 0.6f);
    chaos (s, 1, 1 /* BROWNIAN */, 0.18f, 0.60f, 0.66f, 0.5f, 421);
    macros (s, 0.45f, 0.35f, 0.50f, 0.40f);

    Routings r;
    r.bi  (ModSource::LFO1,       Param::dustColor,       0.150f)
     .bi  (ModSource::LFO2,       Param::impactRate,      0.080f)
     .uni (ModSource::Env2,       Param::dustDensity,     0.200f)
     .bi  (ModSource::Chaos1,     Param::impactHardness,  0.180f)
     .uni (ModSource::Velocity,   Param::impactBrightness, 0.240f)
     .uni (ModSource::NoteRandom, Param::impactRandom,    0.200f)
     .uni (ModSource::Macro1,     Param::evolveMotion,    0.360f)
     .uni (ModSource::Macro1,     Param::impactRate,      0.180f)
     .uni (ModSource::Macro2,     Param::dustColor,       0.300f)
     .uni (ModSource::Macro2,     Param::shapeExcite,     0.240f)
     .uni (ModSource::Macro3,     Param::spaceMix,        0.300f)
     .uni (ModSource::Macro3,     Param::spaceSize,       0.220f)
     .uni (ModSource::Macro4,     Param::spaceDistDrive,  0.260f)
     .uni (ModSource::Macro4,     Param::shapeSurface,    0.220f);
    sharedMacros (r, Param::spaceReverbDecay, Param::impactRandom);
    r.commit (s);
}});

//--------------------------------------------------------------------------
// LAYER — a looped breath poured into a cloud of dust and both fed to the
// same membrane lattice, so the air in the pipes and the air in the room are
// the same air. The organ never quite speaks; the fog keeps eating the edges.
manager.addFactory ({ "Fog Organ", "PAD", { "hollow", "vocal", "breathing", "huge", "chords" }, [] (PatchState& s)
{
    sample (s, BuiltInSamples::Kind::Breath, 1 /* LOOP */, 0.12f, 0.86f, 0.30f, 0.48f, 60, 0.55f);
    dust (s, 7 /* CLOUD */, 0.62f, 0.30f, 0.44f, 0.34f, 0.70f, 0.80f, 353, 0.88f);
    set (s, Param::sourceMode, 1 /* LAYER */);
    set (s, Param::waveLevel, 0.0f);
    set (s, Param::impactLevel, 0.0f);
    set (s, Param::gestureLevel, 0.0f);
    amp (s, 1.10f, 2.60f, 0.90f, 4.00f, 0.6f);
    set (s, Param::masterGain, 5.5f);
    shape (s, 0.46f, 0.26f, 0.50f, 0.40f, 0.78f, 0.18f);
    material (s, MaterialType::Membrane, MaterialType::Liquid, 0.50f);
    topology (s, 3 /* LATTICE */, 0.50f, 0.44f, 907);
    matter (s, 0.90f, 0.52f, 0.10f, 0.88f);
    evolve (s, 0.0f, 0.20f, 0.0f, 0.0f, 0.40f, 0.18f, 0.0f, 0.12f, 0.38f);
    space (s, SpacePresets::Nebula, 0.56f, 0.78f, 0.46f, 0.36f);
    set (s, Param::spaceCompOn, 1.0f);
    set (s, Param::spaceCompAmount, 0.42f);   // fog is even; the loop's own gusts are not

    lfo (s, 1, 0.06f, 0 /* SINE */, 1.0f, false, 2.5f);
    lfo (s, 2, 0.11f, 5 /* SMOOTH RANDOM */, 1.0f, false, 1.5f);
    env (s, 2, 4.00f, 6.0f, 0.88f, 5.0f, 0.65f);
    macros (s, 0.40f, 0.30f, 0.55f, 0.40f);

    Routings r;
    r.bi  (ModSource::LFO1,     Param::sampleStart,   0.060f)
     .bi  (ModSource::LFO2,     Param::dustGrain,     0.160f)
     .uni (ModSource::Env2,     Param::shapeDensity,  0.260f)
     .uni (ModSource::Env2,     Param::dustDensity,   0.180f)
     .uni (ModSource::Velocity, Param::sampleGrain,   0.220f)
     .bi  (ModSource::KeyTrack, Param::dustColor,     0.200f)
     .uni (ModSource::Macro1,   Param::evolveMotion,  0.380f)
     .uni (ModSource::Macro1,   Param::dustJitter,    0.260f)
     .uni (ModSource::Macro2,   Param::dustColor,     0.320f)
     .uni (ModSource::Macro2,   Param::shapeExcite,   0.240f)
     .uni (ModSource::Macro3,   Param::spaceMix,      0.280f)
     .uni (ModSource::Macro3,   Param::spaceSize,     0.200f)
     .uni (ModSource::Macro4,   Param::sampleLevel,   0.300f)
     .uni (ModSource::Macro4,   Param::evolveMelt,    0.220f);
    sharedMacros (r, Param::spaceReverbDecay, Param::dustJitter);
    r.commit (s);
}});

//--------------------------------------------------------------------------
// LAYER — a folded wave rubbed against a dry friction gesture on the same
// organic-metal cluster: embers under a grate. The wave gives it a pitch to
// hold, the friction gives it something to keep doing while it is held.
manager.addFactory ({ "Ember Field", "PAD", { "warm", "dirty", "scraped", "roomy", "drift" }, [] (PatchState& s)
{
    gesture (s, 4 /* FRICTION */, 0.40f, 0.34f, 0.44f, 0.38f, 0.34f, 0.42f, 0.55f);
    wave (s, 3 /* FOLDED */, 0.30f, 0.36f, 0.10f, 3, 0.20f, 0.68f, 0, 0.72f);
    set (s, Param::sourceMode, 1 /* LAYER */);
    set (s, Param::dustLevel, 0.0f);
    set (s, Param::impactLevel, 0.0f);
    set (s, Param::sampleLevel, 0.0f);
    amp (s, 0.50f, 2.00f, 0.82f, 2.80f, 0.5f);
    shape (s, 0.50f, 0.30f, 0.52f, 0.46f, 0.68f, 0.36f);
    material (s, MaterialType::Organic, MaterialType::Metal, 0.34f);
    topology (s, 2 /* CLUSTERS */, 0.48f, 0.42f, 149);
    matter (s, 0.78f, 0.50f, 0.18f, 0.72f);
    evolve (s, 0.0f, 0.0f, 0.18f, 0.0f, 0.48f, 0.22f, 0.0f, 0.26f, 0.34f);
    set (s, Param::evolveScatterSeed, 1231);
    space (s, SpacePresets::Chamber, 0.36f, 0.52f, 0.50f, 0.28f);

    lfo (s, 1, 0.19f, 5 /* SMOOTH RANDOM */, 1.0f, false, 0.8f);
    lfo (s, 2, 0.31f, 1 /* TRIANGLE */, 1.0f, false, 0.0f, 0.40f);
    env (s, 2, 1.80f, 3.5f, 0.66f, 3.0f, 0.5f);
    chaos (s, 1, 2 /* LOGISTIC */, 0.70f, 0.35f, 0.85f, 0.5f, 577);
    macros (s, 0.45f, 0.40f, 0.35f, 0.45f);

    Routings r;
    r.bi  (ModSource::LFO1,     Param::gestureSpeed,     0.150f)
     .bi  (ModSource::LFO2,     Param::waveMorph,        0.100f)
     .uni (ModSource::Env2,     Param::gesturePressure,  0.200f)
     .bi  (ModSource::Chaos1,   Param::gestureRoughness, 0.140f)
     .uni (ModSource::Velocity, Param::gesturePressure,  0.260f)
     .bi  (ModSource::KeyTrack, Param::waveLevel,       -0.180f)
     .uni (ModSource::Macro1,   Param::gestureMotion,    0.380f)
     .uni (ModSource::Macro1,   Param::evolveMotion,     0.260f)
     .uni (ModSource::Macro2,   Param::wavePosition,     0.300f)
     .uni (ModSource::Macro2,   Param::gestureBandwidth, 0.260f)
     .uni (ModSource::Macro3,   Param::spaceMix,         0.300f)
     .uni (ModSource::Macro4,   Param::gestureRoughness, 0.340f)
     .uni (ModSource::Macro4,   Param::evolveTear,       0.220f);
    sharedMacros (r, Param::spaceReverbDecay, Param::chaos1Depth);
    r.commit (s);
}});

//--------------------------------------------------------------------------
// FRACTURE, rhythmic — a thin metal plate that will not stand still. The
// sequencer chops the ring into sixteenths, alternating the low half of the
// spectrum against the high one, so a held chord shivers instead of sustains.
manager.addFactory ({ "Shiver Plate", "PAD", { "metallic", "bright", "rhythmic", "close", "layer" }, [] (PatchState& s)
{
    wave (s, 4 /* METALLIC */, 0.36f, 0.26f, 0.14f, 3, 0.14f, 0.62f, 0, 0.85f);
    amp (s, 0.35f, 1.80f, 0.80f, 2.20f, 0.45f);
    set (s, Param::masterGain, 3.0f);
    shape (s, 0.60f, 0.64f, 0.34f, 0.62f, 0.64f, 0.20f);
    material (s, MaterialType::Metal, MaterialType::Crystal, 0.55f);
    topology (s, 1 /* RING */, 0.52f, 0.56f, 601);
    matter (s, 0.86f, 0.54f, 0.22f, 0.74f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.42f, 0.16f, 0.0f, 0.34f, 0.28f);
    space (s, SpacePresets::Orbit, 0.34f, 0.44f, 0.58f, 0.26f);

    FractureShape f;
    f.fragments = 32;
    f.delayLow = 0.04f; f.delayHigh = 0.28f;
    f.feedbackLow = 0.20f; f.feedbackHigh = 0.42f;
    f.decayLow = 0.35f; f.decayHigh = 0.58f;
    f.spreadLow = 0.30f; f.spreadHigh = 0.95f;
    f.gainLow = 0.95f; f.gainHigh = 0.70f;
    f.panWidth = 0.80f;
    f.probability = 0.85f;
    f.pattern = "XLoHXHoL";
    fracture (s, 1 /* RHYTHMIC */, 0.70f, 0.55f, 0.72f, 0.80f, 0.30f, 0.22f, 0.48f, 0.60f, 0.15f,
              2 /* 32 */, 4 /* 1/16 */, 8, 0.18f, 2 /* PINGPONG */, 0.85f, 0.30f, 89, f);

    lfo (s, 1, 0.24f, 1 /* TRIANGLE */, 1.0f, false, 0.6f);
    env (s, 2, 1.40f, 3.0f, 0.62f, 2.5f, 0.5f);
    macros (s, 0.50f, 0.45f, 0.30f, 0.45f);

    Routings r;
    r.bi  (ModSource::LFO1,     Param::fractureTone,     0.140f)
     .uni (ModSource::Env2,     Param::fractureSpread,   0.220f)
     .uni (ModSource::Velocity, Param::fractureAmount,   0.240f)
     .bi  (ModSource::KeyTrack, Param::fractureDelay,   -0.200f)
     .uni (ModSource::NoteRandom, Param::fractureSwing,  0.180f)
     .uni (ModSource::Macro1,   Param::fractureSwing,    0.260f)
     .uni (ModSource::Macro1,   Param::evolveMotion,     0.240f)
     .uni (ModSource::Macro2,   Param::fractureTone,     0.320f)
     .uni (ModSource::Macro2,   Param::wavePosition,     0.260f)
     .uni (ModSource::Macro3,   Param::spaceMix,         0.320f)
     .uni (ModSource::Macro3,   Param::spaceSize,        0.220f)
     .uni (ModSource::Macro4,   Param::fractureProbability, -0.300f)
     .uni (ModSource::Macro4,   Param::fractureFeedback, 0.240f);
    sharedMacros (r, Param::fractureDecay, Param::fractureRandom);
    r.commit (s);
}});

//--------------------------------------------------------------------------
// FRACTURE, spectral — the object is smeared downwards. A falling pitch
// terrace drops the upper bands by octaves and fifths into the cellar, with
// long delays low in the spectrum, so the chord grows a shadow underneath it.
manager.addFactory ({ "Spectral Shroud", "PAD", { "dark", "cold", "evolving", "huge", "low" }, [] (PatchState& s)
{
    wave (s, 5 /* SPECTRAL */, 0.44f, 0.30f, 0.08f, 4, 0.22f, 0.88f, 0, 0.85f);
    amp (s, 0.95f, 2.40f, 0.86f, 3.80f, 0.6f);
    set (s, Param::masterGain, -2.5f);   // the falling terrace piles energy into the cellar low down
    shape (s, 0.66f, 0.44f, 0.62f, 0.44f, 0.80f, 0.18f);
    material (s, MaterialType::Void, MaterialType::Crystal, 0.34f);
    topology (s, 2 /* CLUSTERS */, 0.42f, 0.62f, 1187);
    matter (s, 0.88f, 0.42f, 0.12f, 0.84f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.20f, 0.60f, 0.12f, 0.0f, 0.10f, 0.30f);
    set (s, Param::evolveMagnetTarget, 0 /* OCTAVE */);
    space (s, SpacePresets::Void, 0.50f, 0.80f, 0.34f, 0.34f);

    FractureShape f;
    f.fragments = 16;
    f.delayLow = 0.45f; f.delayHigh = 0.12f;
    f.feedbackLow = 0.40f; f.feedbackHigh = 0.12f;
    f.decayLow = 0.72f; f.decayHigh = 0.42f;
    f.spreadLow = 0.15f; f.spreadHigh = 0.80f;
    f.gainLow = 0.62f; f.gainHigh = 0.55f;
    f.panWidth = 0.55f;
    f.pitchCycle = kFallingTerrace;
    f.pattern = "XXLXXHXL";
    fracture (s, 0 /* SPECTRAL */, 0.62f, 0.50f, 0.45f, 0.25f, 0.32f, 0.58f, 0.70f, 0.35f, 0.25f,
              1 /* 16 */, 1 /* 1/2 */, 8, 0.0f, 0 /* FORWARD */, 1.0f, 0.10f, 1483, f);

    lfo (s, 1, 0.05f, 0 /* SINE */, 1.0f, false, 3.0f);
    env (s, 2, 5.00f, 7.0f, 0.90f, 5.0f, 0.65f);
    chaos (s, 1, 0 /* WALK */, 0.09f, 0.50f, 0.78f, 0.5f, 1601);
    macros (s, 0.35f, 0.25f, 0.60f, 0.45f);

    Routings r;
    r.uni (ModSource::Env2,     Param::fractureMix,      0.240f)
     .uni (ModSource::Env2,     Param::fractureFeedback, 0.150f)
     .bi  (ModSource::LFO1,     Param::fracturePitch,    0.030f)
     .bi  (ModSource::Chaos1,   Param::wavePosition,     0.100f)
     .uni (ModSource::Velocity, Param::shapeExcite,      0.200f)
     .bi  (ModSource::KeyTrack, Param::fractureTone,     0.220f)
     .uni (ModSource::Macro1,   Param::evolveMotion,     0.320f)
     .uni (ModSource::Macro1,   Param::fractureEvolve,   0.260f)
     .uni (ModSource::Macro2,   Param::fractureTone,     0.300f)
     .uni (ModSource::Macro2,   Param::spaceTone,        0.240f)
     .uni (ModSource::Macro3,   Param::spaceMix,         0.280f)
     .uni (ModSource::Macro3,   Param::spaceSize,        0.180f)
     .uni (ModSource::Macro4,   Param::fractureMix,      0.300f)
     .uni (ModSource::Macro4,   Param::evolveMagnet,     0.260f);
    sharedMacros (r, Param::fractureDecay, Param::fractureProbability);
    r.commit (s);
}});

//--------------------------------------------------------------------------
// FRACTURE, transient — bells cut from paper. A soft pluck barely excites a
// crystal-and-wood star, and the transient fracture catches only the attacks
// and throws them out sparsely across the stereo field. Air, not weight.
manager.addFactory ({ "Paper Bells", "PAD", { "glassy", "soft", "plucked", "wide", "air" }, [] (PatchState& s)
{
    impact (s, 2 /* PLUCK */, 0.22f, 0.52f, 0.30f, 0.44f, 0.60f, 0.30f, 0.20f);
    amp (s, 0.20f, 1.60f, 0.72f, 2.60f, 0.45f);
    set (s, Param::masterGain, 3.0f);
    shape (s, 0.44f, 0.72f, 0.22f, 0.70f, 0.72f, 0.10f);
    material (s, MaterialType::Crystal, MaterialType::Wood, 0.30f);
    topology (s, 5 /* STAR */, 0.34f, 0.66f, 293);
    matter (s, 0.92f, 0.62f, 0.34f, 0.90f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.34f, 0.26f, 0.0f, 0.22f, 0.40f);
    set (s, Param::evolveScatterSeed, 787);
    space (s, SpacePresets::Shimmer, 0.46f, 0.60f, 0.66f, 0.30f);

    FractureShape f;
    f.fragments = 16;
    f.delayLow = 0.08f; f.delayHigh = 0.60f;
    f.feedbackLow = 0.05f; f.feedbackHigh = 0.22f;
    f.decayLow = 0.30f; f.decayHigh = 0.55f;
    f.spreadLow = 0.40f; f.spreadHigh = 1.00f;
    f.gainLow = 0.70f; f.gainHigh = 0.55f;
    f.panWidth = 0.95f;
    f.probability = 0.55f;
    f.pitchCycle = kFifthTerrace;
    f.pattern = "X.oH.LoX";
    fracture (s, 2 /* TRANSIENT */, 0.60f, 0.42f, 0.85f, 0.55f, 0.14f, 0.36f, 0.44f, 0.66f, 0.20f,
              1 /* 16 */, 6 /* 1/4T */, 8, 0.0f, 3 /* RANDOM */, 0.55f, 0.40f, 953, f);

    lfo (s, 1, 0.17f, 5 /* SMOOTH RANDOM */, 1.0f, false, 1.0f);
    env (s, 2, 2.00f, 4.0f, 0.60f, 3.0f, 0.55f);
    macros (s, 0.45f, 0.50f, 0.45f, 0.40f);

    Routings r;
    r.bi  (ModSource::LFO1,       Param::fractureSpread,  0.160f)
     .uni (ModSource::Env2,       Param::fractureProbability, 0.220f)
     .uni (ModSource::Velocity,   Param::impactHardness,  0.280f)
     .uni (ModSource::Velocity,   Param::shapeStrike,     0.200f)
     .uni (ModSource::NoteRandom, Param::fractureDelay,   0.180f)
     .bi  (ModSource::KeyTrack,   Param::impactLength,   -0.220f)
     .uni (ModSource::Macro1,     Param::impactRate,      0.240f)
     .uni (ModSource::Macro1,     Param::evolveMotion,    0.280f)
     .uni (ModSource::Macro2,     Param::impactBrightness, 0.320f)
     .uni (ModSource::Macro2,     Param::fractureTone,    0.240f)
     .uni (ModSource::Macro3,     Param::spaceMix,        0.300f)
     .uni (ModSource::Macro3,     Param::spaceSize,       0.220f)
     .uni (ModSource::Macro4,     Param::fractureProbability, 0.280f)
     .uni (ModSource::Macro4,     Param::shapeCoupling,   0.220f);
    sharedMacros (r, Param::fractureDecay, Param::fractureRandom);
    r.commit (s);
}});

//--------------------------------------------------------------------------
// TEAR is the patch — a field of wires where the quiet ones have become
// detuned twins of the loud ones. Everything beats against its own copy, and
// an envelope pulls the tearing wider through the note without retriggering.
manager.addFactory ({ "Wire Meadow", "PAD", { "metallic", "unstable", "bowed", "wide", "drift" }, [] (PatchState& s)
{
    gesture (s, 1 /* SCRAPE */, 0.42f, 0.28f, 0.22f, 0.46f, 0.28f, 0.36f);
    amp (s, 0.70f, 2.20f, 0.84f, 3.00f, 0.5f);
    set (s, Param::masterGain, 4.0f);
    shape (s, 0.64f, 0.20f, 0.36f, 0.72f, 0.76f, 0.24f);
    material (s, MaterialType::String, MaterialType::Metal, 0.42f);
    topology (s, 3 /* LATTICE */, 0.56f, 0.58f, 1319);
    matter (s, 0.90f, 0.48f, 0.10f, 0.88f);
    evolve (s, 0.0f, 0.0f, 0.34f, 0.0f, 0.46f, 0.20f, 0.0f, 0.14f, 0.44f);
    set (s, Param::evolveScatterSeed, 1993);
    space (s, SpacePresets::Dream, 0.46f, 0.64f, 0.54f, 0.32f);

    lfo (s, 1, 0.08f, 0 /* SINE */, 1.0f, false, 2.0f);
    lfo (s, 2, 0.13f, 5 /* SMOOTH RANDOM */, 1.0f, false, 1.0f);
    env (s, 2, 3.40f, 6.0f, 0.88f, 4.5f, 0.6f);
    macros (s, 0.45f, 0.35f, 0.45f, 0.50f);

    Routings r;
    r.uni (ModSource::Env2,     Param::evolveTear,       0.340f)
     .bi  (ModSource::LFO1,     Param::shapeTension,     0.060f)
     .bi  (ModSource::LFO2,     Param::gesturePosition,  0.120f)
     .uni (ModSource::Velocity, Param::gesturePressure,  0.260f)
     .bi  (ModSource::KeyTrack, Param::evolveTear,      -0.180f)
     .uni (ModSource::NoteRandom, Param::evolveScatter,  0.160f)
     .uni (ModSource::Macro1,   Param::evolveMotion,     0.400f)
     .uni (ModSource::Macro1,   Param::gestureMotion,    0.240f)
     .uni (ModSource::Macro2,   Param::gestureBandwidth, 0.300f)
     .uni (ModSource::Macro2,   Param::shapeExcite,      0.240f)
     .uni (ModSource::Macro3,   Param::spaceMix,         0.300f)
     .uni (ModSource::Macro3,   Param::spaceSize,        0.200f)
     .uni (ModSource::Macro4,   Param::evolveTear,       0.360f)
     .uni (ModSource::Macro4,   Param::shapeTension,     0.180f);
    sharedMacros (r, Param::spaceReverbDecay, Param::gestureRoughness);
    r.commit (s);
}});

//--------------------------------------------------------------------------
// A wax cylinder of a cathedral organ, played back through the cathedral. The
// vinyl dust loop is the excitation; FRACTURE in EVOLVE mode keeps rewriting
// which bands survive, so the recording decays differently every bar.
manager.addFactory ({ "Wax Cathedral", "PAD", { "dirty", "warm", "granular", "huge", "drift" }, [] (PatchState& s)
{
    sample (s, BuiltInSamples::Kind::VinylDust, 1 /* LOOP */, 0.04f, 0.94f, 0.34f, 0.50f, 60, 0.92f);
    amp (s, 0.80f, 2.40f, 0.88f, 3.60f, 0.55f);
    set (s, Param::masterGain, 4.5f);
    shape (s, 0.56f, 0.18f, 0.60f, 0.38f, 0.76f, 0.32f);
    material (s, MaterialType::Organic, MaterialType::Void, 0.40f);
    topology (s, 0 /* CHAIN */, 0.40f, 0.48f, 617);
    matter (s, 0.84f, 0.46f, 0.14f, 0.78f);
    evolve (s, 0.0f, 0.16f, 0.0f, 0.24f, 0.46f, 0.20f, 0.0f, 0.16f, 0.36f);
    set (s, Param::evolveMagnetTarget, 3 /* MINOR */);
    space (s, SpacePresets::Dust, 0.52f, 0.72f, 0.42f, 0.34f);

    FractureShape f;
    f.fragments = 16;
    f.delayLow = 0.20f; f.delayHigh = 0.66f;
    f.feedbackLow = 0.25f; f.feedbackHigh = 0.45f;
    f.decayLow = 0.60f; f.decayHigh = 0.74f;
    f.spreadLow = 0.25f; f.spreadHigh = 0.85f;
    f.gainLow = 0.85f; f.gainHigh = 0.50f;
    f.panWidth = 0.60f;
    f.pitchCycle = kMinorTerrace;
    f.pattern = "XLXH.XLo";
    fracture (s, 3 /* EVOLVE */, 0.58f, 0.45f, 0.55f, 0.60f, 0.35f, 0.50f, 0.68f, 0.42f, 0.62f,
              1 /* 16 */, 2 /* 1/4 */, 8, 0.10f, 2 /* PINGPONG */, 0.90f, 0.35f, 2207, f);

    lfo (s, 1, 0.07f, 5 /* SMOOTH RANDOM */, 1.0f, false, 2.0f);
    env (s, 2, 3.00f, 6.0f, 0.82f, 4.0f, 0.6f);
    chaos (s, 1, 3 /* LORENZ */, 0.14f, 0.45f, 0.70f, 0.5f, 3121);
    macros (s, 0.50f, 0.30f, 0.55f, 0.45f);

    Routings r;
    r.bi  (ModSource::LFO1,     Param::sampleStart,     0.070f)
     .uni (ModSource::Env2,     Param::fractureEvolve,  0.280f)
     .bi  (ModSource::Chaos1,   Param::fractureTone,    0.160f)
     .uni (ModSource::Velocity, Param::sampleGrain,     0.240f)
     .bi  (ModSource::KeyTrack, Param::sampleStart,     0.140f)
     .uni (ModSource::Macro1,   Param::fractureEvolve,  0.300f)
     .uni (ModSource::Macro1,   Param::evolveMotion,    0.280f)
     .uni (ModSource::Macro2,   Param::fractureTone,    0.300f)
     .uni (ModSource::Macro2,   Param::shapeExcite,     0.240f)
     .uni (ModSource::Macro3,   Param::spaceMix,        0.300f)
     .uni (ModSource::Macro3,   Param::spaceSize,       0.220f)
     .uni (ModSource::Macro4,   Param::evolveMagnet,    0.320f)
     .uni (ModSource::Macro4,   Param::sampleGrain,     0.240f);
    sharedMacros (r, Param::fractureDecay, Param::fractureRandom);
    r.commit (s);
}});
}

} // namespace am::FactoryContent
