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


void registerDrone (PresetManager& manager)
{
//==========================================================================
// DRONE — one note, played for a minute
//==========================================================================

manager.addFactory ({ "Gravity Drone", "DRONE", { "drone", "heavy", "friction", "slow" }, [] (PatchState& s)
{
    gesture (s, 4 /* FRICTION */, 0.68f, 0.22f, 0.38f, 0.45f, 0.30f, 0.42f);
    amp (s, 1.60f, 3.0f, 0.92f, 4.0f, 0.6f);
    shape (s, 0.58f, 0.26f, 0.74f, 0.40f, 0.78f, 0.34f);
    material (s, MaterialType::Void, MaterialType::Membrane, 0.42f);
    topology (s, 2 /* CLUSTERS */, 0.56f, 0.38f, 439);
    matter (s, 0.96f, 0.48f, 0.18f, 0.70f);
    evolve (s, 0.0f, 0.32f, 0.0f, 0.28f, 0.76f, 0.18f, 0.0f, 0.10f, 0.42f);
    set (s, Param::evolveMagnetTarget, 0 /* OCTAVE */);
    space (s, SpacePresets::Void, 0.52f, 0.85f, 0.38f, 0.42f);

    lfo (s, 1, 0.05f, 0 /* SINE */, 1.0f, false, 3.0f);
    lfo (s, 2, 0.09f, 1 /* TRIANGLE */, 1.0f, false, 2.0f);
    env (s, 2, 6.0f, 8.0f, 0.80f, 8.0f, 0.7f);
    chaos (s, 1, 1 /* BROWNIAN */, 0.07f, 0.50f, 0.85f, 0.5f, 1117);
    macros (s, 0.45f, 0.30f, 0.55f, 0.50f);

    Routings r;
    r.bi  (ModSource::LFO1,   Param::shapeMass,      0.070f)
     .bi  (ModSource::LFO2,   Param::gesturePressure, 0.120f)
     .uni (ModSource::Env2,   Param::evolveMelt,     0.240f)
     .bi  (ModSource::Chaos1, Param::shapeForm,      0.060f)
     .uni (ModSource::Velocity, Param::gesturePressure, 0.200f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.450f)
     .uni (ModSource::Macro1, Param::gestureMotion,  0.350f)
     .uni (ModSource::Macro2, Param::gestureBandwidth, 0.300f)
     .uni (ModSource::Macro2, Param::shapeExcite,    0.250f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.250f)
     .uni (ModSource::Macro4, Param::evolveMelt,     0.300f)
     .uni (ModSource::Macro4, Param::shapeDecay,     0.150f);
    sharedMacros (r, Param::spaceSize, Param::gestureRoughness);
    r.commit (s);
}});

manager.addFactory ({ "Electric Organism", "DRONE", { "drone", "alive", "electric", "unstable" }, [] (PatchState& s)
{
    gesture (s, 2 /* RUB */, 0.64f, 0.42f, 0.44f, 0.38f, 0.55f, 0.42f);
    amp (s, 0.80f, 2.20f, 0.88f, 2.80f, 0.55f);
    shape (s, 0.70f, 0.64f, 0.46f, 0.54f, 0.72f, 0.44f);
    material (s, MaterialType::Organic, MaterialType::Liquid, 0.50f);
    topology (s, 1 /* RING */, 0.66f, 0.50f, 491);
    matter (s, 0.94f, 0.50f, 0.20f, 0.88f);
    evolve (s, 0.0f, 0.26f, 0.28f, 0.0f, 0.50f, 0.38f, 0.0f, 0.30f, 0.55f);
    set (s, Param::evolveScatterSeed, 1229);
    space (s, SpacePresets::Nebula, 0.48f, 0.70f, 0.52f, 0.45f);

    lfo (s, 1, 0.16f, 5 /* SMOOTH RANDOM */, 1.0f, false);
    lfo (s, 2, 0.31f, 0 /* SINE */, 1.0f, false, 1.5f);
    chaos (s, 1, 3 /* LORENZ */, 0.34f, 0.62f, 0.60f, 0.5f, 1301);
    chaos (s, 2, 0 /* WALK */, 0.12f, 0.45f, 0.80f, 0.5f, 1373);
    env (s, 2, 4.0f, 6.0f, 0.70f, 6.0f);
    macros (s, 0.55f, 0.45f, 0.45f, 0.50f);

    Routings r;
    r.bi  (ModSource::LFO1,   Param::gestureSpeed,   0.160f)
     .bi  (ModSource::LFO2,   Param::shapeTension,   0.050f)
     .bi  (ModSource::Chaos1, Param::evolveScatter,  0.150f)
     .bi  (ModSource::Chaos2, Param::shapeForm,      0.070f)
     .uni (ModSource::Env2,   Param::evolveTear,     0.200f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.450f)
     .uni (ModSource::Macro1, Param::gestureMotion,  0.300f)
     .uni (ModSource::Macro2, Param::gestureBandwidth, 0.300f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.280f)
     .uni (ModSource::Macro4, Param::evolveTear,     0.300f)
     .uni (ModSource::Macro4, Param::gestureRoughness, 0.300f);
    sharedMacros (r, Param::spaceSize, Param::chaos1Depth);
    r.commit (s);
}});

manager.addFactory ({ "Deep Field", "DRONE", { "drone", "vast", "dark", "sub" }, [] (PatchState& s)
{
    wave (s, 5 /* SPECTRAL */, 0.18f, 0.15f, 0.03f, 6, 0.30f, 1.0f, -2);
    amp (s, 2.50f, 4.0f, 0.90f, 5.0f, 0.65f);
    shape (s, 0.50f, 0.04f, 0.78f, 0.36f, 0.80f, 0.18f);
    material (s, MaterialType::Void, MaterialType::Organic, 0.28f);
    topology (s, 2 /* CLUSTERS */, 0.40f, 0.42f, 547);
    matter (s, 0.70f, 0.42f, 0.12f, 0.92f);
    evolve (s, 0.0f, 0.16f, 0.0f, 0.42f, 0.68f, 0.10f, 0.0f, 0.08f, 0.30f);
    set (s, Param::evolveMagnetTarget, 0 /* OCTAVE */);
    space (s, SpacePresets::Void, 0.58f, 0.95f, 0.32f, 0.48f);

    lfo (s, 1, 0.04f, 0 /* SINE */, 1.0f, false, 4.0f);
    lfo (s, 2, 0.06f, 5 /* SMOOTH RANDOM */, 1.0f, false, 2.0f);
    env (s, 2, 8.0f, 10.0f, 0.85f, 10.0f, 0.75f);
    macros (s, 0.40f, 0.25f, 0.60f, 0.35f);

    Routings r;
    r.bi  (ModSource::LFO1,   Param::wavePosition,   0.090f)
     .bi  (ModSource::LFO2,   Param::shapeMass,      0.060f)
     .uni (ModSource::Env2,   Param::shapeDensity,   0.200f)
     .bi  (ModSource::KeyTrack, Param::shapeDecay,  -0.100f)
     .uni (ModSource::Velocity, Param::shapeExcite,  0.150f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.400f)
     .uni (ModSource::Macro2, Param::shapeExcite,    0.300f)
     .uni (ModSource::Macro2, Param::spaceTone,      0.250f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.250f)
     .uni (ModSource::Macro3, Param::spaceSize,      0.150f)
     .uni (ModSource::Macro4, Param::waveDetune,     0.300f)
     .uni (ModSource::Macro4, Param::shapeDensity,   0.200f);
    sharedMacros (r, Param::spaceSize, Param::waveDetune);
    r.commit (s);
}});

//------------------------------------------------------- objects under a bow

manager.addFactory ({ "Tectonic Plate", "DRONE", { "scraped", "dark", "sub", "huge", "drift" }, [] (PatchState& s)
{
    // Two slabs of stone grinding past each other. The movement is the COUPLING
    // between the nodes: a 40-second looped envelope walks it from a loose pile
    // to a welded block and back, which changes what the object is, not how
    // loud it is. Under that, GRAVITY tilts the whole spectrum end over end.
    gesture (s, 4 /* FRICTION */, 0.72f, 0.10f, 0.62f, 0.30f, 0.26f, 0.34f);
    amp (s, 2.20f, 4.0f, 0.94f, 5.0f, 0.65f);
    shape (s, 0.44f, 0.18f, 0.88f, 0.26f, 0.82f, 0.44f);
    material (s, MaterialType::Void, MaterialType::Wood, 0.34f);
    topology (s, 3 /* LATTICE */, 0.30f, 0.30f, 811);
    matter (s, 0.96f, 0.44f, 0.14f, 0.62f);
    evolve (s, 0.0f, 0.20f, 0.0f, 0.0f, 0.66f, 0.16f, 0.0f, 0.06f, 0.44f);
    space (s, SpacePresets::Machine, 0.40f, 0.72f, 0.28f, 0.38f);

    env (s, 2, 18.0f, 22.0f, 0.60f, 8.0f, 0.6f, true);
    env (s, 3, 9.0f, 13.0f, 0.50f, 6.0f, 0.5f, true);
    lfo (s, 1, 0.031f, 0 /* SINE */, 1.0f, false, 4.0f);
    chaos (s, 1, 1 /* BROWNIAN */, 0.05f, 0.55f, 0.88f, 0.5f, 823);
    macros (s, 0.45f, 0.28f, 0.45f, 0.50f);

    Routings r;
    r.bi  (ModSource::Env2,      Param::shapeCoupling,    0.420f)
     .bi  (ModSource::Env3,      Param::evolveGravity,    0.300f)
     .bi  (ModSource::LFO1,      Param::shapeDistribution, 0.180f)
     .bi  (ModSource::Chaos1,    Param::gestureRoughness, 0.180f)
     .bi  (ModSource::Chaos1,    Param::shapeMass,        0.070f)
     .uni (ModSource::Velocity,  Param::gesturePressure,  0.220f)
     .bi  (ModSource::KeyTrack,  Param::shapeMass,       -0.240f)
     .uni (ModSource::Macro1,    Param::evolveMotion,     0.420f)
     .uni (ModSource::Macro1,    Param::gestureMotion,    0.300f)
     .uni (ModSource::Macro2,    Param::gestureBandwidth, 0.320f)
     .uni (ModSource::Macro2,    Param::spaceTone,        0.260f)
     .uni (ModSource::Macro3,    Param::spaceMix,         0.280f)
     .uni (ModSource::Macro4,    Param::spaceDistDrive,   0.300f)
     .uni (ModSource::Macro4,    Param::shapeSurface,     0.260f);
    sharedMacros (r, Param::spaceSize, Param::gestureRoughness);
    r.commit (s);
}});

manager.addFactory ({ "Corroded Vowel", "DRONE", { "vocal", "formant", "dirty", "evolving", "mid" }, [] (PatchState& s)
{
    // A held vowel that rusts. The FORMANT table supplies the mouth, a looped
    // envelope walks the wavetable position from one vowel to the next over
    // half a minute, and MELT rises underneath until the vowel is only a
    // suggestion. A slow SPECTRAL Fracture keeps handing fragments back.
    wave (s, 2 /* FORMANT */, 0.24f, 0.30f, 0.06f, 4, 0.22f, 0.75f, -1, 0.90f);
    amp (s, 1.40f, 3.0f, 0.92f, 4.0f, 0.60f);
    shape (s, 0.54f, 0.44f, 0.52f, 0.50f, 0.70f, 0.42f);
    material (s, MaterialType::Organic, MaterialType::Metal, 0.36f);
    topology (s, 1 /* RING */, 0.58f, 0.46f, 827);
    matter (s, 0.92f, 0.52f, 0.16f, 0.74f);
    evolve (s, 0.0f, 0.26f, 0.0f, 0.30f, 0.52f, 0.22f, 0.0f, 0.12f, 0.42f);
    set (s, Param::evolveMagnetTarget, 6 /* CUSTOM (harmonic) */);
    fracture (s, 0 /* SPECTRAL */, 0.34f, 0.32f, 0.65f, 0.30f, 0.42f, 0.55f, 0.72f, 0.46f, 0.35f,
              2 /* 32 */, 0 /* 1/1 */, 8, 0.0f, 2 /* PINGPONG */, 0.80f, 0.30f, 829,
              FractureShape { 32, 0.15f, 0.70f, 0.30f, 0.62f, 0.55f, 0.80f, 0.30f, 0.95f,
                              1.0f, 0.9f, 0.65f, 0.9f, kMinorTerrace, "XLXHXXLH", nullptr });
    space (s, SpacePresets::Nebula, 0.46f, 0.66f, 0.52f, 0.42f);

    env (s, 2, 14.0f, 17.0f, 0.55f, 7.0f, 0.55f, true);
    lfo (s, 1, 0.043f, 5 /* SMOOTH RANDOM */, 1.0f, false, 3.0f);
    lfo (s, 2, 0.11f, 0 /* SINE */, 1.0f, false, 2.0f);
    chaos (s, 1, 0 /* WALK */, 0.09f, 0.48f, 0.75f, 0.5f, 839);
    macros (s, 0.50f, 0.40f, 0.45f, 0.50f);

    Routings r;
    r.bi  (ModSource::Env2,      Param::wavePosition,     0.400f)
     .uni (ModSource::Env2,      Param::evolveMelt,       0.300f)
     .bi  (ModSource::LFO1,      Param::waveMorph,        0.240f)
     .bi  (ModSource::LFO2,      Param::shapeTension,     0.060f)
     .bi  (ModSource::Chaos1,    Param::shapeBlend,       0.180f)
     .uni (ModSource::Velocity,  Param::shapeExcite,      0.200f)
     .bi  (ModSource::NoteRandom, Param::waveDetune,      0.100f)
     .uni (ModSource::Macro1,    Param::evolveMotion,     0.420f)
     .uni (ModSource::Macro2,    Param::waveScan,         0.300f)
     .uni (ModSource::Macro2,    Param::fractureTone,     0.260f)
     .uni (ModSource::Macro3,    Param::spaceMix,         0.300f)
     .uni (ModSource::Macro4,    Param::evolveMelt,       0.320f)
     .uni (ModSource::Macro4,    Param::shapeBlend,       0.280f);
    sharedMacros (r, Param::fractureDecay, Param::chaos1Depth);
    r.commit (s);
}});

manager.addFactory ({ "Salt Lamp", "DRONE", { "glassy", "warm", "breathing", "close", "mid" }, [] (PatchState& s)
{
    // Small, warm and near. BEND is the whole patch: a very slow lever pivoting
    // low in the spectrum tips the partials up and then back down, so the
    // object leans between a harmonic bar and a stretched one without the
    // fundamental ever moving. Everything else is deliberately still.
    wave (s, 1 /* HARMONIC */, 0.34f, 0.18f, 0.02f, 4, 0.08f, 0.55f, 0, 0.85f);
    amp (s, 1.80f, 3.0f, 0.95f, 4.5f, 0.62f);
    shape (s, 0.30f, 0.12f, 0.40f, 0.62f, 0.76f, 0.14f);
    material (s, MaterialType::Crystal, MaterialType::Wood, 0.46f);
    topology (s, 5 /* STAR */, 0.26f, 0.28f, 853);
    matter (s, 0.94f, 0.50f, 0.16f, 0.56f);
    evolve (s, 0.30f, 0.0f, 0.0f, 0.0f, 0.50f, 0.08f, 0.0f, 0.05f, 0.30f);
    set (s, Param::evolveBendPivot, 0.34f);
    set (s, Param::evolveBendRange, 0.26f);
    set (s, Param::evolveBendCurve, 0.40f);
    space (s, SpacePresets::Chamber, 0.34f, 0.42f, 0.58f, 0.30f);

    env (s, 2, 11.0f, 15.0f, 0.45f, 6.0f, 0.55f, true);
    lfo (s, 1, 0.024f, 1 /* TRIANGLE */, 1.0f, false, 5.0f);
    lfo (s, 2, 0.072f, 0 /* SINE */, 1.0f, false, 3.0f);
    macros (s, 0.40f, 0.42f, 0.35f, 0.50f);

    Routings r;
    r.bi  (ModSource::Env2,      Param::evolveBend,       0.180f)
     .bi  (ModSource::LFO1,      Param::evolveBendPivot,  0.120f)
     .bi  (ModSource::LFO2,      Param::shapeTension,     0.080f)
     .bi  (ModSource::LFO2,      Param::waveDetune,       0.060f)
     .uni (ModSource::Velocity,  Param::shapeExcite,      0.220f)
     .bi  (ModSource::KeyTrack,  Param::shapeDecay,      -0.180f)
     .bi  (ModSource::NoteRandom, Param::evolveBendCurve, 0.140f)
     .uni (ModSource::Macro1,    Param::evolveMotion,     0.400f)
     .uni (ModSource::Macro2,    Param::wavePosition,     0.300f)
     .uni (ModSource::Macro2,    Param::shapeExcite,      0.240f)
     .uni (ModSource::Macro3,    Param::spaceMix,         0.300f)
     .uni (ModSource::Macro3,    Param::spaceSize,        0.220f)
     .uni (ModSource::Macro4,    Param::evolveBendRange,  0.320f)
     .uni (ModSource::Macro4,    Param::shapeBlend,       0.260f);
    sharedMacros (r, Param::spaceSize, Param::evolveBendCurve);
    r.commit (s);
}});

manager.addFactory ({ "Beacon Hum", "DRONE", { "metallic", "cold", "pulsing", "unstable", "low" }, [] (PatchState& s)
{
    // Mains hum in a steel cabinet. TEAR makes detuned twins of the strongest
    // nodes and a looped envelope opens it, so the beat between the twins slows
    // from a shimmer to about one pulse a second and back — the movement is a
    // beat frequency, not a tremolo.
    gesture (s, 5 /* ELECTRICAL */, 0.60f, 0.30f, 0.34f, 0.42f, 0.30f, 0.38f);
    amp (s, 0.90f, 2.5f, 0.92f, 3.0f, 0.55f);
    shape (s, 0.40f, 0.56f, 0.60f, 0.58f, 0.74f, 0.26f);
    material (s, MaterialType::Metal, MaterialType::Crystal, 0.38f);
    topology (s, 0 /* CHAIN */, 0.36f, 0.34f, 857);
    matter (s, 0.94f, 0.50f, 0.18f, 0.58f);
    evolve (s, 0.0f, 0.0f, 0.46f, 0.0f, 0.48f, 0.14f, 0.0f, 0.09f, 0.36f);
    set (s, Param::evolveScatterSeed, 859);
    space (s, SpacePresets::Machine, 0.36f, 0.48f, 0.42f, 0.36f);

    env (s, 2, 12.0f, 16.0f, 0.50f, 6.0f, 0.5f, true);
    lfo (s, 1, 0.055f, 1 /* TRIANGLE */, 1.0f, false, 3.0f);
    chaos (s, 1, 3 /* LORENZ */, 0.16f, 0.42f, 0.70f, 0.5f, 863);
    macros (s, 0.45f, 0.35f, 0.35f, 0.55f);

    Routings r;
    r.bi  (ModSource::Env2,      Param::evolveTear,       0.340f)
     .bi  (ModSource::LFO1,      Param::shapeTension,     0.090f)
     .bi  (ModSource::Chaos1,    Param::gestureSpeed,     0.200f)
     .bi  (ModSource::Chaos1,    Param::shapeDistribution, 0.100f)
     .uni (ModSource::Velocity,  Param::gesturePressure,  0.240f)
     .bi  (ModSource::KeyTrack,  Param::shapeDecay,      -0.200f)
     .bi  (ModSource::NoteRandom, Param::shapeCoupling,   0.120f)
     .uni (ModSource::Macro1,    Param::evolveMotion,     0.400f)
     .uni (ModSource::Macro1,    Param::gestureMotion,    0.280f)
     .uni (ModSource::Macro2,    Param::gestureBandwidth, 0.320f)
     .uni (ModSource::Macro3,    Param::spaceMix,         0.300f)
     .uni (ModSource::Macro4,    Param::evolveTear,       0.320f)
     .uni (ModSource::Macro4,    Param::spaceDistDrive,   0.280f);
    sharedMacros (r, Param::spaceSize, Param::chaos1Rate);
    r.commit (s);
}});

manager.addFactory ({ "Tide Organ", "DRONE", { "blown", "hollow", "breathing", "wide", "low" }, [] (PatchState& s)
{
    // A sea organ: waves push air through stone pipes. The Space is the thing
    // that breathes — size, feedback and mix all ride one 25-second cycle — so
    // the room advances and retreats around a pipe tone that barely changes.
    gesture (s, 3 /* BREATH */, 0.54f, 0.24f, 0.30f, 0.32f, 0.40f, 0.48f);
    amp (s, 2.00f, 3.5f, 0.93f, 5.0f, 0.62f);
    shape (s, 0.38f, 0.30f, 0.66f, 0.42f, 0.72f, 0.30f);
    material (s, MaterialType::Void, MaterialType::Liquid, 0.40f);
    topology (s, 2 /* CLUSTERS */, 0.46f, 0.40f, 877);
    matter (s, 0.92f, 0.46f, 0.16f, 0.84f);
    evolve (s, 0.0f, 0.16f, 0.0f, 0.24f, 0.56f, 0.16f, 0.0f, 0.07f, 0.34f);
    set (s, Param::evolveMagnetTarget, 1 /* FIFTH */);
    space (s, SpacePresets::Void, 0.42f, 0.55f, 0.44f, 0.34f);

    env (s, 2, 11.0f, 14.0f, 0.40f, 8.0f, 0.6f, true);
    lfo (s, 1, 0.040f, 0 /* SINE */, 1.0f, false, 4.0f);
    lfo (s, 2, 0.13f, 5 /* SMOOTH RANDOM */, 1.0f, false, 2.0f);
    macros (s, 0.40f, 0.35f, 0.55f, 0.45f);

    Routings r;
    r.bi  (ModSource::Env2,      Param::spaceSize,        0.400f)
     .bi  (ModSource::Env2,      Param::spaceMix,         0.220f)
     .bi  (ModSource::LFO1,      Param::spaceFeedback,    0.220f)
     .bi  (ModSource::LFO1,      Param::gesturePressure,  0.140f)
     .bi  (ModSource::LFO2,      Param::gestureSpeed,     0.160f)
     .uni (ModSource::Velocity,  Param::gesturePressure,  0.220f)
     .bi  (ModSource::KeyTrack,  Param::gestureBandwidth, 0.220f)
     .bi  (ModSource::NoteRandom, Param::gesturePosition, 0.120f)
     .uni (ModSource::Macro1,    Param::evolveMotion,     0.380f)
     .uni (ModSource::Macro1,    Param::gestureMotion,    0.300f)
     .uni (ModSource::Macro2,    Param::gestureBandwidth, 0.300f)
     .uni (ModSource::Macro3,    Param::spaceMix,         0.320f)
     .uni (ModSource::Macro4,    Param::evolveMagnet,     0.300f)
     .uni (ModSource::Macro4,    Param::shapeBlend,       0.260f);
    sharedMacros (r, Param::spaceSize, Param::gestureRoughness);
    r.commit (s);
}});

manager.addFactory ({ "Bowed Monolith", "DRONE", { "bowed", "metallic", "cold", "huge", "evolving" }, [] (PatchState& s)
{
    // An enormous slab of metal under a bow the size of a girder. MAGNET is the
    // event: over half a minute it hauls the inharmonic plate modes onto the
    // OCTAVE grid and lets them go again, so the object resolves into a chord
    // and then falls back out of tune with itself.
    gesture (s, 0 /* BOW */, 0.66f, 0.20f, 0.26f, 0.28f, 0.22f, 0.40f);
    amp (s, 2.40f, 4.0f, 0.94f, 6.0f, 0.65f);
    set (s, Param::masterGain, -3.5f);
    shape (s, 0.58f, 0.62f, 0.70f, 0.48f, 0.84f, 0.24f);
    material (s, MaterialType::Metal, MaterialType::Void, 0.42f);
    topology (s, 2 /* CLUSTERS */, 0.54f, 0.44f, 881);
    matter (s, 0.96f, 0.46f, 0.14f, 0.86f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.30f, 0.44f, 0.18f, 0.0f, 0.06f, 0.38f);
    set (s, Param::evolveMagnetTarget, 0 /* OCTAVE */);
    space (s, SpacePresets::Void, 0.48f, 0.90f, 0.40f, 0.44f);

    env (s, 2, 16.0f, 20.0f, 0.35f, 9.0f, 0.60f, true);
    lfo (s, 1, 0.028f, 1 /* TRIANGLE */, 1.0f, false, 5.0f);
    chaos (s, 1, 1 /* BROWNIAN */, 0.06f, 0.45f, 0.85f, 0.5f, 883);
    macros (s, 0.50f, 0.30f, 0.55f, 0.55f);

    Routings r;
    r.bi  (ModSource::Env2,      Param::evolveMagnet,     0.420f)
     .bi  (ModSource::Env2,      Param::shapeForm,        0.180f)
     .bi  (ModSource::LFO1,      Param::gesturePressure,  0.160f)
     .bi  (ModSource::Chaos1,    Param::shapeCoupling,    0.160f)
     .uni (ModSource::Velocity,  Param::gesturePressure,  0.240f)
     .bi  (ModSource::KeyTrack,  Param::shapeMass,       -0.220f)
     .bi  (ModSource::NoteRandom, Param::shapeDistribution, 0.120f)
     .uni (ModSource::Macro1,    Param::evolveMotion,     0.420f)
     .uni (ModSource::Macro1,    Param::gestureMotion,    0.300f)
     .uni (ModSource::Macro2,    Param::gestureBandwidth, 0.320f)
     .uni (ModSource::Macro2,    Param::spaceTone,        0.240f)
     .uni (ModSource::Macro3,    Param::spaceMix,         0.300f)
     .uni (ModSource::Macro4,    Param::evolveMagnet,     0.300f)
     .uni (ModSource::Macro4,    Param::shapeForm,        0.260f);
    sharedMacros (r, Param::spaceSize, Param::gestureRoughness);
    r.commit (s);
}});

manager.addFactory ({ "Marrow Pipe", "DRONE", { "scraped", "hollow", "organic", "unstable", "high" }, [] (PatchState& s)
{
    // A bone tube played too hard: the air column keeps almost overblowing.
    // A LORENZ generator drives the scrape speed and the node distribution
    // together, so the tone hunts between two registers the way a real
    // over-pressured pipe does, and never settles on either.
    gesture (s, 1 /* SCRAPE */, 0.50f, 0.46f, 0.42f, 0.52f, 0.50f, 0.24f);
    amp (s, 0.70f, 2.0f, 0.88f, 2.5f, 0.55f);
    shape (s, 0.34f, 0.36f, 0.30f, 0.64f, 0.80f, 0.46f);
    material (s, MaterialType::Wood, MaterialType::Organic, 0.44f);
    topology (s, 0 /* CHAIN */, 0.40f, 0.52f, 887);
    matter (s, 1.0f, 0.70f, 0.20f, 0.60f);
    set (s, Param::masterGain, 1.0f);
    evolve (s, 0.0f, 0.18f, 0.0f, 0.0f, 0.42f, 0.34f, 0.0f, 0.28f, 0.50f);
    set (s, Param::evolveScatterSeed, 907);
    space (s, SpacePresets::Chamber, 0.32f, 0.40f, 0.60f, 0.30f);

    lfo (s, 1, 0.19f, 5 /* SMOOTH RANDOM */, 1.0f, false, 1.5f);
    env (s, 2, 7.0f, 9.0f, 0.45f, 4.0f, 0.5f, true);
    chaos (s, 1, 3 /* LORENZ */, 0.28f, 0.60f, 0.55f, 0.5f, 911);
    chaos (s, 2, 0 /* WALK */, 0.08f, 0.40f, 0.80f, 0.5f, 919);
    macros (s, 0.55f, 0.42f, 0.32f, 0.55f);

    Routings r;
    r.bi  (ModSource::Chaos1,    Param::gestureSpeed,     0.280f)
     .bi  (ModSource::Chaos1,    Param::shapeDistribution, 0.200f)
     .bi  (ModSource::Chaos2,    Param::shapeBlend,       0.340f)
     .bi  (ModSource::LFO1,      Param::gesturePosition,  0.200f)
     .bi  (ModSource::Env2,      Param::shapeForm,        0.300f)
     .bi  (ModSource::Env2,      Param::shapeTension,     0.160f)
     .uni (ModSource::Env2,      Param::evolveMelt,       0.340f)
     .uni (ModSource::Velocity,  Param::gesturePressure,  0.260f)
     .bi  (ModSource::KeyTrack,  Param::gestureSpeed,     0.200f)
     .bi  (ModSource::NoteRandom, Param::gestureRoughness, 0.140f)
     .uni (ModSource::Macro1,    Param::gestureMotion,    0.400f)
     .uni (ModSource::Macro1,    Param::evolveMotion,     0.300f)
     .uni (ModSource::Macro2,    Param::gestureBandwidth, 0.320f)
     .uni (ModSource::Macro3,    Param::spaceMix,         0.300f)
     .uni (ModSource::Macro4,    Param::gestureRoughness, 0.320f)
     .uni (ModSource::Macro4,    Param::shapeSurface,     0.260f);
    sharedMacros (r, Param::spaceSize, Param::chaos1Depth);
    r.commit (s);
}});

manager.addFactory ({ "Cooling Tower", "DRONE", { "synthetic", "dirty", "chaotic", "roomy", "low" }, [] (PatchState& s)
{
    // Plant noise: a brown roar and a fold-tone whine layered, running through
    // a random graph of metal and water. A BROWNIAN generator walks the
    // material blend across the whole span, so what the roar is resonating in
    // slowly turns from pipework into coolant and back.
    dust (s, 2 /* BROWN */, 0.62f, 0.34f, 0.30f, 0.34f, 0.55f, 0.60f, 929, 0.55f);
    wave (s, 3 /* FOLDED */, 0.42f, 0.36f, 0.05f, 3, 0.26f, 0.70f, -1, 0.42f);
    layerSources (s, 0 /* WAVE */, 1 /* DUST */);
    amp (s, 1.30f, 3.0f, 0.90f, 3.5f, 0.58f);
    shape (s, 0.66f, 0.52f, 0.62f, 0.44f, 0.70f, 0.48f);
    material (s, MaterialType::Metal, MaterialType::Liquid, 0.50f);
    topology (s, 4 /* RANDOM */, 0.60f, 0.58f, 937);
    matter (s, 0.90f, 0.52f, 0.18f, 0.76f);
    evolve (s, 0.0f, 0.22f, 0.0f, 0.0f, 0.54f, 0.30f, 0.16f, 0.14f, 0.46f);
    set (s, Param::evolveScatterSeed, 941);
    space (s, SpacePresets::Machine, 0.40f, 0.60f, 0.40f, 0.40f);

    lfo (s, 1, 0.037f, 5 /* SMOOTH RANDOM */, 1.0f, false, 3.0f);
    lfo (s, 2, 0.21f, 0 /* SINE */, 1.0f, false, 2.0f);
    env (s, 2, 10.0f, 13.0f, 0.50f, 6.0f, 0.55f, true);
    chaos (s, 1, 1 /* BROWNIAN */, 0.045f, 0.85f, 0.90f, 0.5f, 947);
    chaos (s, 2, 4 /* TARGETS */, 0.22f, 0.45f, 0.60f, 0.5f, 953);
    macros (s, 0.55f, 0.35f, 0.42f, 0.55f);

    Routings r;
    r.bi  (ModSource::Chaos1,    Param::shapeBlend,       0.480f)
     .bi  (ModSource::Chaos2,    Param::dustColor,        0.220f)
     .bi  (ModSource::LFO1,      Param::waveMorph,        0.220f)
     .bi  (ModSource::LFO2,      Param::shapeCoupling,    0.100f)
     .uni (ModSource::Env2,      Param::evolveCrush,      0.240f)
     .uni (ModSource::Velocity,  Param::dustDensity,      0.220f)
     .bi  (ModSource::KeyTrack,  Param::dustColor,        0.200f)
     .bi  (ModSource::NoteRandom, Param::waveDetune,      0.120f)
     .uni (ModSource::Macro1,    Param::evolveMotion,     0.420f)
     .uni (ModSource::Macro2,    Param::dustColor,        0.300f)
     .uni (ModSource::Macro2,    Param::waveScan,         0.240f)
     .uni (ModSource::Macro3,    Param::spaceMix,         0.300f)
     .uni (ModSource::Macro4,    Param::evolveCrush,      0.300f)
     .uni (ModSource::Macro4,    Param::spaceDistDrive,   0.280f);
    sharedMacros (r, Param::spaceSize, Param::chaos1Rate);
    r.commit (s);
}});
}

} // namespace am::FactoryContent
