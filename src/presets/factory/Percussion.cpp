#include "presets/FactoryBuilders.h"

namespace am::FactoryContent
{

namespace
{
    /** LAYER renders every source that still has level, so a two-source patch has
        to say which two it means: everything else is muted here. */
    void layerSources (PatchState& s, int a, int b)
    {
        set (s, Param::sourceMode, 1.0f);
        static const Param level[5] = { Param::waveLevel, Param::dustLevel, Param::impactLevel,
                                        Param::sampleLevel, Param::gestureLevel };
        for (int i = 0; i < 5; ++i)
            if (i != a && i != b)
                set (s, level[(size_t) i], 0.0f);
    }
}


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

//---------------------------------------------------------------- skins and wood

manager.addFactory ({ "Clay Tabla", "PERCUSSION", { "struck", "organic", "warm", "close", "mid" }, [] (PatchState& s)
{
    // The head is pulled tight by the strike and settles back: Env1 drives BEND,
    // so the pitch bends down through the first fifth of a second and stays.
    impact (s, 6 /* MEMBRANE HIT */, 0.52f, 0.48f, 0.16f, 0.90f, 0.40f, 0.10f, 0.0f, 0.85f);
    amp (s, 0.001f, 0.50f, 0.0f, 0.40f, 0.30f);
    set (s, Param::masterGain, -3.0f);
    shape (s, 0.30f, 0.30f, 0.44f, 0.56f, 0.36f, 0.30f);
    material (s, MaterialType::Membrane, MaterialType::Organic, 0.35f);
    topology (s, 0 /* CHAIN */, 0.34f, 0.45f, 101);
    matter (s, 1.0f, 0.50f, 0.60f, 0.38f);
    evolve (s, 0.34f, 0.0f, 0.0f, 0.0f, 0.52f, 0.06f, 0.0f, 0.50f, 0.28f);
    set (s, Param::evolveBendPivot, 0.30f);
    set (s, Param::evolveBendRange, 0.32f);
    set (s, Param::evolveBendCurve, 0.62f);
    space (s, SpacePresets::Chamber, 0.20f, 0.24f, 0.55f, 0.16f);

    env (s, 1, 0.001f, 0.20f, 0.0f, 0.16f, 0.25f);
    macros (s, 0.20f, 0.42f, 0.26f, 0.45f);

    Routings r;
    r.uni (ModSource::Env1,      Param::evolveBend,       0.320f)
     .uni (ModSource::Velocity,  Param::impactHardness,   0.400f)
     .uni (ModSource::Velocity,  Param::shapeTension,     0.240f)
     .uni (ModSource::Velocity,  Param::shapeStrike,      0.250f)
     .bi  (ModSource::KeyTrack,  Param::shapeDecay,      -0.240f)
     .bi  (ModSource::NoteRandom, Param::shapeSurface,    0.060f)
     .uni (ModSource::Macro1,    Param::evolveMotion,     0.350f)
     .uni (ModSource::Macro2,    Param::impactBrightness, 0.350f)
     .uni (ModSource::Macro2,    Param::shapeExcite,      0.200f)
     .uni (ModSource::Macro3,    Param::spaceMix,         0.300f)
     .uni (ModSource::Macro4,    Param::evolveBend,       0.300f)
     .uni (ModSource::Macro4,    Param::shapeTension,     0.220f);
    sharedMacros (r, Param::ampDecay, Param::impactRandom);
    r.commit (s);
}});

manager.addFactory ({ "Frame Shell", "PERCUSSION", { "struck", "wooden", "noisy", "roomy", "mid" }, [] (PatchState& s)
{
    // A wide shallow frame drum: the head is the IMPACT, the shell rattling
    // along with it is a thin CRACKLE layer that velocity opens up.
    dust (s, 5 /* CRACKLE */, 0.34f, 0.56f, 0.40f, 0.50f, 0.60f, 0.70f, 733, 0.30f);
    impact (s, 6 /* MEMBRANE HIT */, 0.36f, 0.36f, 0.24f, 0.90f, 0.55f, 0.15f, 0.0f, 0.80f);
    layerSources (s, 1 /* DUST */, 2 /* IMPACT */);
    amp (s, 0.002f, 0.55f, 0.06f, 0.45f, 0.30f);
    set (s, Param::masterGain, -2.0f);
    shape (s, 0.40f, 0.26f, 0.56f, 0.40f, 0.42f, 0.46f);
    material (s, MaterialType::Membrane, MaterialType::Wood, 0.45f);
    topology (s, 3 /* LATTICE */, 0.42f, 0.50f, 211);
    matter (s, 0.95f, 0.50f, 0.55f, 0.60f);
    evolve (s, 0.0f, 0.14f, 0.0f, 0.0f, 0.50f, 0.14f, 0.0f, 0.38f, 0.20f);
    space (s, SpacePresets::Chamber, 0.34f, 0.44f, 0.50f, 0.26f);

    env (s, 1, 0.001f, 0.26f, 0.0f, 0.20f, 0.25f);
    macros (s, 0.22f, 0.40f, 0.36f, 0.45f);

    Routings r;
    r.uni (ModSource::Env1,      Param::dustDensity,      0.260f)
     .uni (ModSource::Velocity,  Param::dustDensity,      0.300f)
     .uni (ModSource::Velocity,  Param::impactHardness,   0.340f)
     .uni (ModSource::Velocity,  Param::shapeStrike,      0.200f)
     .bi  (ModSource::KeyTrack,  Param::dustColor,        0.220f)
     .bi  (ModSource::NoteRandom, Param::dustGrain,       0.090f)
     .uni (ModSource::Macro1,    Param::evolveMotion,     0.300f)
     .uni (ModSource::Macro2,    Param::impactBrightness, 0.300f)
     .uni (ModSource::Macro2,    Param::dustColor,        0.250f)
     .uni (ModSource::Macro3,    Param::spaceMix,         0.320f)
     .uni (ModSource::Macro4,    Param::dustLevel,        0.300f)
     .uni (ModSource::Macro4,    Param::shapeSurface,     0.250f);
    sharedMacros (r, Param::ampDecay, Param::dustJitter);
    r.commit (s);
}});

manager.addFactory ({ "Water Timpani", "PERCUSSION", { "struck", "dark", "sub", "huge", "evolving" }, [] (PatchState& s)
{
    // A kettle full of water: MELT rises across the ring, so the pitch sags and
    // the partials widen the longer the note is left alone.
    impact (s, 6 /* MEMBRANE HIT */, 0.22f, 0.24f, 0.34f, 0.95f, 0.65f, 0.12f, 0.0f, 0.90f);
    amp (s, 0.002f, 1.10f, 0.0f, 0.80f, 0.35f);
    set (s, Param::masterGain, -3.5f);
    shape (s, 0.30f, 0.22f, 0.80f, 0.30f, 0.56f, 0.34f);
    material (s, MaterialType::Membrane, MaterialType::Liquid, 0.40f);
    topology (s, 2 /* CLUSTERS */, 0.50f, 0.34f, 277);
    matter (s, 1.0f, 0.46f, 0.60f, 0.50f);
    evolve (s, 0.0f, 0.30f, 0.0f, 0.0f, 0.62f, 0.10f, 0.0f, 0.20f, 0.30f);
    space (s, SpacePresets::Void, 0.30f, 0.72f, 0.34f, 0.30f);

    env (s, 1, 0.030f, 1.40f, 0.0f, 0.90f, 0.65f);
    lfo (s, 1, 0.35f, 0 /* SINE */, 1.0f, true, 0.4f);
    macros (s, 0.30f, 0.30f, 0.40f, 0.50f);

    Routings r;
    r.uni (ModSource::Env1,      Param::evolveMelt,       0.300f)
     .bi  (ModSource::LFO1,      Param::shapeMass,        0.050f)
     .uni (ModSource::Velocity,  Param::impactHardness,   0.300f)
     .uni (ModSource::Velocity,  Param::shapeStrike,      0.260f)
     .bi  (ModSource::KeyTrack,  Param::shapeMass,       -0.280f)
     .bi  (ModSource::NoteRandom, Param::shapePitch,      0.120f)
     .uni (ModSource::Macro1,    Param::evolveMotion,     0.380f)
     .uni (ModSource::Macro2,    Param::impactBrightness, 0.320f)
     .uni (ModSource::Macro2,    Param::spaceTone,        0.240f)
     .uni (ModSource::Macro3,    Param::spaceMix,         0.300f)
     .uni (ModSource::Macro3,    Param::spaceSize,        0.180f)
     .uni (ModSource::Macro4,    Param::evolveMelt,       0.320f)
     .uni (ModSource::Macro4,    Param::shapeMass,        0.220f);
    sharedMacros (r, Param::ampDecay, Param::impactRandom);
    r.commit (s);
}});

manager.addFactory ({ "Rope Snare", "PERCUSSION", { "struck", "harsh", "rhythmic", "dry", "mid" }, [] (PatchState& s)
{
    // A rope-tensioned field drum. Velocity does not just make it louder: it
    // drives source.impact.rate, so a hard hit turns the stroke into a buzz roll.
    impact (s, 3 /* NOISE STRIKE */, 0.58f, 0.74f, 0.06f, 0.92f, 0.30f, 0.34f, 0.26f, 0.80f);
    amp (s, 0.001f, 0.34f, 0.14f, 0.28f, 0.25f);
    shape (s, 0.56f, 0.44f, 0.34f, 0.62f, 0.26f, 0.42f);
    material (s, MaterialType::Membrane, MaterialType::Metal, 0.40f);
    topology (s, 4 /* RANDOM */, 0.44f, 0.56f, 331);
    matter (s, 0.92f, 0.62f, 0.66f, 0.55f);
    evolve (s, 0.0f, 0.0f, 0.16f, 0.0f, 0.44f, 0.22f, 0.0f, 0.55f, 0.28f);
    set (s, Param::evolveScatterSeed, 337);
    fracture (s, 2 /* TRANSIENT */, 0.38f, 0.34f, 0.45f, 0.40f, 0.18f, 0.14f, 0.30f, 0.62f, 0.15f,
              1 /* 16 */, 5 /* 1/32 */, 8, 0.0f, 0 /* FORWARD */, 0.85f, 0.35f, 727,
              FractureShape { 16, 0.01f, 0.14f, 0.08f, 0.28f, 0.25f, 0.45f, 0.25f, 0.80f,
                              1.0f, 1.0f, 0.55f, 0.85f, nullptr, "XoXoXXoo", nullptr });
    space (s, SpacePresets::Chamber, 0.22f, 0.26f, 0.60f, 0.18f);

    env (s, 1, 0.001f, 0.14f, 0.0f, 0.12f, 0.20f);
    macros (s, 0.30f, 0.45f, 0.24f, 0.50f);

    Routings r;
    r.uni (ModSource::Env1,      Param::fractureAmount,   0.220f)
     .uni (ModSource::Velocity,  Param::impactRate,       0.350f)
     .uni (ModSource::Velocity,  Param::impactHardness,   0.300f)
     .uni (ModSource::Velocity,  Param::shapeSurface,     0.200f)
     .bi  (ModSource::KeyTrack,  Param::impactBrightness, 0.200f)
     .bi  (ModSource::NoteRandom, Param::impactRandom,    0.100f)
     .uni (ModSource::Macro1,    Param::impactRate,       0.400f)
     .uni (ModSource::Macro2,    Param::impactBrightness, 0.320f)
     .uni (ModSource::Macro3,    Param::spaceMix,         0.280f)
     .uni (ModSource::Macro4,    Param::fractureAmount,   0.300f)
     .uni (ModSource::Macro4,    Param::evolveTear,       0.220f);
    sharedMacros (r, Param::fractureDecay, Param::fractureRandom);
    r.commit (s);
}});

manager.addFactory ({ "Split Maple", "PERCUSSION", { "struck", "wooden", "clean", "dry", "melodic" }, [] (PatchState& s)
{
    // Claves. Nothing but the click of one hard block on another and the very
    // short body behind it — the reference for how dry this instrument can get.
    impact (s, 1 /* CLICK */, 0.74f, 0.62f, 0.10f, 0.85f, 0.25f, 0.08f, 0.0f, 0.90f);
    amp (s, 0.0005f, 0.16f, 0.0f, 0.12f, 0.20f);
    set (s, Param::masterGain, -1.5f);
    shape (s, 0.18f, 0.24f, 0.30f, 0.56f, 0.18f, 0.14f);
    material (s, MaterialType::Wood, MaterialType::Crystal, 0.22f);
    topology (s, 0 /* CHAIN */, 0.22f, 0.30f, 373);
    matter (s, 1.0f, 0.56f, 0.64f, 0.28f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.44f, 0.04f, 0.0f, 0.30f, 0.08f);
    space (s, SpacePresets::Chamber, 0.13f, 0.16f, 0.62f, 0.10f);

    env (s, 1, 0.0005f, 0.07f, 0.0f, 0.06f, 0.20f);
    macros (s, 0.10f, 0.45f, 0.18f, 0.40f);

    Routings r;
    r.uni (ModSource::Env1,      Param::shapeSurface,     0.180f)
     .uni (ModSource::Velocity,  Param::impactHardness,   0.420f)
     .uni (ModSource::Velocity,  Param::impactBrightness, 0.260f)
     .bi  (ModSource::KeyTrack,  Param::shapeDecay,      -0.200f)
     .bi  (ModSource::NoteRandom, Param::impactRandom,    0.070f)
     .uni (ModSource::Macro1,    Param::evolveMotion,     0.280f)
     .uni (ModSource::Macro2,    Param::impactBrightness, 0.340f)
     .uni (ModSource::Macro2,    Param::shapeExcite,      0.220f)
     .uni (ModSource::Macro3,    Param::spaceMix,         0.300f)
     .uni (ModSource::Macro3,    Param::spaceSize,        0.200f)
     .uni (ModSource::Macro4,    Param::shapeMass,        0.300f)
     .uni (ModSource::Macro4,    Param::shapeBlend,       0.280f);
    sharedMacros (r, Param::ampDecay, Param::impactRandom);
    r.commit (s);
}});

manager.addFactory ({ "Ivory Ladder", "PERCUSSION", { "struck", "bright", "wooden", "melodic", "close" }, [] (PatchState& s)
{
    // Hard bars over a tight lattice, tuned to play chromatically end to end.
    // KeyTrack shortens the ring and lifts the mallet as you climb, so the top
    // two octaves stay crisp instead of turning into a ringing smear.
    impact (s, 0 /* IMPULSE */, 0.62f, 0.58f, 0.22f, 0.88f, 0.35f, 0.06f, 0.0f, 0.85f);
    amp (s, 0.001f, 0.62f, 0.0f, 0.50f, 0.28f);
    set (s, Param::masterGain, -2.0f);
    shape (s, 0.26f, 0.16f, 0.36f, 0.64f, 0.50f, 0.12f);
    material (s, MaterialType::Wood, MaterialType::Crystal, 0.56f);
    topology (s, 3 /* LATTICE */, 0.28f, 0.36f, 419);
    matter (s, 1.0f, 0.58f, 0.62f, 0.42f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.46f, 0.06f, 0.0f, 0.30f, 0.10f);
    space (s, SpacePresets::Chamber, 0.24f, 0.30f, 0.58f, 0.20f);

    env (s, 1, 0.001f, 0.22f, 0.0f, 0.18f, 0.25f);
    macros (s, 0.12f, 0.45f, 0.28f, 0.42f);

    Routings r;
    r.uni (ModSource::Env1,      Param::shapeExcite,      0.180f)
     .uni (ModSource::Velocity,  Param::impactHardness,   0.400f)
     .uni (ModSource::Velocity,  Param::impactBrightness, 0.280f)
     .uni (ModSource::Velocity,  Param::shapeBlend,       0.300f)
     .uni (ModSource::Velocity,  Param::shapeSurface,     0.200f)
     .bi  (ModSource::KeyTrack,  Param::shapeDecay,      -0.300f)
     .bi  (ModSource::KeyTrack,  Param::impactHardness,   0.200f)
     .bi  (ModSource::NoteRandom, Param::shapeSurface,    0.060f)
     .uni (ModSource::Macro1,    Param::evolveMotion,     0.260f)
     .uni (ModSource::Macro2,    Param::impactBrightness, 0.340f)
     .uni (ModSource::Macro3,    Param::spaceMix,         0.300f)
     .uni (ModSource::Macro4,    Param::shapeBlend,       0.320f)
     .uni (ModSource::Macro4,    Param::shapeTension,     0.200f);
    sharedMacros (r, Param::ampDecay, Param::impactRandom);
    r.commit (s);
}});

manager.addFactory ({ "Hollow Log", "PERCUSSION", { "struck", "wooden", "hollow", "low", "roomy" }, [] (PatchState& s)
{
    // A slit drum: one heavy tongue over an empty tube. RING topology closes the
    // loop, MASS is most of the sound and the strike is soft and wide.
    impact (s, 6 /* MEMBRANE HIT */, 0.28f, 0.20f, 0.30f, 0.92f, 0.60f, 0.10f, 0.0f, 0.88f);
    amp (s, 0.002f, 0.70f, 0.0f, 0.55f, 0.35f);
    set (s, Param::masterGain, -2.5f);
    shape (s, 0.22f, 0.36f, 0.74f, 0.34f, 0.36f, 0.28f);
    material (s, MaterialType::Wood, MaterialType::Void, 0.34f);
    topology (s, 1 /* RING */, 0.54f, 0.30f, 443);
    matter (s, 1.0f, 0.46f, 0.58f, 0.44f);
    evolve (s, 0.0f, 0.10f, 0.0f, 0.24f, 0.56f, 0.08f, 0.0f, 0.24f, 0.16f);
    set (s, Param::evolveMagnetTarget, 0 /* OCTAVE */);
    space (s, SpacePresets::Void, 0.28f, 0.52f, 0.36f, 0.26f);

    env (s, 1, 0.002f, 0.40f, 0.0f, 0.30f, 0.30f);
    macros (s, 0.18f, 0.32f, 0.38f, 0.48f);

    Routings r;
    r.uni (ModSource::Env1,      Param::shapeCoupling,    0.200f)
     .uni (ModSource::Velocity,  Param::impactHardness,   0.360f)
     .uni (ModSource::Velocity,  Param::shapeStrike,      0.240f)
     .bi  (ModSource::KeyTrack,  Param::shapeMass,       -0.260f)
     .bi  (ModSource::NoteRandom, Param::shapeCoupling,   0.080f)
     .uni (ModSource::Macro1,    Param::evolveMotion,     0.300f)
     .uni (ModSource::Macro2,    Param::impactBrightness, 0.340f)
     .uni (ModSource::Macro2,    Param::shapeExcite,      0.220f)
     .uni (ModSource::Macro3,    Param::spaceMix,         0.320f)
     .uni (ModSource::Macro3,    Param::spaceSize,        0.200f)
     .uni (ModSource::Macro4,    Param::shapeMass,        0.300f)
     .uni (ModSource::Macro4,    Param::evolveMagnet,     0.260f);
    sharedMacros (r, Param::ampDecay, Param::impactRandom);
    r.commit (s);
}});

manager.addFactory ({ "Rattle Cage", "PERCUSSION", { "granular", "wooden", "dirty", "rhythmic", "mid" }, [] (PatchState& s)
{
    // A box of small wooden things being shaken: the WoodKnock built-in read
    // granularly, then thrown through a random lattice and a 1/16 Fracture grid.
    sample (s, BuiltInSamples::Kind::WoodKnock, 3 /* GRANULAR */, 0.0f, 0.85f, 0.34f, 0.70f, 60, 0.90f);
    amp (s, 0.004f, 0.70f, 0.22f, 0.45f, 0.35f);
    set (s, Param::masterGain, 2.5f);
    shape (s, 0.60f, 0.50f, 0.32f, 0.50f, 0.24f, 0.52f);
    material (s, MaterialType::Wood, MaterialType::Organic, 0.45f);
    topology (s, 4 /* RANDOM */, 0.40f, 0.62f, 461);
    matter (s, 0.88f, 0.60f, 0.54f, 0.72f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.48f, 0.34f, 0.14f, 0.62f, 0.40f);
    set (s, Param::evolveScatterSeed, 467);
    fracture (s, 1 /* RHYTHMIC */, 0.42f, 0.40f, 0.60f, 0.62f, 0.22f, 0.26f, 0.38f, 0.55f, 0.25f,
              1 /* 16 */, 4 /* 1/16 */, 8, 0.18f, 2 /* PINGPONG */, 0.75f, 0.40f, 479,
              FractureShape { 16, 0.03f, 0.30f, 0.10f, 0.35f, 0.30f, 0.55f, 0.30f, 0.85f,
                              1.0f, 1.0f, 0.60f, 0.85f, kFallingTerrace, "XoLXoHXo", nullptr });
    space (s, SpacePresets::Dust, 0.30f, 0.36f, 0.55f, 0.28f);

    env (s, 1, 0.003f, 0.30f, 0.0f, 0.24f, 0.30f);
    chaos (s, 1, 0 /* WALK */, 3.20f, 0.45f, 0.55f, 0.5f, 487);
    macros (s, 0.40f, 0.45f, 0.32f, 0.50f);

    Routings r;
    r.uni (ModSource::Env1,      Param::sampleGrain,      0.220f)
     .bi  (ModSource::Chaos1,    Param::sampleStart,      0.160f)
     .uni (ModSource::Velocity,  Param::sampleGrain,      0.260f)
     .uni (ModSource::Velocity,  Param::shapeSurface,     0.220f)
     .bi  (ModSource::NoteRandom, Param::sampleSpread,    0.140f)
     .bi  (ModSource::KeyTrack,  Param::shapeDecay,      -0.180f)
     .uni (ModSource::Macro1,    Param::evolveMotion,     0.380f)
     .uni (ModSource::Macro2,    Param::fractureTone,     0.300f)
     .uni (ModSource::Macro2,    Param::shapeExcite,      0.240f)
     .uni (ModSource::Macro3,    Param::spaceMix,         0.300f)
     .uni (ModSource::Macro4,    Param::sampleGrain,      0.320f)
     .uni (ModSource::Macro4,    Param::fractureAmount,   0.260f);
    sharedMacros (r, Param::fractureDecay, Param::chaos1Depth);
    r.commit (s);
}});
}

} // namespace am::FactoryContent
