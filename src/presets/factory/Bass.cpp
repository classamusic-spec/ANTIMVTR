#include "presets/FactoryBuilders.h"

namespace am::FactoryContent
{

void registerBass (PresetManager& manager)
{
//==========================================================================
// BASS — weight that still belongs to this instrument
//==========================================================================

manager.addFactory ({ "Carbon Bass", "BASS", { "bass", "organic", "mono", "dark" }, [] (PatchState& s)
{
    wave (s, 3 /* FOLDED */, 0.22f, 0.18f, 0.0f, 2, 0.08f, 0.30f, -1);
    set (s, Param::waveRing, 0.12f);
    set (s, Param::waveModRatio, 1.5f);
    amp (s, 0.003f, 0.45f, 0.68f, 0.20f, 0.35f);
    shape (s, 0.26f, 0.05f, 0.80f, 0.34f, 0.30f, 0.42f);
    material (s, MaterialType::Wood, MaterialType::Organic, 0.35f);
    topology (s, 0 /* CHAIN */, 0.30f, 0.35f, 53);
    matter (s, 0.82f, 0.45f, 0.45f, 0.35f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.30f, 0.62f, 0.0f, 0.0f, 0.30f, 0.10f);
    set (s, Param::evolveMagnetTarget, 0 /* OCTAVE */);
    set (s, Param::masterMode, 1 /* MONO */);
    set (s, Param::masterGlide, 0.06f);
    space (s, SpacePresets::Chamber, 0.16f, 0.30f, 0.45f, 0.20f);

    env (s, 1, 0.001f, 0.28f, 0.0f, 0.20f, 0.3f);
    lfo (s, 1, 4.80f, 0 /* SINE */, 1.0f, true, 0.35f);
    macros (s, 0.20f, 0.35f, 0.20f, 0.40f);

    Routings r;
    r.uni (ModSource::Env1,   Param::shapeSurface,   0.260f)
     .uni (ModSource::Env1,   Param::waveRing,       0.180f)
     .bi  (ModSource::LFO1,   Param::shapePitch,     0.006f)
     .uni (ModSource::Velocity, Param::shapeStrike,  0.300f)
     .bi  (ModSource::KeyTrack, Param::shapeMass,   -0.150f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.300f)
     .uni (ModSource::Macro2, Param::shapeExcite,    0.350f)
     .uni (ModSource::Macro2, Param::waveMorph,      0.250f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.250f)
     .uni (ModSource::Macro4, Param::waveRing,       0.400f)
     .uni (ModSource::Macro4, Param::shapeSurface,   0.200f);
    sharedMacros (r, Param::ampDecay, Param::evolveTear);
    r.commit (s);
}});

manager.addFactory ({ "Torn Bass", "BASS", { "bass", "aggressive", "distorted", "tear" }, [] (PatchState& s)
{
    wave (s, 6 /* FRACTURED */, 0.48f, 0.30f, 0.05f, 3, 0.14f, 0.45f, -1);
    amp (s, 0.002f, 0.35f, 0.72f, 0.24f, 0.3f);
    set (s, Param::masterGain, -3.0f);   // struck material has a hot transient: keep headroom at the bottom of the keyboard
    shape (s, 0.42f, 0.52f, 0.72f, 0.40f, 0.34f, 0.52f);
    material (s, MaterialType::Metal, MaterialType::Wood, 0.55f);
    topology (s, 4 /* RANDOM */, 0.55f, 0.40f, 89);
    matter (s, 0.88f, 0.55f, 0.58f, 0.45f);
    evolve (s, 0.0f, 0.0f, 0.42f, 0.24f, 0.58f, 0.20f, 0.16f, 0.42f, 0.30f);
    set (s, Param::evolveScatterSeed, 617);
    set (s, Param::masterMode, 1 /* MONO */);
    set (s, Param::masterGlide, 0.03f);
    space (s, SpacePresets::Machine, 0.22f, 0.28f, 0.42f, 0.30f);
    set (s, Param::spaceDistDrive, 0.42f);

    env (s, 1, 0.001f, 0.22f, 0.10f, 0.18f, 0.25f);
    lfo (s, 1, 7.20f, 3 /* SQUARE */, 1.0f, true, 0.15f);
    chaos (s, 1, 2 /* LOGISTIC */, 3.40f, 0.45f, 0.80f, 0.5f, 733);
    macros (s, 0.30f, 0.45f, 0.20f, 0.45f);

    Routings r;
    r.uni (ModSource::Env1,   Param::evolveTear,     0.300f)
     .uni (ModSource::Env1,   Param::shapeSurface,   0.220f)
     .bi  (ModSource::LFO1,   Param::evolveCrush,    0.080f)
     .bi  (ModSource::Chaos1, Param::evolveScatter,  0.100f)
     .uni (ModSource::Velocity, Param::spaceDistDrive, 0.220f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.350f)
     .uni (ModSource::Macro2, Param::shapeExcite,    0.300f)
     .uni (ModSource::Macro2, Param::spaceTone,      0.250f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.250f)
     .uni (ModSource::Macro4, Param::evolveTear,     0.400f)
     .uni (ModSource::Macro4, Param::evolveCrush,    0.200f);
    sharedMacros (r, Param::ampDecay, Param::chaos1Depth);
    r.commit (s);
}});

manager.addFactory ({ "Rust Sub", "BASS", { "bass", "sub", "gritty", "gesture" }, [] (PatchState& s)
{
    gesture (s, 4 /* FRICTION */, 0.58f, 0.26f, 0.28f, 0.35f, 0.18f, 0.22f);
    amp (s, 0.02f, 0.60f, 0.80f, 0.35f, 0.4f);
    shape (s, 0.22f, 0.02f, 0.86f, 0.30f, 0.28f, 0.36f);
    material (s, MaterialType::Organic, MaterialType::Membrane, 0.30f);
    topology (s, 0 /* CHAIN */, 0.26f, 0.30f, 167);
    matter (s, 0.92f, 0.30f, 0.25f, 0.30f);
    evolve (s, 0.0f, 0.12f, 0.0f, 0.36f, 0.66f, 0.0f, 0.0f, 0.22f, 0.15f);
    set (s, Param::evolveMagnetTarget, 0 /* OCTAVE */);
    set (s, Param::masterMode, 2 /* LEGATO */);
    set (s, Param::masterGlide, 0.12f);
    space (s, SpacePresets::Void, 0.14f, 0.45f, 0.30f, 0.25f);

    lfo (s, 1, 0.35f, 1 /* TRIANGLE */, 1.0f, true, 0.5f);
    env (s, 1, 0.05f, 0.80f, 0.40f, 0.60f);
    macros (s, 0.25f, 0.30f, 0.20f, 0.50f);

    Routings r;
    r.bi  (ModSource::LFO1,   Param::gesturePressure, 0.120f)
     .uni (ModSource::Env1,   Param::gestureSpeed,    0.250f)
     .uni (ModSource::Velocity, Param::gestureRoughness, 0.300f)
     .bi  (ModSource::KeyTrack, Param::gestureBandwidth, 0.180f)
     .uni (ModSource::Macro1, Param::gestureMotion,   0.400f)
     .uni (ModSource::Macro2, Param::gestureBandwidth, 0.350f)
     .uni (ModSource::Macro2, Param::shapeExcite,     0.250f)
     .uni (ModSource::Macro3, Param::spaceMix,        0.250f)
     .uni (ModSource::Macro4, Param::gestureRoughness, 0.400f)
     .uni (ModSource::Macro4, Param::shapeSurface,    0.250f);
    sharedMacros (r, Param::ampDecay, Param::gestureRoughness);
    r.commit (s);
}});

//--------------------------------------------------------------------------
// A steel cable pulled sideways and let go. The chain topology passes the
// energy down the run, and a quiet spectral FRACTURE puts an octave-up ghost
// on top of the fundamental — which is what keeps the note on a phone speaker.
manager.addFactory ({ "Cable Pull", "BASS", { "metallic", "plucked", "dirty", "close", "low" }, [] (PatchState& s)
{
    impact (s, 2 /* PLUCK */, 0.62f, 0.58f, 0.30f, 0.72f, 0.40f, 0.10f);
    amp (s, 0.002f, 0.75f, 0.74f, 0.24f, 0.35f);
    set (s, Param::masterGain, -2.0f);
    shape (s, 0.34f, 0.16f, 0.74f, 0.52f, 0.50f, 0.34f);
    material (s, MaterialType::String, MaterialType::Metal, 0.38f);
    topology (s, 0 /* CHAIN */, 0.34f, 0.42f, 61);
    matter (s, 0.88f, 0.64f, 0.62f, 0.30f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.26f, 0.56f, 0.0f, 0.0f, 0.20f, 0.10f);
    set (s, Param::evolveMagnetTarget, 0 /* OCTAVE */);
    set (s, Param::masterMode, 1 /* MONO */);
    set (s, Param::masterGlide, 0.04f);
    space (s, SpacePresets::Chamber, 0.14f, 0.26f, 0.48f, 0.18f);

    FractureShape f;
    f.fragments = 8;
    f.delayLow = 0.02f; f.delayHigh = 0.12f;
    f.feedbackLow = 0.10f; f.feedbackHigh = 0.25f;
    f.decayLow = 0.30f; f.decayHigh = 0.45f;
    f.spreadLow = 0.10f; f.spreadHigh = 0.45f;
    f.gainLow = 0.35f; f.gainHigh = 1.15f;
    f.panWidth = 0.25f;
    f.pitchCycle = kOctaveTerrace;
    f.pattern = "XHXHXHXH";
    fracture (s, 0 /* SPECTRAL */, 0.52f, 0.44f, 0.30f, 0.20f, 0.15f, 0.16f, 0.38f, 0.70f, 0.10f,
              0 /* 8 */, 4 /* 1/16 */, 8, 0.0f, 0 /* FORWARD */, 1.0f, 0.08f, 137, f);

    env (s, 1, 0.001f, 0.30f, 0.05f, 0.20f, 0.3f);
    lfo (s, 1, 5.50f, 0 /* SINE */, 1.0f, true, 0.20f);
    macros (s, 0.20f, 0.40f, 0.20f, 0.45f);

    Routings r;
    r.uni (ModSource::Env1,       Param::shapeSurface,    0.280f)
     .uni (ModSource::Env1,       Param::fractureMix,     0.180f)
     .bi  (ModSource::LFO1,       Param::shapeTension,    0.030f)
     .uni (ModSource::Velocity,   Param::shapeStrike,     0.320f)
     .uni (ModSource::Velocity,   Param::impactBrightness, 0.240f)
     .bi  (ModSource::KeyTrack,   Param::shapeMass,      -0.180f)
     .uni (ModSource::NoteRandom, Param::impactRandom,    0.140f)
     .uni (ModSource::Macro1,     Param::evolveMotion,    0.300f)
     .uni (ModSource::Macro1,     Param::lfo1Rate,        0.100f)
     .uni (ModSource::Macro2,     Param::impactBrightness, 0.340f)
     .uni (ModSource::Macro2,     Param::fractureMix,     0.220f)
     .uni (ModSource::Macro3,     Param::spaceMix,        0.240f)
     .uni (ModSource::Macro4,     Param::shapeSurface,    0.320f)
     .uni (ModSource::Macro4,     Param::shapeTension,    0.200f);
    sharedMacros (r, Param::ampDecay, Param::impactRandom);
    r.commit (s);
}});

//--------------------------------------------------------------------------
// LAYER — a plank hit flat with the heel of a hand, and an octave-down wave
// sitting directly underneath it. The knock carries the pitch and the note's
// edge; the wave carries the weight. Neither works on its own down here.
manager.addFactory ({ "Plank Drop", "BASS", { "wooden", "struck", "dark", "sub", "impact" }, [] (PatchState& s)
{
    wave (s, 0 /* BASIC */, 0.12f, 0.10f, 0.0f, 1, 0.03f, 0.10f, -1, 0.45f);
    impact (s, 6 /* MEMBRANE HIT */, 0.60f, 0.52f, 0.34f, 0.70f, 0.45f, 0.14f, 0.0f, 0.95f);
    set (s, Param::sourceMode, 1 /* LAYER */);
    set (s, Param::dustLevel, 0.0f);
    set (s, Param::sampleLevel, 0.0f);
    set (s, Param::gestureLevel, 0.0f);
    amp (s, 0.003f, 0.80f, 0.70f, 0.28f, 0.30f);
    set (s, Param::masterGain, -4.5f);
    shape (s, 0.30f, 0.22f, 0.80f, 0.38f, 0.44f, 0.30f);
    material (s, MaterialType::Wood, MaterialType::Membrane, 0.42f);
    topology (s, 2 /* CLUSTERS */, 0.28f, 0.34f, 173);
    matter (s, 0.80f, 0.56f, 0.58f, 0.28f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.64f, 0.0f, 0.0f, 0.24f, 0.08f);
    set (s, Param::masterMode, 1 /* MONO */);
    set (s, Param::masterGlide, 0.02f);
    space (s, SpacePresets::Chamber, 0.12f, 0.22f, 0.40f, 0.16f);
    set (s, Param::spaceEqMid, 2.5f);   // the knock lives here; it is what carries the pitch on a laptop

    env (s, 1, 0.001f, 0.22f, 0.0f, 0.18f, 0.25f);
    env (s, 2, 0.004f, 0.45f, 0.30f, 0.30f, 0.4f);
    macros (s, 0.20f, 0.35f, 0.20f, 0.45f);

    Routings r;
    r.uni (ModSource::Env1,     Param::impactHardness,  0.300f)
     .uni (ModSource::Env1,     Param::shapeSurface,    0.220f)
     .uni (ModSource::Env2,     Param::waveLevel,       0.180f)
     .uni (ModSource::Velocity, Param::shapeStrike,     0.320f)
     .uni (ModSource::Velocity, Param::impactHardness,  0.240f)
     .bi  (ModSource::KeyTrack, Param::waveLevel,      -0.260f)
     .uni (ModSource::Macro1,   Param::evolveMotion,    0.280f)
     .uni (ModSource::Macro1,   Param::impactRandom,    0.200f)
     .uni (ModSource::Macro2,   Param::impactBrightness, 0.340f)
     .uni (ModSource::Macro2,   Param::shapeExcite,     0.240f)
     .uni (ModSource::Macro3,   Param::spaceMix,        0.240f)
     .uni (ModSource::Macro4,   Param::waveLevel,       0.300f)
     .uni (ModSource::Macro4,   Param::shapeMass,       0.200f);
    sharedMacros (r, Param::ampDecay, Param::impactRandom);
    r.commit (s);
}});

//--------------------------------------------------------------------------
// A drum skin with a finger dragged across it hard enough that the membrane
// stops behaving. Dry friction into a membrane cluster, then a tube stage in
// SPACE that the same pressure drives — lean on the key and it breaks up.
manager.addFactory ({ "Driven Skin", "BASS", { "harsh", "dirty", "scraped", "close", "low" }, [] (PatchState& s)
{
    gesture (s, 4 /* FRICTION */, 0.62f, 0.30f, 0.42f, 0.30f, 0.20f, 0.30f);
    amp (s, 0.015f, 0.80f, 0.86f, 0.30f, 0.4f);
    set (s, Param::masterGain, 0.0f);
    shape (s, 0.28f, 0.30f, 0.76f, 0.34f, 0.34f, 0.48f);
    material (s, MaterialType::Membrane, MaterialType::Organic, 0.38f);
    topology (s, 2 /* CLUSTERS */, 0.30f, 0.28f, 419);
    matter (s, 0.90f, 0.44f, 0.30f, 0.26f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.30f, 0.60f, 0.0f, 0.12f, 0.28f, 0.14f);
    set (s, Param::evolveMagnetTarget, 0 /* OCTAVE */);
    set (s, Param::masterMode, 2 /* LEGATO */);
    set (s, Param::masterGlide, 0.08f);
    space (s, SpacePresets::Machine, 0.18f, 0.24f, 0.44f, 0.22f);
    set (s, Param::spaceDistMode, 1 /* TUBE */);
    set (s, Param::spaceDistDrive, 0.38f);

    env (s, 1, 0.006f, 0.45f, 0.35f, 0.30f, 0.35f);
    lfo (s, 1, 6.20f, 1 /* TRIANGLE */, 1.0f, true, 0.25f);
    chaos (s, 1, 2 /* LOGISTIC */, 2.40f, 0.35f, 0.82f, 0.5f, 811);
    macros (s, 0.25f, 0.40f, 0.20f, 0.55f);

    Routings r;
    r.uni (ModSource::Env1,     Param::gestureSpeed,     0.280f)
     .bi  (ModSource::LFO1,     Param::gesturePressure,  0.060f)
     .bi  (ModSource::Chaos1,   Param::gestureRoughness, 0.120f)
     .uni (ModSource::Velocity, Param::spaceDistDrive,   0.280f)
     .uni (ModSource::Velocity, Param::gesturePressure,  0.220f)
     .bi  (ModSource::KeyTrack, Param::gestureBandwidth, 0.220f)
     .uni (ModSource::Macro1,   Param::gestureMotion,    0.320f)
     .uni (ModSource::Macro1,   Param::evolveMotion,     0.240f)
     .uni (ModSource::Macro2,   Param::gestureBandwidth, 0.320f)
     .uni (ModSource::Macro2,   Param::spaceTone,        0.240f)
     .uni (ModSource::Macro3,   Param::spaceMix,         0.240f)
     .uni (ModSource::Macro4,   Param::spaceDistDrive,   0.340f)
     .uni (ModSource::Macro4,   Param::gestureRoughness, 0.260f);
    sharedMacros (r, Param::ampDecay, Param::chaos1Depth);
    r.commit (s);
}});

//--------------------------------------------------------------------------
// An upright bass bowed near the bridge, rosin and all. The bow keeps feeding
// the string chain for as long as the key is down, so this is the one bass in
// the set that sustains without a synthesiser's help.
manager.addFactory ({ "Rosin Bow", "BASS", { "organic", "bowed", "warm", "roomy", "low" }, [] (PatchState& s)
{
    gesture (s, 0 /* BOW */, 0.56f, 0.34f, 0.24f, 0.36f, 0.22f, 0.26f);
    amp (s, 0.06f, 0.90f, 0.82f, 0.40f, 0.45f);
    set (s, Param::masterGain, -1.0f);
    shape (s, 0.36f, 0.06f, 0.70f, 0.44f, 0.44f, 0.28f);
    material (s, MaterialType::Organic, MaterialType::String, 0.52f);
    topology (s, 0 /* CHAIN */, 0.38f, 0.44f, 233);
    matter (s, 0.90f, 0.46f, 0.14f, 0.36f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.20f, 0.54f, 0.06f, 0.0f, 0.18f, 0.16f);
    set (s, Param::evolveMagnetTarget, 6 /* CUSTOM */);
    set (s, Param::masterMode, 2 /* LEGATO */);
    set (s, Param::masterGlide, 0.10f);
    space (s, SpacePresets::Chamber, 0.22f, 0.36f, 0.46f, 0.20f);

    lfo (s, 1, 4.60f, 0 /* SINE */, 1.0f, true, 0.60f);
    env (s, 1, 0.05f, 0.60f, 0.55f, 0.40f, 0.4f);
    macros (s, 0.30f, 0.35f, 0.25f, 0.45f);

    Routings r;
    r.bi  (ModSource::LFO1,     Param::shapePitch,       0.100f)
     .uni (ModSource::Env1,     Param::gesturePressure,  0.240f)
     .uni (ModSource::Env1,     Param::gestureBandwidth, 0.180f)
     .uni (ModSource::Velocity, Param::gestureSpeed,     0.280f)
     .uni (ModSource::Velocity, Param::shapeExcite,      0.200f)
     .bi  (ModSource::KeyTrack, Param::gesturePosition,  0.220f)
     .uni (ModSource::Macro1,   Param::gestureMotion,    0.340f)
     .uni (ModSource::Macro1,   Param::lfo1Depth,        0.200f)
     .uni (ModSource::Macro2,   Param::gestureBandwidth, 0.320f)
     .uni (ModSource::Macro2,   Param::shapeExcite,      0.240f)
     .uni (ModSource::Macro3,   Param::spaceMix,         0.260f)
     .uni (ModSource::Macro4,   Param::gestureRoughness, 0.320f)
     .uni (ModSource::Macro4,   Param::gesturePosition,  0.220f);
    sharedMacros (r, Param::ampDecay, Param::gestureRoughness);
    r.commit (s);
}});

//--------------------------------------------------------------------------
// A stone dropped down a dry well: the sample is the fall, the wood-and-void
// object is the shaft. The lower you play it the longer the shaft gets, so
// the note reads as depth rather than as pitch.
manager.addFactory ({ "Well Stone", "BASS", { "hollow", "struck", "dark", "distant", "sub" }, [] (PatchState& s)
{
    sample (s, BuiltInSamples::Kind::StoneDrop, 0 /* ONE SHOT */, 0.0f, 0.80f, 0.22f, 0.30f, 48, 0.90f);
    amp (s, 0.004f, 0.90f, 0.72f, 0.38f, 0.3f);
    set (s, Param::masterGain, -2.0f);
    shape (s, 0.32f, 0.28f, 0.82f, 0.30f, 0.52f, 0.26f);
    material (s, MaterialType::Wood, MaterialType::Void, 0.46f);
    topology (s, 2 /* CLUSTERS */, 0.32f, 0.30f, 587);
    matter (s, 0.86f, 0.56f, 0.36f, 0.40f);
    evolve (s, 0.0f, 0.14f, 0.0f, 0.0f, 0.66f, 0.10f, 0.0f, 0.16f, 0.18f);
    set (s, Param::masterMode, 1 /* MONO */);
    set (s, Param::masterGlide, 0.03f);
    space (s, SpacePresets::Void, 0.24f, 0.52f, 0.34f, 0.24f);

    env (s, 1, 0.002f, 0.40f, 0.10f, 0.28f, 0.3f);
    lfo (s, 1, 0.60f, 5 /* SMOOTH RANDOM */, 1.0f, true, 0.30f);
    macros (s, 0.25f, 0.30f, 0.30f, 0.45f);

    Routings r;
    r.uni (ModSource::Env1,     Param::shapeSurface,   0.240f)
     .uni (ModSource::Env1,     Param::sampleGrain,    0.180f)
     .bi  (ModSource::LFO1,     Param::sampleStart,    0.050f)
     .uni (ModSource::Velocity, Param::shapeStrike,    0.300f)
     .uni (ModSource::Velocity, Param::sampleLevel,    0.200f)
     .bi  (ModSource::KeyTrack, Param::shapeDecay,    -0.240f)
     .uni (ModSource::Macro1,   Param::evolveMotion,   0.300f)
     .uni (ModSource::Macro1,   Param::evolveMelt,     0.220f)
     .uni (ModSource::Macro2,   Param::shapeExcite,    0.320f)
     .uni (ModSource::Macro2,   Param::spaceTone,      0.240f)
     .uni (ModSource::Macro3,   Param::spaceMix,       0.280f)
     .uni (ModSource::Macro3,   Param::spaceSize,      0.220f)
     .uni (ModSource::Macro4,   Param::shapeMass,      0.280f)
     .uni (ModSource::Macro4,   Param::sampleStart,    0.200f);
    sharedMacros (r, Param::ampDecay, Param::sampleGrain);
    r.commit (s);
}});

//--------------------------------------------------------------------------
// A bridge girder hit with a hammer, heard from underneath. The star topology
// hangs everything off one central node, so the strike is one event rather
// than a chord of them, and the wood in the blend stops it ringing forever.
manager.addFactory ({ "Bridge Iron", "BASS", { "metallic", "struck", "harsh", "close", "low" }, [] (PatchState& s)
{
    impact (s, 4 /* METAL STRIKE */, 0.70f, 0.64f, 0.34f, 0.76f, 0.35f, 0.16f);
    amp (s, 0.002f, 0.80f, 0.76f, 0.28f, 0.3f);
    set (s, Param::masterGain, -3.0f);
    shape (s, 0.40f, 0.58f, 0.70f, 0.56f, 0.52f, 0.40f);
    material (s, MaterialType::Metal, MaterialType::Wood, 0.48f);
    topology (s, 5 /* STAR */, 0.44f, 0.36f, 743);
    matter (s, 0.88f, 0.68f, 0.66f, 0.34f);
    evolve (s, 0.0f, 0.0f, 0.16f, 0.0f, 0.58f, 0.10f, 0.0f, 0.30f, 0.16f);
    set (s, Param::evolveScatterSeed, 1277);
    set (s, Param::masterMode, 1 /* MONO */);
    set (s, Param::masterGlide, 0.02f);
    space (s, SpacePresets::Machine, 0.16f, 0.30f, 0.42f, 0.20f);
    set (s, Param::spaceDistDrive, 0.24f);

    FractureShape f;
    f.fragments = 8;
    f.delayLow = 0.03f; f.delayHigh = 0.20f;
    f.feedbackLow = 0.08f; f.feedbackHigh = 0.20f;
    f.decayLow = 0.25f; f.decayHigh = 0.40f;
    f.spreadLow = 0.15f; f.spreadHigh = 0.55f;
    f.gainLow = 0.30f; f.gainHigh = 0.80f;
    f.panWidth = 0.35f;
    f.probability = 0.75f;
    f.pattern = "XHoHXHoH";
    fracture (s, 2 /* TRANSIENT */, 0.54f, 0.40f, 0.42f, 0.40f, 0.12f, 0.14f, 0.32f, 0.74f, 0.12f,
              0 /* 8 */, 5 /* 1/32 */, 8, 0.0f, 0 /* FORWARD */, 0.75f, 0.25f, 1873, f);
    set (s, Param::fracturePitch, 12.0f);   // the caught transients sit an octave up: that is the part a small speaker gets

    env (s, 1, 0.001f, 0.26f, 0.0f, 0.20f, 0.25f);
    macros (s, 0.25f, 0.45f, 0.20f, 0.50f);

    Routings r;
    r.uni (ModSource::Env1,       Param::shapeSurface,    0.300f)
     .uni (ModSource::Env1,       Param::fractureMix,     0.200f)
     .uni (ModSource::Velocity,   Param::shapeStrike,     0.320f)
     .uni (ModSource::Velocity,   Param::spaceDistDrive,  0.200f)
     .uni (ModSource::NoteRandom, Param::impactHardness,  0.180f)
     .bi  (ModSource::KeyTrack,   Param::shapeDecay,     -0.220f)
     .uni (ModSource::Macro1,     Param::evolveMotion,    0.280f)
     .uni (ModSource::Macro1,     Param::evolveTear,      0.200f)
     .uni (ModSource::Macro2,     Param::impactBrightness, 0.340f)
     .uni (ModSource::Macro2,     Param::fractureTone,    0.240f)
     .uni (ModSource::Macro3,     Param::spaceMix,        0.240f)
     .uni (ModSource::Macro4,     Param::spaceDistDrive,  0.320f)
     .uni (ModSource::Macro4,     Param::shapeSurface,    0.240f);
    sharedMacros (r, Param::ampDecay, Param::fractureRandom);
    r.commit (s);
}});

//--------------------------------------------------------------------------
// Gut strings on a wooden body, plucked with the flesh of the thumb. A ring
// topology gives the body its own small feedback path, which is what puts the
// woody bloom a few milliseconds after the pluck instead of on it.
manager.addFactory ({ "Gut String", "BASS", { "wooden", "plucked", "warm", "close", "melodic" }, [] (PatchState& s)
{
    impact (s, 2 /* PLUCK */, 0.42f, 0.48f, 0.36f, 0.66f, 0.55f, 0.12f);
    amp (s, 0.004f, 0.85f, 0.68f, 0.32f, 0.35f);
    set (s, Param::masterGain, -2.0f);
    shape (s, 0.38f, 0.10f, 0.68f, 0.40f, 0.52f, 0.32f);
    material (s, MaterialType::Wood, MaterialType::String, 0.56f);
    topology (s, 1 /* RING */, 0.46f, 0.48f, 307);
    matter (s, 0.84f, 0.60f, 0.50f, 0.38f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.18f, 0.52f, 0.0f, 0.0f, 0.22f, 0.12f);
    set (s, Param::evolveMagnetTarget, 6 /* CUSTOM */);
    set (s, Param::masterMode, 2 /* LEGATO */);
    set (s, Param::masterGlide, 0.06f);
    space (s, SpacePresets::Chamber, 0.20f, 0.32f, 0.50f, 0.18f);
    set (s, Param::spaceEqHigh, 3.0f);

    env (s, 1, 0.001f, 0.34f, 0.08f, 0.24f, 0.3f);
    env (s, 2, 0.030f, 0.55f, 0.30f, 0.35f, 0.45f);
    macros (s, 0.20f, 0.40f, 0.25f, 0.45f);

    Routings r;
    r.uni (ModSource::Env1,       Param::shapeSurface,    0.220f)
     .uni (ModSource::Env2,       Param::shapeCoupling,   0.200f)
     .uni (ModSource::Velocity,   Param::shapeStrike,     0.300f)
     .uni (ModSource::Velocity,   Param::impactHardness,  0.260f)
     .uni (ModSource::NoteRandom, Param::impactCurve,     0.160f)
     .bi  (ModSource::KeyTrack,   Param::shapeMass,      -0.200f)
     .uni (ModSource::Macro1,     Param::evolveMotion,    0.280f)
     .uni (ModSource::Macro1,     Param::shapeCoupling,   0.220f)
     .uni (ModSource::Macro2,     Param::impactBrightness, 0.340f)
     .uni (ModSource::Macro2,     Param::shapeExcite,     0.240f)
     .uni (ModSource::Macro3,     Param::spaceMix,        0.260f)
     .uni (ModSource::Macro4,     Param::shapeBlend,      0.300f)
     .uni (ModSource::Macro4,     Param::shapeSurface,    0.220f);
    sharedMacros (r, Param::ampDecay, Param::impactRandom);
    r.commit (s);
}});

//--------------------------------------------------------------------------
// Mains hum with a fault in it, run through a metal-and-void object with no
// order to its nodes. The electrical gesture sputters rather than sustains,
// so the bass arrives with a buzz that never repeats the same way twice.
manager.addFactory ({ "Live Wire", "BASS", { "synthetic", "dirty", "unstable", "dry", "low" }, [] (PatchState& s)
{
    gesture (s, 5 /* ELECTRICAL */, 0.54f, 0.44f, 0.36f, 0.28f, 0.30f, 0.34f);
    amp (s, 0.008f, 0.70f, 0.80f, 0.26f, 0.35f);
    set (s, Param::masterGain, -1.0f);
    shape (s, 0.44f, 0.40f, 0.66f, 0.48f, 0.36f, 0.44f);
    material (s, MaterialType::Metal, MaterialType::Void, 0.44f);
    topology (s, 4 /* RANDOM */, 0.40f, 0.38f, 1039);
    matter (s, 0.86f, 0.52f, 0.24f, 0.32f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.34f, 0.56f, 0.22f, 0.0f, 0.40f, 0.24f);
    set (s, Param::evolveMagnetTarget, 0 /* OCTAVE */);
    set (s, Param::evolveScatterSeed, 2311);
    set (s, Param::masterMode, 1 /* MONO */);
    set (s, Param::masterGlide, 0.02f);
    space (s, SpacePresets::Machine, 0.14f, 0.20f, 0.50f, 0.24f);
    set (s, Param::spaceDistMode, 3 /* CRUSH */);
    set (s, Param::spaceDistDrive, 0.22f);

    env (s, 1, 0.003f, 0.30f, 0.20f, 0.22f, 0.3f);
    lfo (s, 1, 8.40f, 3 /* SQUARE */, 1.0f, true, 0.10f, 0.30f);
    chaos (s, 1, 3 /* LORENZ */, 4.20f, 0.40f, 0.75f, 0.5f, 1523);
    macros (s, 0.35f, 0.40f, 0.15f, 0.50f);

    Routings r;
    r.uni (ModSource::Env1,       Param::gestureBandwidth, 0.260f)
     .bi  (ModSource::LFO1,       Param::gestureRoughness, 0.100f)
     .bi  (ModSource::Chaos1,     Param::gestureSpeed,     0.180f)
     .uni (ModSource::Velocity,   Param::gesturePressure,  0.280f)
     .uni (ModSource::NoteRandom, Param::gesturePosition,  0.220f)
     .bi  (ModSource::KeyTrack,   Param::gestureBandwidth, 0.200f)
     .uni (ModSource::Macro1,     Param::evolveScatter,    0.280f)
     .uni (ModSource::Macro1,     Param::gestureMotion,    0.320f)
     .uni (ModSource::Macro2,     Param::gestureBandwidth, 0.320f)
     .uni (ModSource::Macro2,     Param::spaceTone,        0.220f)
     .uni (ModSource::Macro3,     Param::spaceMix,         0.240f)
     .uni (ModSource::Macro4,     Param::spaceDistDrive,   0.300f)
     .uni (ModSource::Macro4,     Param::gestureRoughness, 0.280f);
    sharedMacros (r, Param::ampDecay, Param::chaos1Rate);
    r.commit (s);
}});

//--------------------------------------------------------------------------
// MAGNET is the patch. Every partial of a crystal-void object is dragged onto
// the octave grid, which leaves an object with no inharmonicity anywhere in
// it: the most defined note in the set, and the one that survives a mono fold.
manager.addFactory ({ "Anchor Line", "BASS", { "clean", "cold", "static", "dry", "sub" }, [] (PatchState& s)
{
    wave (s, 0 /* BASIC */, 0.20f, 0.14f, 0.0f, 1, 0.04f, 0.14f, -1);
    set (s, Param::wavePhaseRandom, 0.0f);   // the patch is about definition: no unison beating under it
    amp (s, 0.004f, 0.90f, 0.82f, 0.30f, 0.35f);
    set (s, Param::masterGain, -4.0f);
    shape (s, 0.30f, 0.40f, 0.72f, 0.46f, 0.48f, 0.14f);
    material (s, MaterialType::Crystal, MaterialType::Void, 0.36f);
    topology (s, 3 /* LATTICE */, 0.26f, 0.44f, 1451);
    matter (s, 0.88f, 0.56f, 0.38f, 0.22f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.62f, 0.54f, 0.0f, 0.0f, 0.16f, 0.06f);
    set (s, Param::evolveMagnetTarget, 0 /* OCTAVE */);
    set (s, Param::masterMode, 1 /* MONO */);
    set (s, Param::masterGlide, 0.03f);
    space (s, SpacePresets::Chamber, 0.10f, 0.20f, 0.50f, 0.14f);

    env (s, 1, 0.002f, 0.35f, 0.12f, 0.24f, 0.3f);
    env (s, 2, 0.20f, 0.90f, 0.60f, 0.40f, 0.4f);
    macros (s, 0.15f, 0.40f, 0.20f, 0.50f);

    Routings r;
    r.uni (ModSource::Env1,     Param::shapeSurface,  0.200f)
     .uni (ModSource::Env2,     Param::evolveMagnet,  0.300f)
     .uni (ModSource::Velocity, Param::shapeStrike,   0.320f)
     .uni (ModSource::Velocity, Param::shapeExcite,   0.220f)
     .bi  (ModSource::KeyTrack, Param::shapeMass,    -0.200f)
     .uni (ModSource::Macro1,   Param::evolveMotion,  0.260f)
     .uni (ModSource::Macro1,   Param::evolveSpeed,   0.200f)
     .uni (ModSource::Macro2,   Param::shapeExcite,   0.340f)
     .uni (ModSource::Macro2,   Param::wavePosition,  0.240f)
     .uni (ModSource::Macro3,   Param::spaceMix,      0.240f)
     .uni (ModSource::Macro4,   Param::evolveMagnet, -0.360f)
     .uni (ModSource::Macro4,   Param::shapeTension,  0.240f);
    sharedMacros (r, Param::ampDecay, Param::shapeDistribution);
    r.commit (s);
}});

//--------------------------------------------------------------------------
// CRUSH is the patch. A crackle field drives a wood-and-metal object with no
// order to it, and the operator quantises the partials onto a coarse grid and
// the weights onto a few steps: gravel in a mill rather than a note.
manager.addFactory ({ "Gravel Mill", "BASS", { "dirty", "harsh", "noisy", "close", "low" }, [] (PatchState& s)
{
    dust (s, 5 /* CRACKLE */, 0.72f, 0.34f, 0.42f, 0.40f, 0.30f, 0.30f, 1721);
    amp (s, 0.006f, 0.80f, 0.80f, 0.26f, 0.35f);
    set (s, Param::masterGain, 1.0f);
    shape (s, 0.42f, 0.34f, 0.70f, 0.42f, 0.42f, 0.44f);
    material (s, MaterialType::Wood, MaterialType::Metal, 0.46f);
    topology (s, 4 /* RANDOM */, 0.42f, 0.34f, 1913);
    matter (s, 0.90f, 0.52f, 0.26f, 0.30f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.20f, 0.58f, 0.14f, 0.40f, 0.34f, 0.22f);
    set (s, Param::evolveMagnetTarget, 0 /* OCTAVE */);
    set (s, Param::masterMode, 1 /* MONO */);
    set (s, Param::masterGlide, 0.02f);
    space (s, SpacePresets::Machine, 0.16f, 0.26f, 0.44f, 0.22f);
    set (s, Param::spaceDistMode, 2 /* FOLD */);
    set (s, Param::spaceDistDrive, 0.20f);

    env (s, 1, 0.002f, 0.40f, 0.25f, 0.26f, 0.3f);
    lfo (s, 1, 3.60f, 4 /* RANDOM */, 1.0f, true, 0.20f);
    chaos (s, 1, 2 /* LOGISTIC */, 6.00f, 0.35f, 0.78f, 0.5f, 2999);
    macros (s, 0.35f, 0.35f, 0.20f, 0.55f);

    Routings r;
    r.uni (ModSource::Env1,       Param::evolveCrush,   0.240f)
     .bi  (ModSource::LFO1,       Param::dustDensity,   0.140f)
     .bi  (ModSource::Chaos1,     Param::dustGrain,     0.160f)
     .uni (ModSource::Velocity,   Param::evolveCrush,  -0.260f)
     .uni (ModSource::Velocity,   Param::dustDensity,   0.220f)
     .uni (ModSource::NoteRandom, Param::dustColor,     0.200f)
     .bi  (ModSource::KeyTrack,   Param::dustColor,     0.180f)
     .uni (ModSource::Macro1,     Param::evolveMotion,  0.300f)
     .uni (ModSource::Macro2,     Param::dustColor,     0.320f)
     .uni (ModSource::Macro2,     Param::shapeExcite,   0.240f)
     .uni (ModSource::Macro3,     Param::spaceMix,      0.240f)
     .uni (ModSource::Macro4,     Param::evolveCrush,   0.380f)
     .uni (ModSource::Macro4,     Param::spaceDistDrive, 0.240f);
    sharedMacros (r, Param::ampDecay, Param::chaos1Depth);
    r.commit (s);
}});

//--------------------------------------------------------------------------
// TEAR is the patch. The quiet half of a wire lattice becomes detuned twins
// of the loud half, so the note beats against its own copy and fills a stereo
// field a single cable never could. The fundamental is left alone; only the
// upper clusters are pulled apart.
manager.addFactory ({ "Split Cable", "BASS", { "metallic", "unstable", "plucked", "wide", "low" }, [] (PatchState& s)
{
    impact (s, 2 /* PLUCK */, 0.56f, 0.52f, 0.32f, 0.70f, 0.45f, 0.14f);
    amp (s, 0.003f, 0.85f, 0.74f, 0.32f, 0.35f);
    set (s, Param::masterGain, -3.0f);
    shape (s, 0.48f, 0.22f, 0.66f, 0.62f, 0.50f, 0.30f);
    material (s, MaterialType::String, MaterialType::Metal, 0.52f);
    topology (s, 3 /* LATTICE */, 0.50f, 0.56f, 2657);
    matter (s, 0.86f, 0.58f, 0.54f, 0.68f);
    evolve (s, 0.0f, 0.0f, 0.38f, 0.0f, 0.50f, 0.16f, 0.0f, 0.24f, 0.30f);
    set (s, Param::evolveScatterSeed, 3319);
    set (s, Param::masterMode, 0 /* POLY */);
    space (s, SpacePresets::Orbit, 0.20f, 0.34f, 0.52f, 0.22f);

    env (s, 1, 0.001f, 0.32f, 0.10f, 0.24f, 0.3f);
    env (s, 2, 0.30f, 1.20f, 0.70f, 0.50f, 0.45f);
    lfo (s, 1, 0.90f, 0 /* SINE */, 1.0f, true, 0.40f);
    macros (s, 0.30f, 0.40f, 0.30f, 0.55f);

    Routings r;
    r.uni (ModSource::Env1,       Param::shapeSurface,  0.220f)
     .uni (ModSource::Env2,       Param::evolveTear,    0.300f)
     .bi  (ModSource::LFO1,       Param::shapeStereo,   0.140f)
     .uni (ModSource::Velocity,   Param::shapeStrike,   0.300f)
     .uni (ModSource::NoteRandom, Param::evolveScatter, 0.180f)
     .bi  (ModSource::KeyTrack,   Param::evolveTear,   -0.200f)
     .uni (ModSource::Macro1,     Param::evolveMotion,  0.320f)
     .uni (ModSource::Macro1,     Param::evolveSpeed,   0.220f)
     .uni (ModSource::Macro2,     Param::impactBrightness, 0.340f)
     .uni (ModSource::Macro2,     Param::shapeExcite,   0.220f)
     .uni (ModSource::Macro3,     Param::spaceMix,      0.260f)
     .uni (ModSource::Macro4,     Param::evolveTear,    0.360f)
     .uni (ModSource::Macro4,     Param::shapeStereo,   0.240f);
    sharedMacros (r, Param::ampDecay, Param::impactRandom);
    r.commit (s);
}});

//--------------------------------------------------------------------------
// MELT is the patch. An iron chain is heated through the note: the upper
// partials sag by rank and damp faster while the fundamental stays put, so
// the bass keeps its pitch and loses its posture. Play harder to melt it less.
manager.addFactory ({ "Sagging Iron", "BASS", { "dark", "morphing", "struck", "roomy", "low" }, [] (PatchState& s)
{
    impact (s, 5 /* DAMPED SINE */, 0.44f, 0.42f, 0.44f, 0.68f, 0.50f, 0.12f);
    amp (s, 0.003f, 0.95f, 0.78f, 0.40f, 0.35f);
    set (s, Param::masterGain, -4.5f);
    shape (s, 0.40f, 0.26f, 0.74f, 0.38f, 0.54f, 0.30f);
    material (s, MaterialType::Metal, MaterialType::Organic, 0.54f);
    topology (s, 0 /* CHAIN */, 0.40f, 0.40f, 3607);
    matter (s, 0.88f, 0.52f, 0.44f, 0.34f);
    evolve (s, 0.0f, 0.30f, 0.0f, 0.18f, 0.52f, 0.08f, 0.0f, 0.14f, 0.22f);
    set (s, Param::evolveMagnetTarget, 0 /* OCTAVE */);
    set (s, Param::masterMode, 2 /* LEGATO */);
    set (s, Param::masterGlide, 0.07f);
    space (s, SpacePresets::Chamber, 0.22f, 0.40f, 0.42f, 0.22f);

    env (s, 1, 0.001f, 0.30f, 0.0f, 0.20f, 0.25f);
    env (s, 2, 0.35f, 1.60f, 0.85f, 0.60f, 0.5f);
    lfo (s, 1, 0.45f, 1 /* TRIANGLE */, 1.0f, true, 0.50f);
    macros (s, 0.35f, 0.35f, 0.30f, 0.55f);

    Routings r;
    r.uni (ModSource::Env1,     Param::shapeSurface,  0.200f)
     .uni (ModSource::Env2,     Param::evolveMelt,    0.320f)
     .bi  (ModSource::LFO1,     Param::shapeMass,     0.060f)
     .uni (ModSource::Velocity, Param::evolveMelt,   -0.240f)
     .uni (ModSource::Velocity, Param::shapeStrike,   0.280f)
     .bi  (ModSource::KeyTrack, Param::shapeDecay,   -0.220f)
     .uni (ModSource::Macro1,   Param::evolveMotion,  0.300f)
     .uni (ModSource::Macro1,   Param::evolveSpeed,   0.220f)
     .uni (ModSource::Macro2,   Param::impactBrightness, 0.340f)
     .uni (ModSource::Macro2,   Param::shapeExcite,   0.240f)
     .uni (ModSource::Macro3,   Param::spaceMix,      0.260f)
     .uni (ModSource::Macro4,   Param::evolveMelt,    0.380f)
     .uni (ModSource::Macro4,   Param::evolveGravity, 0.200f);
    sharedMacros (r, Param::ampDecay, Param::shapeSurface);
    r.commit (s);
}});

//--------------------------------------------------------------------------
// FRACTURE, rhythmic — the patch plays the sixteenths, not the player. The
// fragment sequencer gates a metal-and-wood girder in a ping-pong pattern
// with swing, so one held key produces a bass line with a hole in every bar.
manager.addFactory ({ "Girder Stutter", "BASS", { "metallic", "rhythmic", "dirty", "dry", "low" }, [] (PatchState& s)
{
    wave (s, 6 /* FRACTURED */, 0.34f, 0.24f, 0.06f, 1, 0.05f, 0.18f, -1);
    amp (s, 0.002f, 0.90f, 0.86f, 0.22f, 0.3f);
    set (s, Param::masterGain, -1.5f);
    shape (s, 0.44f, 0.48f, 0.68f, 0.50f, 0.40f, 0.36f);
    material (s, MaterialType::Metal, MaterialType::Wood, 0.42f);
    topology (s, 1 /* RING */, 0.44f, 0.38f, 4271);
    matter (s, 0.84f, 0.54f, 0.40f, 0.36f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.24f, 0.56f, 0.10f, 0.0f, 0.30f, 0.18f);
    set (s, Param::evolveMagnetTarget, 0 /* OCTAVE */);
    set (s, Param::masterMode, 1 /* MONO */);
    set (s, Param::masterGlide, 0.02f);
    space (s, SpacePresets::Orbit, 0.16f, 0.28f, 0.46f, 0.20f);

    FractureShape f;
    f.fragments = 16;
    f.delayLow = 0.02f; f.delayHigh = 0.18f;
    f.feedbackLow = 0.15f; f.feedbackHigh = 0.35f;
    f.decayLow = 0.28f; f.decayHigh = 0.46f;
    f.spreadLow = 0.10f; f.spreadHigh = 0.60f;
    f.gainLow = 0.90f; f.gainHigh = 0.75f;
    f.panWidth = 0.40f;
    f.probability = 0.90f;
    f.pattern = "X.LXH.LX";
    fracture (s, 1 /* RHYTHMIC */, 0.75f, 0.62f, 0.45f, 0.85f, 0.22f, 0.14f, 0.36f, 0.60f, 0.18f,
              1 /* 16 */, 4 /* 1/16 */, 8, 0.22f, 2 /* PINGPONG */, 0.90f, 0.20f, 5231, f);

    env (s, 1, 0.001f, 0.26f, 0.10f, 0.18f, 0.25f);
    lfo (s, 1, 2.20f, 3 /* SQUARE */, 1.0f, true, 0.15f, 0.40f);
    macros (s, 0.40f, 0.40f, 0.20f, 0.50f);

    Routings r;
    r.uni (ModSource::Env1,       Param::shapeSurface,   0.240f)
     .bi  (ModSource::LFO1,       Param::fractureTone,   0.120f)
     .uni (ModSource::Velocity,   Param::fractureAmount, 0.260f)
     .uni (ModSource::Velocity,   Param::shapeExcite,    0.220f)
     .uni (ModSource::NoteRandom, Param::fractureSwing,  0.200f)
     .bi  (ModSource::KeyTrack,   Param::fractureDelay, -0.180f)
     .uni (ModSource::Macro1,     Param::fractureSwing,  0.280f)
     .uni (ModSource::Macro1,     Param::evolveMotion,   0.240f)
     .uni (ModSource::Macro2,     Param::fractureTone,   0.320f)
     .uni (ModSource::Macro2,     Param::wavePosition,   0.240f)
     .uni (ModSource::Macro3,     Param::spaceMix,       0.260f)
     .uni (ModSource::Macro4,     Param::fractureProbability, -0.320f)
     .uni (ModSource::Macro4,     Param::fractureFeedback, 0.220f);
    sharedMacros (r, Param::fractureDecay, Param::fractureRandom);
    r.commit (s);
}});

//--------------------------------------------------------------------------
// FRACTURE in EVOLVE mode — a chain dragged along a river bottom. The
// fragment table rewrites which bands survive as the note goes on, so a held
// low note keeps turning over instead of decaying in a straight line.
manager.addFactory ({ "Dredge Line", "BASS", { "dark", "evolving", "hollow", "roomy", "low" }, [] (PatchState& s)
{
    gesture (s, 2 /* RUB */, 0.58f, 0.24f, 0.34f, 0.40f, 0.26f, 0.26f);
    amp (s, 0.020f, 0.95f, 0.84f, 0.34f, 0.4f);
    set (s, Param::masterGain, 0.0f);
    shape (s, 0.36f, 0.16f, 0.78f, 0.36f, 0.50f, 0.30f);
    material (s, MaterialType::Organic, MaterialType::Void, 0.42f);
    topology (s, 2 /* CLUSTERS */, 0.36f, 0.32f, 6199);
    matter (s, 0.90f, 0.44f, 0.22f, 0.40f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.28f, 0.60f, 0.14f, 0.0f, 0.18f, 0.26f);
    set (s, Param::evolveMagnetTarget, 0 /* OCTAVE */);
    set (s, Param::masterMode, 2 /* LEGATO */);
    set (s, Param::masterGlide, 0.09f);
    space (s, SpacePresets::Void, 0.22f, 0.46f, 0.36f, 0.24f);

    FractureShape f;
    f.fragments = 8;
    f.delayLow = 0.10f; f.delayHigh = 0.40f;
    f.feedbackLow = 0.25f; f.feedbackHigh = 0.40f;
    f.decayLow = 0.45f; f.decayHigh = 0.60f;
    f.spreadLow = 0.15f; f.spreadHigh = 0.70f;
    f.gainLow = 0.85f; f.gainHigh = 0.60f;
    f.panWidth = 0.45f;
    f.pitchCycle = kFallingTerrace;
    f.pattern = "XLXoXLXH";
    fracture (s, 3 /* EVOLVE */, 0.55f, 0.38f, 0.50f, 0.55f, 0.30f, 0.34f, 0.52f, 0.44f, 0.60f,
              0 /* 8 */, 2 /* 1/4 */, 8, 0.0f, 1 /* BACKWARD */, 0.95f, 0.25f, 7481, f);

    env (s, 1, 0.010f, 0.50f, 0.40f, 0.32f, 0.35f);
    lfo (s, 1, 0.40f, 5 /* SMOOTH RANDOM */, 1.0f, true, 0.50f);
    macros (s, 0.40f, 0.35f, 0.35f, 0.50f);

    Routings r;
    r.uni (ModSource::Env1,     Param::gestureSpeed,     0.240f)
     .bi  (ModSource::LFO1,     Param::fractureEvolve,   0.180f)
     .uni (ModSource::Velocity, Param::gesturePressure,  0.260f)
     .uni (ModSource::Velocity, Param::fractureMix,      0.180f)
     .bi  (ModSource::KeyTrack, Param::gestureBandwidth, 0.220f)
     .uni (ModSource::Macro1,   Param::fractureEvolve,   0.320f)
     .uni (ModSource::Macro1,   Param::gestureMotion,    0.280f)
     .uni (ModSource::Macro2,   Param::gestureBandwidth, 0.320f)
     .uni (ModSource::Macro2,   Param::fractureTone,     0.240f)
     .uni (ModSource::Macro3,   Param::spaceMix,         0.260f)
     .uni (ModSource::Macro3,   Param::spaceSize,        0.200f)
     .uni (ModSource::Macro4,   Param::fractureMix,      0.300f)
     .uni (ModSource::Macro4,   Param::gestureRoughness, 0.260f);
    sharedMacros (r, Param::fractureDecay, Param::fractureProbability);
    r.commit (s);
}});

//--------------------------------------------------------------------------
// LAYER — a folded wave an octave down for the floor and a resonant band of
// dust sitting on top of it for the grit. The two are tuned to the same note
// but only one of them is audible on a laptop, which is the point.
manager.addFactory ({ "Grit Column", "BASS", { "dirty", "noisy", "cold", "close", "sub" }, [] (PatchState& s)
{
    dust (s, 4 /* FILTERED */, 0.62f, 0.52f, 0.26f, 0.30f, 0.34f, 0.34f, 8171, 0.62f);
    wave (s, 3 /* FOLDED */, 0.26f, 0.22f, 0.0f, 1, 0.03f, 0.10f, -1, 0.80f);
    set (s, Param::waveModRatio, 2.0f);
    set (s, Param::sourceMode, 1 /* LAYER */);
    set (s, Param::impactLevel, 0.0f);
    set (s, Param::sampleLevel, 0.0f);
    set (s, Param::gestureLevel, 0.0f);
    amp (s, 0.005f, 0.85f, 0.82f, 0.26f, 0.35f);
    set (s, Param::masterGain, -3.5f);
    shape (s, 0.34f, 0.30f, 0.76f, 0.40f, 0.42f, 0.38f);
    material (s, MaterialType::Membrane, MaterialType::Metal, 0.34f);
    topology (s, 2 /* CLUSTERS */, 0.32f, 0.36f, 9227);
    matter (s, 0.82f, 0.50f, 0.28f, 0.30f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.34f, 0.58f, 0.0f, 0.0f, 0.24f, 0.14f);
    set (s, Param::evolveMagnetTarget, 0 /* OCTAVE */);
    set (s, Param::masterMode, 1 /* MONO */);
    set (s, Param::masterGlide, 0.03f);
    space (s, SpacePresets::Chamber, 0.12f, 0.24f, 0.48f, 0.16f);

    env (s, 1, 0.002f, 0.38f, 0.20f, 0.24f, 0.3f);
    lfo (s, 1, 5.20f, 0 /* SINE */, 1.0f, true, 0.25f);
    macros (s, 0.25f, 0.45f, 0.20f, 0.55f);

    Routings r;
    r.uni (ModSource::Env1,     Param::dustColor,     0.240f)
     .uni (ModSource::Env1,     Param::shapeSurface,  0.200f)
     .bi  (ModSource::LFO1,     Param::dustDensity,   0.100f)
     .uni (ModSource::Velocity, Param::dustLevel,     0.260f)
     .uni (ModSource::Velocity, Param::shapeExcite,   0.200f)
     .bi  (ModSource::KeyTrack, Param::waveLevel,    -0.240f)
     .uni (ModSource::Macro1,   Param::evolveMotion,  0.280f)
     .uni (ModSource::Macro1,   Param::dustJitter,    0.220f)
     .uni (ModSource::Macro2,   Param::dustColor,     0.340f)
     .uni (ModSource::Macro2,   Param::waveMorph,     0.240f)
     .uni (ModSource::Macro3,   Param::spaceMix,      0.240f)
     .uni (ModSource::Macro4,   Param::dustLevel,     0.320f)
     .uni (ModSource::Macro4,   Param::dustGrain,     0.240f);
    sharedMacros (r, Param::ampDecay, Param::dustJitter);
    r.commit (s);
}});

//--------------------------------------------------------------------------
// LAYER — the rope is plucked and then rubbed while it is still moving. The
// pluck is the attack and the rub is the sustain, on the same string-and-wood
// chain, so the note has a front and a body that were made by two hands.
manager.addFactory ({ "Struck Rope", "BASS", { "organic", "plucked", "scraped", "close", "melodic" }, [] (PatchState& s)
{
    gesture (s, 2 /* RUB */, 0.44f, 0.26f, 0.30f, 0.44f, 0.24f, 0.28f, 0.55f);
    impact (s, 2 /* PLUCK */, 0.48f, 0.46f, 0.28f, 0.70f, 0.50f, 0.16f, 0.0f, 0.85f);
    set (s, Param::sourceMode, 1 /* LAYER */);
    set (s, Param::waveLevel, 0.0f);
    set (s, Param::dustLevel, 0.0f);
    set (s, Param::sampleLevel, 0.0f);
    amp (s, 0.003f, 0.90f, 0.80f, 0.34f, 0.35f);
    set (s, Param::masterGain, -3.0f);
    shape (s, 0.40f, 0.14f, 0.70f, 0.46f, 0.50f, 0.28f);
    material (s, MaterialType::String, MaterialType::Wood, 0.44f);
    topology (s, 0 /* CHAIN */, 0.42f, 0.46f, 10007);
    matter (s, 0.86f, 0.56f, 0.48f, 0.38f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.22f, 0.52f, 0.08f, 0.0f, 0.20f, 0.16f);
    set (s, Param::evolveMagnetTarget, 6 /* CUSTOM */);
    set (s, Param::masterMode, 2 /* LEGATO */);
    set (s, Param::masterGlide, 0.05f);
    space (s, SpacePresets::Chamber, 0.18f, 0.32f, 0.50f, 0.18f);

    env (s, 1, 0.001f, 0.30f, 0.0f, 0.22f, 0.25f);
    env (s, 2, 0.12f, 0.80f, 0.65f, 0.45f, 0.45f);
    macros (s, 0.25f, 0.40f, 0.25f, 0.50f);

    Routings r;
    r.uni (ModSource::Env1,       Param::impactHardness,  0.260f)
     .uni (ModSource::Env2,       Param::gesturePressure, 0.240f)
     .uni (ModSource::Env2,       Param::gestureSpeed,    0.180f)
     .uni (ModSource::Velocity,   Param::shapeStrike,     0.300f)
     .uni (ModSource::Velocity,   Param::impactBrightness, 0.240f)
     .uni (ModSource::NoteRandom, Param::gesturePosition, 0.180f)
     .bi  (ModSource::KeyTrack,   Param::gestureLevel,   -0.220f)
     .uni (ModSource::Macro1,     Param::gestureMotion,   0.320f)
     .uni (ModSource::Macro1,     Param::evolveMotion,    0.240f)
     .uni (ModSource::Macro2,     Param::impactBrightness, 0.320f)
     .uni (ModSource::Macro2,     Param::gestureBandwidth, 0.260f)
     .uni (ModSource::Macro3,     Param::spaceMix,        0.260f)
     .uni (ModSource::Macro4,     Param::gestureLevel,    0.320f)
     .uni (ModSource::Macro4,     Param::gestureRoughness, 0.240f);
    sharedMacros (r, Param::ampDecay, Param::gestureRoughness);
    r.commit (s);
}});

//--------------------------------------------------------------------------
// Knuckles on the underside of a table, read as grains and dropped an octave
// into the table itself. A rhythmic FRACTURE at 1/8 turns one knock into the
// three or four it takes to say a pitch down here.
manager.addFactory ({ "Knuckle Wood", "BASS", { "wooden", "granular", "rhythmic", "dry", "low" }, [] (PatchState& s)
{
    sample (s, BuiltInSamples::Kind::WoodKnock, 3 /* GRANULAR */, 0.0f, 0.70f, 0.58f, 0.46f, 48, 0.95f);
    amp (s, 0.004f, 1.10f, 0.86f, 0.30f, 0.35f);
    set (s, Param::masterGain, -0.5f);
    shape (s, 0.36f, 0.12f, 0.72f, 0.42f, 0.66f, 0.30f);
    material (s, MaterialType::Wood, MaterialType::Membrane, 0.38f);
    topology (s, 1 /* RING */, 0.40f, 0.42f, 11003);
    matter (s, 0.86f, 0.80f, 0.42f, 0.34f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.24f, 0.56f, 0.10f, 0.0f, 0.26f, 0.16f);
    set (s, Param::evolveMagnetTarget, 0 /* OCTAVE */);
    set (s, Param::masterMode, 1 /* MONO */);
    set (s, Param::masterGlide, 0.03f);
    space (s, SpacePresets::Dust, 0.18f, 0.30f, 0.44f, 0.20f);
    set (s, Param::spaceCompOn, 1.0f);
    set (s, Param::spaceCompAmount, 0.75f);   // knocks arrive in gusts; a bass part cannot

    FractureShape f;
    f.fragments = 8;
    f.delayLow = 0.03f; f.delayHigh = 0.22f;
    f.feedbackLow = 0.12f; f.feedbackHigh = 0.30f;
    f.decayLow = 0.30f; f.decayHigh = 0.48f;
    f.spreadLow = 0.10f; f.spreadHigh = 0.55f;
    f.gainLow = 0.85f; f.gainHigh = 0.70f;
    f.panWidth = 0.35f;
    f.probability = 0.85f;
    f.pattern = "XoLXoHXo";
    fracture (s, 1 /* RHYTHMIC */, 0.62f, 0.45f, 0.38f, 0.70f, 0.20f, 0.18f, 0.40f, 0.58f, 0.16f,
              0 /* 8 */, 3 /* 1/8 */, 8, 0.14f, 0 /* FORWARD */, 0.85f, 0.22f, 12071, f);

    env (s, 1, 0.001f, 0.30f, 0.10f, 0.22f, 0.25f);
    lfo (s, 1, 1.60f, 5 /* SMOOTH RANDOM */, 1.0f, true, 0.20f);
    macros (s, 0.35f, 0.40f, 0.25f, 0.50f);

    Routings r;
    r.uni (ModSource::Env1,       Param::sampleGrain,    0.240f)
     .bi  (ModSource::LFO1,       Param::sampleStart,    0.060f)
     .uni (ModSource::Velocity,   Param::shapeStrike,    0.300f)
     .uni (ModSource::Velocity,   Param::sampleGrain,   -0.220f)
     .uni (ModSource::NoteRandom, Param::sampleStart,    0.150f)
     .bi  (ModSource::KeyTrack,   Param::fractureDelay, -0.180f)
     .uni (ModSource::Macro1,     Param::fractureSwing,  0.260f)
     .uni (ModSource::Macro1,     Param::evolveMotion,   0.240f)
     .uni (ModSource::Macro2,     Param::shapeExcite,    0.340f)
     .uni (ModSource::Macro2,     Param::fractureTone,   0.240f)
     .uni (ModSource::Macro3,     Param::spaceMix,       0.260f)
     .uni (ModSource::Macro4,     Param::sampleGrain,    0.320f)
     .uni (ModSource::Macro4,     Param::fractureMix,    0.240f);
    sharedMacros (r, Param::ampDecay, Param::fractureRandom);
    r.commit (s);
}});

//--------------------------------------------------------------------------
// LAYER — a stone dropped into tar, with a sine an octave down for the hole
// it leaves. The void-and-membrane star swallows the transient almost at
// once, so what is left of the sample is the weight, not the event.
manager.addFactory ({ "Tar Drum", "BASS", { "dark", "soft", "struck", "distant", "sub" }, [] (PatchState& s)
{
    wave (s, 0 /* BASIC */, 0.14f, 0.08f, 0.0f, 1, 0.02f, 0.08f, -1, 0.50f);
    sample (s, BuiltInSamples::Kind::StoneDrop, 0 /* ONE SHOT */, 0.02f, 0.62f, 0.26f, 0.26f, 48, 0.85f);
    set (s, Param::sourceMode, 1 /* LAYER */);
    set (s, Param::dustLevel, 0.0f);
    set (s, Param::impactLevel, 0.0f);
    set (s, Param::gestureLevel, 0.0f);
    amp (s, 0.006f, 1.00f, 0.72f, 0.42f, 0.3f);
    set (s, Param::masterGain, -2.5f);
    shape (s, 0.28f, 0.20f, 0.84f, 0.32f, 0.52f, 0.22f);
    material (s, MaterialType::Void, MaterialType::Membrane, 0.48f);
    topology (s, 5 /* STAR */, 0.30f, 0.28f, 13001);
    matter (s, 0.88f, 0.38f, 0.30f, 0.26f);
    evolve (s, 0.0f, 0.22f, 0.0f, 0.26f, 0.66f, 0.0f, 0.0f, 0.12f, 0.14f);
    set (s, Param::evolveMagnetTarget, 0 /* OCTAVE */);
    set (s, Param::masterMode, 1 /* MONO */);
    set (s, Param::masterGlide, 0.04f);
    space (s, SpacePresets::Void, 0.20f, 0.50f, 0.30f, 0.22f);
    set (s, Param::spaceEqMid, 2.0f);

    env (s, 1, 0.002f, 0.45f, 0.15f, 0.30f, 0.3f);
    env (s, 2, 0.25f, 1.10f, 0.55f, 0.50f, 0.45f);
    macros (s, 0.20f, 0.30f, 0.30f, 0.50f);

    Routings r;
    r.uni (ModSource::Env1,     Param::shapeSurface,  0.200f)
     .uni (ModSource::Env2,     Param::evolveMelt,    0.240f)
     .uni (ModSource::Velocity, Param::sampleLevel,   0.280f)
     .uni (ModSource::Velocity, Param::shapeStrike,   0.240f)
     .bi  (ModSource::KeyTrack, Param::waveLevel,    -0.260f)
     .uni (ModSource::Macro1,   Param::evolveMotion,  0.260f)
     .uni (ModSource::Macro1,   Param::evolveMelt,    0.220f)
     .uni (ModSource::Macro2,   Param::shapeExcite,   0.340f)
     .uni (ModSource::Macro2,   Param::spaceTone,     0.240f)
     .uni (ModSource::Macro3,   Param::spaceMix,      0.260f)
     .uni (ModSource::Macro3,   Param::spaceSize,     0.200f)
     .uni (ModSource::Macro4,   Param::waveLevel,     0.300f)
     .uni (ModSource::Macro4,   Param::shapeMass,     0.220f);
    sharedMacros (r, Param::ampDecay, Param::sampleGrain);
    r.commit (s);
}});

//--------------------------------------------------------------------------
// BEND is the patch. A liquid object with a pivot low in the spectrum: hold
// the key and the partials above it lift while the ones below sink, so a
// rubbery bass opens outwards like a hand instead of decaying.
manager.addFactory ({ "Rubber Vault", "BASS", { "organic", "morphing", "soft", "roomy", "low" }, [] (PatchState& s)
{
    wave (s, 3 /* FOLDED */, 0.30f, 0.34f, 0.06f, 1, 0.04f, 0.12f, -1);
    set (s, Param::waveModRatio, 1.25f);
    set (s, Param::wavePhaseRandom, 0.0f);   // one voice, one phase: a sub that beats against itself is not a sub
    amp (s, 0.010f, 1.10f, 0.90f, 0.36f, 0.4f);
    set (s, Param::masterGain, -0.5f);
    shape (s, 0.44f, 0.36f, 0.68f, 0.36f, 0.50f, 0.34f);
    material (s, MaterialType::Liquid, MaterialType::Membrane, 0.44f);
    topology (s, 4 /* RANDOM */, 0.38f, 0.48f, 14009);
    matter (s, 0.86f, 0.50f, 0.30f, 0.44f);
    evolve (s, 0.42f, 0.0f, 0.0f, 0.0f, 0.48f, 0.10f, 0.0f, 0.16f, 0.28f);
    set (s, Param::evolveBendPivot, 0.28f);
    set (s, Param::evolveBendRange, 0.36f);
    set (s, Param::evolveBendCurve, 0.62f);
    set (s, Param::masterMode, 2 /* LEGATO */);
    set (s, Param::masterGlide, 0.08f);
    space (s, SpacePresets::Dream, 0.22f, 0.42f, 0.46f, 0.24f);

    env (s, 1, 0.004f, 0.40f, 0.25f, 0.28f, 0.3f);
    env (s, 2, 0.40f, 1.40f, 0.80f, 0.55f, 0.5f);
    lfo (s, 1, 0.55f, 0 /* SINE */, 1.0f, true, 0.45f);
    macros (s, 0.35f, 0.40f, 0.30f, 0.55f);

    Routings r;
    r.uni (ModSource::Env1,     Param::shapeSurface,   0.200f)
     .uni (ModSource::Env2,     Param::evolveBend,     0.280f)
     .bi  (ModSource::LFO1,     Param::evolveBendPivot, 0.120f)
     .uni (ModSource::Velocity, Param::waveMorph,      0.280f)
     .uni (ModSource::Velocity, Param::shapeExcite,    0.200f)
     .bi  (ModSource::KeyTrack, Param::evolveBendRange, -0.200f)
     .uni (ModSource::Macro1,   Param::evolveMotion,   0.300f)
     .uni (ModSource::Macro1,   Param::evolveSpeed,    0.220f)
     .uni (ModSource::Macro2,   Param::wavePosition,   0.320f)
     .uni (ModSource::Macro2,   Param::shapeExcite,    0.240f)
     .uni (ModSource::Macro3,   Param::spaceMix,       0.260f)
     .uni (ModSource::Macro4,   Param::evolveBend,     0.360f)
     .uni (ModSource::Macro4,   Param::evolveBendRange, 0.260f);
    sharedMacros (r, Param::ampDecay, Param::evolveBendCurve);
    r.commit (s);
}});

//--------------------------------------------------------------------------
// The kiln floor is CUSTOM: material B pulls the structure nowhere, so FORM
// alone says what the object is, and a slow envelope walks it from a harmonic
// set to a stochastic one while the note is down. A noise strike lights it.
manager.addFactory ({ "Kiln Floor", "BASS", { "hollow", "evolving", "wooden", "close", "low" }, [] (PatchState& s)
{
    impact (s, 3 /* NOISE STRIKE */, 0.50f, 0.44f, 0.36f, 0.72f, 0.45f, 0.20f);
    amp (s, 0.003f, 1.05f, 0.88f, 0.32f, 0.35f);
    set (s, Param::masterGain, -0.5f);
    shape (s, 0.40f, 0.14f, 0.72f, 0.44f, 0.56f, 0.32f);
    material (s, MaterialType::Wood, MaterialType::Custom, 0.62f);
    topology (s, 3 /* LATTICE */, 0.38f, 0.46f, 15013);
    matter (s, 0.86f, 0.52f, 0.44f, 0.36f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.20f, 0.54f, 0.12f, 0.0f, 0.22f, 0.20f);
    set (s, Param::evolveMagnetTarget, 6 /* CUSTOM */);
    set (s, Param::masterMode, 1 /* MONO */);
    set (s, Param::masterGlide, 0.04f);
    space (s, SpacePresets::Chamber, 0.16f, 0.30f, 0.46f, 0.20f);

    FractureShape f;
    f.fragments = 8;
    f.delayLow = 0.04f; f.delayHigh = 0.26f;
    f.feedbackLow = 0.10f; f.feedbackHigh = 0.28f;
    f.decayLow = 0.32f; f.decayHigh = 0.50f;
    f.spreadLow = 0.12f; f.spreadHigh = 0.50f;
    f.gainLow = 0.55f; f.gainHigh = 0.95f;
    f.panWidth = 0.30f;
    f.pitchCycle = kFifthTerrace;
    f.pattern = "XHXLXHXL";
    fracture (s, 0 /* SPECTRAL */, 0.48f, 0.34f, 0.34f, 0.30f, 0.18f, 0.20f, 0.42f, 0.66f, 0.14f,
              0 /* 8 */, 4 /* 1/16 */, 8, 0.0f, 0 /* FORWARD */, 1.0f, 0.12f, 16001, f);

    env (s, 1, 0.001f, 0.32f, 0.12f, 0.22f, 0.3f);
    env (s, 2, 0.50f, 1.80f, 0.85f, 0.60f, 0.5f);
    macros (s, 0.35f, 0.40f, 0.25f, 0.55f);

    Routings r;
    r.uni (ModSource::Env1,     Param::shapeSurface,   0.220f)
     .uni (ModSource::Env2,     Param::shapeForm,      0.420f)
     .uni (ModSource::Velocity, Param::shapeStrike,    0.300f)
     .uni (ModSource::Velocity, Param::impactHardness, 0.240f)
     .bi  (ModSource::KeyTrack, Param::shapeForm,      0.180f)
     .uni (ModSource::NoteRandom, Param::impactRandom, 0.160f)
     .uni (ModSource::Macro1,   Param::evolveMotion,   0.280f)
     .uni (ModSource::Macro1,   Param::evolveSpeed,    0.220f)
     .uni (ModSource::Macro2,   Param::impactBrightness, 0.340f)
     .uni (ModSource::Macro2,   Param::fractureTone,   0.240f)
     .uni (ModSource::Macro3,   Param::spaceMix,       0.260f)
     .uni (ModSource::Macro4,   Param::shapeForm,      0.300f)
     .uni (ModSource::Macro4,   Param::shapeBlend,     0.260f);
    sharedMacros (r, Param::fractureDecay, Param::fractureRandom);
    r.commit (s);
}});

//--------------------------------------------------------------------------
// FREEZE is the patch. One damped-sine strike into a crystal-void object, and
// then the damping is floored: the piling cannot stop ringing, so what you
// hold under the note is the strike itself, sustained until you let go.
manager.addFactory ({ "Frozen Piling", "BASS", { "cold", "static", "struck", "close", "sub" }, [] (PatchState& s)
{
    impact (s, 5 /* DAMPED SINE */, 0.36f, 0.34f, 0.50f, 0.74f, 0.40f, 0.10f);
    amp (s, 0.004f, 1.10f, 0.88f, 0.30f, 0.35f);
    set (s, Param::masterGain, -6.0f);
    shape (s, 0.32f, 0.30f, 0.76f, 0.44f, 0.46f, 0.12f);
    material (s, MaterialType::Crystal, MaterialType::Void, 0.50f);
    topology (s, 2 /* CLUSTERS */, 0.28f, 0.40f, 17011);
    matter (s, 0.92f, 0.48f, 0.56f, 0.24f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.34f, 0.52f, 0.06f, 0.0f, 0.08f, 0.10f);
    set (s, Param::evolveFreeze, 1.0f);
    set (s, Param::evolveMagnetTarget, 0 /* OCTAVE */);
    set (s, Param::masterMode, 1 /* MONO */);
    set (s, Param::masterGlide, 0.02f);
    space (s, SpacePresets::Nebula, 0.18f, 0.36f, 0.42f, 0.20f);

    env (s, 1, 0.001f, 0.34f, 0.10f, 0.24f, 0.25f);
    lfo (s, 1, 0.30f, 0 /* SINE */, 1.0f, true, 0.60f);
    macros (s, 0.15f, 0.40f, 0.25f, 0.50f);

    Routings r;
    r.uni (ModSource::Env1,       Param::shapeSurface,   0.200f)
     .bi  (ModSource::LFO1,       Param::spaceTone,      0.120f)
     .uni (ModSource::Velocity,   Param::shapeStrike,    0.340f)
     .uni (ModSource::Velocity,   Param::impactBrightness, 0.240f)
     .uni (ModSource::NoteRandom, Param::impactHardness, 0.180f)
     .bi  (ModSource::KeyTrack,   Param::impactLength,  -0.220f)
     .uni (ModSource::Macro1,     Param::evolveMotion,   0.260f)
     .uni (ModSource::Macro1,     Param::evolveSpeed,    0.200f)
     .uni (ModSource::Macro2,     Param::shapeExcite,    0.340f)
     .uni (ModSource::Macro2,     Param::spaceTone,      0.220f)
     .uni (ModSource::Macro3,     Param::spaceMix,       0.260f)
     .uni (ModSource::Macro4,     Param::evolveMagnet,   0.320f)
     .uni (ModSource::Macro4,     Param::shapeCoupling,  0.220f);
    sharedMacros (r, Param::ampDecay, Param::evolveGravity);
    r.commit (s);
}});

//--------------------------------------------------------------------------
// The bottom of the category: an organic-void chain two octaves down with the
// mass right up and almost nothing above the second partial. It is not a
// sound so much as a floor — put anything at all on top of it and it works.
manager.addFactory ({ "Peat Bottom", "BASS", { "dark", "soft", "warm", "dry", "sub" }, [] (PatchState& s)
{
    wave (s, 0 /* BASIC */, 0.10f, 0.06f, 0.0f, 1, 0.02f, 0.06f, -1);
    amp (s, 0.008f, 1.20f, 0.92f, 0.34f, 0.4f);
    set (s, Param::masterGain, -2.0f);
    shape (s, 0.20f, 0.04f, 0.86f, 0.28f, 0.52f, 0.16f);
    material (s, MaterialType::Organic, MaterialType::Void, 0.30f);
    topology (s, 0 /* CHAIN */, 0.22f, 0.26f, 18013);
    matter (s, 0.78f, 0.46f, 0.22f, 0.18f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.40f, 0.66f, 0.0f, 0.0f, 0.10f, 0.06f);
    set (s, Param::evolveMagnetTarget, 0 /* OCTAVE */);
    set (s, Param::masterMode, 2 /* LEGATO */);
    set (s, Param::masterGlide, 0.10f);
    space (s, SpacePresets::Chamber, 0.08f, 0.18f, 0.34f, 0.12f);
    set (s, Param::spaceEqHigh, -4.0f);

    env (s, 1, 0.006f, 0.50f, 0.40f, 0.30f, 0.35f);
    lfo (s, 1, 0.20f, 0 /* SINE */, 1.0f, true, 0.80f);
    macros (s, 0.15f, 0.35f, 0.15f, 0.45f);

    Routings r;
    r.uni (ModSource::Env1,     Param::shapeExcite,   0.220f)
     .bi  (ModSource::LFO1,     Param::shapeMass,     0.050f)
     .uni (ModSource::Velocity, Param::shapeExcite,   0.260f)
     .uni (ModSource::Velocity, Param::waveMorph,     0.180f)
     .bi  (ModSource::KeyTrack, Param::shapeMass,    -0.240f)
     .uni (ModSource::Macro1,   Param::evolveMotion,  0.240f)
     .uni (ModSource::Macro1,   Param::lfo1Rate,      0.060f)
     .uni (ModSource::Macro2,   Param::shapeExcite,   0.340f)
     .uni (ModSource::Macro2,   Param::spaceEqHigh,   6.000f)
     .uni (ModSource::Macro3,   Param::spaceMix,      0.220f)
     .uni (ModSource::Macro4,   Param::shapeMass,     0.240f)
     .uni (ModSource::Macro4,   Param::shapeSurface,  0.260f);
    sharedMacros (r, Param::ampDecay, Param::shapeDistribution);
    r.commit (s);
}});
}

} // namespace am::FactoryContent
