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

manager.addFactory ({ "Bone Marimba", "PERCUSSION", { "struck", "wooden", "warm", "resonant", "roomy" }, [] (PatchState& s)
{
    // A bar, not a head. This was a MEMBRANE HIT into a wood/membrane chain in a
    // small room, which put it within a whisker of two other marimbas and of a
    // hand drum on the uniqueness gate — the exciter a drum uses, on a patch
    // whose name says bar. It is now a soft mallet landing on bone-hard wood
    // over an open resonator: an IMPULSE, a cluster of bars rather than a
    // chain, a ring twice as long, and a hall instead of a room.
    impact (s, 0 /* IMPULSE */, 0.34f, 0.46f, 0.30f, 0.85f, 0.45f, 0.14f, 0.0f, 0.85f);
    amp (s, 0.001f, 0.95f, 0.0f, 0.75f, 0.30f);
    set (s, Param::masterGain, -3.0f);   // struck material has a hot transient: keep headroom at the bottom of the keyboard
    shape (s, 0.28f, 0.20f, 0.42f, 0.48f, 0.62f, 0.22f);
    material (s, MaterialType::Wood, MaterialType::Void, 0.34f);
    topology (s, 2 /* CLUSTERS */, 0.30f, 0.40f, 311);
    matter (s, 1.0f, 0.52f, 0.55f, 0.45f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.34f, 0.52f, 0.10f, 0.0f, 0.25f, 0.12f);
    set (s, Param::evolveMagnetTarget, 6 /* CUSTOM (harmonic) */);
    space (s, SpacePresets::Void, 0.30f, 0.55f, 0.42f, 0.26f);

    env (s, 1, 0.001f, 0.34f, 0.0f, 0.28f, 0.30f);
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
     .uni (ModSource::Velocity,  Param::shapeMass,       -0.220f)
     .bi  (ModSource::KeyTrack,  Param::shapeDecay,      -0.240f)
     .bi  (ModSource::NoteRandom, Param::shapeSurface,    0.060f)
     .uni (ModSource::Macro1,    Param::evolveMotion,     0.350f)
     .uni (ModSource::Macro2,    Param::impactBrightness, 0.350f)
     .uni (ModSource::Macro2,    Param::shapeExcite,      0.200f)
     .uni (ModSource::Macro3,    Param::spaceMix,         0.300f)
     .uni (ModSource::Macro4,    Param::evolveBend,       0.300f)
     .uni (ModSource::Macro4,    Param::shapeTension,     0.220f)
     .uni (ModSource::Macro1,    Param::evolveMelt,            0.300f)
     .uni (ModSource::Macro3,    Param::spaceTone,             0.300f)
     .uni (ModSource::Macro4,    Param::shapeDecay,            0.300f);
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
    set (s, Param::masterGain, -3.0f);
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
     .uni (ModSource::Velocity,  Param::shapeMass,       -0.220f)
     .uni (ModSource::Velocity,  Param::shapeTension,     0.240f)
     .bi  (ModSource::KeyTrack,  Param::dustColor,        0.220f)
     .bi  (ModSource::NoteRandom, Param::dustGrain,       0.090f)
     .uni (ModSource::Macro1,    Param::evolveMotion,     0.300f)
     .uni (ModSource::Macro2,    Param::impactBrightness, 0.300f)
     .uni (ModSource::Macro2,    Param::dustColor,        0.250f)
     .uni (ModSource::Macro3,    Param::spaceMix,         0.320f)
     .uni (ModSource::Macro4,    Param::dustLevel,        0.300f)
     .uni (ModSource::Macro4,    Param::shapeSurface,     0.250f)
     .uni (ModSource::Macro1,    Param::impactRate,            0.300f)
     .uni (ModSource::Macro6,    Param::evolveTear,            0.300f);
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
     .uni (ModSource::Velocity,  Param::impactBrightness, 0.280f)
     .uni (ModSource::Velocity,  Param::shapeTension,     0.260f)
     .bi  (ModSource::KeyTrack,  Param::shapeMass,       -0.280f)
     .bi  (ModSource::NoteRandom, Param::shapePitch,      0.120f)
     .uni (ModSource::Macro1,    Param::evolveMotion,     0.380f)
     .uni (ModSource::Macro2,    Param::impactBrightness, 0.320f)
     .uni (ModSource::Macro2,    Param::spaceTone,        0.240f)
     .uni (ModSource::Macro3,    Param::spaceMix,         0.300f)
     .uni (ModSource::Macro3,    Param::spaceSize,        0.180f)
     .uni (ModSource::Macro4,    Param::evolveMelt,       0.320f)
     .uni (ModSource::Macro4,    Param::shapeMass,        0.220f)
     .uni (ModSource::Macro1,    Param::evolveMelt,            0.300f)
     .uni (ModSource::Macro4,    Param::impactHardness,        0.300f)
     .uni (ModSource::Macro1,    Param::evolveTear,            0.300f)
     .uni (ModSource::Macro6,    Param::evolveTear,            0.300f)
     .uni (ModSource::Macro1,    Param::evolveCrush,           0.300f)
     .uni (ModSource::Macro6,    Param::shapeDistribution,     0.300f)
     .uni (ModSource::Macro6,    Param::shapeSurface,          0.300f);
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
     .uni (ModSource::Macro4,    Param::evolveTear,       0.220f)
     .uni (ModSource::Macro4,    Param::shapeDecay,            0.300f);
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
     .uni (ModSource::Macro4,    Param::shapeBlend,       0.280f)
     .uni (ModSource::Macro1,    Param::evolveTear,            0.300f)
     .uni (ModSource::Macro6,    Param::shapeDistribution,     0.300f);
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
     .uni (ModSource::Macro4,    Param::shapeTension,     0.200f)
     .uni (ModSource::Macro2,    Param::shapeSurface,          0.300f)
     .uni (ModSource::Macro2,    Param::shapeExcite,           0.300f)
     .uni (ModSource::Macro1,    Param::impactRate,            0.300f)
     .uni (ModSource::Macro1,    Param::evolveCrush,           0.300f);
    sharedMacros (r, Param::ampDecay, Param::impactRandom);
    r.commit (s);
}});

manager.addFactory ({ "Hollow Log", "PERCUSSION", { "struck", "wooden", "hollow", "low", "roomy" }, [] (PatchState& s)
{
    // A slit drum: one heavy tongue over an empty tube. RING topology closes the
    // loop, MASS is most of the sound and the strike is soft and wide.
    impact (s, 6 /* MEMBRANE HIT */, 0.28f, 0.20f, 0.30f, 0.92f, 0.60f, 0.10f, 0.0f, 0.88f);
    amp (s, 0.002f, 0.70f, 0.0f, 0.55f, 0.35f);
    set (s, Param::masterGain, -3.5f);
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
    r.uni (ModSource::Env1,      Param::shapeSurface,     0.200f)
     .uni (ModSource::Velocity,  Param::impactHardness,   0.360f)
     .uni (ModSource::Velocity,  Param::impactBrightness, 0.300f)
     .bi  (ModSource::KeyTrack,  Param::shapeMass,       -0.260f)
     .bi  (ModSource::NoteRandom, Param::shapeDistribution, 0.080f)
     .uni (ModSource::Macro1,    Param::evolveMotion,     0.300f)
     .uni (ModSource::Macro2,    Param::impactBrightness, 0.340f)
     .uni (ModSource::Macro2,    Param::shapeExcite,      0.220f)
     .uni (ModSource::Macro3,    Param::spaceMix,         0.320f)
     .uni (ModSource::Macro3,    Param::spaceSize,        0.200f)
     .uni (ModSource::Macro4,    Param::shapeMass,        0.300f)
     .uni (ModSource::Macro4,    Param::evolveMagnet,     0.260f)
     .uni (ModSource::Macro1,    Param::evolveMelt,            0.300f)
     .uni (ModSource::Macro2,    Param::spaceTone,             0.300f)
     .uni (ModSource::Macro4,    Param::shapeDecay,            0.300f);
    sharedMacros (r, Param::ampDecay, Param::impactRandom);
    r.commit (s);
}});

manager.addFactory ({ "Rattle Cage", "PERCUSSION", { "granular", "wooden", "dirty", "rhythmic", "mid" }, [] (PatchState& s)
{
    // A box of small wooden things being shaken: the WoodKnock built-in read
    // granularly, then thrown through a random lattice and a 1/16 Fracture grid.
    sample (s, BuiltInSamples::Kind::WoodKnock, 3 /* GRANULAR */, 0.0f, 0.85f, 0.34f, 0.70f, 60, 0.90f);
    amp (s, 0.004f, 0.70f, 0.22f, 0.45f, 0.35f);
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
     .uni (ModSource::Macro4,    Param::fractureAmount,   0.260f)
     .uni (ModSource::Macro1,    Param::evolveScatter,         0.300f)
     .uni (ModSource::Macro4,    Param::shapeDecay,            0.300f);
    sharedMacros (r, Param::fractureDecay, Param::chaos1Depth);
    r.commit (s);
}});

//--------------------------------------------------------- metal, tine to gong

manager.addFactory ({ "Chrome Tine", "PERCUSSION", { "plucked", "bright", "metallic", "high", "dry" }, [] (PatchState& s)
{
    // One tooth of a music box. A STAR topology hangs every partial off a single
    // hub, so the tine is all attack and one clean pitch, and then it is over.
    impact (s, 2 /* PLUCK */, 0.68f, 0.72f, 0.10f, 0.88f, 0.30f, 0.06f, 0.0f, 0.85f);
    amp (s, 0.0008f, 0.34f, 0.0f, 0.26f, 0.22f);
    set (s, Param::masterGain, -2.0f);
    shape (s, 0.20f, 0.44f, 0.20f, 0.72f, 0.42f, 0.16f);
    material (s, MaterialType::Crystal, MaterialType::Metal, 0.42f);
    topology (s, 5 /* STAR */, 0.24f, 0.28f, 509);
    matter (s, 1.0f, 0.66f, 0.66f, 0.34f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.18f, 0.38f, 0.05f, 0.0f, 0.35f, 0.10f);
    set (s, Param::evolveMagnetTarget, 6 /* CUSTOM (harmonic) */);
    space (s, SpacePresets::Chamber, 0.20f, 0.22f, 0.66f, 0.14f);

    env (s, 1, 0.0008f, 0.12f, 0.0f, 0.10f, 0.20f);
    macros (s, 0.12f, 0.48f, 0.24f, 0.42f);

    Routings r;
    r.uni (ModSource::Env1,      Param::shapeExcite,      0.200f)
     .uni (ModSource::Velocity,  Param::impactHardness,   0.400f)
     .uni (ModSource::Velocity,  Param::impactBrightness, 0.300f)
     .bi  (ModSource::KeyTrack,  Param::shapeDecay,      -0.280f)
     .bi  (ModSource::NoteRandom, Param::shapePitch,      0.060f)
     .uni (ModSource::Macro1,    Param::evolveMotion,     0.280f)
     .uni (ModSource::Macro2,    Param::impactBrightness, 0.360f)
     .uni (ModSource::Macro2,    Param::shapeTension,     0.240f)
     .uni (ModSource::Macro3,    Param::spaceMix,         0.320f)
     .uni (ModSource::Macro4,    Param::shapeBlend,       0.340f)
     .uni (ModSource::Macro4,    Param::evolveMagnet,     0.260f)
     .uni (ModSource::Macro1,    Param::evolveMelt,            0.300f);
    sharedMacros (r, Param::ampDecay, Param::impactRandom);
    r.commit (s);
}});

manager.addFactory ({ "Brass Thumb", "PERCUSSION", { "plucked", "metallic", "dirty", "resonant", "close" }, [] (PatchState& s)
{
    // A thumb piano bolted to a plate that buzzes back. SURFACE carries the
    // rattle and a short SPECTRAL Fracture smears each note into the plate.
    impact (s, 2 /* PLUCK */, 0.46f, 0.54f, 0.14f, 0.90f, 0.40f, 0.16f, 0.0f, 0.85f);
    amp (s, 0.001f, 0.55f, 0.04f, 0.40f, 0.28f);
    set (s, Param::masterGain, -1.0f);
    shape (s, 0.42f, 0.48f, 0.34f, 0.60f, 0.44f, 0.62f);
    material (s, MaterialType::Metal, MaterialType::Wood, 0.38f);
    topology (s, 1 /* RING */, 0.62f, 0.44f, 521);
    matter (s, 1.0f, 0.62f, 0.60f, 0.56f);
    evolve (s, 0.0f, 0.0f, 0.12f, 0.0f, 0.46f, 0.18f, 0.0f, 0.48f, 0.24f);
    set (s, Param::evolveScatterSeed, 523);
    fracture (s, 0 /* SPECTRAL */, 0.36f, 0.32f, 0.50f, 0.35f, 0.24f, 0.18f, 0.40f, 0.58f, 0.18f,
              1 /* 16 */, 6 /* 1/4T */, 8, 0.0f, 0 /* FORWARD */, 0.90f, 0.25f, 541,
              FractureShape { 16, 0.02f, 0.20f, 0.12f, 0.40f, 0.35f, 0.60f, 0.25f, 0.85f,
                              1.0f, 1.0f, 0.50f, 0.9f, kFifthTerrace, "XLXHXXLH", nullptr });
    space (s, SpacePresets::Chamber, 0.24f, 0.28f, 0.58f, 0.22f);

    env (s, 1, 0.001f, 0.20f, 0.0f, 0.18f, 0.25f);
    macros (s, 0.28f, 0.44f, 0.26f, 0.52f);

    Routings r;
    r.uni (ModSource::Env1,      Param::shapeSurface,     0.240f)
     .uni (ModSource::Velocity,  Param::impactHardness,   0.380f)
     .uni (ModSource::Velocity,  Param::shapeSurface,     0.260f)
     .bi  (ModSource::KeyTrack,  Param::shapeDecay,      -0.220f)
     .bi  (ModSource::NoteRandom, Param::fracturePitch,   0.080f)
     .uni (ModSource::Macro1,    Param::evolveMotion,     0.320f)
     .uni (ModSource::Macro2,    Param::impactBrightness, 0.320f)
     .uni (ModSource::Macro2,    Param::fractureTone,     0.260f)
     .uni (ModSource::Macro3,    Param::spaceMix,         0.300f)
     .uni (ModSource::Macro4,    Param::shapeSurface,     0.340f)
     .uni (ModSource::Macro4,    Param::fractureAmount,   0.260f)
     .uni (ModSource::Macro1,    Param::impactRate,            0.300f)
     .uni (ModSource::Macro2,    Param::spaceTone,             0.300f)
     .uni (ModSource::Macro4,    Param::shapeDecay,            0.300f);
    sharedMacros (r, Param::fractureDecay, Param::impactRandom);
    r.commit (s);
}});

manager.addFactory ({ "Anvil Tooth", "PERCUSSION", { "struck", "harsh", "metallic", "dirty", "dry" }, [] (PatchState& s)
{
    // Hammer on a small anvil, recorded a foot away. CRUSH quantises the modes
    // to a coarse grid so the object sounds cast rather than tuned.
    impact (s, 4 /* METAL STRIKE */, 0.80f, 0.66f, 0.16f, 0.88f, 0.28f, 0.12f, 0.0f, 0.75f);
    amp (s, 0.0008f, 0.40f, 0.0f, 0.30f, 0.22f);
    set (s, Param::masterGain, -2.0f);
    shape (s, 0.34f, 0.60f, 0.30f, 0.70f, 0.34f, 0.50f);
    material (s, MaterialType::Metal, MaterialType::Wood, 0.28f);
    topology (s, 0 /* CHAIN */, 0.38f, 0.34f, 557);
    matter (s, 1.0f, 0.68f, 0.70f, 0.30f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.42f, 0.10f, 0.44f, 0.30f, 0.14f);
    space (s, SpacePresets::Machine, 0.22f, 0.20f, 0.62f, 0.18f);

    env (s, 1, 0.0008f, 0.10f, 0.0f, 0.10f, 0.20f);
    macros (s, 0.14f, 0.46f, 0.20f, 0.55f);

    Routings r;
    r.uni (ModSource::Env1,      Param::evolveCrush,      0.220f)
     .uni (ModSource::Velocity,  Param::impactHardness,   0.420f)
     .uni (ModSource::Velocity,  Param::evolveCrush,     -0.240f)
     .uni (ModSource::Velocity,  Param::shapeTension,     0.220f)
     .bi  (ModSource::KeyTrack,  Param::shapeDecay,      -0.240f)
     .bi  (ModSource::NoteRandom, Param::impactRandom,    0.090f)
     .uni (ModSource::Macro1,    Param::evolveMotion,     0.300f)
     .uni (ModSource::Macro2,    Param::impactBrightness, 0.340f)
     .uni (ModSource::Macro3,    Param::spaceMix,         0.300f)
     .uni (ModSource::Macro4,    Param::evolveCrush,      0.340f)
     .uni (ModSource::Macro4,    Param::spaceDistDrive,   0.280f)
     .uni (ModSource::Macro1,    Param::evolveMelt,            0.300f)
     .uni (ModSource::Macro2,    Param::shapeSurface,          0.300f)
     .uni (ModSource::Macro3,    Param::spaceTone,             0.300f)
     .uni (ModSource::Macro6,    Param::evolveTear,            0.300f)
     .uni (ModSource::Macro2,    Param::spaceTone,             0.300f);
    sharedMacros (r, Param::ampDecay, Param::impactRandom);
    r.commit (s);
}});

manager.addFactory ({ "Steel Tongue", "PERCUSSION", { "struck", "warm", "metallic", "resonant", "melodic" }, [] (PatchState& s)
{
    // A tongue drum: round, hollow steel with a long hum under the note. MAGNET
    // on the OCTAVE grid keeps the hum consonant with whatever you play.
    impact (s, 5 /* DAMPED SINE */, 0.34f, 0.40f, 0.42f, 0.90f, 0.55f, 0.08f, 0.0f, 0.85f);
    amp (s, 0.002f, 1.00f, 0.0f, 0.85f, 0.35f);
    set (s, Param::masterGain, -3.0f);
    shape (s, 0.28f, 0.40f, 0.54f, 0.48f, 0.62f, 0.22f);
    material (s, MaterialType::Metal, MaterialType::Membrane, 0.46f);
    topology (s, 1 /* RING */, 0.48f, 0.36f, 569);
    matter (s, 1.0f, 0.52f, 0.56f, 0.52f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.42f, 0.48f, 0.06f, 0.0f, 0.22f, 0.14f);
    set (s, Param::evolveMagnetTarget, 0 /* OCTAVE */);
    space (s, SpacePresets::Dream, 0.30f, 0.46f, 0.52f, 0.30f);

    env (s, 1, 0.002f, 0.60f, 0.0f, 0.45f, 0.35f);
    lfo (s, 1, 0.22f, 0 /* SINE */, 1.0f, true, 0.8f);
    macros (s, 0.20f, 0.36f, 0.34f, 0.48f);

    Routings r;
    r.uni (ModSource::Env1,      Param::shapeExcite,      0.180f)
     .bi  (ModSource::LFO1,      Param::shapeMass,        0.050f)
     .uni (ModSource::Velocity,  Param::impactHardness,   0.340f)
     .uni (ModSource::Velocity,  Param::impactBrightness, 0.300f)
     .uni (ModSource::Velocity,  Param::shapeBlend,      -0.240f)
     .uni (ModSource::Velocity,  Param::shapeMass,       -0.240f)
     .bi  (ModSource::KeyTrack,  Param::shapeDecay,      -0.260f)
     .bi  (ModSource::NoteRandom, Param::shapeDistribution, 0.070f)
     .uni (ModSource::Macro1,    Param::evolveMotion,     0.300f)
     .uni (ModSource::Macro2,    Param::impactBrightness, 0.320f)
     .uni (ModSource::Macro2,    Param::shapeExcite,      0.220f)
     .uni (ModSource::Macro3,    Param::spaceMix,         0.300f)
     .uni (ModSource::Macro4,    Param::evolveMagnet,     0.320f)
     .uni (ModSource::Macro4,    Param::shapeBlend,       0.260f)
     .uni (ModSource::Macro1,    Param::evolveTear,            0.300f)
     .uni (ModSource::Macro4,    Param::shapeDecay,            0.300f);
    sharedMacros (r, Param::ampDecay, Param::impactRandom);
    r.commit (s);
}});

manager.addFactory ({ "Bell Foundry", "PERCUSSION", { "struck", "metallic", "cold", "huge", "resonant" }, [] (PatchState& s)
{
    // A cast bell with the hum note sitting an octave under the strike tone.
    // CLUSTERS keep the partial groups apart; the tail is nearly a minute long.
    impact (s, 4 /* METAL STRIKE */, 0.58f, 0.50f, 0.56f, 0.92f, 0.45f, 0.10f, 0.0f, 0.72f);
    amp (s, 0.002f, 1.60f, 0.0f, 1.60f, 0.45f);
    set (s, Param::masterGain, -2.5f);
    shape (s, 0.44f, 0.56f, 0.58f, 0.56f, 0.80f, 0.24f);
    material (s, MaterialType::Metal, MaterialType::Crystal, 0.44f);
    topology (s, 2 /* CLUSTERS */, 0.44f, 0.40f, 587);
    matter (s, 1.0f, 0.54f, 0.58f, 0.66f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.36f, 0.54f, 0.10f, 0.0f, 0.14f, 0.20f);
    set (s, Param::evolveMagnetTarget, 0 /* OCTAVE */);
    space (s, SpacePresets::Void, 0.38f, 0.78f, 0.44f, 0.36f);

    env (s, 1, 0.002f, 1.20f, 0.0f, 1.00f, 0.55f);
    lfo (s, 1, 0.13f, 1 /* TRIANGLE */, 1.0f, true, 1.2f);
    macros (s, 0.26f, 0.38f, 0.45f, 0.45f);

    Routings r;
    r.uni (ModSource::Env1,      Param::evolveMagnet,     0.220f)
     .bi  (ModSource::LFO1,      Param::shapeTension,     0.045f)
     .uni (ModSource::Velocity,  Param::impactHardness,   0.360f)
     .uni (ModSource::Velocity,  Param::impactBrightness, 0.260f)
     .uni (ModSource::Velocity,  Param::shapeBlend,       0.240f)
     .bi  (ModSource::KeyTrack,  Param::shapeDecay,      -0.300f)
     .bi  (ModSource::NoteRandom, Param::shapeDistribution, 0.100f)
     .uni (ModSource::Macro1,    Param::evolveMotion,     0.340f)
     .uni (ModSource::Macro2,    Param::impactBrightness, 0.320f)
     .uni (ModSource::Macro2,    Param::spaceTone,        0.240f)
     .uni (ModSource::Macro3,    Param::spaceMix,         0.320f)
     .uni (ModSource::Macro3,    Param::spaceSize,        0.200f)
     .uni (ModSource::Macro4,    Param::shapeBlend,       0.300f)
     .uni (ModSource::Macro4,    Param::shapeDensity,     0.240f)
     .uni (ModSource::Macro1,    Param::evolveMelt,            0.300f)
     .uni (ModSource::Macro4,    Param::shapeMass,             0.300f);
    sharedMacros (r, Param::ampDecay, Param::impactRandom);
    r.commit (s);
}});

manager.addFactory ({ "Gong Weather", "PERCUSSION", { "struck", "metallic", "evolving", "huge", "dark" }, [] (PatchState& s)
{
    // A gong does not decay, it blooms: a slow repeat at about one hit a second
    // keeps feeding the plate while MELT widens it, so the sound is still
    // arriving four seconds after the stroke.
    impact (s, 4 /* METAL STRIKE */, 0.40f, 0.30f, 0.70f, 0.95f, 0.60f, 0.30f, 0.16f, 0.62f);
    amp (s, 0.004f, 2.20f, 0.30f, 2.00f, 0.55f);
    set (s, Param::masterGain, -3.0f);
    shape (s, 0.62f, 0.66f, 0.66f, 0.46f, 0.76f, 0.44f);
    material (s, MaterialType::Metal, MaterialType::Void, 0.46f);
    topology (s, 4 /* RANDOM */, 0.58f, 0.54f, 599);
    matter (s, 1.0f, 0.50f, 0.52f, 0.82f);
    evolve (s, 0.0f, 0.36f, 0.0f, 0.0f, 0.44f, 0.30f, 0.0f, 0.16f, 0.46f);
    set (s, Param::evolveScatterSeed, 601);
    fracture (s, 0 /* SPECTRAL */, 0.34f, 0.30f, 0.70f, 0.30f, 0.30f, 0.45f, 0.60f, 0.42f, 0.30f,
              2 /* 32 */, 2 /* 1/4 */, 8, 0.0f, 2 /* PINGPONG */, 0.80f, 0.35f, 607,
              FractureShape { 32, 0.10f, 0.55f, 0.20f, 0.50f, 0.45f, 0.72f, 0.30f, 0.95f,
                              1.0f, 0.9f, 0.70f, 0.85f, kOctaveTerrace, "XLXHXLXH", nullptr });
    space (s, SpacePresets::Void, 0.44f, 0.88f, 0.36f, 0.44f);

    env (s, 2, 0.60f, 3.00f, 0.40f, 2.50f, 0.70f);
    lfo (s, 1, 0.09f, 5 /* SMOOTH RANDOM */, 1.0f, true, 1.5f);
    macros (s, 0.45f, 0.30f, 0.50f, 0.50f);

    Routings r;
    r.uni (ModSource::Env2,      Param::evolveMelt,       0.280f)
     .uni (ModSource::Env2,      Param::shapeDensity,     0.220f)
     .bi  (ModSource::LFO1,      Param::shapeDensity,     0.090f)
     .uni (ModSource::Velocity,  Param::impactHardness,   0.320f)
     .uni (ModSource::Velocity,  Param::impactRate,       0.180f)
     .uni (ModSource::Velocity,  Param::shapeBlend,      -0.240f)
     .bi  (ModSource::KeyTrack,  Param::shapeMass,       -0.240f)
     .bi  (ModSource::NoteRandom, Param::impactRandom,    0.120f)
     .uni (ModSource::Macro1,    Param::evolveMotion,     0.420f)
     .uni (ModSource::Macro2,    Param::impactBrightness, 0.320f)
     .uni (ModSource::Macro2,    Param::spaceTone,        0.260f)
     .uni (ModSource::Macro3,    Param::spaceMix,         0.300f)
     .uni (ModSource::Macro4,    Param::evolveMelt,       0.320f)
     .uni (ModSource::Macro4,    Param::fractureAmount,   0.240f)
     .uni (ModSource::Macro1,    Param::evolveTear,            0.300f)
     .uni (ModSource::Macro3,    Param::spaceTone,             0.300f);
    sharedMacros (r, Param::fractureDecay, Param::evolveSpeed);
    r.commit (s);
}});

manager.addFactory ({ "Hissing Dome", "PERCUSSION", { "struck", "metallic", "air", "wide", "noisy" }, [] (PatchState& s)
{
    // A cymbal roll you can hold down. The repeat runs at about ten strokes a
    // second and velocity drives it, so playing softer thins the wash out into
    // separate strokes instead of just turning it down.
    impact (s, 3 /* NOISE STRIKE */, 0.44f, 0.86f, 0.24f, 0.90f, 0.50f, 0.42f, 0.62f, 0.55f);
    amp (s, 0.020f, 0.80f, 0.55f, 0.90f, 0.45f);
    shape (s, 0.72f, 0.62f, 0.24f, 0.66f, 0.60f, 0.40f);
    material (s, MaterialType::Metal, MaterialType::Void, 0.34f);
    topology (s, 4 /* RANDOM */, 0.52f, 0.66f, 613);
    matter (s, 0.90f, 0.66f, 0.44f, 0.92f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.34f, 0.34f, 0.0f, 0.55f, 0.36f);
    set (s, Param::evolveScatterSeed, 617);
    fracture (s, 0 /* SPECTRAL */, 0.40f, 0.36f, 0.75f, 0.30f, 0.34f, 0.40f, 0.55f, 0.70f, 0.22f,
              2 /* 32 */, 4 /* 1/16 */, 8, 0.0f, 3 /* RANDOM */, 0.85f, 0.40f, 619,
              FractureShape { 32, 0.04f, 0.35f, 0.20f, 0.50f, 0.40f, 0.68f, 0.40f, 1.0f,
                              1.0f, 1.0f, 0.80f, 0.9f, nullptr, "XHXHXXHH", nullptr });
    space (s, SpacePresets::Shimmer, 0.36f, 0.60f, 0.70f, 0.34f);

    env (s, 1, 0.030f, 0.60f, 0.35f, 0.50f, 0.45f);
    chaos (s, 1, 4 /* TARGETS */, 6.50f, 0.45f, 0.50f, 0.5f, 641);
    macros (s, 0.42f, 0.55f, 0.40f, 0.50f);

    Routings r;
    r.uni (ModSource::Env1,      Param::impactRate,       0.200f)
     .bi  (ModSource::Chaos1,    Param::impactBrightness, 0.140f)
     .uni (ModSource::Velocity,  Param::impactRate,       0.300f)
     .uni (ModSource::Velocity,  Param::impactBrightness, 0.240f)
     .bi  (ModSource::KeyTrack,  Param::impactHardness,   0.200f)
     .bi  (ModSource::NoteRandom, Param::evolveScatter,   0.100f)
     .uni (ModSource::Macro1,    Param::impactRate,       0.350f)
     .uni (ModSource::Macro2,    Param::impactBrightness, 0.320f)
     .uni (ModSource::Macro2,    Param::fractureTone,     0.260f)
     .uni (ModSource::Macro3,    Param::spaceMix,         0.320f)
     .uni (ModSource::Macro4,    Param::shapeDensity,     0.300f)
     .uni (ModSource::Macro4,    Param::fractureAmount,   0.260f)
     .uni (ModSource::Macro1,    Param::evolveMelt,            0.300f)
     .uni (ModSource::Macro2,    Param::spaceTone,             0.300f)
     .uni (ModSource::Macro4,    Param::shapeDecay,            0.300f);
    sharedMacros (r, Param::fractureDecay, Param::impactRandom);
    r.commit (s);
}});

manager.addFactory ({ "Wire Hammer", "PERCUSSION", { "struck", "bright", "rhythmic", "resonant", "melodic" }, [] (PatchState& s)
{
    // A hammered dulcimer. Two hard beaters on a course of wires: the repeat
    // sits at tremolo speed, a 1/16 Fracture grid throws the tremolo around the
    // stereo field the way a real pair of hammers never quite lines up, and the
    // synced delays of ORBIT put the board at the far end of a hall.
    impact (s, 2 /* PLUCK */, 0.56f, 0.66f, 0.08f, 0.92f, 0.32f, 0.22f, 0.44f, 0.72f);
    amp (s, 0.001f, 0.70f, 0.32f, 0.55f, 0.30f);
    set (s, Param::masterGain, -2.0f);
    shape (s, 0.34f, 0.10f, 0.32f, 0.68f, 0.56f, 0.20f);
    material (s, MaterialType::String, MaterialType::Wood, 0.34f);
    topology (s, 0 /* CHAIN */, 0.30f, 0.40f, 643);
    matter (s, 1.0f, 0.60f, 0.58f, 0.48f);
    evolve (s, 0.0f, 0.0f, 0.14f, 0.0f, 0.46f, 0.16f, 0.0f, 0.50f, 0.22f);
    set (s, Param::evolveScatterSeed, 647);
    fracture (s, 1 /* RHYTHMIC */, 0.40f, 0.38f, 0.62f, 0.58f, 0.26f, 0.22f, 0.42f, 0.62f, 0.20f,
              1 /* 16 */, 4 /* 1/16 */, 8, 0.22f, 2 /* PINGPONG */, 0.85f, 0.30f, 653,
              FractureShape { 16, 0.02f, 0.24f, 0.10f, 0.38f, 0.35f, 0.62f, 0.35f, 0.90f,
                              1.0f, 1.0f, 0.70f, 0.9f, kFifthTerrace, "XoXHXoXL", nullptr });
    space (s, SpacePresets::Orbit, 0.26f, 0.34f, 0.62f, 0.22f);

    env (s, 1, 0.001f, 0.24f, 0.0f, 0.20f, 0.25f);
    macros (s, 0.35f, 0.48f, 0.30f, 0.50f);

    Routings r;
    r.uni (ModSource::Env1,      Param::fractureAmount,   0.200f)
     .uni (ModSource::Velocity,  Param::impactRate,       0.280f)
     .uni (ModSource::Velocity,  Param::impactHardness,   0.360f)
     .uni (ModSource::Velocity,  Param::shapeBlend,      -0.240f)
     .bi  (ModSource::KeyTrack,  Param::shapeDecay,      -0.260f)
     .bi  (ModSource::NoteRandom, Param::impactRandom,    0.100f)
     .uni (ModSource::Macro1,    Param::impactRate,       0.380f)
     .uni (ModSource::Macro2,    Param::impactBrightness, 0.320f)
     .uni (ModSource::Macro3,    Param::spaceMix,         0.300f)
     .uni (ModSource::Macro4,    Param::fractureSwing,    0.300f)
     .uni (ModSource::Macro4,    Param::evolveTear,       0.240f)
     .uni (ModSource::Macro2,    Param::spaceTone,             0.300f)
     .uni (ModSource::Macro4,    Param::shapeDecay,            0.300f);
    sharedMacros (r, Param::fractureDecay, Param::fractureRandom);
    r.commit (s);
}});

//---------------------------------------------- glass, ceramic and struck air

manager.addFactory ({ "Singing Rim", "PERCUSSION", { "struck", "glassy", "cold", "high", "resonant" }, [] (PatchState& s)
{
    // A wine glass tapped at the rim. CRYSTAL over VOID rings almost without
    // damping, and MAGNET on the FIFTH grid keeps the upper partials consonant
    // instead of letting the glass go sour as it hangs.
    impact (s, 0 /* IMPULSE */, 0.82f, 0.78f, 0.14f, 0.86f, 0.30f, 0.05f, 0.0f, 0.70f);
    amp (s, 0.001f, 1.80f, 0.0f, 1.80f, 0.50f);
    set (s, Param::masterGain, -2.0f);
    shape (s, 0.16f, 0.72f, 0.20f, 0.78f, 0.86f, 0.08f);
    material (s, MaterialType::Crystal, MaterialType::Void, 0.30f);
    topology (s, 5 /* STAR */, 0.20f, 0.24f, 661);
    matter (s, 1.0f, 0.60f, 0.52f, 0.72f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.48f, 0.40f, 0.06f, 0.0f, 0.12f, 0.16f);
    set (s, Param::evolveMagnetTarget, 1 /* FIFTH */);
    space (s, SpacePresets::Dream, 0.36f, 0.62f, 0.68f, 0.34f);

    env (s, 1, 0.001f, 1.40f, 0.0f, 1.20f, 0.60f);
    lfo (s, 1, 0.17f, 0 /* SINE */, 1.0f, true, 1.5f);
    macros (s, 0.20f, 0.45f, 0.42f, 0.45f);

    Routings r;
    r.uni (ModSource::Env1,      Param::evolveMagnet,     0.200f)
     .bi  (ModSource::LFO1,      Param::shapePitch,       0.030f)
     .uni (ModSource::Velocity,  Param::impactHardness,   0.340f)
     .uni (ModSource::Velocity,  Param::shapeSurface,     0.240f)
     .bi  (ModSource::KeyTrack,  Param::shapeDecay,      -0.240f)
     .bi  (ModSource::NoteRandom, Param::shapeDistribution, 0.090f)
     .uni (ModSource::Macro1,    Param::evolveMotion,     0.300f)
     .uni (ModSource::Macro2,    Param::impactBrightness, 0.320f)
     .uni (ModSource::Macro2,    Param::shapeTension,     0.240f)
     .uni (ModSource::Macro3,    Param::spaceMix,         0.320f)
     .uni (ModSource::Macro3,    Param::spaceSize,        0.200f)
     .uni (ModSource::Macro4,    Param::evolveMagnet,     0.300f)
     .uni (ModSource::Macro4,    Param::shapeBlend,       0.260f)
     .uni (ModSource::Macro1,    Param::evolveMelt,            0.300f)
     .uni (ModSource::Macro2,    Param::shapeExcite,           0.300f)
     .uni (ModSource::Macro6,    Param::shapeDistribution,     0.300f);
    sharedMacros (r, Param::ampDecay, Param::impactRandom);
    r.commit (s);
}});

manager.addFactory ({ "Porcelain Crack", "PERCUSSION", { "struck", "glassy", "cold", "unstable", "close" }, [] (PatchState& s)
{
    // The GlassStrike built-in fired once into a ceramic lattice that is already
    // broken: TEAR turns the quiet nodes into detuned twins of the loud ones, so
    // one plate rings as two halves beating against each other.
    sample (s, BuiltInSamples::Kind::GlassStrike, 0 /* ONE SHOT */, 0.0f, 0.70f, 0.20f, 0.45f, 60, 0.95f);
    amp (s, 0.001f, 0.85f, 0.0f, 0.70f, 0.35f);
    set (s, Param::masterGain, -1.5f);
    shape (s, 0.38f, 0.66f, 0.26f, 0.62f, 0.58f, 0.34f);
    material (s, MaterialType::Crystal, MaterialType::Wood, 0.36f);
    topology (s, 3 /* LATTICE */, 0.46f, 0.44f, 673);
    matter (s, 0.94f, 0.62f, 0.60f, 0.68f);
    evolve (s, 0.0f, 0.0f, 0.52f, 0.0f, 0.46f, 0.20f, 0.0f, 0.36f, 0.34f);
    set (s, Param::evolveScatterSeed, 677);
    space (s, SpacePresets::Chamber, 0.26f, 0.32f, 0.62f, 0.24f);

    env (s, 1, 0.004f, 0.70f, 0.0f, 0.55f, 0.45f);
    chaos (s, 1, 2 /* LOGISTIC */, 1.60f, 0.40f, 0.62f, 0.5f, 683);
    macros (s, 0.35f, 0.45f, 0.28f, 0.55f);

    Routings r;
    r.uni (ModSource::Env1,      Param::evolveTear,       0.260f)
     .bi  (ModSource::Chaos1,    Param::shapeDistribution, 0.120f)
     .uni (ModSource::Velocity,  Param::evolveTear,       0.220f)
     .uni (ModSource::Velocity,  Param::sampleStart,     -0.180f)
     .uni (ModSource::Velocity,  Param::shapeBlend,      -0.260f)
     .bi  (ModSource::KeyTrack,  Param::shapeDecay,      -0.220f)
     .bi  (ModSource::NoteRandom, Param::shapeDistribution, 0.120f)
     .uni (ModSource::Macro1,    Param::evolveMotion,     0.340f)
     .uni (ModSource::Macro2,    Param::shapeExcite,      0.300f)
     .uni (ModSource::Macro2,    Param::spaceTone,        0.240f)
     .uni (ModSource::Macro3,    Param::spaceMix,         0.300f)
     .uni (ModSource::Macro4,    Param::evolveTear,       0.320f)
     .uni (ModSource::Macro4,    Param::shapeSurface,     0.260f)
     .uni (ModSource::Macro1,    Param::evolveMelt,            0.300f);
    sharedMacros (r, Param::ampDecay, Param::chaos1Depth);
    r.commit (s);
}});

manager.addFactory ({ "Bottle Garden", "PERCUSSION", { "struck", "blown", "hollow", "glassy", "wide" }, [] (PatchState& s)
{
    // A row of bottles struck and blown at the same moment: a hard CLICK on the
    // glass and a BREATH across the neck, layered, so the note has a rim and a
    // column of air in it at once.
    impact (s, 1 /* CLICK */, 0.70f, 0.66f, 0.08f, 0.86f, 0.28f, 0.10f, 0.0f, 0.62f);
    gesture (s, 3 /* BREATH */, 0.42f, 0.34f, 0.30f, 0.36f, 0.34f, 0.44f, 0.42f);
    layerSources (s, 2 /* IMPACT */, 4 /* GESTURE */);
    amp (s, 0.004f, 0.70f, 0.30f, 0.55f, 0.40f);
    set (s, Param::masterGain, -1.0f);
    shape (s, 0.24f, 0.54f, 0.44f, 0.52f, 0.52f, 0.20f);
    material (s, MaterialType::Crystal, MaterialType::Void, 0.44f);
    topology (s, 2 /* CLUSTERS */, 0.38f, 0.38f, 691);
    matter (s, 0.92f, 0.56f, 0.54f, 0.80f);
    evolve (s, 0.0f, 0.12f, 0.0f, 0.26f, 0.50f, 0.12f, 0.0f, 0.26f, 0.22f);
    set (s, Param::evolveMagnetTarget, 0 /* OCTAVE */);
    space (s, SpacePresets::Nebula, 0.34f, 0.52f, 0.60f, 0.32f);

    env (s, 1, 0.006f, 0.40f, 0.0f, 0.35f, 0.35f);
    lfo (s, 1, 0.45f, 5 /* SMOOTH RANDOM */, 1.0f, true, 0.5f);
    macros (s, 0.30f, 0.42f, 0.40f, 0.50f);

    Routings r;
    r.uni (ModSource::Env1,      Param::gesturePressure,  0.240f)
     .bi  (ModSource::LFO1,      Param::gestureSpeed,     0.140f)
     .uni (ModSource::Velocity,  Param::impactHardness,   0.360f)
     .uni (ModSource::Velocity,  Param::gesturePressure,  0.260f)
     .bi  (ModSource::KeyTrack,  Param::gestureBandwidth, 0.200f)
     .bi  (ModSource::NoteRandom, Param::gesturePosition, 0.120f)
     .uni (ModSource::Macro1,    Param::gestureMotion,    0.360f)
     .uni (ModSource::Macro2,    Param::impactBrightness, 0.300f)
     .uni (ModSource::Macro2,    Param::gestureBandwidth, 0.260f)
     .uni (ModSource::Macro3,    Param::spaceMix,         0.320f)
     .uni (ModSource::Macro4,    Param::gestureLevel,     0.320f)
     .uni (ModSource::Macro4,    Param::evolveMagnet,     0.240f)
     .uni (ModSource::Macro1,    Param::evolveMelt,            0.300f)
     .uni (ModSource::Macro6,    Param::shapeDistribution,     0.300f);
    sharedMacros (r, Param::ampDecay, Param::gestureRoughness);
    r.commit (s);
}});

manager.addFactory ({ "Cathedral Vibes", "PERCUSSION", { "struck", "metallic", "warm", "roomy", "melodic" }, [] (PatchState& s)
{
    // A vibraphone with the motor running: LFO1 opens and closes the excitation
    // about three times a second, which is the fan under the bars, not a
    // tremolo on the output. MAGNET on the MAJOR grid keeps the bars in tune.
    impact (s, 5 /* DAMPED SINE */, 0.40f, 0.52f, 0.36f, 0.88f, 0.48f, 0.06f, 0.0f, 0.82f);
    amp (s, 0.002f, 1.20f, 0.16f, 0.95f, 0.40f);
    set (s, Param::masterGain, -3.0f);
    shape (s, 0.24f, 0.50f, 0.42f, 0.58f, 0.68f, 0.16f);
    material (s, MaterialType::Metal, MaterialType::Custom, 0.46f);
    topology (s, 3 /* LATTICE */, 0.32f, 0.34f, 701);
    matter (s, 1.0f, 0.54f, 0.56f, 0.50f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.38f, 0.48f, 0.06f, 0.0f, 0.20f, 0.14f);
    set (s, Param::evolveMagnetTarget, 2 /* MAJOR */);
    space (s, SpacePresets::Chamber, 0.34f, 0.58f, 0.52f, 0.30f);

    env (s, 1, 0.002f, 0.50f, 0.0f, 0.40f, 0.35f);
    lfo (s, 1, 3.20f, 0 /* SINE */, 1.0f, true, 0.15f);
    macros (s, 0.35f, 0.40f, 0.42f, 0.48f);

    Routings r;
    r.bi  (ModSource::LFO1,      Param::shapeExcite,      0.180f)
     .bi  (ModSource::LFO1,      Param::shapeStereo,      0.120f)
     .uni (ModSource::Env1,      Param::shapeSurface,     0.160f)
     .uni (ModSource::Velocity,  Param::impactHardness,   0.340f)
     .uni (ModSource::Velocity,  Param::shapeBlend,      -0.280f)
     .bi  (ModSource::KeyTrack,  Param::shapeDecay,      -0.280f)
     .bi  (ModSource::NoteRandom, Param::shapeSurface,    0.060f)
     .uni (ModSource::Macro1,    Param::lfo1Rate,         0.220f)
     .uni (ModSource::Macro1,    Param::lfo1Depth,        0.300f)
     .uni (ModSource::Macro2,    Param::impactBrightness, 0.320f)
     .uni (ModSource::Macro3,    Param::spaceMix,         0.320f)
     .uni (ModSource::Macro4,    Param::evolveMagnet,     0.300f)
     .uni (ModSource::Macro4,    Param::shapeBlend,       0.260f)
     .uni (ModSource::Macro1,    Param::evolveMelt,            0.300f)
     .uni (ModSource::Macro2,    Param::shapeExcite,           0.300f);
    sharedMacros (r, Param::ampDecay, Param::impactRandom);
    r.commit (s);
}});

manager.addFactory ({ "Stone Circle", "PERCUSSION", { "struck", "dark", "hollow", "distant", "low" }, [] (PatchState& s)
{
    // One rock struck against another at the far end of a cave. The StoneDrop
    // built-in carries the body and a bare IMPULSE lands on top of it for the
    // contact, layered so the click and the mass arrive from the same place.
    impact (s, 0 /* IMPULSE */, 0.44f, 0.30f, 0.20f, 0.90f, 0.35f, 0.14f, 0.0f, 0.45f);
    sample (s, BuiltInSamples::Kind::StoneDrop, 0 /* ONE SHOT */, 0.0f, 0.90f, 0.24f, 0.55f, 48, 0.85f);
    layerSources (s, 2 /* IMPACT */, 3 /* SAMPLE */);
    amp (s, 0.002f, 0.90f, 0.0f, 0.80f, 0.40f);
    set (s, Param::masterGain, -2.5f);
    shape (s, 0.36f, 0.40f, 0.70f, 0.36f, 0.44f, 0.38f);
    material (s, MaterialType::Void, MaterialType::Wood, 0.42f);
    topology (s, 2 /* CLUSTERS */, 0.48f, 0.42f, 709);
    matter (s, 0.90f, 0.48f, 0.56f, 0.62f);
    evolve (s, 0.0f, 0.18f, 0.0f, 0.0f, 0.58f, 0.14f, 0.0f, 0.18f, 0.20f);
    space (s, SpacePresets::Void, 0.46f, 0.84f, 0.34f, 0.42f);

    env (s, 1, 0.002f, 0.50f, 0.0f, 0.40f, 0.40f);
    macros (s, 0.22f, 0.32f, 0.55f, 0.45f);

    Routings r;
    r.uni (ModSource::Env1,      Param::evolveMelt,       0.200f)
     .uni (ModSource::Velocity,  Param::impactHardness,   0.360f)
     .uni (ModSource::Velocity,  Param::sampleStart,     -0.150f)
     .uni (ModSource::Velocity,  Param::shapeSurface,     0.200f)
     .uni (ModSource::Velocity,  Param::shapeTension,     0.240f)
     .bi  (ModSource::KeyTrack,  Param::shapeMass,       -0.260f)
     .bi  (ModSource::NoteRandom, Param::sampleSpread,    0.120f)
     .uni (ModSource::Macro1,    Param::evolveMotion,     0.300f)
     .uni (ModSource::Macro2,    Param::impactBrightness, 0.300f)
     .uni (ModSource::Macro2,    Param::spaceTone,        0.280f)
     .uni (ModSource::Macro3,    Param::spaceMix,         0.340f)
     .uni (ModSource::Macro3,    Param::spaceSize,        0.200f)
     .uni (ModSource::Macro4,    Param::shapeMass,        0.300f)
     .uni (ModSource::Macro4,    Param::sampleLevel,      0.260f)
     .uni (ModSource::Macro1,    Param::evolveMelt,            0.300f)
     .uni (ModSource::Macro3,    Param::spaceTone,             0.300f)
     .uni (ModSource::Macro6,    Param::evolveTear,            0.300f);
    sharedMacros (r, Param::ampDecay, Param::impactRandom);
    r.commit (s);
}});

manager.addFactory ({ "Horsehair Drum", "PERCUSSION", { "scraped", "soft", "organic", "close", "noisy" }, [] (PatchState& s)
{
    // Wire brushes on a head: the drag is a SCRAPE gesture held under the note
    // and the tap is a soft NOISE STRIKE on top of it. Velocity crossfades the
    // two, so the softest playing is all drag and the hardest is all tap.
    impact (s, 3 /* NOISE STRIKE */, 0.30f, 0.46f, 0.12f, 0.92f, 0.45f, 0.28f, 0.0f, 0.50f);
    gesture (s, 1 /* SCRAPE */, 0.40f, 0.52f, 0.62f, 0.44f, 0.46f, 0.50f, 0.50f);
    layerSources (s, 2 /* IMPACT */, 4 /* GESTURE */);
    amp (s, 0.010f, 0.60f, 0.40f, 0.45f, 0.40f);
    shape (s, 0.46f, 0.32f, 0.46f, 0.44f, 0.34f, 0.56f);
    material (s, MaterialType::Membrane, MaterialType::Organic, 0.48f);
    topology (s, 3 /* LATTICE */, 0.44f, 0.52f, 719);
    matter (s, 0.88f, 0.56f, 0.48f, 0.66f);
    evolve (s, 0.0f, 0.16f, 0.0f, 0.0f, 0.46f, 0.26f, 0.0f, 0.58f, 0.38f);
    set (s, Param::evolveScatterSeed, 727);
    space (s, SpacePresets::Chamber, 0.24f, 0.30f, 0.48f, 0.20f);

    env (s, 1, 0.008f, 0.30f, 0.0f, 0.26f, 0.30f);
    chaos (s, 1, 0 /* WALK */, 2.40f, 0.42f, 0.55f, 0.5f, 733);
    macros (s, 0.38f, 0.40f, 0.28f, 0.55f);

    Routings r;
    r.uni (ModSource::Env1,      Param::impactLevel,      0.200f)
     .bi  (ModSource::Chaos1,    Param::gesturePosition,  0.160f)
     .uni (ModSource::Velocity,  Param::impactLevel,      0.450f)
     .uni (ModSource::Velocity,  Param::gestureLevel,    -0.420f)
     .uni (ModSource::Velocity,  Param::impactHardness,   0.280f)
     .bi  (ModSource::KeyTrack,  Param::gestureSpeed,     0.220f)
     .bi  (ModSource::NoteRandom, Param::gestureRoughness, 0.120f)
     .uni (ModSource::Macro1,    Param::gestureMotion,    0.380f)
     .uni (ModSource::Macro2,    Param::gestureBandwidth, 0.300f)
     .uni (ModSource::Macro2,    Param::impactBrightness, 0.260f)
     .uni (ModSource::Macro3,    Param::spaceMix,         0.300f)
     .uni (ModSource::Macro4,    Param::gestureRoughness, 0.320f)
     .uni (ModSource::Macro4,    Param::shapeSurface,     0.260f)
     .uni (ModSource::Macro6,    Param::gestureRoughness,      0.300f);
    sharedMacros (r, Param::ampDecay, Param::chaos1Depth);
    r.commit (s);
}});
}

} // namespace am::FactoryContent
