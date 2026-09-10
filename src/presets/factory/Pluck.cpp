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

void registerPluck (PresetManager& manager)
{
//==========================================================================
// PLUCK — short, precise, physical
//==========================================================================

manager.addFactory ({ "Magnet Bells", "PLUCK", { "pluck", "bells", "tuned", "magnet" }, [] (PatchState& s)
{
    impact (s, 4 /* METAL STRIKE */, 0.82f, 0.60f, 0.12f, 0.85f, 0.40f, 0.10f);
    amp (s, 0.001f, 0.85f, 0.0f, 0.70f, 0.3f);
    shape (s, 0.38f, 0.66f, 0.30f, 0.62f, 0.66f, 0.14f);
    material (s, MaterialType::Metal, MaterialType::Crystal, 0.45f);
    topology (s, 1 /* RING */, 0.34f, 0.58f, 13);
    matter (s, 1.0f, 0.60f, 0.80f, 0.65f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.72f, 0.40f, 0.10f, 0.0f, 0.30f, 0.18f);
    set (s, Param::evolveMagnetTarget, 5 /* SCALE */);
    space (s, SpacePresets::Orbit, 0.34f, 0.45f, 0.55f, 0.40f);

    env (s, 1, 0.001f, 0.45f, 0.0f, 0.40f, 0.3f);
    lfo (s, 1, 0.60f, 5 /* SMOOTH RANDOM */, 1.0f, true);
    macros (s, 0.25f, 0.40f, 0.35f, 0.55f);

    Routings r;
    r.uni (ModSource::Env1,   Param::evolveScatter,  0.120f)
     .bi  (ModSource::LFO1,   Param::shapeTension,   0.040f)
     .uni (ModSource::Velocity, Param::impactHardness, 0.300f)
     .uni (ModSource::Velocity, Param::shapeStrike,  0.220f)
     .bi  (ModSource::NoteRandom, Param::shapePitch, 0.004f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.350f)
     .uni (ModSource::Macro2, Param::impactBrightness, 0.320f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.300f)
     .uni (ModSource::Macro3, Param::spaceFeedback,  0.200f)
     .uni (ModSource::Macro4, Param::evolveMagnet,   0.280f)
     .uni (ModSource::Macro4, Param::shapeForm,      0.150f);
    sharedMacros (r, Param::spaceFeedback, Param::impactRandom);
    r.commit (s);
}});

manager.addFactory ({ "Anti-String", "PLUCK", { "pluck", "string", "impossible", "inharmonic" }, [] (PatchState& s)
{
    impact (s, 2 /* PLUCK */, 0.62f, 0.55f, 0.16f, 0.78f, 0.42f, 0.06f);
    amp (s, 0.001f, 1.40f, 0.0f, 1.10f, 0.3f);
    shape (s, 0.44f, 0.18f, 0.36f, 0.74f, 0.70f, 0.24f);
    material (s, MaterialType::String, MaterialType::Void, 0.38f);
    topology (s, 0 /* CHAIN */, 0.48f, 0.55f, 29);
    matter (s, 1.0f, 0.56f, 0.68f, 0.60f);
    evolve (s, 0.32f, 0.0f, 0.0f, 0.0f, 0.46f, 0.16f, 0.0f, 0.22f, 0.20f);
    set (s, Param::evolveBendPivot, 0.28f);
    set (s, Param::evolveBendRange, 0.42f);
    set (s, Param::evolveBendCurve, 0.62f);
    space (s, SpacePresets::Dream, 0.32f, 0.55f, 0.55f, 0.28f);

    env (s, 1, 0.001f, 1.20f, 0.0f, 1.0f, 0.35f);
    lfo (s, 1, 0.32f, 1 /* TRIANGLE */, 1.0f, true, 0.4f);
    macros (s, 0.30f, 0.40f, 0.30f, 0.45f);

    Routings r;
    r.uni (ModSource::Env1,   Param::evolveBend,     0.180f)
     .bi  (ModSource::LFO1,   Param::shapeSurface,   0.050f)
     .uni (ModSource::Velocity, Param::impactBrightness, 0.280f)
     .uni (ModSource::Velocity, Param::shapeExcite,  0.200f)
     .bi  (ModSource::KeyTrack, Param::evolveBendPivot, 0.150f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.320f)
     .uni (ModSource::Macro2, Param::shapeExcite,    0.300f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.300f)
     .uni (ModSource::Macro4, Param::evolveBend,     0.350f)
     .uni (ModSource::Macro4, Param::shapeTension,   0.180f);
    sharedMacros (r, Param::ampDecay, Param::evolveTear);
    r.commit (s);
}});

manager.addFactory ({ "Copper Thorn", "PLUCK", { "pluck", "metal", "short", "percussive" }, [] (PatchState& s)
{
    impact (s, 1 /* CLICK */, 0.88f, 0.78f, 0.08f, 0.85f, 0.30f, 0.14f);
    amp (s, 0.001f, 0.40f, 0.0f, 0.30f, 0.25f);
    shape (s, 0.30f, 0.50f, 0.28f, 0.58f, 0.48f, 0.34f);
    material (s, MaterialType::Metal, MaterialType::Wood, 0.30f);
    topology (s, 4 /* RANDOM */, 0.60f, 0.42f, 97);
    matter (s, 1.0f, 0.66f, 0.70f, 0.55f);
    evolve (s, 0.0f, 0.0f, 0.20f, 0.34f, 0.50f, 0.24f, 0.22f, 0.45f, 0.20f);
    set (s, Param::evolveScatterSeed, 331);
    set (s, Param::evolveMagnetTarget, 3 /* MINOR */);
    space (s, SpacePresets::Orbit, 0.30f, 0.35f, 0.50f, 0.45f);

    env (s, 1, 0.001f, 0.20f, 0.0f, 0.18f, 0.25f);
    chaos (s, 1, 4 /* TARGETS */, 2.20f, 0.40f, 0.60f, 0.5f, 811);
    macros (s, 0.30f, 0.50f, 0.30f, 0.35f);

    Routings r;
    r.uni (ModSource::Env1,   Param::evolveCrush,    0.150f)
     .bi  (ModSource::Chaos1, Param::evolveScatter,  0.120f)
     .uni (ModSource::Velocity, Param::impactHardness, 0.320f)
     .uni (ModSource::Velocity, Param::shapeStrike,  0.250f)
     .bi  (ModSource::NoteRandom, Param::shapeSurface, 0.060f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.400f)
     .uni (ModSource::Macro2, Param::impactBrightness, 0.300f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.300f)
     .uni (ModSource::Macro4, Param::evolveTear,     0.300f)
     .uni (ModSource::Macro4, Param::evolveCrush,    0.200f);
    sharedMacros (r, Param::spaceFeedback, Param::impactRandom);
    r.commit (s);
}});

//--------------------------------------------------------------------------
// Strings under a finger, a nail and a plectrum: gut, nylon, steel and wire,
// and the two boxes that answer them.
//--------------------------------------------------------------------------

manager.addFactory ({ "Nylon Thumb", "PLUCK", { "warm", "soft", "plucked", "close", "melodic" }, [] (PatchState& s)
{
    impact (s, 2 /* PLUCK */, 0.30f, 0.30f, 0.10f, 0.85f, 0.55f, 0.06f);
    amp (s, 0.002f, 0.95f, 0.0f, 0.55f, 0.35f);
    shape (s, 0.36f, 0.10f, 0.42f, 0.48f, 0.45f, 0.16f);
    material (s, MaterialType::String, MaterialType::Organic, 0.40f);
    topology (s, 0 /* CHAIN */, 0.28f, 0.30f, 19);
    matter (s, 0.94f, 0.48f, 0.55f, 0.35f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.52f, 0.04f, 0.0f, 0.18f, 0.08f);
    space (s, SpacePresets::Chamber, 0.20f, 0.28f, 0.42f, 0.15f);

    env (s, 1, 0.001f, 0.30f, 0.0f, 0.25f, 0.30f);
    macros (s, 0.15f, 0.35f, 0.20f, 0.35f);

    Routings r;
    r.uni (ModSource::Env1,        Param::shapeSurface,     0.150f)
     .uni (ModSource::Velocity,    Param::impactHardness,   0.300f)
     .uni (ModSource::Velocity,    Param::impactBrightness, 0.280f)
     .uni (ModSource::Velocity,    Param::shapeBlend,       0.220f)
     .uni (ModSource::Velocity,    Param::shapeMass,       -0.150f)
     .bi  (ModSource::NoteRandom,  Param::shapePitch,       0.005f)
     .bi  (ModSource::NoteRandom,  Param::shapeSurface,     0.050f)
     .bi  (ModSource::KeyTrack,    Param::shapeDecay,      -0.220f)
     .uni (ModSource::Macro1,      Param::evolveMotion,     0.300f)
     .uni (ModSource::Macro2,      Param::impactBrightness, 0.300f)
     .uni (ModSource::Macro2,      Param::shapeMass,       -0.200f)
     .uni (ModSource::Macro3,      Param::spaceMix,         0.300f)
     .uni (ModSource::Macro3,      Param::spaceSize,        0.250f)
     .uni (ModSource::Macro4,      Param::impactHardness,   0.300f)
     .uni (ModSource::Macro4,      Param::shapeSurface,     0.200f);
    sharedMacros (r, Param::ampDecay, Param::impactRandom);
    r.commit (s);
}});

manager.addFactory ({ "Gut Course", "PLUCK", { "organic", "wooden", "plucked", "noisy", "close" }, [] (PatchState& s)
{
    impact (s, 2 /* PLUCK */, 0.55f, 0.50f, 0.08f, 0.85f, 0.45f, 0.08f, 0.0f, 0.80f);
    dust (s, 5 /* CRACKLE */, 0.25f, 0.55f, 0.02f, 0.50f, 0.40f, 0.40f, 89, 0.16f);
    layerOnly (s, Param::impactLevel, Param::dustLevel);
    amp (s, 0.001f, 1.10f, 0.0f, 0.60f, 0.30f);
    shape (s, 0.42f, 0.14f, 0.38f, 0.52f, 0.50f, 0.22f);
    material (s, MaterialType::Organic, MaterialType::Wood, 0.45f);
    topology (s, 0 /* CHAIN */, 0.38f, 0.35f, 89);
    matter (s, 0.93f, 0.52f, 0.50f, 0.40f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.50f, 0.06f, 0.0f, 0.20f, 0.10f);
    space (s, SpacePresets::Chamber, 0.24f, 0.35f, 0.50f, 0.20f);

    env (s, 1, 0.001f, 0.20f, 0.0f, 0.15f, 0.25f);
    macros (s, 0.15f, 0.35f, 0.25f, 0.40f);

    Routings r;
    r.uni (ModSource::Env1,        Param::dustDensity,      0.250f)
     .uni (ModSource::Velocity,    Param::dustLevel,        0.250f)
     .uni (ModSource::Velocity,    Param::impactBrightness, 0.300f)
     .uni (ModSource::Velocity,    Param::shapeBlend,       0.220f)
     .bi  (ModSource::NoteRandom,  Param::shapePitch,       0.009f)
     .bi  (ModSource::NoteRandom,  Param::shapeTension,     0.040f)
     .bi  (ModSource::KeyTrack,    Param::shapeDecay,      -0.200f)
     .uni (ModSource::Macro1,      Param::evolveMotion,     0.300f)
     .uni (ModSource::Macro2,      Param::impactBrightness, 0.300f)
     .uni (ModSource::Macro2,      Param::dustColor,        0.250f)
     .uni (ModSource::Macro3,      Param::spaceMix,         0.300f)
     .uni (ModSource::Macro4,      Param::dustLevel,        0.300f)
     .uni (ModSource::Macro4,      Param::shapeCoupling,    0.200f);
    sharedMacros (r, Param::ampDecay, Param::dustJitter);
    r.commit (s);
}});

manager.addFactory ({ "Steel Fret", "PLUCK", { "metallic", "bright", "plucked", "dirty", "dry" }, [] (PatchState& s)
{
    impact (s, 2 /* PLUCK */, 0.72f, 0.62f, 0.06f, 0.90f, 0.40f, 0.08f);
    amp (s, 0.001f, 1.30f, 0.0f, 0.70f, 0.30f);
    shape (s, 0.40f, 0.13f, 0.34f, 0.62f, 0.52f, 0.20f);
    material (s, MaterialType::String, MaterialType::Metal, 0.40f);
    topology (s, 0 /* CHAIN */, 0.30f, 0.35f, 163);
    matter (s, 0.95f, 0.55f, 0.62f, 0.35f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.50f, 0.06f, 0.0f, 0.25f, 0.12f);

    FractureShape buzz;
    buzz.fragments   = 16;
    buzz.delayLow    = 0.01f; buzz.delayHigh    = 0.07f;
    buzz.feedbackLow = 0.05f; buzz.feedbackHigh = 0.22f;
    buzz.decayLow    = 0.15f; buzz.decayHigh    = 0.30f;
    buzz.spreadLow   = 0.20f; buzz.spreadHigh   = 0.60f;
    buzz.panWidth    = 0.45f;
    buzz.probability = 0.85f;
    buzz.pattern     = "XHXHXHXH";
    fracture (s, 2 /* TRANSIENT */, 0.35f, 0.22f, 0.45f, 0.30f, 0.15f, 0.10f, 0.30f, 0.65f, 0.08f,
              1 /* 16 */, 5, 8, 0.0f, 0, 0.85f, 0.30f, 199, buzz);

    space (s, SpacePresets::Chamber, 0.20f, 0.30f, 0.55f, 0.15f);

    env (s, 1, 0.001f, 0.35f, 0.0f, 0.25f, 0.25f);
    macros (s, 0.20f, 0.45f, 0.20f, 0.35f);

    Routings r;
    r.uni (ModSource::Env1,        Param::fractureAmount,   0.150f)
     .uni (ModSource::Velocity,    Param::fractureAmount,   0.350f)
     .uni (ModSource::Velocity,    Param::fractureMix,      0.200f)
     .uni (ModSource::Velocity,    Param::impactBrightness, 0.300f)
     .uni (ModSource::Velocity,    Param::shapeBlend,       0.220f)
     .bi  (ModSource::NoteRandom,  Param::shapePitch,       0.004f)
     .bi  (ModSource::KeyTrack,    Param::shapeDecay,      -0.200f)
     .uni (ModSource::Macro1,      Param::evolveMotion,     0.300f)
     .uni (ModSource::Macro1,      Param::fractureEvolve,   0.250f)
     .uni (ModSource::Macro2,      Param::impactBrightness, 0.300f)
     .uni (ModSource::Macro3,      Param::spaceMix,         0.300f)
     .uni (ModSource::Macro4,      Param::fractureMix,      0.300f)
     .uni (ModSource::Macro4,      Param::shapeSurface,     0.200f);
    sharedMacros (r, Param::fractureDecay, Param::fractureRandom);
    r.commit (s);
}});

manager.addFactory ({ "Koto Bridge", "PLUCK", { "wooden", "bright", "plucked", "morphing", "roomy" }, [] (PatchState& s)
{
    impact (s, 2 /* PLUCK */, 0.65f, 0.55f, 0.07f, 0.90f, 0.40f, 0.07f);
    amp (s, 0.001f, 1.50f, 0.0f, 0.90f, 0.30f);
    shape (s, 0.35f, 0.16f, 0.40f, 0.55f, 0.55f, 0.18f);
    material (s, MaterialType::String, MaterialType::Wood, 0.35f);
    topology (s, 3 /* LATTICE */, 0.32f, 0.42f, 233);
    matter (s, 0.96f, 0.52f, 0.50f, 0.45f);
    evolve (s, 0.30f, 0.0f, 0.0f, 0.0f, 0.50f, 0.05f, 0.0f, 0.30f, 0.20f);
    set (s, Param::evolveBendPivot, 0.30f);
    set (s, Param::evolveBendRange, 0.30f);
    set (s, Param::evolveBendCurve, 0.45f);
    space (s, SpacePresets::Nebula, 0.28f, 0.50f, 0.50f, 0.20f);

    env (s, 1, 0.001f, 0.50f, 0.0f, 0.35f, 0.30f);
    macros (s, 0.25f, 0.40f, 0.25f, 0.45f);

    Routings r;
    r.uni (ModSource::Env1,        Param::evolveBend,       0.250f)
     .uni (ModSource::Velocity,    Param::evolveBend,       0.250f)
     .uni (ModSource::Velocity,    Param::impactBrightness, 0.300f)
     .uni (ModSource::Velocity,    Param::shapeBlend,       0.220f)
     .bi  (ModSource::NoteRandom,  Param::evolveBendPivot,  0.080f)
     .bi  (ModSource::NoteRandom,  Param::shapePitch,       0.005f)
     .bi  (ModSource::KeyTrack,    Param::shapeDecay,      -0.200f)
     .uni (ModSource::Macro1,      Param::evolveMotion,     0.300f)
     .uni (ModSource::Macro1,      Param::evolveSpeed,      0.250f)
     .uni (ModSource::Macro2,      Param::impactBrightness, 0.300f)
     .uni (ModSource::Macro2,      Param::shapeMass,       -0.200f)
     .uni (ModSource::Macro3,      Param::spaceMix,         0.300f)
     .uni (ModSource::Macro4,      Param::evolveBend,       0.300f)
     .uni (ModSource::Macro4,      Param::evolveBendRange,  0.250f);
    sharedMacros (r, Param::ampDecay, Param::impactRandom);
    r.commit (s);
}});

manager.addFactory ({ "Wire Harp", "PLUCK", { "metallic", "bright", "plucked", "resonant", "wide" }, [] (PatchState& s)
{
    impact (s, 2 /* PLUCK */, 0.50f, 0.55f, 0.10f, 0.85f, 0.45f, 0.10f);
    amp (s, 0.001f, 2.60f, 0.0f, 2.00f, 0.30f);
    shape (s, 0.42f, 0.30f, 0.34f, 0.58f, 0.70f, 0.10f);
    material (s, MaterialType::Metal, MaterialType::String, 0.45f);
    topology (s, 3 /* LATTICE */, 0.35f, 0.50f, 307);
    matter (s, 1.0f, 0.52f, 0.50f, 0.70f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.42f, 0.05f, 0.0f, 0.15f, 0.12f);
    space (s, SpacePresets::Shimmer, 0.36f, 0.60f, 0.62f, 0.28f);

    env (s, 1, 0.010f, 1.60f, 0.0f, 1.20f, 0.40f);
    lfo (s, 1, 0.20f, 1 /* TRIANGLE */, 1.0f, true, 0.7f);
    macros (s, 0.20f, 0.40f, 0.40f, 0.35f);

    Routings r;
    r.uni (ModSource::Env1,        Param::shapeSurface,     0.100f)
     .bi  (ModSource::LFO1,        Param::shapeTension,     0.020f)
     .uni (ModSource::Velocity,    Param::impactBrightness, 0.300f)
     .uni (ModSource::Velocity,    Param::shapeBlend,       0.220f)
     .uni (ModSource::Velocity,    Param::shapeMass,       -0.150f)
     .bi  (ModSource::NoteRandom,  Param::shapePitch,       0.006f)
     .bi  (ModSource::KeyTrack,    Param::shapeDecay,      -0.220f)
     .uni (ModSource::Macro1,      Param::evolveMotion,     0.300f)
     .uni (ModSource::Macro2,      Param::impactBrightness, 0.300f)
     .uni (ModSource::Macro2,      Param::spaceTone,        0.250f)
     .uni (ModSource::Macro3,      Param::spaceMix,         0.300f)
     .uni (ModSource::Macro3,      Param::spaceSize,        0.250f)
     .uni (ModSource::Macro4,      Param::evolveGravity,    0.250f)
     .uni (ModSource::Macro4,      Param::shapeCoupling,    0.200f);
    sharedMacros (r, Param::spaceFeedback, Param::impactRandom);
    r.commit (s);
}});

manager.addFactory ({ "Glass Inhale", "PLUCK", { "glassy", "cold", "plucked", "breathing", "distant" }, [] (PatchState& s)
{
    sample (s, BuiltInSamples::Kind::GlassStrike, 2 /* REVERSE */, 0.0f, 0.80f, 0.30f, 0.40f, 72, 0.55f);
    impact (s, 4 /* METAL STRIKE */, 0.60f, 0.58f, 0.15f, 0.85f, 0.45f, 0.10f, 0.0f, 0.60f);
    layerOnly (s, Param::sampleLevel, Param::impactLevel);
    amp (s, 0.001f, 1.60f, 0.10f, 1.10f, 0.35f);
    shape (s, 0.30f, 0.85f, 0.26f, 0.60f, 0.58f, 0.08f);
    material (s, MaterialType::Crystal, MaterialType::Void, 0.40f);
    topology (s, 5 /* STAR */, 0.22f, 0.58f, 271);
    matter (s, 0.88f, 0.50f, 0.55f, 0.80f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.45f, 0.05f, 0.0f, 0.20f, 0.15f);
    space (s, SpacePresets::Dream, 0.38f, 0.62f, 0.60f, 0.30f);

    env (s, 1, 0.001f, 0.80f, 0.0f, 0.60f, 0.30f);
    macros (s, 0.20f, 0.40f, 0.40f, 0.40f);

    Routings r;
    r.uni (ModSource::Env1,        Param::sampleLevel,     -0.150f)
     .uni (ModSource::Velocity,    Param::sampleLevel,      0.250f)
     .uni (ModSource::Velocity,    Param::impactBrightness, 0.300f)
     .uni (ModSource::Velocity,    Param::shapeBlend,       0.220f)
     .bi  (ModSource::NoteRandom,  Param::sampleStart,      0.060f)
     .bi  (ModSource::NoteRandom,  Param::shapePitch,       0.005f)
     .bi  (ModSource::KeyTrack,    Param::shapeDecay,      -0.200f)
     .uni (ModSource::Macro1,      Param::evolveMotion,     0.300f)
     .uni (ModSource::Macro2,      Param::impactBrightness, 0.300f)
     .uni (ModSource::Macro2,      Param::spaceTone,        0.250f)
     .uni (ModSource::Macro3,      Param::spaceMix,         0.300f)
     .uni (ModSource::Macro3,      Param::spaceSize,        0.200f)
     .uni (ModSource::Macro4,      Param::sampleLevel,      0.300f)
     .uni (ModSource::Macro4,      Param::shapeBlend,       0.250f);
    sharedMacros (r, Param::ampDecay, Param::sampleStart);
    r.commit (s);
}});

manager.addFactory ({ "Rubber Band", "PLUCK", { "organic", "soft", "plucked", "morphing", "dry" }, [] (PatchState& s)
{
    impact (s, 2 /* PLUCK */, 0.25f, 0.30f, 0.14f, 0.85f, 0.60f, 0.10f);
    amp (s, 0.002f, 0.75f, 0.0f, 0.40f, 0.35f);
    shape (s, 0.45f, 0.34f, 0.50f, 0.30f, 0.45f, 0.35f);
    material (s, MaterialType::Membrane, MaterialType::Organic, 0.45f);
    topology (s, 1 /* RING */, 0.45f, 0.40f, 383);
    matter (s, 0.90f, 0.55f, 0.48f, 0.45f);
    evolve (s, 0.0f, 0.45f, 0.0f, 0.0f, 0.58f, 0.08f, 0.0f, 0.40f, 0.30f);
    space (s, SpacePresets::Chamber, 0.20f, 0.30f, 0.40f, 0.15f);

    env (s, 1, 0.001f, 0.60f, 0.0f, 0.40f, 0.35f);
    macros (s, 0.35f, 0.30f, 0.20f, 0.45f);

    Routings r;
    r.uni (ModSource::Env1,        Param::evolveMelt,       0.300f)
     .uni (ModSource::Velocity,    Param::evolveMelt,      -0.200f)
     .uni (ModSource::Velocity,    Param::shapeBlend,       0.220f)
     .uni (ModSource::Velocity,    Param::impactBrightness, 0.250f)
     .bi  (ModSource::NoteRandom,  Param::shapeTension,     0.050f)
     .bi  (ModSource::NoteRandom,  Param::shapePitch,       0.006f)
     .bi  (ModSource::KeyTrack,    Param::shapeMass,       -0.180f)
     .uni (ModSource::Macro1,      Param::evolveMotion,     0.300f)
     .uni (ModSource::Macro1,      Param::evolveSpeed,      0.250f)
     .uni (ModSource::Macro2,      Param::impactBrightness, 0.300f)
     .uni (ModSource::Macro2,      Param::shapeMass,       -0.200f)
     .uni (ModSource::Macro3,      Param::spaceMix,         0.300f)
     .uni (ModSource::Macro4,      Param::evolveMelt,       0.300f)
     .uni (ModSource::Macro4,      Param::shapeTension,     0.200f);
    sharedMacros (r, Param::ampDecay, Param::impactRandom);
    r.commit (s);
}});

manager.addFactory ({ "Thumb Iron", "PLUCK", { "metallic", "wooden", "plucked", "close", "melodic" }, [] (PatchState& s)
{
    impact (s, 2 /* PLUCK */, 0.60f, 0.45f, 0.06f, 0.85f, 0.45f, 0.10f);
    amp (s, 0.001f, 1.00f, 0.0f, 0.60f, 0.30f);
    shape (s, 0.33f, 0.48f, 0.45f, 0.50f, 0.52f, 0.20f);
    material (s, MaterialType::Metal, MaterialType::Wood, 0.45f);
    topology (s, 2 /* CLUSTERS */, 0.40f, 0.38f, 431);
    matter (s, 0.93f, 0.54f, 0.54f, 0.50f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.55f, 0.10f, 0.0f, 0.22f, 0.12f);
    space (s, SpacePresets::Chamber, 0.24f, 0.35f, 0.45f, 0.18f);

    env (s, 1, 0.001f, 0.35f, 0.0f, 0.25f, 0.25f);
    macros (s, 0.15f, 0.35f, 0.25f, 0.35f);

    Routings r;
    r.uni (ModSource::Env1,        Param::shapeSurface,     0.150f)
     .uni (ModSource::Velocity,    Param::impactHardness,   0.300f)
     .uni (ModSource::Velocity,    Param::impactBrightness, 0.280f)
     .uni (ModSource::Velocity,    Param::shapeBlend,       0.220f)
     .uni (ModSource::Velocity,    Param::shapeSurface,     0.150f)
     .bi  (ModSource::NoteRandom,  Param::shapeMass,        0.060f)
     .bi  (ModSource::NoteRandom,  Param::shapePitch,       0.007f)
     .bi  (ModSource::KeyTrack,    Param::shapeDecay,      -0.200f)
     .uni (ModSource::Macro1,      Param::evolveMotion,     0.300f)
     .uni (ModSource::Macro2,      Param::impactBrightness, 0.300f)
     .uni (ModSource::Macro2,      Param::shapeMass,       -0.200f)
     .uni (ModSource::Macro3,      Param::spaceMix,         0.300f)
     .uni (ModSource::Macro4,      Param::shapeCoupling,    0.300f)
     .uni (ModSource::Macro4,      Param::shapeSurface,     0.200f);
    sharedMacros (r, Param::ampDecay, Param::impactRandom);
    r.commit (s);
}});

//--------------------------------------------------------------------------
// What answers the pluck: sympathetic wires, a dead hand on the bridge, a
// cloud of static, and two ways of getting a string moving without a finger.
//--------------------------------------------------------------------------

manager.addFactory ({ "Sitar Shadow", "PLUCK", { "metallic", "bright", "plucked", "resonant", "roomy" }, [] (PatchState& s)
{
    impact (s, 2 /* PLUCK */, 0.68f, 0.60f, 0.07f, 0.90f, 0.40f, 0.10f);
    amp (s, 0.001f, 1.90f, 0.0f, 1.30f, 0.30f);
    shape (s, 0.48f, 0.20f, 0.36f, 0.60f, 0.58f, 0.28f);
    material (s, MaterialType::String, MaterialType::Metal, 0.50f);
    topology (s, 4 /* RANDOM */, 0.50f, 0.55f, 449);
    matter (s, 0.96f, 0.55f, 0.62f, 0.60f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.48f, 0.06f, 0.0f, 0.20f, 0.12f);

    FractureShape sympathetic;
    sympathetic.fragments   = 32;
    sympathetic.delayLow    = 0.04f; sympathetic.delayHigh    = 0.30f;
    sympathetic.feedbackLow = 0.20f; sympathetic.feedbackHigh = 0.50f;
    sympathetic.decayLow    = 0.40f; sympathetic.decayHigh    = 0.68f;
    sympathetic.spreadLow   = 0.30f; sympathetic.spreadHigh   = 0.90f;
    sympathetic.panWidth    = 0.70f;
    sympathetic.pitchCycle  = kFifthTerrace;
    sympathetic.pattern     = "XLXHXLXH";
    fracture (s, 0 /* SPECTRAL */, 0.45f, 0.32f, 0.60f, 0.25f, 0.38f, 0.28f, 0.62f, 0.60f, 0.06f,
              2 /* 32 */, 4, 8, 0.0f, 0, 1.0f, 0.18f, 521, sympathetic);

    space (s, SpacePresets::Nebula, 0.32f, 0.55f, 0.50f, 0.25f);

    env (s, 1, 0.001f, 0.70f, 0.0f, 0.50f, 0.30f);
    macros (s, 0.25f, 0.40f, 0.30f, 0.45f);

    Routings r;
    r.uni (ModSource::Env1,        Param::fractureFeedback, 0.150f)
     .uni (ModSource::Velocity,    Param::fractureFeedback, 0.200f)
     .uni (ModSource::Velocity,    Param::impactBrightness, 0.300f)
     .uni (ModSource::Velocity,    Param::shapeSurface,     0.250f)
     .uni (ModSource::Velocity,    Param::shapeBlend,       0.220f)
     .bi  (ModSource::NoteRandom,  Param::fractureSpread,   0.100f)
     .bi  (ModSource::KeyTrack,    Param::shapeDecay,      -0.200f)
     .bi  (ModSource::KeyTrack,    Param::fractureTone,     0.150f)
     .uni (ModSource::Macro1,      Param::fractureEvolve,   0.300f)
     .uni (ModSource::Macro1,      Param::evolveMotion,     0.250f)
     .uni (ModSource::Macro2,      Param::impactBrightness, 0.300f)
     .uni (ModSource::Macro2,      Param::fractureTone,     0.200f)
     .uni (ModSource::Macro3,      Param::spaceMix,         0.300f)
     .uni (ModSource::Macro4,      Param::fractureMix,      0.300f)
     .uni (ModSource::Macro4,      Param::shapeSurface,     0.200f);
    sharedMacros (r, Param::fractureDecay, Param::fractureRandom);
    r.commit (s);
}});

manager.addFactory ({ "Void Pizzicato", "PLUCK", { "dark", "hollow", "plucked", "low", "huge" }, [] (PatchState& s)
{
    impact (s, 2 /* PLUCK */, 0.35f, 0.25f, 0.12f, 0.85f, 0.55f, 0.08f);
    amp (s, 0.002f, 1.80f, 0.0f, 1.30f, 0.35f);
    shape (s, 0.25f, 0.55f, 0.58f, 0.42f, 0.50f, 0.15f);
    material (s, MaterialType::Void, MaterialType::String, 0.35f);
    topology (s, 0 /* CHAIN */, 0.25f, 0.35f, 509);
    matter (s, 0.97f, 0.52f, 0.28f, 0.60f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.66f, 0.05f, 0.0f, 0.12f, 0.15f);
    space (s, SpacePresets::Void, 0.32f, 0.75f, 0.30f, 0.25f);

    env (s, 1, 0.002f, 1.20f, 0.0f, 0.90f, 0.35f);
    macros (s, 0.20f, 0.25f, 0.45f, 0.40f);

    Routings r;
    r.uni (ModSource::Env1,        Param::shapeSurface,     0.150f)
     .uni (ModSource::Velocity,    Param::shapeBlend,       0.220f)
     .uni (ModSource::Velocity,    Param::impactBrightness, 0.280f)
     .uni (ModSource::Velocity,    Param::shapeMass,       -0.150f)
     .uni (ModSource::Velocity,    Param::evolveGravity,   -0.150f)
     .bi  (ModSource::NoteRandom,  Param::shapePitch,       0.006f)
     .bi  (ModSource::KeyTrack,    Param::shapeMass,       -0.220f)
     .bi  (ModSource::KeyTrack,    Param::shapeDecay,      -0.180f)
     .uni (ModSource::Macro1,      Param::evolveMotion,     0.300f)
     .uni (ModSource::Macro2,      Param::impactBrightness, 0.300f)
     .uni (ModSource::Macro2,      Param::shapeMass,       -0.200f)
     .uni (ModSource::Macro3,      Param::spaceMix,         0.300f)
     .uni (ModSource::Macro3,      Param::spaceSize,        0.250f)
     .uni (ModSource::Macro4,      Param::evolveGravity,    0.250f)
     .uni (ModSource::Macro4,      Param::shapeMass,        0.200f);
    sharedMacros (r, Param::ampDecay, Param::impactRandom);
    r.commit (s);
}});

manager.addFactory ({ "Ice Splinter", "PLUCK", { "cold", "glassy", "plucked", "high", "wide" }, [] (PatchState& s)
{
    impact (s, 1 /* CLICK */, 0.85f, 0.80f, 0.03f, 0.90f, 0.30f, 0.12f);
    amp (s, 0.001f, 0.50f, 0.0f, 0.35f, 0.25f);
    shape (s, 0.26f, 0.88f, 0.18f, 0.65f, 0.50f, 0.06f);
    material (s, MaterialType::Crystal, MaterialType::Liquid, 0.30f);
    topology (s, 5 /* STAR */, 0.18f, 0.60f, 653);
    matter (s, 0.97f, 0.60f, 0.60f, 0.85f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.45f, 0.06f, 0.0f, 0.30f, 0.15f);

    FractureShape shards;
    shards.fragments   = 32;
    shards.delayLow    = 0.02f; shards.delayHigh    = 0.16f;
    shards.feedbackLow = 0.08f; shards.feedbackHigh = 0.30f;
    shards.decayLow    = 0.20f; shards.decayHigh    = 0.45f;
    shards.spreadLow   = 0.45f; shards.spreadHigh   = 0.95f;
    shards.panWidth    = 0.85f;
    shards.probability = 0.90f;
    shards.pitchCycle  = kOctaveTerrace;
    shards.pattern     = "XHXHXHXH";
    fracture (s, 0 /* SPECTRAL */, 0.40f, 0.28f, 0.70f, 0.30f, 0.20f, 0.15f, 0.35f, 0.70f, 0.08f,
              2 /* 32 */, 5, 8, 0.0f, 0, 0.90f, 0.25f, 683, shards);

    space (s, SpacePresets::Shimmer, 0.34f, 0.50f, 0.70f, 0.25f);

    env (s, 1, 0.001f, 0.25f, 0.0f, 0.20f, 0.25f);
    macros (s, 0.20f, 0.45f, 0.30f, 0.35f);

    Routings r;
    r.uni (ModSource::Env1,        Param::fractureAmount,   0.120f)
     .uni (ModSource::Velocity,    Param::impactBrightness, 0.320f)
     .uni (ModSource::Velocity,    Param::impactHardness,   0.280f)
     .uni (ModSource::Velocity,    Param::shapeBlend,       0.220f)
     .uni (ModSource::Velocity,    Param::fractureSpread,   0.200f)
     .bi  (ModSource::NoteRandom,  Param::shapePitch,       0.006f)
     .bi  (ModSource::NoteRandom,  Param::fractureSpread,   0.100f)
     .bi  (ModSource::KeyTrack,    Param::shapeDecay,      -0.200f)
     .uni (ModSource::Macro1,      Param::fractureEvolve,   0.300f)
     .uni (ModSource::Macro1,      Param::evolveMotion,     0.250f)
     .uni (ModSource::Macro2,      Param::impactBrightness, 0.300f)
     .uni (ModSource::Macro2,      Param::spaceTone,        0.250f)
     .uni (ModSource::Macro3,      Param::spaceMix,         0.300f)
     .uni (ModSource::Macro4,      Param::fractureMix,      0.300f)
     .uni (ModSource::Macro4,      Param::shapeTension,     0.200f);
    sharedMacros (r, Param::fractureDecay, Param::fractureRandom);
    r.commit (s);
}});

manager.addFactory ({ "Palm Mute", "PLUCK", { "dark", "soft", "plucked", "dry", "rhythmic" }, [] (PatchState& s)
{
    impact (s, 2 /* PLUCK */, 0.60f, 0.45f, 0.05f, 0.90f, 0.45f, 0.06f);
    amp (s, 0.001f, 0.28f, 0.0f, 0.15f, 0.25f);
    shape (s, 0.38f, 0.12f, 0.48f, 0.50f, 0.16f, 0.25f);
    material (s, MaterialType::String, MaterialType::Membrane, 0.40f);
    topology (s, 0 /* CHAIN */, 0.32f, 0.30f, 587);
    matter (s, 0.92f, 0.50f, 0.75f, 0.30f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.55f, 0.04f, 0.0f, 0.20f, 0.08f);
    space (s, SpacePresets::Chamber, 0.14f, 0.20f, 0.45f, 0.10f);

    env (s, 1, 0.001f, 0.15f, 0.0f, 0.12f, 0.25f);
    macros (s, 0.15f, 0.35f, 0.15f, 0.40f);

    Routings r;
    r.uni (ModSource::Env1,        Param::shapeSurface,     0.150f)
     .uni (ModSource::Velocity,    Param::shapeDecay,       0.300f)
     .uni (ModSource::Velocity,    Param::ampDecay,         0.150f)
     .uni (ModSource::Velocity,    Param::impactBrightness, 0.300f)
     .uni (ModSource::Velocity,    Param::shapeBlend,       0.220f)
     .bi  (ModSource::NoteRandom,  Param::shapeSurface,     0.060f)
     .bi  (ModSource::KeyTrack,    Param::shapeDecay,      -0.150f)
     .uni (ModSource::Macro1,      Param::evolveMotion,     0.300f)
     .uni (ModSource::Macro2,      Param::impactBrightness, 0.300f)
     .uni (ModSource::Macro2,      Param::shapeMass,       -0.200f)
     .uni (ModSource::Macro3,      Param::spaceMix,         0.300f)
     .uni (ModSource::Macro4,      Param::shapeDecay,       0.350f)
     .uni (ModSource::Macro4,      Param::ampDecay,         0.300f);
    sharedMacros (r, Param::ampDecay, Param::impactRandom);
    r.commit (s);
}});

manager.addFactory ({ "Dust Harp", "PLUCK", { "glassy", "noisy", "plucked", "granular", "wide" }, [] (PatchState& s)
{
    dust (s, 5 /* CRACKLE */, 0.72f, 0.55f, 0.05f, 0.65f, 0.60f, 0.60f, 907, 0.55f);
    impact (s, 0 /* IMPULSE */, 0.60f, 0.50f, 0.20f, 0.85f, 0.40f, 0.15f, 0.0f, 0.35f);
    layerOnly (s, Param::dustLevel, Param::impactLevel);
    amp (s, 0.002f, 1.20f, 0.05f, 0.70f, 0.35f);
    shape (s, 0.44f, 0.78f, 0.30f, 0.55f, 0.55f, 0.12f);
    material (s, MaterialType::Custom, MaterialType::Crystal, 0.50f);
    topology (s, 3 /* LATTICE */, 0.30f, 0.55f, 967);
    matter (s, 0.96f, 0.60f, 0.45f, 0.70f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.48f, 0.08f, 0.0f, 0.25f, 0.15f);
    space (s, SpacePresets::Dust, 0.32f, 0.50f, 0.55f, 0.25f);

    env (s, 1, 0.001f, 0.30f, 0.0f, 0.25f, 0.25f);
    macros (s, 0.25f, 0.35f, 0.30f, 0.45f);

    Routings r;
    r.uni (ModSource::Env1,        Param::dustDensity,      0.300f)
     .uni (ModSource::Velocity,    Param::dustDensity,      0.250f)
     .uni (ModSource::Velocity,    Param::dustColor,        0.200f)
     .uni (ModSource::Velocity,    Param::shapeBlend,       0.220f)
     .bi  (ModSource::NoteRandom,  Param::dustColor,        0.120f)
     .bi  (ModSource::NoteRandom,  Param::shapePitch,       0.005f)
     .bi  (ModSource::KeyTrack,    Param::shapeDecay,      -0.200f)
     .uni (ModSource::Macro1,      Param::evolveMotion,     0.300f)
     .uni (ModSource::Macro1,      Param::dustJitter,       0.250f)
     .uni (ModSource::Macro2,      Param::dustColor,        0.300f)
     .uni (ModSource::Macro2,      Param::shapeExcite,      0.250f)
     .uni (ModSource::Macro3,      Param::spaceMix,         0.300f)
     .uni (ModSource::Macro4,      Param::dustDensity,      0.300f)
     .uni (ModSource::Macro4,      Param::dustGrain,        0.250f);
    sharedMacros (r, Param::ampDecay, Param::dustJitter);
    r.commit (s);
}});

manager.addFactory ({ "Needle Ping", "PLUCK", { "metallic", "clean", "plucked", "high", "close" }, [] (PatchState& s)
{
    sample (s, BuiltInSamples::Kind::MetalPing, 0 /* ONE SHOT */, 0.0f, 0.60f, 0.25f, 0.35f, 72, 0.70f);
    amp (s, 0.001f, 0.90f, 0.0f, 0.50f, 0.30f);
    shape (s, 0.30f, 0.52f, 0.25f, 0.58f, 0.50f, 0.10f);
    material (s, MaterialType::Metal, MaterialType::Crystal, 0.50f);
    topology (s, 1 /* RING */, 0.25f, 0.50f, 743);
    matter (s, 0.85f, 0.55f, 0.55f, 0.60f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.50f, 0.05f, 0.0f, 0.20f, 0.12f);
    space (s, SpacePresets::Orbit, 0.30f, 0.40f, 0.60f, 0.35f);

    env (s, 1, 0.001f, 0.40f, 0.0f, 0.30f, 0.25f);
    macros (s, 0.20f, 0.35f, 0.35f, 0.35f);

    Routings r;
    r.uni (ModSource::Env1,        Param::shapeSurface,     0.150f)
     .uni (ModSource::Velocity,    Param::sampleLevel,      0.250f)
     .uni (ModSource::Velocity,    Param::shapeBlend,       0.220f)
     .uni (ModSource::Velocity,    Param::shapeMass,       -0.150f)
     .uni (ModSource::Velocity,    Param::spaceMix,        -0.120f)
     .bi  (ModSource::NoteRandom,  Param::sampleStart,      0.050f)
     .bi  (ModSource::NoteRandom,  Param::shapePitch,       0.006f)
     .bi  (ModSource::KeyTrack,    Param::shapeDecay,      -0.200f)
     .uni (ModSource::Macro1,      Param::evolveMotion,     0.300f)
     .uni (ModSource::Macro2,      Param::shapeMass,       -0.200f)
     .uni (ModSource::Macro2,      Param::spaceTone,        0.250f)
     .uni (ModSource::Macro3,      Param::spaceMix,         0.300f)
     .uni (ModSource::Macro3,      Param::spaceFeedback,    0.200f)
     .uni (ModSource::Macro4,      Param::sampleLevel,      0.300f)
     .uni (ModSource::Macro4,      Param::shapeBlend,       0.250f);
    sharedMacros (r, Param::spaceFeedback, Param::sampleStart);
    r.commit (s);
}});

manager.addFactory ({ "Nail Scrape", "PLUCK", { "wooden", "harsh", "scraped", "dry", "close" }, [] (PatchState& s)
{
    gesture (s, 1 /* SCRAPE */, 0.60f, 0.55f, 0.55f, 0.40f, 0.25f, 0.50f, 0.80f);
    amp (s, 0.001f, 0.60f, 0.0f, 0.35f, 0.25f);
    shape (s, 0.42f, 0.62f, 0.38f, 0.50f, 0.40f, 0.30f);
    material (s, MaterialType::Wood, MaterialType::Organic, 0.40f);
    topology (s, 4 /* RANDOM */, 0.40f, 0.45f, 811);
    matter (s, 0.90f, 0.62f, 0.40f, 0.50f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.52f, 0.10f, 0.0f, 0.35f, 0.20f);
    space (s, SpacePresets::Chamber, 0.22f, 0.32f, 0.50f, 0.18f);

    env (s, 1, 0.001f, 0.12f, 0.0f, 0.10f, 0.25f);
    macros (s, 0.30f, 0.40f, 0.20f, 0.45f);

    Routings r;
    r.uni (ModSource::Env1,        Param::gesturePressure,  0.350f)
     .uni (ModSource::Velocity,    Param::gestureRoughness, 0.300f)
     .uni (ModSource::Velocity,    Param::gesturePressure,  0.250f)
     .uni (ModSource::Velocity,    Param::shapeExcite,      0.200f)
     .bi  (ModSource::NoteRandom,  Param::gesturePosition,  0.150f)
     .bi  (ModSource::NoteRandom,  Param::gestureSpeed,     0.120f)
     .bi  (ModSource::KeyTrack,    Param::shapeDecay,      -0.180f)
     .uni (ModSource::Macro1,      Param::gestureMotion,    0.300f)
     .uni (ModSource::Macro1,      Param::evolveMotion,     0.250f)
     .uni (ModSource::Macro2,      Param::gestureBandwidth, 0.300f)
     .uni (ModSource::Macro2,      Param::gestureRoughness, 0.250f)
     .uni (ModSource::Macro3,      Param::spaceMix,         0.300f)
     .uni (ModSource::Macro4,      Param::gesturePressure,  0.300f)
     .uni (ModSource::Macro4,      Param::shapeSurface,     0.200f);
    sharedMacros (r, Param::ampDecay, Param::gestureRoughness);
    r.commit (s);
}});

manager.addFactory ({ "Bow Snap", "PLUCK", { "organic", "warm", "bowed", "plucked", "close" }, [] (PatchState& s)
{
    gesture (s, 0 /* BOW */, 0.75f, 0.60f, 0.40f, 0.25f, 0.10f, 0.55f, 0.50f);
    impact (s, 2 /* PLUCK */, 0.50f, 0.50f, 0.06f, 0.90f, 0.40f, 0.06f, 0.0f, 0.60f);
    layerOnly (s, Param::gestureLevel, Param::impactLevel);
    amp (s, 0.001f, 1.10f, 0.0f, 0.60f, 0.30f);
    shape (s, 0.38f, 0.18f, 0.36f, 0.55f, 0.50f, 0.20f);
    material (s, MaterialType::Organic, MaterialType::String, 0.50f);
    topology (s, 0 /* CHAIN */, 0.30f, 0.40f, 877);
    matter (s, 0.93f, 0.58f, 0.50f, 0.40f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.50f, 0.05f, 0.0f, 0.20f, 0.10f);
    space (s, SpacePresets::Chamber, 0.26f, 0.40f, 0.50f, 0.20f);

    env (s, 1, 0.001f, 0.18f, 0.0f, 0.15f, 0.25f);
    macros (s, 0.20f, 0.35f, 0.25f, 0.40f);

    Routings r;
    r.uni (ModSource::Env1,        Param::gesturePressure,  0.300f)
     .uni (ModSource::Env1,        Param::gestureSpeed,     0.200f)
     .uni (ModSource::Velocity,    Param::gesturePressure,  0.250f)
     .uni (ModSource::Velocity,    Param::impactBrightness, 0.300f)
     .uni (ModSource::Velocity,    Param::shapeBlend,       0.220f)
     .bi  (ModSource::NoteRandom,  Param::gesturePosition,  0.100f)
     .bi  (ModSource::KeyTrack,    Param::shapeDecay,      -0.200f)
     .uni (ModSource::Macro1,      Param::gestureMotion,    0.300f)
     .uni (ModSource::Macro1,      Param::evolveMotion,     0.250f)
     .uni (ModSource::Macro2,      Param::impactBrightness, 0.300f)
     .uni (ModSource::Macro2,      Param::gestureBandwidth, 0.250f)
     .uni (ModSource::Macro3,      Param::spaceMix,         0.300f)
     .uni (ModSource::Macro4,      Param::gestureLevel,     0.300f)
     .uni (ModSource::Macro4,      Param::gestureRoughness, 0.250f);
    sharedMacros (r, Param::ampDecay, Param::gestureRoughness);
    r.commit (s);
}});
}

} // namespace am::FactoryContent
