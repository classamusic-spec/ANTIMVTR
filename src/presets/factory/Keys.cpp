#include "presets/FactoryBuilders.h"

namespace am::FactoryContent
{

namespace
{
    /** LAYER exactly two sources: every source the patch did not ask for is silenced. */
    void layerOnly (PatchState& s, Param a, Param b)
    {
        set (s, Param::sourceMode, 1.0f /* LAYER */);
        for (const Param p : { Param::waveLevel, Param::dustLevel, Param::impactLevel,
                               Param::sampleLevel, Param::gestureLevel })
            if (p != a && p != b)
                set (s, p, 0.0f);
    }
}

void registerKeys (PresetManager& manager)
{
//==========================================================================
// KEYS — struck and played, an object under the fingers
//==========================================================================

manager.addFactory ({ "Crystal Ghost", "KEYS", { "keys", "bells", "crystal", "bright" }, [] (PatchState& s)
{
    impact (s, 4 /* METAL STRIKE */, 0.72f, 0.68f, 0.18f, 0.80f, 0.45f, 0.12f);
    amp (s, 0.001f, 1.60f, 0.0f, 1.40f, 0.35f);
    shape (s, 0.46f, 0.84f, 0.24f, 0.68f, 0.82f, 0.10f);
    material (s, MaterialType::Crystal, MaterialType::Void, 0.30f);
    topology (s, 5 /* STAR */, 0.28f, 0.62f, 3);
    matter (s, 0.98f, 0.62f, 0.72f, 0.70f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.52f, 0.44f, 0.08f, 0.0f, 0.25f, 0.12f);
    set (s, Param::evolveMagnetTarget, 5 /* SCALE */);
    space (s, SpacePresets::Shimmer, 0.40f, 0.60f, 0.62f, 0.30f);

    env (s, 1, 0.001f, 0.90f, 0.0f, 0.90f, 0.3f);
    lfo (s, 1, 0.24f, 0 /* SINE */, 1.0f, true, 0.6f);
    macros (s, 0.25f, 0.45f, 0.40f, 0.35f);

    Routings r;
    r.uni (ModSource::Env1,   Param::shapeSurface,   0.200f)
     .bi  (ModSource::LFO1,   Param::shapeTension,   0.030f)
     .uni (ModSource::Velocity, Param::impactBrightness, 0.320f)
     .uni (ModSource::Velocity, Param::shapeStrike,  0.200f)
     .bi  (ModSource::KeyTrack, Param::shapeDecay,  -0.180f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.300f)
     .uni (ModSource::Macro2, Param::impactBrightness, 0.350f)
     .uni (ModSource::Macro2, Param::shapeExcite,    0.250f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.320f)
     .uni (ModSource::Macro4, Param::evolveMagnet,   0.350f)
     .uni (ModSource::Macro4, Param::shapeTension,   0.200f);
    sharedMacros (r, Param::spaceSize, Param::impactRandom);
    r.commit (s);
}});

manager.addFactory ({ "Dust Piano", "KEYS", { "keys", "piano", "noisy", "intimate" }, [] (PatchState& s)
{
    dust (s, 5 /* CRACKLE */, 0.68f, 0.55f, 0.22f, 0.45f, 0.35f, 0.55f, 17);
    amp (s, 0.002f, 1.10f, 0.14f, 0.90f, 0.35f);
    shape (s, 0.52f, 0.14f, 0.44f, 0.56f, 0.60f, 0.20f);
    material (s, MaterialType::String, MaterialType::Wood, 0.42f);
    topology (s, 0 /* CHAIN */, 0.42f, 0.52f, 41);
    matter (s, 0.96f, 0.58f, 0.45f, 0.55f);
    evolve (s, 0.0f, 0.10f, 0.0f, 0.30f, 0.50f, 0.14f, 0.0f, 0.20f, 0.18f);
    set (s, Param::evolveMagnetTarget, 6 /* CUSTOM (harmonic) */);
    space (s, SpacePresets::Chamber, 0.30f, 0.35f, 0.52f, 0.22f);

    env (s, 1, 0.001f, 0.60f, 0.0f, 0.50f, 0.3f);
    env (s, 2, 0.30f, 2.50f, 0.30f, 2.0f);
    chaos (s, 1, 0 /* WALK */, 0.60f, 0.35f, 0.85f, 0.5f, 59);
    macros (s, 0.20f, 0.40f, 0.30f, 0.45f);

    Routings r;
    r.uni (ModSource::Env1,   Param::dustDensity,    0.300f)
     .uni (ModSource::Env2,   Param::shapeSurface,   0.150f)
     .bi  (ModSource::Chaos1, Param::dustColor,      0.100f)
     .uni (ModSource::Velocity, Param::dustGrain,    0.260f)
     .uni (ModSource::Velocity, Param::shapeStrike,  0.250f)
     .bi  (ModSource::KeyTrack, Param::shapeDecay,  -0.200f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.300f)
     .uni (ModSource::Macro2, Param::dustColor,      0.320f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.280f)
     .uni (ModSource::Macro4, Param::dustDensity,    0.350f)
     .uni (ModSource::Macro4, Param::dustGrain,      0.250f);
    sharedMacros (r, Param::ampDecay, Param::dustJitter);
    r.commit (s);
}});

manager.addFactory ({ "Ash Keys", "KEYS", { "keys", "electric", "soft", "vintage" }, [] (PatchState& s)
{
    impact (s, 2 /* PLUCK */, 0.38f, 0.42f, 0.26f, 0.72f, 0.55f, 0.08f, 0.0f, 0.85f);
    amp (s, 0.002f, 1.30f, 0.22f, 0.80f, 0.45f);
    shape (s, 0.34f, 0.16f, 0.48f, 0.50f, 0.58f, 0.30f);
    material (s, MaterialType::Metal, MaterialType::Wood, 0.60f);
    topology (s, 2 /* CLUSTERS */, 0.36f, 0.48f, 71);
    matter (s, 0.94f, 0.48f, 0.50f, 0.50f);
    evolve (s, 0.10f, 0.0f, 0.0f, 0.40f, 0.52f, 0.06f, 0.0f, 0.18f, 0.14f);
    set (s, Param::evolveBendPivot, 0.35f);
    set (s, Param::evolveMagnetTarget, 6 /* CUSTOM (harmonic) */);
    space (s, SpacePresets::Chamber, 0.28f, 0.40f, 0.50f, 0.25f);
    set (s, Param::spaceChorusOn, 1.0f);
    set (s, Param::spaceChorusRate, 0.55f);
    set (s, Param::spaceChorusDepth, 0.35f);
    set (s, Param::spaceChorusMix, 0.35f);

    env (s, 1, 0.001f, 0.70f, 0.0f, 0.60f, 0.35f);
    lfo (s, 1, 4.20f, 0 /* SINE */, 1.0f, true, 0.8f);
    macros (s, 0.20f, 0.40f, 0.30f, 0.35f);

    Routings r;
    r.uni (ModSource::Env1,   Param::shapeExcite,    0.220f)
     .bi  (ModSource::LFO1,   Param::shapePitch,     0.004f)
     .uni (ModSource::Velocity, Param::impactHardness, 0.300f)
     .uni (ModSource::Velocity, Param::shapeSurface, 0.180f)
     .bi  (ModSource::KeyTrack, Param::shapeMass,   -0.140f)
     .uni (ModSource::Macro1, Param::lfo1Depth,      0.350f)
     .uni (ModSource::Macro2, Param::impactBrightness, 0.350f)
     .uni (ModSource::Macro2, Param::shapeTension,   0.150f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.300f)
     .uni (ModSource::Macro4, Param::evolveBend,     0.250f)
     .uni (ModSource::Macro4, Param::shapeSurface,   0.200f);
    sharedMacros (r, Param::ampDecay, Param::impactRandom);
    r.commit (s);
}});

//--------------------------------------------------------------------------
// The tuned bars: wood, aluminium, glass and steel under a mallet, then the
// bells and the tines. Everything here answers the hand before the ear.
//--------------------------------------------------------------------------

manager.addFactory ({ "Rosewood Marimba", "KEYS", { "wooden", "warm", "struck", "close", "melodic" }, [] (PatchState& s)
{
    impact (s, 3 /* NOISE STRIKE */, 0.55f, 0.42f, 0.10f, 0.85f, 0.45f, 0.10f);
    amp (s, 0.006f, 0.60f, 0.0f, 0.30f, 0.30f);
    shape (s, 0.34f, 0.30f, 0.50f, 0.42f, 0.58f, 0.22f);
    material (s, MaterialType::Wood, MaterialType::Organic, 0.35f);
    topology (s, 2 /* CLUSTERS */, 0.34f, 0.35f, 23);
    matter (s, 0.92f, 0.60f, 0.46f, 0.45f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.46f, 0.05f, 0.0f, 0.20f, 0.10f);
    set (s, Param::masterTranspose, 12.0f);   // a marimba sits an octave above the piano
    space (s, SpacePresets::Nebula, 0.20f, 0.45f, 0.50f, 0.18f);

    env (s, 1, 0.001f, 0.35f, 0.0f, 0.30f, 0.30f);
    macros (s, 0.15f, 0.35f, 0.25f, 0.30f);

    Routings r;
    r.uni (ModSource::Env1,        Param::shapeSurface,     0.150f)
     .uni (ModSource::Velocity,    Param::impactBrightness, 0.300f)
     .uni (ModSource::Velocity,    Param::impactHardness,   0.250f)
     .uni (ModSource::Velocity,    Param::shapeBlend,       0.220f)
     .uni (ModSource::Velocity,    Param::shapeSurface,     0.150f)
     .bi  (ModSource::NoteRandom,  Param::shapeMass,        0.040f)
     .bi  (ModSource::NoteRandom,  Param::shapePitch,       0.004f)
     .bi  (ModSource::KeyTrack,    Param::shapeDecay,      -0.200f)
     .uni (ModSource::Macro1,      Param::evolveMotion,     0.300f)
     .uni (ModSource::Macro1,      Param::evolveScatter,    0.300f)
     .uni (ModSource::Macro1,      Param::shapeDistribution, 0.220f)
     .uni (ModSource::Macro2,      Param::impactBrightness, 0.350f)
     .uni (ModSource::Macro2,      Param::shapeMass,       -0.200f)
     .uni (ModSource::Macro3,      Param::spaceMix,         0.300f)
     .uni (ModSource::Macro3,      Param::spaceSize,        0.250f)
     .uni (ModSource::Macro4,      Param::shapeSurface,     0.300f)
     .uni (ModSource::Macro4,      Param::impactLength,     0.220f);
    sharedMacros (r, Param::ampDecay, Param::impactRandom);
    r.commit (s);
}});

manager.addFactory ({ "Motor Vibraphone", "KEYS", { "metallic", "clean", "struck", "pulsing", "roomy" }, [] (PatchState& s)
{
    impact (s, 3 /* NOISE STRIKE */, 0.30f, 0.36f, 0.14f, 0.80f, 0.50f, 0.06f);
    amp (s, 0.002f, 1.80f, 0.32f, 1.30f, 0.40f);
    shape (s, 0.28f, 0.50f, 0.38f, 0.50f, 0.66f, 0.08f);
    material (s, MaterialType::Metal, MaterialType::Crystal, 0.45f);
    topology (s, 3 /* LATTICE */, 0.22f, 0.30f, 101);
    matter (s, 1.0f, 0.45f, 0.50f, 0.60f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.52f, 0.04f, 0.0f, 0.15f, 0.10f);
    space (s, SpacePresets::Chamber, 0.34f, 0.55f, 0.55f, 0.20f);

    env (s, 1, 0.001f, 0.55f, 0.0f, 0.40f, 0.30f);
    lfo (s, 1, 4.60f, 0 /* SINE */, 1.0f, true, 0.45f);
    macros (s, 0.30f, 0.35f, 0.30f, 0.25f);

    Routings r;
    r.bi  (ModSource::LFO1,        Param::ampSustain,       0.200f)
     .bi  (ModSource::LFO1,        Param::shapePitch,       0.002f)
     .uni (ModSource::Env1,        Param::shapeSurface,     0.150f)
     .uni (ModSource::Velocity,    Param::impactBrightness, 0.300f)
     .uni (ModSource::Velocity,    Param::shapeBlend,       0.220f)
     .uni (ModSource::Velocity,    Param::shapeMass,       -0.150f)
     .bi  (ModSource::NoteRandom,  Param::shapeSurface,     0.050f)
     .bi  (ModSource::KeyTrack,    Param::shapeDecay,      -0.160f)
     .uni (ModSource::Macro1,      Param::lfo1Depth,        0.400f)
     .uni (ModSource::Macro1,      Param::lfo1Rate,         0.060f)
     .uni (ModSource::Macro1,      Param::evolveScatter,    0.300f)
     .uni (ModSource::Macro2,      Param::impactBrightness, 0.300f)
     .uni (ModSource::Macro2,      Param::shapeMass,       -0.200f)
     .uni (ModSource::Macro3,      Param::spaceMix,         0.300f)
     .uni (ModSource::Macro3,      Param::spaceSize,         0.250f)
     .uni (ModSource::Macro4,      Param::shapeTension,     0.180f)
     .uni (ModSource::Macro4,      Param::impactHardness,   0.250f);
    sharedMacros (r, Param::spaceSize, Param::impactRandom);
    r.commit (s);
}});

manager.addFactory ({ "Glass Celeste", "KEYS", { "glassy", "bright", "struck", "high", "wide" }, [] (PatchState& s)
{
    sample (s, BuiltInSamples::Kind::GlassStrike, 0 /* ONE SHOT */, 0.0f, 0.55f, 0.30f, 0.35f, 72, 0.75f);
    amp (s, 0.001f, 1.10f, 0.0f, 0.90f, 0.35f);
    shape (s, 0.30f, 0.83f, 0.24f, 0.58f, 0.60f, 0.06f);
    material (s, MaterialType::Crystal, MaterialType::Void, 0.30f);
    topology (s, 5 /* STAR */, 0.20f, 0.55f, 211);
    matter (s, 0.85f, 0.50f, 0.50f, 0.75f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.35f, 0.50f, 0.06f, 0.0f, 0.20f, 0.14f);
    set (s, Param::evolveMagnetTarget, 5 /* SCALE */);
    space (s, SpacePresets::Shimmer, 0.36f, 0.55f, 0.62f, 0.25f);

    env (s, 1, 0.001f, 0.80f, 0.0f, 0.60f, 0.30f);
    macros (s, 0.20f, 0.40f, 0.35f, 0.30f);

    Routings r;
    r.uni (ModSource::Env1,        Param::evolveMagnet,     0.150f)
     .uni (ModSource::Velocity,    Param::sampleLevel,      0.200f)
     .uni (ModSource::Velocity,    Param::shapeBlend,       0.220f)
     .uni (ModSource::Velocity,    Param::shapeSurface,     0.120f)
     .bi  (ModSource::NoteRandom,  Param::shapePitch,       0.005f)
     .bi  (ModSource::NoteRandom,  Param::sampleStart,      0.030f)
     .bi  (ModSource::KeyTrack,    Param::shapeDecay,      -0.220f)
     .uni (ModSource::Macro1,      Param::evolveMotion,     0.300f)
     .uni (ModSource::Macro1,      Param::evolveScatter,    0.300f)
     .uni (ModSource::Macro1,      Param::evolveSpeed,      0.250f)
     .uni (ModSource::Macro2,      Param::shapeMass,       -0.200f)
     .uni (ModSource::Macro2,      Param::spaceTone,        0.250f)
     .uni (ModSource::Macro3,      Param::spaceMix,         0.300f)
     .uni (ModSource::Macro3,      Param::spaceSize,        0.200f)
     .uni (ModSource::Macro4,      Param::evolveMagnet,     0.300f)
     .uni (ModSource::Macro4,      Param::shapeBlend,       0.250f);
    sharedMacros (r, Param::spaceFeedback, Param::evolveTear);
    r.commit (s);
}});

manager.addFactory ({ "Hollow Steel", "KEYS", { "metallic", "warm", "struck", "hollow", "close" }, [] (PatchState& s)
{
    impact (s, 6 /* MEMBRANE HIT */, 0.35f, 0.30f, 0.25f, 0.80f, 0.50f, 0.12f);
    amp (s, 0.009f, 1.90f, 0.0f, 1.30f, 0.35f);
    shape (s, 0.38f, 0.40f, 0.37f, 0.46f, 0.58f, 0.14f);
    material (s, MaterialType::Metal, MaterialType::Membrane, 0.42f);
    topology (s, 1 /* RING */, 0.42f, 0.40f, 37);
    matter (s, 0.97f, 0.52f, 0.40f, 0.50f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.58f, 0.06f, 0.0f, 0.20f, 0.12f);
    space (s, SpacePresets::Orbit, 0.26f, 0.45f, 0.45f, 0.30f);

    env (s, 1, 0.001f, 0.70f, 0.0f, 0.50f, 0.30f);
    macros (s, 0.20f, 0.30f, 0.25f, 0.35f);

    Routings r;
    r.uni (ModSource::Env1,        Param::shapeSurface,     0.120f)
     .uni (ModSource::Velocity,    Param::impactHardness,   0.300f)
     .uni (ModSource::Velocity,    Param::shapeBlend,       0.220f)
     .uni (ModSource::Velocity,    Param::shapeMass,       -0.150f)
     .bi  (ModSource::NoteRandom,  Param::shapeMass,        0.050f)
     .bi  (ModSource::NoteRandom,  Param::shapePitch,       0.006f)
     .bi  (ModSource::KeyTrack,    Param::shapeDecay,      -0.180f)
     .bi  (ModSource::KeyTrack,    Param::shapeMass,        0.180f)
     .uni (ModSource::Macro1,      Param::evolveMotion,     0.300f)
     .uni (ModSource::Macro1,      Param::evolveScatter,    0.300f)
     .uni (ModSource::Macro1,      Param::evolveSpeed,      0.250f)
     .uni (ModSource::Macro2,      Param::impactBrightness, 0.350f)
     .uni (ModSource::Macro2,      Param::shapeTension,     0.150f)
     .uni (ModSource::Macro2,      Param::shapeMass,       -0.200f)
     .uni (ModSource::Macro3,      Param::spaceMix,         0.300f)
     .uni (ModSource::Macro3,      Param::spaceSize,         0.250f)
     .uni (ModSource::Macro4,      Param::evolveGravity,    0.250f)
     .uni (ModSource::Macro4,      Param::shapeMass,        0.200f);
    sharedMacros (r, Param::ampDecay, Param::impactRandom);
    r.commit (s);
}});

manager.addFactory ({ "Bell Choir", "KEYS", { "metallic", "bright", "struck", "huge", "chords" }, [] (PatchState& s)
{
    impact (s, 4 /* METAL STRIKE */, 0.68f, 0.50f, 0.40f, 0.80f, 0.40f, 0.15f);
    amp (s, 0.001f, 2.40f, 0.0f, 2.00f, 0.30f);
    shape (s, 0.45f, 0.72f, 0.38f, 0.55f, 0.70f, 0.10f);
    material (s, MaterialType::Metal, MaterialType::Crystal, 0.55f);
    topology (s, 2 /* CLUSTERS */, 0.30f, 0.62f, 149);
    matter (s, 1.0f, 0.58f, 0.56f, 0.80f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.78f, 0.44f, 0.08f, 0.0f, 0.22f, 0.22f);
    set (s, Param::evolveMagnetTarget, 5 /* SCALE */);
    FractureShape rack;
    rack.fragments   = 16;
    rack.delayLow    = 0.06f; rack.delayHigh    = 0.34f;
    rack.feedbackLow = 0.12f; rack.feedbackHigh = 0.36f;
    rack.decayLow    = 0.40f; rack.decayHigh    = 0.65f;
    rack.spreadLow   = 0.35f; rack.spreadHigh   = 0.95f;
    rack.panWidth    = 0.80f;
    rack.probability = 0.85f;
    rack.pitchCycle  = kOctaveTerrace;
    rack.pattern     = "XLXHXLXH";
    fracture (s, 0 /* SPECTRAL */, 0.35f, 0.24f, 0.70f, 0.25f, 0.25f, 0.30f, 0.55f, 0.55f, 0.06f,
              1 /* 16 */, 3, 8, 0.0f, 0, 0.85f, 0.20f, 163, rack);

    space (s, SpacePresets::Void, 0.36f, 0.70f, 0.45f, 0.28f);

    env (s, 1, 0.40f, 2.20f, 0.20f, 1.60f, 0.50f);
    macros (s, 0.25f, 0.40f, 0.40f, 0.45f);

    Routings r;
    r.uni (ModSource::Env1,        Param::evolveMagnet,     0.250f)
     .uni (ModSource::Velocity,    Param::impactBrightness, 0.320f)
     .uni (ModSource::Velocity,    Param::shapeBlend,       0.220f)
     .uni (ModSource::Velocity,    Param::evolveMagnet,    -0.200f)
     .bi  (ModSource::NoteRandom,  Param::shapePitch,       0.008f)
     .bi  (ModSource::NoteRandom,  Param::shapeForm,        0.040f)
     .bi  (ModSource::KeyTrack,    Param::shapeDecay,      -0.200f)
     .bi  (ModSource::KeyTrack,    Param::fractureTone,     0.150f)
     .uni (ModSource::Macro1,      Param::evolveMotion,     0.350f)
     .uni (ModSource::Macro1,      Param::fractureEvolve,   0.250f)
     .uni (ModSource::Macro1,      Param::evolveScatter,    0.300f)
     .uni (ModSource::Macro2,      Param::impactBrightness, 0.300f)
     .uni (ModSource::Macro2,      Param::shapeMass,       -0.220f)
     .uni (ModSource::Macro2,      Param::spaceTone,        0.250f)
     .uni (ModSource::Macro3,      Param::spaceMix,         0.300f)
     .uni (ModSource::Macro3,      Param::spaceSize,        0.250f)
     .uni (ModSource::Macro4,      Param::evolveMagnet,     0.300f)
     .uni (ModSource::Macro4,      Param::shapeForm,        0.200f);
    sharedMacros (r, Param::ampDecay, Param::impactRandom);
    r.commit (s);
}});

manager.addFactory ({ "Temple Gong", "KEYS", { "metallic", "dark", "struck", "morphing", "huge" }, [] (PatchState& s)
{
    impact (s, 4 /* METAL STRIKE */, 0.50f, 0.35f, 0.60f, 0.75f, 0.55f, 0.20f, 0.0f, 0.90f);
    amp (s, 0.004f, 3.00f, 0.08f, 2.40f, 0.40f);
    shape (s, 0.62f, 0.55f, 0.55f, 0.40f, 0.72f, 0.28f);
    material (s, MaterialType::Metal, MaterialType::Membrane, 0.35f);
    topology (s, 4 /* RANDOM */, 0.55f, 0.70f, 401);
    matter (s, 1.0f, 0.62f, 0.54f, 0.85f);
    evolve (s, 0.0f, 0.42f, 0.0f, 0.0f, 0.55f, 0.12f, 0.0f, 0.12f, 0.35f);
    space (s, SpacePresets::Void, 0.42f, 0.75f, 0.35f, 0.35f);

    env (s, 2, 0.80f, 4.00f, 0.40f, 3.00f, 0.55f);
    lfo (s, 1, 0.18f, 1 /* TRIANGLE */, 1.0f, true, 0.8f);
    macros (s, 0.30f, 0.30f, 0.45f, 0.40f);

    Routings r;
    r.uni (ModSource::Env2,        Param::evolveMelt,       0.300f)
     .bi  (ModSource::LFO1,        Param::shapeTension,     0.030f)
     .uni (ModSource::Velocity,    Param::evolveMelt,      -0.180f)
     .uni (ModSource::Velocity,    Param::impactBrightness, 0.300f)
     .uni (ModSource::Velocity,    Param::shapeSurface,     0.250f)
     .bi  (ModSource::NoteRandom,  Param::shapePitch,       0.010f)
     .bi  (ModSource::KeyTrack,    Param::shapeMass,       -0.200f)
     .uni (ModSource::Macro1,      Param::evolveMotion,     0.350f)
     .uni (ModSource::Macro1,      Param::evolveSpeed,      0.200f)
     .uni (ModSource::Macro1,      Param::evolveScatter,    0.300f)
     .uni (ModSource::Macro2,      Param::impactBrightness, 0.300f)
     .uni (ModSource::Macro2,      Param::shapeMass,       -0.200f)
     .uni (ModSource::Macro3,      Param::spaceMix,         0.300f)
     .uni (ModSource::Macro3,      Param::spaceSize,         0.250f)
     .uni (ModSource::Macro4,      Param::evolveMelt,       0.350f)
     .uni (ModSource::Macro4,      Param::shapeSurface,     0.200f)
     .uni (ModSource::Macro6,      Param::impactRandom,     0.300f);
    sharedMacros (r, Param::spaceSize, Param::evolveCrush);
    r.commit (s);
}});

manager.addFactory ({ "Tine Bark", "KEYS", { "metallic", "bright", "struck", "close", "melodic" }, [] (PatchState& s)
{
    wave (s, 1 /* HARMONIC */, 0.15f, 0.20f, 0.0f, 1, 0.05f, 0.30f, 0, 0.50f);
    impact (s, 1 /* CLICK */, 0.62f, 0.55f, 0.12f, 0.90f, 0.40f, 0.10f, 0.0f, 0.55f);
    layerOnly (s, Param::waveLevel, Param::impactLevel);
    amp (s, 0.003f, 2.00f, 0.14f, 1.10f, 0.45f);
    shape (s, 0.30f, 0.20f, 0.45f, 0.52f, 0.50f, 0.18f);
    material (s, MaterialType::Metal, MaterialType::String, 0.50f);
    topology (s, 0 /* CHAIN */, 0.30f, 0.40f, 53);
    matter (s, 0.80f, 0.50f, 0.55f, 0.40f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.50f, 0.05f, 0.0f, 0.18f, 0.10f);
    space (s, SpacePresets::Chamber, 0.26f, 0.35f, 0.50f, 0.20f);
    set (s, Param::spaceChorusOn, 1.0f);
    set (s, Param::spaceChorusRate, 0.45f);
    set (s, Param::spaceChorusDepth, 0.30f);
    set (s, Param::spaceChorusMix, 0.30f);

    env (s, 1, 0.001f, 0.45f, 0.0f, 0.35f, 0.30f);
    lfo (s, 1, 5.20f, 0 /* SINE */, 1.0f, true, 0.9f);
    macros (s, 0.20f, 0.40f, 0.25f, 0.40f);

    Routings r;
    r.uni (ModSource::Env1,        Param::shapeExcite,      0.180f)
     .bi  (ModSource::LFO1,        Param::shapePitch,       0.004f)
     .uni (ModSource::Velocity,    Param::impactLevel,      0.350f)
     .uni (ModSource::Velocity,    Param::impactBrightness, 0.300f)
     .uni (ModSource::Velocity,    Param::shapeBlend,       0.220f)
     .uni (ModSource::Velocity,    Param::waveMorph,        0.150f)
     .bi  (ModSource::NoteRandom,  Param::waveDetune,       0.050f)
     .bi  (ModSource::KeyTrack,    Param::shapeDecay,      -0.200f)
     .uni (ModSource::Macro1,      Param::lfo1Depth,        0.350f)
     .uni (ModSource::Macro1,      Param::evolveScatter,    0.300f)
     .uni (ModSource::Macro1,      Param::evolveSpeed,      0.250f)
     .uni (ModSource::Macro2,      Param::impactBrightness, 0.300f)
     .uni (ModSource::Macro2,      Param::wavePosition,     0.250f)
     .uni (ModSource::Macro2,      Param::shapeMass,       -0.200f)
     .uni (ModSource::Macro3,      Param::spaceMix,         0.300f)
     .uni (ModSource::Macro3,      Param::spaceSize,         0.250f)
     .uni (ModSource::Macro4,      Param::impactLevel,      0.250f)
     .uni (ModSource::Macro4,      Param::shapeMix,         0.200f);
    sharedMacros (r, Param::ampDecay, Param::impactRandom);
    r.commit (s);
}});

manager.addFactory ({ "Wet Tine", "KEYS", { "organic", "warm", "struck", "unstable", "wide" }, [] (PatchState& s)
{
    impact (s, 2 /* PLUCK */, 0.45f, 0.40f, 0.20f, 0.80f, 0.50f, 0.08f);
    amp (s, 0.002f, 1.50f, 0.08f, 1.00f, 0.45f);
    shape (s, 0.50f, 0.95f, 0.44f, 0.48f, 0.62f, 0.40f);
    material (s, MaterialType::Liquid, MaterialType::Metal, 0.40f);
    topology (s, 2 /* CLUSTERS */, 0.45f, 0.50f, 613);
    matter (s, 0.95f, 0.60f, 0.30f, 0.65f);
    evolve (s, 0.12f, 0.0f, 0.0f, 0.0f, 0.50f, 0.08f, 0.0f, 0.25f, 0.20f);
    set (s, Param::evolveBendPivot, 0.40f);
    set (s, Param::evolveBendRange, 0.30f);
    space (s, SpacePresets::Dream, 0.38f, 0.60f, 0.50f, 0.28f);
    set (s, Param::spaceChorusOn, 1.0f);
    set (s, Param::spaceChorusRate, 0.35f);
    set (s, Param::spaceChorusDepth, 0.50f);
    set (s, Param::spaceChorusMix, 0.45f);

    env (s, 1, 0.001f, 0.90f, 0.0f, 0.70f, 0.35f);
    lfo (s, 1, 0.28f, 5 /* SMOOTH RANDOM */, 1.0f, true, 0.5f);
    macros (s, 0.30f, 0.35f, 0.35f, 0.40f);

    Routings r;
    r.uni (ModSource::Env1,        Param::shapeSurface,     0.150f)
     .bi  (ModSource::LFO1,        Param::shapeTension,     0.040f)
     .uni (ModSource::Velocity,    Param::shapeSurface,     0.300f)
     .uni (ModSource::Velocity,    Param::impactBrightness, 0.280f)
     .uni (ModSource::Velocity,    Param::shapeBlend,       0.220f)
     .bi  (ModSource::NoteRandom,  Param::shapePitch,       0.012f)
     .bi  (ModSource::NoteRandom,  Param::shapeSurface,     0.080f)
     .bi  (ModSource::KeyTrack,    Param::shapeDecay,      -0.180f)
     .uni (ModSource::Macro1,      Param::evolveMotion,     0.300f)
     .uni (ModSource::Macro1,      Param::spaceChorusRate,  0.200f)
     .uni (ModSource::Macro1,      Param::evolveScatter,    0.300f)
     .uni (ModSource::Macro2,      Param::impactBrightness, 0.300f)
     .uni (ModSource::Macro2,      Param::shapeMass,       -0.200f)
     .uni (ModSource::Macro3,      Param::spaceMix,         0.300f)
     .uni (ModSource::Macro3,      Param::spaceSize,         0.250f)
     .uni (ModSource::Macro4,      Param::shapeSurface,     0.300f)
     .uni (ModSource::Macro4,      Param::shapeBlend,       0.250f);
    sharedMacros (r, Param::ampDecay, Param::impactRandom);
    r.commit (s);
}});

//--------------------------------------------------------------------------
// Struck wire: hammers, tangents and the things players lay across the
// strings to stop them behaving. Fracture is the preparation, not an effect.
//--------------------------------------------------------------------------

manager.addFactory ({ "Felt Upright", "KEYS", { "wooden", "soft", "struck", "close", "chords" }, [] (PatchState& s)
{
    impact (s, 3 /* NOISE STRIKE */, 0.22f, 0.28f, 0.18f, 0.90f, 0.55f, 0.08f);
    amp (s, 0.012f, 2.40f, 0.0f, 0.30f, 0.40f);
    shape (s, 0.42f, 0.12f, 0.44f, 0.56f, 0.52f, 0.12f);
    material (s, MaterialType::String, MaterialType::Wood, 0.30f);
    topology (s, 3 /* LATTICE */, 0.38f, 0.30f, 71);
    matter (s, 0.94f, 0.52f, 0.55f, 0.35f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.50f, 0.03f, 0.0f, 0.15f, 0.08f);
    space (s, SpacePresets::Chamber, 0.18f, 0.34f, 0.40f, 0.12f);

    env (s, 1, 0.001f, 0.40f, 0.0f, 0.30f, 0.30f);
    macros (s, 0.15f, 0.30f, 0.20f, 0.30f);

    Routings r;
    r.uni (ModSource::Env1,        Param::shapeSurface,     0.150f)
     .uni (ModSource::Velocity,    Param::impactHardness,   0.350f)
     .uni (ModSource::Velocity,    Param::impactBrightness, 0.280f)
     .uni (ModSource::Velocity,    Param::shapeBlend,       0.220f)
     .uni (ModSource::Velocity,    Param::shapeMass,       -0.120f)
     .bi  (ModSource::NoteRandom,  Param::shapePitch,       0.005f)
     .bi  (ModSource::NoteRandom,  Param::shapeTension,     0.030f)
     .bi  (ModSource::KeyTrack,    Param::shapeDecay,      -0.240f)
     .bi  (ModSource::KeyTrack,    Param::shapeMass,       -0.180f)
     .uni (ModSource::Macro1,      Param::evolveMotion,     0.300f)
     .uni (ModSource::Macro1,      Param::evolveScatter,    0.300f)
     .uni (ModSource::Macro1,      Param::evolveSpeed,      0.250f)
     .uni (ModSource::Macro2,      Param::impactBrightness, 0.350f)
     .uni (ModSource::Macro2,      Param::shapeMass,       -0.200f)
     .uni (ModSource::Macro3,      Param::spaceMix,         0.320f)
     .uni (ModSource::Macro3,      Param::spaceSize,        0.250f)
     .uni (ModSource::Macro4,      Param::ampRelease,       0.350f)
     .uni (ModSource::Macro4,      Param::shapeDecay,       0.250f);
    sharedMacros (r, Param::ampDecay, Param::impactRandom);
    r.commit (s);
}});

manager.addFactory ({ "Prepared Grand", "KEYS", { "metallic", "dirty", "struck", "noisy", "close" }, [] (PatchState& s)
{
    impact (s, 3 /* NOISE STRIKE */, 0.50f, 0.45f, 0.12f, 0.85f, 0.50f, 0.15f, 0.0f, 0.75f);
    dust (s, 5 /* CRACKLE */, 0.55f, 0.62f, 0.10f, 0.60f, 0.50f, 0.50f, 331, 0.30f);
    layerOnly (s, Param::impactLevel, Param::dustLevel);
    amp (s, 0.002f, 0.90f, 0.0f, 0.50f, 0.35f);
    shape (s, 0.48f, 0.24f, 0.42f, 0.60f, 0.42f, 0.35f);
    material (s, MaterialType::String, MaterialType::Metal, 0.35f);
    topology (s, 4 /* RANDOM */, 0.50f, 0.45f, 809);
    matter (s, 0.90f, 0.55f, 0.60f, 0.55f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.50f, 0.15f, 0.0f, 0.35f, 0.20f);

    FractureShape rattle;
    rattle.fragments   = 16;
    rattle.delayLow    = 0.02f; rattle.delayHigh    = 0.14f;
    rattle.feedbackLow = 0.05f; rattle.feedbackHigh = 0.28f;
    rattle.decayLow    = 0.20f; rattle.decayHigh    = 0.38f;
    rattle.spreadLow   = 0.30f; rattle.spreadHigh   = 0.80f;
    rattle.panWidth    = 0.60f;
    rattle.probability = 0.80f;
    rattle.pattern     = "XLXHXLXH";
    fracture (s, 2 /* TRANSIENT */, 0.45f, 0.32f, 0.50f, 0.30f, 0.20f, 0.15f, 0.40f, 0.55f, 0.10f,
              1 /* 16 */, 5, 8, 0.0f, 0, 0.80f, 0.35f, 77, rattle);

    space (s, SpacePresets::Chamber, 0.24f, 0.35f, 0.50f, 0.20f);

    env (s, 1, 0.001f, 0.50f, 0.0f, 0.35f, 0.30f);
    macros (s, 0.25f, 0.35f, 0.25f, 0.40f);

    Routings r;
    r.uni (ModSource::Env1,        Param::dustDensity,      0.250f)
     .uni (ModSource::Velocity,    Param::dustLevel,        0.300f)
     .uni (ModSource::Velocity,    Param::impactBrightness, 0.300f)
     .uni (ModSource::Velocity,    Param::shapeBlend,       0.220f)
     .uni (ModSource::Velocity,    Param::fractureAmount,   0.250f)
     .bi  (ModSource::NoteRandom,  Param::dustColor,        0.150f)
     .bi  (ModSource::NoteRandom,  Param::shapeSurface,     0.080f)
     .bi  (ModSource::KeyTrack,    Param::shapeDecay,      -0.200f)
     .uni (ModSource::Macro1,      Param::evolveMotion,     0.300f)
     .uni (ModSource::Macro1,      Param::fractureEvolve,   0.250f)
     .uni (ModSource::Macro1,      Param::evolveScatter,    0.300f)
     .uni (ModSource::Macro2,      Param::impactBrightness, 0.300f)
     .uni (ModSource::Macro2,      Param::shapeMass,       -0.200f)
     .uni (ModSource::Macro3,      Param::spaceMix,         0.300f)
     .uni (ModSource::Macro3,      Param::spaceSize,         0.250f)
     .uni (ModSource::Macro4,      Param::dustLevel,        0.300f)
     .uni (ModSource::Macro4,      Param::fractureMix,      0.250f);
    sharedMacros (r, Param::fractureDecay, Param::fractureRandom);
    r.commit (s);
}});

manager.addFactory ({ "Steel Clavier", "KEYS", { "metallic", "bright", "struck", "dry", "melodic" }, [] (PatchState& s)
{
    impact (s, 1 /* CLICK */, 0.70f, 0.60f, 0.06f, 0.90f, 0.35f, 0.06f);
    amp (s, 0.0006f, 0.30f, 0.04f, 0.10f, 0.25f);
    shape (s, 0.30f, 0.14f, 0.30f, 0.62f, 0.44f, 0.10f);
    material (s, MaterialType::String, MaterialType::Crystal, 0.25f);
    topology (s, 1 /* RING */, 0.25f, 0.28f, 173);
    matter (s, 0.93f, 0.60f, 0.58f, 0.30f);
    evolve (s, 0.10f, 0.0f, 0.0f, 0.0f, 0.50f, 0.04f, 0.0f, 0.20f, 0.10f);
    set (s, Param::evolveBendPivot, 0.50f);
    set (s, Param::evolveBendRange, 0.20f);
    set (s, Param::masterTranspose, 12.0f);   // a clavichord is a small, high box
    space (s, SpacePresets::Chamber, 0.16f, 0.22f, 0.55f, 0.10f);

    env (s, 1, 0.001f, 0.30f, 0.0f, 0.20f, 0.25f);
    macros (s, 0.15f, 0.40f, 0.15f, 0.30f);

    Routings r;
    r.uni (ModSource::Env1,        Param::shapeSurface,     0.150f)
     .uni (ModSource::Velocity,    Param::shapePitch,       0.006f)
     .uni (ModSource::Velocity,    Param::impactBrightness, 0.320f)
     .uni (ModSource::Velocity,    Param::impactHardness,   0.280f)
     .uni (ModSource::Velocity,    Param::shapeBlend,       0.220f)
     .bi  (ModSource::NoteRandom,  Param::shapeTension,     0.030f)
     .bi  (ModSource::KeyTrack,    Param::shapeDecay,      -0.220f)
     .uni (ModSource::Macro1,      Param::evolveMotion,     0.300f)
     .uni (ModSource::Macro1,      Param::evolveScatter,    0.300f)
     .uni (ModSource::Macro1,      Param::evolveSpeed,      0.250f)
     .uni (ModSource::Macro2,      Param::impactBrightness, 0.300f)
     .uni (ModSource::Macro2,      Param::shapeMass,       -0.200f)
     .uni (ModSource::Macro3,      Param::spaceMix,         0.300f)
     .uni (ModSource::Macro3,      Param::spaceSize,        0.250f)
     .uni (ModSource::Macro4,      Param::evolveBend,       0.300f)
     .uni (ModSource::Macro4,      Param::shapeTension,     0.180f);
    sharedMacros (r, Param::ampDecay, Param::impactRandom);
    r.commit (s);
}});

manager.addFactory ({ "Cimbalom Wire", "KEYS", { "metallic", "bright", "struck", "resonant", "roomy" }, [] (PatchState& s)
{
    impact (s, 3 /* NOISE STRIKE */, 0.60f, 0.55f, 0.08f, 0.85f, 0.45f, 0.12f);
    amp (s, 0.0015f, 1.20f, 0.0f, 0.80f, 0.30f);
    shape (s, 0.50f, 0.16f, 0.38f, 0.58f, 0.62f, 0.16f);
    material (s, MaterialType::String, MaterialType::Metal, 0.45f);
    topology (s, 3 /* LATTICE */, 0.45f, 0.50f, 227);
    matter (s, 0.97f, 0.55f, 0.70f, 0.60f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.48f, 0.06f, 0.0f, 0.18f, 0.12f);

    FractureShape courses;
    courses.fragments   = 16;
    courses.delayLow    = 0.03f; courses.delayHigh    = 0.22f;
    courses.feedbackLow = 0.10f; courses.feedbackHigh = 0.35f;
    courses.decayLow    = 0.35f; courses.decayHigh    = 0.60f;
    courses.spreadLow   = 0.25f; courses.spreadHigh   = 0.85f;
    courses.panWidth    = 0.70f;
    courses.pitchCycle  = kFifthTerrace;
    courses.pattern     = "XXLXXHXX";
    fracture (s, 0 /* SPECTRAL */, 0.40f, 0.28f, 0.60f, 0.20f, 0.30f, 0.25f, 0.55f, 0.60f, 0.05f,
              1 /* 16 */, 4, 8, 0.0f, 0, 1.0f, 0.15f, 313, courses);

    space (s, SpacePresets::Nebula, 0.30f, 0.55f, 0.55f, 0.25f);

    env (s, 1, 0.001f, 0.90f, 0.0f, 0.70f, 0.30f);
    macros (s, 0.20f, 0.40f, 0.30f, 0.40f);

    Routings r;
    r.uni (ModSource::Env1,        Param::fractureFeedback, 0.150f)
     .uni (ModSource::Velocity,    Param::impactBrightness, 0.300f)
     .uni (ModSource::Velocity,    Param::shapeBlend,       0.220f)
     .uni (ModSource::Velocity,    Param::fractureMix,      0.200f)
     .bi  (ModSource::NoteRandom,  Param::shapePitch,       0.006f)
     .bi  (ModSource::KeyTrack,    Param::shapeDecay,      -0.200f)
     .bi  (ModSource::KeyTrack,    Param::fractureTone,     0.150f)
     .uni (ModSource::Macro1,      Param::evolveMotion,     0.300f)
     .uni (ModSource::Macro1,      Param::fractureEvolve,   0.250f)
     .uni (ModSource::Macro1,      Param::evolveScatter,    0.300f)
     .uni (ModSource::Macro2,      Param::impactBrightness, 0.300f)
     .uni (ModSource::Macro2,      Param::fractureTone,     0.200f)
     .uni (ModSource::Macro2,      Param::shapeMass,       -0.200f)
     .uni (ModSource::Macro3,      Param::spaceMix,         0.300f)
     .uni (ModSource::Macro3,      Param::spaceSize,         0.250f)
     .uni (ModSource::Macro4,      Param::fractureAmount,   0.300f)
     .uni (ModSource::Macro4,      Param::fractureMix,      0.300f)
     .uni (ModSource::Macro4,      Param::fractureFeedback, 0.250f);
    sharedMacros (r, Param::fractureDecay, Param::fractureRandom);
    r.commit (s);
}});

manager.addFactory ({ "Paper Damper", "KEYS", { "wooden", "soft", "struck", "dry", "hollow" }, [] (PatchState& s)
{
    impact (s, 0 /* IMPULSE */, 0.40f, 0.22f, 0.40f, 0.85f, 0.60f, 0.10f);
    amp (s, 0.004f, 0.26f, 0.0f, 0.14f, 0.30f);
    shape (s, 0.55f, 0.33f, 0.54f, 0.35f, 0.24f, 0.30f);
    material (s, MaterialType::Membrane, MaterialType::Wood, 0.50f);
    topology (s, 2 /* CLUSTERS */, 0.40f, 0.40f, 419);
    matter (s, 0.88f, 0.50f, 0.60f, 0.40f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.62f, 0.05f, 0.0f, 0.20f, 0.10f);
    set (s, Param::masterTranspose, -12.0f);   // cloth on the bass strings
    space (s, SpacePresets::Dust, 0.20f, 0.35f, 0.40f, 0.15f);

    env (s, 1, 0.001f, 0.25f, 0.0f, 0.18f, 0.25f);
    macros (s, 0.15f, 0.25f, 0.15f, 0.35f);

    Routings r;
    r.uni (ModSource::Env1,        Param::shapeSurface,     0.150f)
     .uni (ModSource::Velocity,    Param::shapeBlend,       0.220f)
     .uni (ModSource::Velocity,    Param::impactBrightness, 0.250f)
     .uni (ModSource::Velocity,    Param::shapeDecay,       0.150f)
     .uni (ModSource::Velocity,    Param::shapeMass,       -0.150f)
     .bi  (ModSource::NoteRandom,  Param::shapeSurface,     0.080f)
     .bi  (ModSource::NoteRandom,  Param::shapeMass,        0.050f)
     .bi  (ModSource::KeyTrack,    Param::shapeMass,       -0.200f)
     .uni (ModSource::Macro1,      Param::evolveMotion,     0.300f)
     .uni (ModSource::Macro1,      Param::evolveScatter,    0.300f)
     .uni (ModSource::Macro1,      Param::evolveSpeed,      0.250f)
     .uni (ModSource::Macro2,      Param::impactBrightness, 0.350f)
     .uni (ModSource::Macro2,      Param::shapeMass,       -0.200f)
     .uni (ModSource::Macro3,      Param::spaceMix,         0.300f)
     .uni (ModSource::Macro3,      Param::spaceSize,         0.250f)
     .uni (ModSource::Macro4,      Param::shapeDecay,       0.300f)
     .uni (ModSource::Macro4,      Param::shapeSurface,     0.200f);
    sharedMacros (r, Param::ampDecay, Param::impactRandom);
    r.commit (s);
}});

manager.addFactory ({ "Iron Clavinet", "KEYS", { "metallic", "dirty", "struck", "rhythmic", "close" }, [] (PatchState& s)
{
    impact (s, 2 /* PLUCK */, 0.75f, 0.62f, 0.05f, 0.90f, 0.35f, 0.08f);
    amp (s, 0.001f, 0.70f, 0.05f, 0.30f, 0.30f);
    shape (s, 0.35f, 0.42f, 0.32f, 0.55f, 0.40f, 0.25f);
    material (s, MaterialType::Metal, MaterialType::Custom, 0.40f);
    topology (s, 1 /* RING */, 0.35f, 0.35f, 577);
    matter (s, 0.95f, 0.62f, 0.72f, 0.35f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.50f, 0.10f, 0.45f, 0.30f, 0.15f);
    space (s, SpacePresets::Machine, 0.30f, 0.35f, 0.55f, 0.25f);

    env (s, 1, 0.001f, 0.35f, 0.0f, 0.25f, 0.25f);
    macros (s, 0.25f, 0.40f, 0.25f, 0.40f);

    Routings r;
    r.uni (ModSource::Env1,        Param::evolveCrush,      0.200f)
     .uni (ModSource::Velocity,    Param::evolveCrush,     -0.250f)
     .uni (ModSource::Velocity,    Param::impactBrightness, 0.300f)
     .uni (ModSource::Velocity,    Param::shapeBlend,       0.220f)
     .uni (ModSource::Velocity,    Param::spaceDistDrive,   0.200f)
     .bi  (ModSource::NoteRandom,  Param::shapePitch,       0.004f)
     .bi  (ModSource::KeyTrack,    Param::shapeDecay,      -0.200f)
     .uni (ModSource::Macro1,      Param::evolveMotion,     0.300f)
     .uni (ModSource::Macro1,      Param::evolveSpeed,      0.250f)
     .uni (ModSource::Macro1,      Param::evolveScatter,    0.300f)
     .uni (ModSource::Macro2,      Param::impactBrightness, 0.300f)
     .uni (ModSource::Macro2,      Param::shapeMass,       -0.200f)
     .uni (ModSource::Macro3,      Param::spaceMix,         0.300f)
     .uni (ModSource::Macro3,      Param::spaceSize,         0.250f)
     .uni (ModSource::Macro4,      Param::evolveCrush,      0.350f)
     .uni (ModSource::Macro4,      Param::spaceDistDrive,   0.250f);
    sharedMacros (r, Param::ampDecay, Param::impactRandom);
    r.commit (s);
}});

manager.addFactory ({ "Bronze Bowl", "KEYS", { "metallic", "warm", "struck", "breathing", "wide" }, [] (PatchState& s)
{
    impact (s, 4 /* METAL STRIKE */, 0.45f, 0.35f, 0.50f, 0.80f, 0.50f, 0.12f);
    amp (s, 0.003f, 2.60f, 0.0f, 2.20f, 0.35f);
    shape (s, 0.32f, 0.86f, 0.45f, 0.50f, 0.68f, 0.18f);
    material (s, MaterialType::Crystal, MaterialType::Metal, 0.50f);
    topology (s, 1 /* RING */, 0.28f, 0.45f, 359);
    matter (s, 1.0f, 0.52f, 0.50f, 0.70f);
    evolve (s, 0.40f, 0.0f, 0.0f, 0.0f, 0.50f, 0.06f, 0.0f, 0.12f, 0.25f);
    set (s, Param::evolveBendPivot, 0.55f);
    set (s, Param::evolveBendRange, 0.35f);
    set (s, Param::evolveBendCurve, 0.60f);
    space (s, SpacePresets::Nebula, 0.40f, 0.65f, 0.50f, 0.30f);

    env (s, 1, 0.001f, 1.60f, 0.0f, 1.20f, 0.40f);
    macros (s, 0.25f, 0.30f, 0.40f, 0.45f);

    Routings r;
    r.uni (ModSource::Env1,        Param::evolveBend,      -0.220f)
     .uni (ModSource::Velocity,    Param::evolveBendPivot,  0.200f)
     .uni (ModSource::Velocity,    Param::impactBrightness, 0.300f)
     .uni (ModSource::Velocity,    Param::shapeBlend,       0.220f)
     .bi  (ModSource::NoteRandom,  Param::shapePitch,       0.007f)
     .bi  (ModSource::NoteRandom,  Param::evolveBendRange,  0.080f)
     .bi  (ModSource::KeyTrack,    Param::shapeDecay,      -0.180f)
     .uni (ModSource::Macro1,      Param::evolveMotion,     0.300f)
     .uni (ModSource::Macro1,      Param::evolveScatter,    0.300f)
     .uni (ModSource::Macro1,      Param::evolveSpeed,      0.250f)
     .uni (ModSource::Macro2,      Param::impactBrightness, 0.300f)
     .uni (ModSource::Macro2,      Param::shapeMass,       -0.200f)
     .uni (ModSource::Macro3,      Param::spaceMix,         0.300f)
     .uni (ModSource::Macro3,      Param::spaceSize,        0.250f)
     .uni (ModSource::Macro4,      Param::evolveBend,       0.300f)
     .uni (ModSource::Macro4,      Param::evolveBendRange,  0.250f);
    sharedMacros (r, Param::ampDecay, Param::impactRandom);
    r.commit (s);
}});

manager.addFactory ({ "Frost Keys", "KEYS", { "cold", "glassy", "struck", "air", "distant" }, [] (PatchState& s)
{
    wave (s, 5 /* SPECTRAL */, 0.35f, 0.40f, 0.15f, 2, 0.12f, 0.60f, 1, 0.28f);
    impact (s, 4 /* METAL STRIKE */, 0.55f, 0.62f, 0.30f, 0.85f, 0.45f, 0.10f, 0.0f, 0.70f);
    layerOnly (s, Param::waveLevel, Param::impactLevel);
    amp (s, 0.005f, 2.20f, 0.12f, 1.80f, 0.40f);
    shape (s, 0.28f, 0.90f, 0.26f, 0.62f, 0.66f, 0.08f);
    material (s, MaterialType::Void, MaterialType::Crystal, 0.45f);
    topology (s, 5 /* STAR */, 0.20f, 0.60f, 887);
    matter (s, 0.90f, 0.50f, 0.55f, 0.85f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.38f, 0.05f, 0.0f, 0.15f, 0.15f);

    FractureShape air;
    air.fragments   = 32;
    air.delayLow    = 0.05f; air.delayHigh    = 0.35f;
    air.feedbackLow = 0.10f; air.feedbackHigh = 0.40f;
    air.decayLow    = 0.40f; air.decayHigh    = 0.75f;
    air.spreadLow   = 0.40f; air.spreadHigh   = 0.95f;
    air.gainLow     = 0.85f; air.gainHigh     = 1.00f;
    air.panWidth    = 0.80f;
    air.pitchCycle  = kOctaveTerrace;
    air.pattern     = "XHXHXHXH";
    fracture (s, 0 /* SPECTRAL */, 0.38f, 0.30f, 0.70f, 0.25f, 0.28f, 0.30f, 0.60f, 0.65f, 0.08f,
              2 /* 32 */, 3, 8, 0.0f, 0, 1.0f, 0.20f, 941, air);

    space (s, SpacePresets::Shimmer, 0.42f, 0.65f, 0.68f, 0.30f);

    env (s, 1, 0.010f, 1.40f, 0.0f, 1.00f, 0.45f);
    macros (s, 0.25f, 0.45f, 0.45f, 0.35f);

    Routings r;
    r.uni (ModSource::Env1,        Param::shapeExcite,      0.150f)
     .uni (ModSource::Velocity,    Param::impactBrightness, 0.300f)
     .uni (ModSource::Velocity,    Param::waveLevel,       -0.150f)
     .uni (ModSource::Velocity,    Param::shapeBlend,       0.220f)
     .bi  (ModSource::NoteRandom,  Param::waveScan,         0.100f)
     .bi  (ModSource::NoteRandom,  Param::shapePitch,       0.005f)
     .bi  (ModSource::KeyTrack,    Param::shapeDecay,      -0.200f)
     .bi  (ModSource::KeyTrack,    Param::waveLevel,       -0.100f)
     .uni (ModSource::Macro1,      Param::evolveMotion,     0.300f)
     .uni (ModSource::Macro1,      Param::fractureEvolve,   0.250f)
     .uni (ModSource::Macro1,      Param::evolveScatter,    0.300f)
     .uni (ModSource::Macro2,      Param::impactBrightness, 0.300f)
     .uni (ModSource::Macro2,      Param::spaceTone,        0.250f)
     .uni (ModSource::Macro2,      Param::shapeMass,       -0.200f)
     .uni (ModSource::Macro3,      Param::spaceMix,         0.300f)
     .uni (ModSource::Macro3,      Param::spaceSize,        0.200f)
     .uni (ModSource::Macro4,      Param::shapeBlend,       0.300f)
     .uni (ModSource::Macro4,      Param::waveLevel,        0.200f);
    sharedMacros (r, Param::fractureDecay, Param::fractureRandom);
    r.commit (s);
}});

//--------------------------------------------------------------------------
// Keyboards that are not keyboards: a bow that strikes, a wiring fault, and
// three objects recorded rather than modelled but still played by hand.
//--------------------------------------------------------------------------

manager.addFactory ({ "Rosin Keys", "KEYS", { "organic", "warm", "bowed", "struck", "roomy" }, [] (PatchState& s)
{
    gesture (s, 0 /* BOW */, 0.55f, 0.45f, 0.35f, 0.30f, 0.15f, 0.55f, 0.85f);
    amp (s, 0.015f, 1.40f, 0.0f, 0.90f, 0.40f);
    shape (s, 0.40f, 0.62f, 0.40f, 0.52f, 0.58f, 0.20f);
    material (s, MaterialType::Organic, MaterialType::String, 0.45f);
    topology (s, 0 /* CHAIN */, 0.35f, 0.45f, 617);
    matter (s, 0.92f, 0.65f, 0.35f, 0.50f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.50f, 0.06f, 0.0f, 0.20f, 0.15f);
    space (s, SpacePresets::Nebula, 0.34f, 0.55f, 0.50f, 0.25f);

    env (s, 1, 0.004f, 0.35f, 0.0f, 0.30f, 0.30f);
    macros (s, 0.25f, 0.35f, 0.30f, 0.35f);

    Routings r;
    r.uni (ModSource::Env1,        Param::gesturePressure,  0.300f)
     .uni (ModSource::Velocity,    Param::gesturePressure,  0.250f)
     .uni (ModSource::Velocity,    Param::gestureSpeed,     0.200f)
     .uni (ModSource::Velocity,    Param::shapeBlend,       0.220f)
     .bi  (ModSource::NoteRandom,  Param::gesturePosition,  0.120f)
     .bi  (ModSource::NoteRandom,  Param::shapePitch,       0.005f)
     .bi  (ModSource::KeyTrack,    Param::gestureSpeed,     0.150f)
     .bi  (ModSource::KeyTrack,    Param::shapeDecay,      -0.180f)
     .uni (ModSource::Macro1,      Param::gestureMotion,    0.300f)
     .uni (ModSource::Macro1,      Param::evolveMotion,     0.250f)
     .uni (ModSource::Macro1,      Param::evolveScatter,    0.300f)
     .uni (ModSource::Macro2,      Param::gestureBandwidth, 0.300f)
     .uni (ModSource::Macro2,      Param::shapeExcite,      0.200f)
     .uni (ModSource::Macro2,      Param::shapeMass,       -0.200f)
     .uni (ModSource::Macro3,      Param::spaceMix,         0.300f)
     .uni (ModSource::Macro3,      Param::spaceSize,         0.250f)
     .uni (ModSource::Macro4,      Param::gestureRoughness, 0.350f)
     .uni (ModSource::Macro4,      Param::gesturePressure,  0.200f);
    sharedMacros (r, Param::ampDecay, Param::gestureRoughness);
    r.commit (s);
}});

manager.addFactory ({ "Static Piano", "KEYS", { "synthetic", "harsh", "struck", "chaotic", "close" }, [] (PatchState& s)
{
    gesture (s, 5 /* ELECTRICAL */, 0.45f, 0.50f, 0.60f, 0.35f, 0.30f, 0.60f, 0.45f);
    impact (s, 1 /* CLICK */, 0.60f, 0.55f, 0.08f, 0.85f, 0.40f, 0.15f, 0.0f, 0.60f);
    layerOnly (s, Param::gestureLevel, Param::impactLevel);
    amp (s, 0.002f, 1.00f, 0.06f, 0.60f, 0.35f);
    shape (s, 0.45f, 0.68f, 0.35f, 0.55f, 0.45f, 0.30f);
    material (s, MaterialType::Custom, MaterialType::Metal, 0.40f);
    topology (s, 4 /* RANDOM */, 0.40f, 0.50f, 727);
    matter (s, 0.90f, 0.60f, 0.60f, 0.55f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.50f, 0.12f, 0.0f, 0.35f, 0.20f);
    space (s, SpacePresets::Machine, 0.28f, 0.40f, 0.55f, 0.25f);

    env (s, 1, 0.001f, 0.40f, 0.0f, 0.30f, 0.25f);
    chaos (s, 1, 0 /* WALK */, 3.00f, 0.40f, 0.65f, 0.5f, 137);
    macros (s, 0.30f, 0.40f, 0.25f, 0.45f);

    Routings r;
    r.bi  (ModSource::Chaos1,      Param::gesturePressure,  0.200f)
     .uni (ModSource::Env1,        Param::gestureLevel,    -0.200f)
     .uni (ModSource::Velocity,    Param::gestureLevel,     0.300f)
     .uni (ModSource::Velocity,    Param::impactBrightness, 0.300f)
     .uni (ModSource::Velocity,    Param::shapeBlend,       0.220f)
     .bi  (ModSource::NoteRandom,  Param::gestureRoughness, 0.150f)
     .bi  (ModSource::KeyTrack,    Param::shapeDecay,      -0.200f)
     .uni (ModSource::Macro1,      Param::gestureMotion,    0.300f)
     .uni (ModSource::Macro1,      Param::chaos1Depth,      0.250f)
     .uni (ModSource::Macro1,      Param::evolveScatter,    0.300f)
     .uni (ModSource::Macro2,      Param::impactBrightness, 0.300f)
     .uni (ModSource::Macro2,      Param::gestureBandwidth, 0.250f)
     .uni (ModSource::Macro2,      Param::shapeMass,       -0.200f)
     .uni (ModSource::Macro3,      Param::spaceMix,         0.300f)
     .uni (ModSource::Macro3,      Param::spaceSize,         0.250f)
     .uni (ModSource::Macro4,      Param::gestureRoughness, 0.300f)
     .uni (ModSource::Macro4,      Param::gestureLevel,     0.200f);
    sharedMacros (r, Param::ampDecay, Param::chaos1Rate);
    r.commit (s);
}});

manager.addFactory ({ "Stone Piano", "KEYS", { "dark", "dirty", "struck", "granular", "huge" }, [] (PatchState& s)
{
    sample (s, BuiltInSamples::Kind::StoneDrop, 3 /* GRANULAR */, 0.0f, 0.85f, 0.35f, 0.45f, 48, 0.80f);
    amp (s, 0.002f, 1.20f, 0.0f, 0.70f, 0.35f);
    shape (s, 0.50f, 0.66f, 0.60f, 0.40f, 0.40f, 0.25f);
    material (s, MaterialType::Organic, MaterialType::Void, 0.35f);
    topology (s, 3 /* LATTICE */, 0.45f, 0.55f, 1009);
    matter (s, 0.85f, 0.54f, 0.56f, 0.60f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.55f, 0.08f, 0.0f, 0.25f, 0.15f);
    space (s, SpacePresets::Void, 0.30f, 0.60f, 0.35f, 0.20f);

    env (s, 1, 0.001f, 0.60f, 0.0f, 0.45f, 0.30f);
    macros (s, 0.20f, 0.30f, 0.35f, 0.35f);

    Routings r;
    r.uni (ModSource::Env1,        Param::sampleGrain,     -0.150f)
     .uni (ModSource::Velocity,    Param::sampleGrain,     -0.200f)
     .uni (ModSource::Velocity,    Param::shapeBlend,       0.220f)
     .uni (ModSource::Velocity,    Param::sampleLevel,      0.200f)
     .uni (ModSource::Velocity,    Param::shapeSurface,     0.150f)
     .bi  (ModSource::NoteRandom,  Param::sampleStart,      0.060f)
     .bi  (ModSource::NoteRandom,  Param::sampleSpread,     0.100f)
     .bi  (ModSource::KeyTrack,    Param::shapeMass,       -0.200f)
     .uni (ModSource::Macro1,      Param::sampleGrain,      0.300f)
     .uni (ModSource::Macro1,      Param::evolveMotion,     0.250f)
     .uni (ModSource::Macro1,      Param::evolveScatter,    0.300f)
     .uni (ModSource::Macro2,      Param::shapeExcite,      0.300f)
     .uni (ModSource::Macro2,      Param::shapeMass,       -0.200f)
     .uni (ModSource::Macro3,      Param::spaceMix,         0.300f)
     .uni (ModSource::Macro3,      Param::spaceSize,        0.200f)
     .uni (ModSource::Macro4,      Param::sampleSpread,     0.300f)
     .uni (ModSource::Macro4,      Param::shapeSurface,     0.200f);
    sharedMacros (r, Param::ampDecay, Param::sampleStart);
    r.commit (s);
}});

manager.addFactory ({ "Knock Box", "KEYS", { "wooden", "organic", "struck", "dry", "granular" }, [] (PatchState& s)
{
    sample (s, BuiltInSamples::Kind::WoodKnock, 3 /* GRANULAR */, 0.0f, 0.90f, 0.22f, 0.50f, 60, 0.85f);
    amp (s, 0.001f, 0.55f, 0.0f, 0.30f, 0.30f);
    shape (s, 0.45f, 0.28f, 0.45f, 0.45f, 0.36f, 0.30f);
    material (s, MaterialType::Wood, MaterialType::Membrane, 0.45f);
    topology (s, 2 /* CLUSTERS */, 0.40f, 0.40f, 1213);
    matter (s, 0.90f, 0.54f, 0.58f, 0.45f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.52f, 0.10f, 0.0f, 0.30f, 0.15f);
    space (s, SpacePresets::Chamber, 0.20f, 0.30f, 0.45f, 0.15f);

    env (s, 1, 0.001f, 0.25f, 0.0f, 0.20f, 0.25f);
    chaos (s, 1, 4 /* TARGETS */, 3.00f, 0.30f, 0.70f, 0.5f, 271);
    macros (s, 0.25f, 0.30f, 0.20f, 0.40f);

    Routings r;
    r.bi  (ModSource::Chaos1,      Param::sampleStart,      0.080f)
     .bi  (ModSource::NoteRandom,  Param::sampleStart,      0.120f)
     .bi  (ModSource::NoteRandom,  Param::shapeSurface,     0.100f)
     .bi  (ModSource::NoteRandom,  Param::shapeMass,        0.060f)
     .bi  (ModSource::NoteRandom,  Param::shapePitch,       0.006f)
     .uni (ModSource::Velocity,    Param::shapeBlend,       0.220f)
     .uni (ModSource::Velocity,    Param::sampleGrain,     -0.180f)
     .uni (ModSource::Velocity,    Param::shapeExcite,      0.220f)
     .bi  (ModSource::KeyTrack,    Param::shapeDecay,      -0.200f)
     .uni (ModSource::Macro1,      Param::chaos1Depth,      0.300f)
     .uni (ModSource::Macro1,      Param::evolveMotion,     0.250f)
     .uni (ModSource::Macro1,      Param::evolveScatter,    0.300f)
     .uni (ModSource::Macro2,      Param::shapeExcite,      0.300f)
     .uni (ModSource::Macro2,      Param::sampleGrain,      0.200f)
     .uni (ModSource::Macro2,      Param::shapeMass,       -0.200f)
     .uni (ModSource::Macro3,      Param::spaceMix,         0.300f)
     .uni (ModSource::Macro3,      Param::spaceSize,         0.250f)
     .uni (ModSource::Macro4,      Param::shapeSurface,     0.300f)
     .uni (ModSource::Macro4,      Param::sampleSpread,     0.250f);
    sharedMacros (r, Param::ampDecay, Param::sampleSpread);
    r.commit (s);
}});

manager.addFactory ({ "Vinyl Keys", "KEYS", { "warm", "dirty", "struck", "noisy", "distant" }, [] (PatchState& s)
{
    sample (s, BuiltInSamples::Kind::VinylDust, 1 /* LOOP */, 0.05f, 0.90f, 0.30f, 0.50f, 60, 0.30f);
    impact (s, 2 /* PLUCK */, 0.40f, 0.35f, 0.15f, 0.85f, 0.50f, 0.10f, 0.0f, 0.70f);
    layerOnly (s, Param::sampleLevel, Param::impactLevel);
    amp (s, 0.002f, 1.30f, 0.08f, 0.80f, 0.40f);
    shape (s, 0.40f, 0.18f, 0.45f, 0.50f, 0.50f, 0.25f);
    material (s, MaterialType::Custom, MaterialType::Wood, 0.40f);
    topology (s, 0 /* CHAIN */, 0.30f, 0.40f, 1327);
    matter (s, 0.85f, 0.50f, 0.60f, 0.50f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.52f, 0.06f, 0.0f, 0.20f, 0.12f);

    FractureShape smear;
    smear.fragments   = 16;
    smear.delayLow    = 0.04f; smear.delayHigh    = 0.20f;
    smear.feedbackLow = 0.08f; smear.feedbackHigh = 0.30f;
    smear.decayLow    = 0.30f; smear.decayHigh    = 0.55f;
    smear.spreadLow   = 0.35f; smear.spreadHigh   = 0.85f;
    smear.panWidth    = 0.55f;
    smear.probability = 0.90f;
    smear.pattern     = "XLXXHXLX";
    fracture (s, 0 /* SPECTRAL */, 0.32f, 0.25f, 0.55f, 0.25f, 0.22f, 0.20f, 0.45f, 0.45f, 0.06f,
              1 /* 16 */, 4, 8, 0.0f, 0, 0.90f, 0.25f, 1483, smear);

    space (s, SpacePresets::Dust, 0.30f, 0.50f, 0.42f, 0.25f);

    env (s, 1, 0.001f, 0.60f, 0.0f, 0.45f, 0.30f);
    macros (s, 0.20f, 0.30f, 0.35f, 0.40f);

    Routings r;
    r.uni (ModSource::Env1,        Param::shapeExcite,      0.150f)
     .uni (ModSource::Velocity,    Param::impactBrightness, 0.300f)
     .uni (ModSource::Velocity,    Param::sampleLevel,     -0.200f)
     .uni (ModSource::Velocity,    Param::shapeBlend,       0.220f)
     .bi  (ModSource::NoteRandom,  Param::sampleStart,      0.080f)
     .bi  (ModSource::KeyTrack,    Param::shapeDecay,      -0.200f)
     .bi  (ModSource::KeyTrack,    Param::sampleLevel,     -0.120f)
     .uni (ModSource::Macro1,      Param::evolveMotion,     0.300f)
     .uni (ModSource::Macro1,      Param::fractureEvolve,   0.250f)
     .uni (ModSource::Macro1,      Param::evolveScatter,    0.300f)
     .uni (ModSource::Macro2,      Param::impactBrightness, 0.300f)
     .uni (ModSource::Macro2,      Param::fractureTone,     0.250f)
     .uni (ModSource::Macro2,      Param::shapeMass,       -0.200f)
     .uni (ModSource::Macro3,      Param::spaceMix,         0.300f)
     .uni (ModSource::Macro3,      Param::spaceSize,         0.250f)
     .uni (ModSource::Macro4,      Param::sampleLevel,      0.300f)
     .uni (ModSource::Macro4,      Param::fractureMix,      0.250f);
    sharedMacros (r, Param::fractureDecay, Param::fractureRandom);
    r.commit (s);
}});

manager.addFactory ({ "Torn Celesta", "KEYS", { "glassy", "cold", "struck", "unstable", "wide" }, [] (PatchState& s)
{
    impact (s, 4 /* METAL STRIKE */, 0.60f, 0.65f, 0.20f, 0.90f, 0.40f, 0.10f);
    amp (s, 0.001f, 1.60f, 0.0f, 1.20f, 0.30f);
    shape (s, 0.36f, 0.80f, 0.28f, 0.55f, 0.60f, 0.12f);
    material (s, MaterialType::Crystal, MaterialType::Liquid, 0.35f);
    topology (s, 5 /* STAR */, 0.25f, 0.50f, 1451);
    matter (s, 1.0f, 0.55f, 0.56f, 0.75f);
    evolve (s, 0.0f, 0.0f, 0.35f, 0.0f, 0.50f, 0.08f, 0.0f, 0.25f, 0.20f);

    FractureShape twins;
    twins.fragments   = 16;
    twins.delayLow    = 0.03f; twins.delayHigh    = 0.18f;
    twins.feedbackLow = 0.10f; twins.feedbackHigh = 0.30f;
    twins.decayLow    = 0.30f; twins.decayHigh    = 0.55f;
    twins.spreadLow   = 0.40f; twins.spreadHigh   = 0.90f;
    twins.panWidth    = 0.75f;
    twins.probability = 0.85f;
    twins.pitchCycle  = kMinorTerrace;
    twins.pattern     = "XHXLXHXL";
    fracture (s, 2 /* TRANSIENT */, 0.35f, 0.25f, 0.65f, 0.30f, 0.20f, 0.18f, 0.45f, 0.60f, 0.10f,
              1 /* 16 */, 4, 8, 0.0f, 0, 0.85f, 0.30f, 1597, twins);

    space (s, SpacePresets::Dream, 0.36f, 0.60f, 0.55f, 0.28f);

    env (s, 1, 0.001f, 1.00f, 0.0f, 0.80f, 0.35f);
    macros (s, 0.25f, 0.40f, 0.35f, 0.45f);

    Routings r;
    r.uni (ModSource::Env1,        Param::evolveTear,       0.150f)
     .uni (ModSource::Velocity,    Param::evolveTear,       0.350f)
     .uni (ModSource::Velocity,    Param::impactBrightness, 0.300f)
     .uni (ModSource::Velocity,    Param::shapeBlend,       0.220f)
     .bi  (ModSource::NoteRandom,  Param::shapePitch,       0.006f)
     .bi  (ModSource::KeyTrack,    Param::shapeDecay,      -0.200f)
     .uni (ModSource::Macro1,      Param::evolveMotion,     0.300f)
     .uni (ModSource::Macro1,      Param::evolveSpeed,      0.250f)
     .uni (ModSource::Macro1,      Param::evolveScatter,    0.300f)
     .uni (ModSource::Macro2,      Param::impactBrightness, 0.300f)
     .uni (ModSource::Macro2,      Param::shapeMass,       -0.200f)
     .uni (ModSource::Macro3,      Param::spaceMix,         0.300f)
     .uni (ModSource::Macro3,      Param::spaceSize,        0.250f)
     .uni (ModSource::Macro4,      Param::evolveTear,       0.300f)
     .uni (ModSource::Macro4,      Param::shapeSurface,     0.200f)
     .uni (ModSource::Macro4,      Param::evolveScatter,    0.250f);
    sharedMacros (r, Param::fractureDecay, Param::impactRandom);
    r.commit (s);
}});
}

} // namespace am::FactoryContent
