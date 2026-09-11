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
     .bi  (ModSource::NoteRandom, Param::shapeDistribution, 0.120f)
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
     .bi  (ModSource::Chaos1,    Param::shapeMass,        0.100f)
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
     .bi  (ModSource::LFO2,      Param::shapeMass,        0.080f)
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

//------------------------------------------------ clouds, tape and cold glass

manager.addFactory ({ "Glacier Slab", "DRONE", { "cold", "glassy", "morphing", "huge", "drift" }, [] (PatchState& s)
{
    // A sheet of ice thawing and refreezing. MELT is on a thirty-second loop:
    // the partials sag and lose their certainty, the high weights fade, and
    // then the whole slab tightens back up. The Fracture behind it holds the
    // fragments that came loose while it was soft.
    wave (s, 5 /* SPECTRAL */, 0.30f, 0.24f, 0.0f, 4, 0.05f, 0.85f, -1, 0.88f);
    amp (s, 2.20f, 4.0f, 0.93f, 5.0f, 0.62f);
    shape (s, 0.52f, 0.58f, 0.44f, 0.60f, 0.82f, 0.20f);
    material (s, MaterialType::Crystal, MaterialType::Liquid, 0.34f);
    topology (s, 3 /* LATTICE */, 0.40f, 0.42f, 967);
    matter (s, 0.94f, 0.50f, 0.16f, 0.78f);
    evolve (s, 0.0f, 0.22f, 0.0f, 0.0f, 0.46f, 0.16f, 0.0f, 0.08f, 0.40f);
    fracture (s, 3 /* EVOLVE */, 0.36f, 0.30f, 0.70f, 0.28f, 0.46f, 0.60f, 0.78f, 0.56f, 0.45f,
              2 /* 32 */, 1 /* 1/2 */, 8, 0.0f, 2 /* PINGPONG */, 0.75f, 0.30f, 971,
              FractureShape { 32, 0.18f, 0.75f, 0.35f, 0.66f, 0.60f, 0.84f, 0.35f, 0.95f,
                              1.0f, 0.85f, 0.70f, 0.9f, kFifthTerrace, "XLXXHXLX", nullptr });
    space (s, SpacePresets::Dream, 0.46f, 0.74f, 0.60f, 0.40f);

    env (s, 2, 14.0f, 18.0f, 0.40f, 8.0f, 0.60f, true);
    lfo (s, 1, 0.033f, 0 /* SINE */, 1.0f, false, 4.0f);
    lfo (s, 2, 0.087f, 5 /* SMOOTH RANDOM */, 1.0f, false, 3.0f);
    macros (s, 0.45f, 0.38f, 0.50f, 0.50f);

    Routings r;
    r.uni (ModSource::Env2,      Param::evolveMelt,       0.300f)
     .bi  (ModSource::Env2,      Param::shapeTension,     0.180f)
     .bi  (ModSource::LFO1,      Param::shapeBlend,       0.220f)
     .bi  (ModSource::LFO2,      Param::wavePosition,     0.220f)
     .uni (ModSource::Velocity,  Param::shapeExcite,      0.220f)
     .bi  (ModSource::KeyTrack,  Param::shapeDecay,      -0.180f)
     .bi  (ModSource::NoteRandom, Param::waveDetune,      0.100f)
     .uni (ModSource::Macro1,    Param::evolveMotion,     0.400f)
     .uni (ModSource::Macro2,    Param::waveScan,         0.280f)
     .uni (ModSource::Macro2,    Param::fractureTone,     0.260f)
     .uni (ModSource::Macro3,    Param::spaceMix,         0.300f)
     .uni (ModSource::Macro4,    Param::evolveMelt,       0.320f)
     .uni (ModSource::Macro4,    Param::shapeBlend,       0.280f);
    sharedMacros (r, Param::fractureDecay, Param::fractureRandom);
    r.commit (s);
}});

manager.addFactory ({ "Cinder Cloud", "DRONE", { "granular", "dark", "chaotic", "wide", "low" }, [] (PatchState& s)
{
    // A cloud of hot ash falling on a drum head. DUST in CLOUD mode is thousands
    // of tiny impacts a second, and the density, the grain and the jitter are
    // all walking, so the cloud thickens and thins without ever repeating.
    dust (s, 7 /* CLOUD */, 0.52f, 0.38f, 0.44f, 0.55f, 0.70f, 0.80f, 977, 0.90f);
    amp (s, 1.20f, 3.0f, 0.90f, 3.5f, 0.58f);
    shape (s, 0.60f, 0.34f, 0.68f, 0.38f, 0.74f, 0.44f);
    material (s, MaterialType::Membrane, MaterialType::Void, 0.44f);
    topology (s, 2 /* CLUSTERS */, 0.52f, 0.50f, 983);
    matter (s, 0.96f, 0.56f, 0.18f, 0.86f);
    evolve (s, 0.0f, 0.20f, 0.0f, 0.0f, 0.58f, 0.36f, 0.0f, 0.11f, 0.48f);
    set (s, Param::evolveScatterSeed, 991);
    space (s, SpacePresets::Dust, 0.44f, 0.70f, 0.40f, 0.42f);

    env (s, 2, 13.0f, 16.0f, 0.45f, 7.0f, 0.55f, true);
    lfo (s, 1, 0.052f, 5 /* SMOOTH RANDOM */, 1.0f, false, 3.0f);
    chaos (s, 1, 1 /* BROWNIAN */, 0.07f, 0.60f, 0.86f, 0.5f, 997);
    chaos (s, 2, 0 /* WALK */, 0.19f, 0.40f, 0.70f, 0.5f, 1009);
    macros (s, 0.50f, 0.32f, 0.48f, 0.55f);

    Routings r;
    r.bi  (ModSource::Chaos1,    Param::dustDensity,      0.380f)
     .bi  (ModSource::Chaos1,    Param::shapeDistribution, 0.160f)
     .bi  (ModSource::Chaos2,    Param::dustGrain,        0.260f)
     .bi  (ModSource::LFO1,      Param::dustJitter,       0.240f)
     .bi  (ModSource::Env2,      Param::shapeForm,        0.260f)
     .uni (ModSource::Env2,      Param::evolveMelt,       0.240f)
     .uni (ModSource::Velocity,  Param::dustDensity,      0.220f)
     .bi  (ModSource::KeyTrack,  Param::dustColor,        0.240f)
     .bi  (ModSource::NoteRandom, Param::dustSpread,      0.140f)
     .uni (ModSource::Macro1,    Param::evolveMotion,     0.400f)
     .uni (ModSource::Macro2,    Param::dustColor,        0.320f)
     .uni (ModSource::Macro3,    Param::spaceMix,         0.300f)
     .uni (ModSource::Macro4,    Param::dustGrain,        0.320f)
     .uni (ModSource::Macro4,    Param::shapeSurface,     0.260f);
    sharedMacros (r, Param::spaceSize, Param::chaos1Depth);
    r.commit (s);
}});

manager.addFactory ({ "Vinyl Chapel", "DRONE", { "noisy", "warm", "distant", "breathing", "mid" }, [] (PatchState& s)
{
    // A drone made entirely of surface noise: the VinylDust built-in looped
    // through a stone room until the crackle becomes a pitch. A slow walk drags
    // the loop start across the sample, so the grain of the record keeps
    // changing while the room around it opens and closes.
    sample (s, BuiltInSamples::Kind::VinylDust, 1 /* LOOP */, 0.05f, 0.95f, 0.30f, 0.60f, 48, 0.92f);
    amp (s, 1.60f, 3.0f, 0.92f, 4.0f, 0.60f);
    shape (s, 0.48f, 0.42f, 0.56f, 0.46f, 0.78f, 0.38f);
    material (s, MaterialType::Void, MaterialType::Organic, 0.40f);
    topology (s, 4 /* RANDOM */, 0.46f, 0.54f, 1013);
    matter (s, 0.96f, 0.58f, 0.16f, 0.74f);
    evolve (s, 0.0f, 0.14f, 0.0f, 0.26f, 0.54f, 0.24f, 0.0f, 0.09f, 0.38f);
    set (s, Param::evolveMagnetTarget, 5 /* SCALE */);
    space (s, SpacePresets::Void, 0.48f, 0.72f, 0.44f, 0.44f);

    env (s, 2, 12.0f, 15.0f, 0.45f, 7.0f, 0.55f, true);
    lfo (s, 1, 0.036f, 1 /* TRIANGLE */, 1.0f, false, 4.0f);
    lfo (s, 2, 0.10f, 0 /* SINE */, 1.0f, false, 2.0f);
    chaos (s, 1, 0 /* WALK */, 0.06f, 0.55f, 0.82f, 0.5f, 1019);
    macros (s, 0.42f, 0.36f, 0.55f, 0.48f);

    Routings r;
    r.bi  (ModSource::Chaos1,    Param::sampleStart,      0.320f)
     .bi  (ModSource::Env2,      Param::spaceSize,        0.300f)
     .uni (ModSource::Env2,      Param::evolveMagnet,     0.280f)
     .bi  (ModSource::LFO1,      Param::shapeMass,        0.140f)
     .bi  (ModSource::LFO2,      Param::sampleGrain,      0.200f)
     .uni (ModSource::Velocity,  Param::shapeExcite,      0.220f)
     .bi  (ModSource::KeyTrack,  Param::sampleGrain,      0.220f)
     .bi  (ModSource::NoteRandom, Param::sampleSpread,    0.140f)
     .uni (ModSource::Macro1,    Param::evolveMotion,     0.380f)
     .uni (ModSource::Macro2,    Param::spaceTone,        0.300f)
     .uni (ModSource::Macro2,    Param::shapeExcite,      0.240f)
     .uni (ModSource::Macro3,    Param::spaceMix,         0.320f)
     .uni (ModSource::Macro4,    Param::sampleGrain,      0.300f)
     .uni (ModSource::Macro4,    Param::evolveMagnet,     0.260f);
    sharedMacros (r, Param::spaceSize, Param::chaos1Rate);
    r.commit (s);
}});

manager.addFactory ({ "Breath Cavern", "DRONE", { "vocal", "organic", "breathing", "roomy", "mid" }, [] (PatchState& s)
{
    // A lung that never runs out. The Breath built-in read granularly and fed
    // into an organic ring: the grains carry the noise of a real exhalation,
    // the ring turns it into a throat, and a looped envelope opens and closes
    // that throat on a twenty-second cycle.
    sample (s, BuiltInSamples::Kind::Breath, 3 /* GRANULAR */, 0.10f, 0.90f, 0.42f, 0.65f, 55, 0.92f);
    amp (s, 1.10f, 2.5f, 0.90f, 3.0f, 0.58f);
    set (s, Param::masterGain, 2.5f);
    shape (s, 0.44f, 0.40f, 0.48f, 0.52f, 0.70f, 0.46f);
    material (s, MaterialType::Organic, MaterialType::Membrane, 0.42f);
    topology (s, 1 /* RING */, 0.60f, 0.46f, 1021);
    matter (s, 0.96f, 0.60f, 0.18f, 0.70f);
    evolve (s, 0.0f, 0.24f, 0.0f, 0.0f, 0.50f, 0.26f, 0.0f, 0.16f, 0.44f);
    set (s, Param::evolveScatterSeed, 1031);
    space (s, SpacePresets::Nebula, 0.42f, 0.62f, 0.54f, 0.38f);

    env (s, 2, 9.0f, 11.0f, 0.40f, 5.0f, 0.55f, true);
    env (s, 3, 20.0f, 24.0f, 0.50f, 8.0f, 0.60f, true);
    lfo (s, 1, 0.14f, 5 /* SMOOTH RANDOM */, 1.0f, false, 2.0f);
    chaos (s, 1, 0 /* WALK */, 0.11f, 0.45f, 0.72f, 0.5f, 1033);
    macros (s, 0.48f, 0.40f, 0.44f, 0.52f);

    Routings r;
    r.bi  (ModSource::Env2,      Param::shapeTension,     0.220f)
     .bi  (ModSource::Env2,      Param::sampleGrain,      0.240f)
     .bi  (ModSource::Env3,      Param::shapeMass,        0.180f)
     .bi  (ModSource::Chaos1,    Param::sampleStart,      0.280f)
     .bi  (ModSource::LFO1,      Param::shapeForm,        0.140f)
     .uni (ModSource::Velocity,  Param::sampleGrain,      0.220f)
     .bi  (ModSource::KeyTrack,  Param::sampleSpread,     0.200f)
     .bi  (ModSource::NoteRandom, Param::sampleStart,     0.120f)
     .uni (ModSource::Macro1,    Param::evolveMotion,     0.400f)
     .uni (ModSource::Macro2,    Param::shapeExcite,      0.300f)
     .uni (ModSource::Macro2,    Param::spaceTone,        0.240f)
     .uni (ModSource::Macro3,    Param::spaceMix,         0.300f)
     .uni (ModSource::Macro4,    Param::sampleGrain,      0.320f)
     .uni (ModSource::Macro4,    Param::shapeSurface,     0.260f);
    sharedMacros (r, Param::spaceSize, Param::chaos1Depth);
    r.commit (s);
}});

manager.addFactory ({ "Cleaved String", "DRONE", { "metallic", "unstable", "harsh", "close", "mid" }, [] (PatchState& s)
{
    // TEAR is the whole idea: the least important nodes become detuned twins of
    // the most important ones, and the clusters that are left are pulled apart
    // in pitch, stereo and ring time. One wire becomes two wires that disagree,
    // and a looped envelope decides how far apart they are allowed to get.
    wave (s, 0 /* BASIC */, 0.46f, 0.22f, 0.03f, 4, 0.05f, 0.60f, 0, 0.85f);
    amp (s, 0.80f, 2.2f, 0.90f, 2.8f, 0.55f);
    set (s, Param::masterGain, -2.5f);
    shape (s, 0.42f, 0.14f, 0.38f, 0.66f, 0.76f, 0.30f);
    material (s, MaterialType::String, MaterialType::Metal, 0.40f);
    topology (s, 1 /* RING */, 0.44f, 0.38f, 1039);
    matter (s, 0.94f, 0.56f, 0.18f, 0.64f);
    evolve (s, 0.0f, 0.0f, 0.50f, 0.0f, 0.46f, 0.20f, 0.0f, 0.13f, 0.42f);
    set (s, Param::evolveScatterSeed, 1049);
    fracture (s, 3 /* EVOLVE */, 0.32f, 0.28f, 0.55f, 0.32f, 0.38f, 0.42f, 0.70f, 0.50f, 0.40f,
              1 /* 16 */, 1 /* 1/2 */, 8, 0.0f, 0 /* FORWARD */, 0.85f, 0.30f, 1051,
              FractureShape { 16, 0.12f, 0.58f, 0.25f, 0.55f, 0.50f, 0.76f, 0.30f, 0.90f,
                              1.0f, 0.9f, 0.60f, 0.9f, kMinorTerrace, "XLXHXLXH", nullptr });
    space (s, SpacePresets::Chamber, 0.34f, 0.44f, 0.52f, 0.34f);

    env (s, 2, 10.0f, 14.0f, 0.45f, 6.0f, 0.55f, true);
    lfo (s, 1, 0.047f, 1 /* TRIANGLE */, 1.0f, false, 3.0f);
    chaos (s, 1, 2 /* LOGISTIC */, 0.13f, 0.50f, 0.66f, 0.5f, 1061);
    macros (s, 0.48f, 0.42f, 0.34f, 0.55f);

    Routings r;
    r.bi  (ModSource::Env2,      Param::evolveTear,       0.240f)
     .bi  (ModSource::LFO1,      Param::shapeMass,        0.140f)
     .bi  (ModSource::Chaos1,    Param::shapeDistribution, 0.220f)
     .bi  (ModSource::Chaos1,    Param::waveDetune,       0.120f)
     .uni (ModSource::Velocity,  Param::shapeExcite,      0.220f)
     .bi  (ModSource::KeyTrack,  Param::shapeDecay,      -0.200f)
     .uni (ModSource::Macro1,    Param::evolveMotion,     0.420f)
     .uni (ModSource::Macro2,    Param::wavePosition,     0.300f)
     .uni (ModSource::Macro2,    Param::fractureTone,     0.240f)
     .uni (ModSource::Macro3,    Param::spaceMix,         0.300f)
     .uni (ModSource::Macro4,    Param::evolveTear,       0.320f)
     .uni (ModSource::Macro4,    Param::shapeDecay,       0.260f);
    sharedMacros (r, Param::fractureDecay, Param::chaos1Depth);
    r.commit (s);
}});

manager.addFactory ({ "Pressure Vessel", "DRONE", { "metallic", "hollow", "morphing", "low", "roomy" }, [] (PatchState& s)
{
    // GRAVITY tilts the whole node set: below the middle the energy and the
    // ring time lift toward the top partials, above it everything sinks. A
    // looped envelope takes it end over end across a thirty-second cycle, so
    // the object empties upward and refills downward while its level and its
    // pitch both stay where they are.
    wave (s, 3 /* FOLDED */, 0.38f, 0.28f, 0.0f, 4, 0.05f, 0.50f, -1, 0.88f);
    amp (s, 1.50f, 3.0f, 0.92f, 4.0f, 0.60f);
    set (s, Param::masterGain, -3.0f);
    shape (s, 0.66f, 0.46f, 0.62f, 0.50f, 0.76f, 0.32f);
    material (s, MaterialType::Metal, MaterialType::Membrane, 0.44f);
    topology (s, 3 /* LATTICE */, 0.50f, 0.44f, 1063);
    matter (s, 0.96f, 0.52f, 0.16f, 0.68f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.50f, 0.18f, 0.0f, 0.10f, 0.36f);
    space (s, SpacePresets::Orbit, 0.38f, 0.56f, 0.46f, 0.40f);

    env (s, 2, 15.0f, 18.0f, 0.50f, 8.0f, 0.55f, true);
    lfo (s, 1, 0.029f, 0 /* SINE */, 1.0f, false, 4.0f);
    lfo (s, 2, 0.094f, 5 /* SMOOTH RANDOM */, 1.0f, false, 2.0f);
    macros (s, 0.42f, 0.40f, 0.42f, 0.52f);

    Routings r;
    r.bi  (ModSource::Env2,      Param::evolveGravity,    0.420f)
     .bi  (ModSource::Env2,      Param::wavePosition,     0.240f)
     .bi  (ModSource::LFO1,      Param::shapeMass,        0.160f)
     .bi  (ModSource::LFO1,      Param::shapeDensity,     0.200f)
     .bi  (ModSource::LFO2,      Param::shapeForm,        0.240f)
     .bi  (ModSource::LFO2,      Param::shapeSurface,     0.120f)
     .uni (ModSource::Velocity,  Param::shapeExcite,      0.220f)
     .bi  (ModSource::KeyTrack,  Param::shapeMass,       -0.220f)
     .bi  (ModSource::NoteRandom, Param::waveDetune,      0.120f)
     .uni (ModSource::Macro1,    Param::evolveMotion,     0.400f)
     .uni (ModSource::Macro2,    Param::wavePosition,     0.300f)
     .uni (ModSource::Macro2,    Param::spaceTone,        0.240f)
     .uni (ModSource::Macro3,    Param::spaceMix,         0.320f)
     .uni (ModSource::Macro4,    Param::evolveGravity,    0.300f)
     .uni (ModSource::Macro4,    Param::shapeDensity,     0.260f);
    sharedMacros (r, Param::spaceSize, Param::waveDetune);
    r.commit (s);
}});

manager.addFactory ({ "Amber Orchard", "DRONE", { "wooden", "glassy", "static", "distant", "drift" }, [] (PatchState& s)
{
    // FREEZE captures the object and floors its damping: the orchard of struck
    // wood cannot change shape at all, and rings for about seventy seconds
    // rather than forever, since the damping floor is a T60 and not a hold.
    // What can change is what is fed into it — a slow mallet roll whose rate,
    // brightness and randomness all drift, exciting different parts of a
    // structure set in amber — and Env3 leans that roll in harder across the
    // first half minute to stand against the ring going quiet.
    impact (s, 2 /* PLUCK */, 0.48f, 0.50f, 0.22f, 0.70f, 0.40f, 0.45f, 0.36f, 1.0f);
    amp (s, 1.20f, 3.0f, 0.88f, 3.5f, 0.55f);
    shape (s, 0.40f, 0.22f, 0.40f, 0.60f, 0.72f, 0.24f);
    material (s, MaterialType::Wood, MaterialType::Crystal, 0.48f);
    topology (s, 0 /* CHAIN */, 0.34f, 0.42f, 1069);
    matter (s, 0.94f, 0.80f, 0.30f, 0.70f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.50f, 0.10f, 0.0f, 0.10f, 0.20f);
    set (s, Param::evolveFreeze, 1.0f);
    space (s, SpacePresets::Dream, 0.46f, 0.76f, 0.58f, 0.42f);

    env (s, 2, 13.0f, 17.0f, 0.45f, 7.0f, 0.55f, true);
    env (s, 3, 24.0f, 6.0f, 1.0f, 6.0f, 0.55f);
    lfo (s, 1, 0.038f, 1 /* TRIANGLE */, 1.0f, false, 4.0f);
    chaos (s, 1, 1 /* BROWNIAN */, 0.08f, 0.55f, 0.84f, 0.5f, 1087);
    macros (s, 0.40f, 0.42f, 0.50f, 0.50f);

    Routings r;
    r.bi  (ModSource::Env2,      Param::impactBrightness, 0.360f)
     .uni (ModSource::Env3,      Param::impactRate,       0.300f)
     .uni (ModSource::Env3,      Param::shapeExcite,      0.180f)
     .bi  (ModSource::Chaos1,    Param::impactRate,       0.200f)
     .bi  (ModSource::Chaos1,    Param::impactHardness,   0.200f)
     .bi  (ModSource::LFO1,      Param::spaceSize,        0.240f)
     .bi  (ModSource::LFO1,      Param::impactRandom,     0.180f)
     .uni (ModSource::Velocity,  Param::impactRate,       0.240f)
     .bi  (ModSource::KeyTrack,  Param::impactBrightness, 0.220f)
     .bi  (ModSource::NoteRandom, Param::impactRandom,    0.140f)
     .uni (ModSource::Macro1,    Param::impactRate,       0.380f)
     .uni (ModSource::Macro2,    Param::impactBrightness, 0.320f)
     .uni (ModSource::Macro3,    Param::spaceMix,         0.320f)
     .uni (ModSource::Macro4,    Param::impactRandom,     0.300f)
     .uni (ModSource::Macro4,    Param::shapeBlend,       0.260f);
    sharedMacros (r, Param::ampDecay, Param::impactRandom);
    r.commit (s);
}});

manager.addFactory ({ "Harbour Buoy", "DRONE", { "metallic", "cold", "distant", "huge", "pulsing" }, [] (PatchState& s)
{
    // A bell buoy on a long swell: a strike about every second and a half, the
    // interval jittered so it never becomes a tempo, into a bell hung off a
    // single STAR hub whose tail is longer than the gap between strikes. The
    // drone is the overlap of tolls that have not finished yet, heard from far
    // enough away that the water has softened them.
    impact (s, 4 /* METAL STRIKE */, 0.50f, 0.34f, 0.62f, 0.75f, 0.55f, 0.55f, 0.06f, 0.62f);
    amp (s, 0.60f, 2.5f, 0.90f, 3.0f, 0.55f);
    set (s, Param::masterGain, -2.0f);
    shape (s, 0.42f, 0.60f, 0.62f, 0.56f, 0.86f, 0.26f);
    material (s, MaterialType::Metal, MaterialType::Void, 0.40f);
    topology (s, 5 /* STAR */, 0.36f, 0.30f, 1091);
    matter (s, 0.96f, 0.50f, 0.44f, 0.80f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.34f, 0.52f, 0.22f, 0.0f, 0.07f, 0.34f);
    set (s, Param::evolveMagnetTarget, 1 /* FIFTH */);
    fracture (s, 0 /* SPECTRAL */, 0.30f, 0.26f, 0.68f, 0.28f, 0.40f, 0.62f, 0.80f, 0.44f, 0.30f,
              2 /* 32 */, 0 /* 1/1 */, 8, 0.0f, 3 /* RANDOM */, 0.70f, 0.35f, 1093,
              FractureShape { 32, 0.20f, 0.80f, 0.30f, 0.60f, 0.58f, 0.86f, 0.35f, 0.95f,
                              1.0f, 0.85f, 0.75f, 0.85f, kOctaveTerrace, "XLXHXLXH", nullptr });
    space (s, SpacePresets::Dream, 0.50f, 0.86f, 0.48f, 0.46f);

    env (s, 2, 16.0f, 20.0f, 0.45f, 9.0f, 0.60f, true);
    lfo (s, 1, 0.026f, 0 /* SINE */, 1.0f, false, 5.0f);
    chaos (s, 1, 1 /* BROWNIAN */, 0.05f, 0.50f, 0.88f, 0.5f, 1097);
    macros (s, 0.42f, 0.30f, 0.55f, 0.50f);

    Routings r;
    r.bi  (ModSource::Env2,      Param::evolveMagnet,     0.320f)
     .bi  (ModSource::Env2,      Param::impactLength,     0.200f)
     .bi  (ModSource::Chaos1,    Param::impactRate,       0.180f)
     .bi  (ModSource::LFO1,      Param::shapeMass,        0.140f)
     .uni (ModSource::Velocity,  Param::impactHardness,   0.280f)
     .uni (ModSource::Velocity,  Param::impactRate,       0.160f)
     .bi  (ModSource::KeyTrack,  Param::shapeDecay,      -0.220f)
     .bi  (ModSource::NoteRandom, Param::impactRandom,    0.150f)
     .uni (ModSource::Macro1,    Param::impactRate,       0.340f)
     .uni (ModSource::Macro2,    Param::impactBrightness, 0.320f)
     .uni (ModSource::Macro2,    Param::spaceTone,        0.240f)
     .uni (ModSource::Macro3,    Param::spaceMix,         0.300f)
     .uni (ModSource::Macro4,    Param::evolveMagnet,     0.300f)
     .uni (ModSource::Macro4,    Param::fractureAmount,   0.260f);
    sharedMacros (r, Param::fractureDecay, Param::impactRandom);
    r.commit (s);
}});

//------------------------------------------------- filaments, water and dust

manager.addFactory ({ "Tungsten Thread", "DRONE", { "metallic", "bright", "morphing", "air", "wide" }, [] (PatchState& s)
{
    // A filament glowing at the top of the register. CRUSH is the event: a
    // looped envelope quantises the node frequencies onto a coarse grid and
    // then releases them, so the thread steps between a handful of pitches and
    // a continuous line and back, four or five times a minute.
    wave (s, 4 /* METALLIC */, 0.52f, 0.30f, 0.0f, 3, 0.06f, 0.80f, 1, 0.82f);
    amp (s, 1.30f, 3.0f, 0.90f, 3.5f, 0.58f);
    shape (s, 0.46f, 0.68f, 0.18f, 0.72f, 0.80f, 0.22f);
    material (s, MaterialType::Metal, MaterialType::Crystal, 0.52f);
    topology (s, 5 /* STAR */, 0.30f, 0.32f, 1103);
    matter (s, 0.94f, 0.56f, 0.16f, 0.82f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.44f, 0.20f, 0.30f, 0.12f, 0.40f);
    fracture (s, 0 /* SPECTRAL */, 0.34f, 0.30f, 0.72f, 0.30f, 0.44f, 0.58f, 0.76f, 0.66f, 0.35f,
              2 /* 32 */, 1 /* 1/2 */, 8, 0.0f, 2 /* PINGPONG */, 0.80f, 0.30f, 1109,
              FractureShape { 32, 0.14f, 0.66f, 0.30f, 0.60f, 0.55f, 0.82f, 0.40f, 0.98f,
                              1.0f, 0.9f, 0.80f, 0.9f, kOctaveTerrace, "XHXLXHXX", nullptr });
    space (s, SpacePresets::Shimmer, 0.42f, 0.66f, 0.72f, 0.38f);

    env (s, 2, 11.0f, 14.0f, 0.40f, 6.0f, 0.55f, true);
    lfo (s, 1, 0.034f, 1 /* TRIANGLE */, 1.0f, false, 4.0f);
    lfo (s, 2, 0.12f, 5 /* SMOOTH RANDOM */, 1.0f, false, 2.0f);
    macros (s, 0.45f, 0.48f, 0.48f, 0.55f);

    Routings r;
    r.bi  (ModSource::Env2,      Param::evolveCrush,      0.360f)
     .bi  (ModSource::Env2,      Param::wavePosition,     0.200f)
     .bi  (ModSource::LFO1,      Param::shapeTension,     0.160f)
     .bi  (ModSource::LFO2,      Param::shapeBlend,       0.200f)
     .uni (ModSource::Velocity,  Param::shapeExcite,      0.220f)
     .bi  (ModSource::KeyTrack,  Param::shapeDecay,      -0.200f)
     .bi  (ModSource::NoteRandom, Param::waveDetune,      0.100f)
     .uni (ModSource::Macro1,    Param::evolveMotion,     0.400f)
     .uni (ModSource::Macro2,    Param::waveMorph,        0.300f)
     .uni (ModSource::Macro2,    Param::fractureTone,     0.240f)
     .uni (ModSource::Macro3,    Param::spaceMix,         0.300f)
     .uni (ModSource::Macro4,    Param::evolveCrush,      0.320f)
     .uni (ModSource::Macro4,    Param::shapeTension,     0.240f);
    sharedMacros (r, Param::fractureDecay, Param::evolveSpeed);
    r.commit (s);
}});

manager.addFactory ({ "Drowned Nave", "DRONE", { "organic", "soft", "wide", "breathing", "distant" }, [] (PatchState& s)
{
    // A flooded church. A hand rubbing a wet stone edge, layered with pink
    // noise for the water in the air, resonating in a random graph that is more
    // liquid than solid. The room breathes around it on one long cycle while a
    // second walks the ring time of the material, so the building fills and
    // drains; the whole thing sits far back.
    dust (s, 1 /* PINK */, 0.44f, 0.42f, 0.36f, 0.40f, 0.68f, 0.80f, 1117, 0.34f);
    gesture (s, 2 /* RUB */, 0.52f, 0.26f, 0.40f, 0.36f, 0.42f, 0.36f, 0.80f);
    layerSources (s, 1 /* DUST */, 4 /* GESTURE */);
    amp (s, 1.90f, 3.5f, 0.92f, 4.5f, 0.62f);
    shape (s, 0.56f, 0.38f, 0.60f, 0.44f, 0.78f, 0.42f);
    material (s, MaterialType::Liquid, MaterialType::Void, 0.46f);
    topology (s, 4 /* RANDOM */, 0.56f, 0.52f, 1123);
    matter (s, 0.96f, 0.50f, 0.16f, 0.88f);
    evolve (s, 0.0f, 0.28f, 0.0f, 0.22f, 0.56f, 0.24f, 0.0f, 0.07f, 0.42f);
    set (s, Param::evolveMagnetTarget, 3 /* MINOR */);
    space (s, SpacePresets::Dream, 0.52f, 0.82f, 0.50f, 0.46f);

    env (s, 2, 15.0f, 19.0f, 0.45f, 8.0f, 0.60f, true);
    lfo (s, 1, 0.030f, 0 /* SINE */, 1.0f, false, 5.0f);
    lfo (s, 2, 0.083f, 5 /* SMOOTH RANDOM */, 1.0f, false, 3.0f);
    chaos (s, 1, 1 /* BROWNIAN */, 0.055f, 0.50f, 0.86f, 0.5f, 1129);
    macros (s, 0.45f, 0.34f, 0.58f, 0.50f);

    Routings r;
    r.bi  (ModSource::Env2,      Param::shapeDecay,       0.280f)
     .uni (ModSource::Env2,      Param::evolveMagnet,     0.260f)
     .bi  (ModSource::LFO1,      Param::spaceSize,        0.260f)
     .bi  (ModSource::LFO2,      Param::gesturePosition,  0.200f)
     .bi  (ModSource::Chaos1,    Param::shapeBlend,       0.300f)
     .uni (ModSource::Velocity,  Param::gesturePressure,  0.240f)
     .bi  (ModSource::KeyTrack,  Param::dustColor,        0.220f)
     .bi  (ModSource::NoteRandom, Param::gestureRoughness, 0.120f)
     .uni (ModSource::Macro1,    Param::evolveMotion,     0.400f)
     .uni (ModSource::Macro1,    Param::gestureMotion,    0.300f)
     .uni (ModSource::Macro2,    Param::gestureBandwidth, 0.300f)
     .uni (ModSource::Macro2,    Param::dustColor,        0.240f)
     .uni (ModSource::Macro3,    Param::spaceMix,         0.320f)
     .uni (ModSource::Macro4,    Param::evolveMelt,       0.300f)
     .uni (ModSource::Macro4,    Param::dustLevel,        0.240f);
    sharedMacros (r, Param::spaceSize, Param::chaos1Depth);
    r.commit (s);
}});

manager.addFactory ({ "Moth Static", "DRONE", { "granular", "cold", "unstable", "high", "close" }, [] (PatchState& s)
{
    // Small dry wings against a lamp. CRACKLE fires irregular grains into a
    // random graph of dry organic matter and crystal, and a SPECTRAL Fracture
    // with long feedback keeps catching them and letting them go, so the
    // flutter is never the same twice and never quite stops.
    dust (s, 5 /* CRACKLE */, 0.82f, 0.66f, 0.44f, 0.62f, 0.66f, 0.75f, 1151, 1.0f);
    amp (s, 0.60f, 2.0f, 0.95f, 2.4f, 0.52f);
    set (s, Param::masterGain, 2.0f);
    shape (s, 0.58f, 0.56f, 0.22f, 0.68f, 0.66f, 0.50f);
    material (s, MaterialType::Organic, MaterialType::Crystal, 0.52f);
    topology (s, 4 /* RANDOM */, 0.42f, 0.60f, 1153);
    matter (s, 1.0f, 0.64f, 0.20f, 0.78f);
    evolve (s, 0.0f, 0.0f, 0.18f, 0.0f, 0.40f, 0.42f, 0.0f, 0.34f, 0.52f);
    set (s, Param::evolveScatterSeed, 1163);
    fracture (s, 0 /* SPECTRAL */, 0.42f, 0.38f, 0.78f, 0.34f, 0.52f, 0.35f, 0.66f, 0.72f, 0.30f,
              2 /* 32 */, 3 /* 1/8 */, 8, 0.0f, 3 /* RANDOM */, 0.70f, 0.45f, 1171,
              FractureShape { 32, 0.03f, 0.30f, 0.25f, 0.60f, 0.45f, 0.72f, 0.45f, 1.0f,
                              1.0f, 1.0f, 0.85f, 0.85f, kMinorTerrace, "XHXoXHoX", nullptr });
    space (s, SpacePresets::Dust, 0.38f, 0.44f, 0.66f, 0.36f);

    env (s, 2, 8.0f, 11.0f, 0.40f, 5.0f, 0.50f, true);
    lfo (s, 1, 0.17f, 5 /* SMOOTH RANDOM */, 1.0f, false, 1.5f);
    chaos (s, 1, 2 /* LOGISTIC */, 0.42f, 0.55f, 0.58f, 0.5f, 1181);
    chaos (s, 2, 1 /* BROWNIAN */, 0.06f, 0.50f, 0.85f, 0.5f, 1187);
    macros (s, 0.55f, 0.50f, 0.36f, 0.55f);

    Routings r;
    r.bi  (ModSource::Chaos1,    Param::dustDensity,      0.440f)
     .bi  (ModSource::Chaos1,    Param::dustJitter,       0.300f)
     .bi  (ModSource::Chaos2,    Param::shapeBlend,       0.320f)
     .bi  (ModSource::Env2,      Param::shapeDistribution, 0.280f)
     .bi  (ModSource::LFO1,      Param::dustColor,        0.200f)
     .uni (ModSource::Velocity,  Param::dustDensity,      0.240f)
     .bi  (ModSource::KeyTrack,  Param::dustGrain,       -0.200f)
     .bi  (ModSource::NoteRandom, Param::dustSpread,      0.140f)
     .uni (ModSource::Macro1,    Param::evolveMotion,     0.420f)
     .uni (ModSource::Macro2,    Param::dustColor,        0.320f)
     .uni (ModSource::Macro2,    Param::fractureTone,     0.240f)
     .uni (ModSource::Macro3,    Param::spaceMix,         0.300f)
     .uni (ModSource::Macro4,    Param::evolveTear,       0.300f)
     .uni (ModSource::Macro4,    Param::fractureFeedback, 0.240f);
    sharedMacros (r, Param::fractureDecay, Param::chaos1Rate);
    r.commit (s);
}});

manager.addFactory ({ "Bellows Vault", "DRONE", { "warm", "hollow", "pulsing", "sub", "huge" }, [] (PatchState& s)
{
    // A church organ two octaves below where anyone sings, with the bellows
    // still working: a harmonic rank layered with brown noise for the wind
    // leaking round the pallets, and a slow pump on the excitation so the
    // whole vault fills and empties instead of just sitting there.
    wave (s, 1 /* HARMONIC */, 0.22f, 0.14f, 0.0f, 4, 0.06f, 0.55f, -2, 0.86f);
    dust (s, 2 /* BROWN */, 0.40f, 0.24f, 0.26f, 0.30f, 0.50f, 0.55f, 1193, 0.26f);
    layerSources (s, 0 /* WAVE */, 1 /* DUST */);
    amp (s, 2.00f, 4.0f, 0.94f, 5.0f, 0.64f);
    set (s, Param::masterGain, -2.5f);
    shape (s, 0.34f, 0.24f, 0.84f, 0.34f, 0.80f, 0.22f);
    material (s, MaterialType::Membrane, MaterialType::Void, 0.48f);
    topology (s, 2 /* CLUSTERS */, 0.44f, 0.34f, 1201);
    matter (s, 0.94f, 0.46f, 0.14f, 0.72f);
    evolve (s, 0.0f, 0.12f, 0.0f, 0.32f, 0.58f, 0.12f, 0.0f, 0.06f, 0.30f);
    set (s, Param::evolveMagnetTarget, 0 /* OCTAVE */);
    space (s, SpacePresets::Void, 0.46f, 0.88f, 0.34f, 0.44f);

    env (s, 2, 7.0f, 9.0f, 0.45f, 5.0f, 0.60f, true);
    env (s, 3, 19.0f, 23.0f, 0.50f, 8.0f, 0.55f, true);
    lfo (s, 1, 0.026f, 0 /* SINE */, 1.0f, false, 5.0f);
    macros (s, 0.38f, 0.30f, 0.52f, 0.48f);

    Routings r;
    r.bi  (ModSource::Env2,      Param::shapeExcite,      0.280f)
     .bi  (ModSource::Env2,      Param::dustDensity,      0.240f)
     .bi  (ModSource::Env3,      Param::shapeMass,        0.240f)
     .uni (ModSource::Env3,      Param::evolveMagnet,     0.240f)
     .bi  (ModSource::LFO1,      Param::shapeBlend,       0.180f)
     .uni (ModSource::Velocity,  Param::shapeExcite,      0.220f)
     .bi  (ModSource::KeyTrack,  Param::shapeMass,       -0.260f)
     .bi  (ModSource::NoteRandom, Param::waveDetune,      0.100f)
     .uni (ModSource::Macro1,    Param::evolveMotion,     0.360f)
     .uni (ModSource::Macro2,    Param::wavePosition,     0.300f)
     .uni (ModSource::Macro2,    Param::spaceTone,        0.240f)
     .uni (ModSource::Macro3,    Param::spaceMix,         0.320f)
     .uni (ModSource::Macro4,    Param::dustLevel,        0.300f)
     .uni (ModSource::Macro4,    Param::shapeMass,        0.240f);
    sharedMacros (r, Param::spaceSize, Param::dustJitter);
    r.commit (s);
}});

manager.addFactory ({ "Sympathy Wires", "DRONE", { "resonant", "wooden", "drift", "roomy", "mid" }, [] (PatchState& s)
{
    // A bank of sympathetic strings behind a soundboard. A quiet bowed tone
    // keeps them all alive while a mallet touches one every few seconds, and a
    // long loop walks the material's ring time, so the wires hand the note
    // between them: the short ones drop out, the long ones take over, and the
    // chord reshuffles itself without anybody playing a new note.
    wave (s, 0 /* BASIC */, 0.28f, 0.16f, 0.0f, 3, 0.04f, 0.45f, 0, 0.30f);
    impact (s, 2 /* PLUCK */, 0.40f, 0.52f, 0.12f, 0.70f, 0.35f, 0.60f, 0.16f, 0.85f);
    layerSources (s, 0 /* WAVE */, 2 /* IMPACT */);
    amp (s, 1.20f, 3.0f, 0.90f, 3.5f, 0.58f);
    set (s, Param::masterGain, -6.0f);
    shape (s, 0.40f, 0.12f, 0.36f, 0.66f, 0.84f, 0.20f);
    material (s, MaterialType::String, MaterialType::Wood, 0.42f);
    topology (s, 0 /* CHAIN */, 0.34f, 0.44f, 1213);
    matter (s, 0.96f, 0.54f, 0.34f, 0.62f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.28f, 0.50f, 0.18f, 0.0f, 0.09f, 0.34f);
    set (s, Param::evolveMagnetTarget, 5 /* SCALE */);
    fracture (s, 1 /* RHYTHMIC */, 0.30f, 0.26f, 0.60f, 0.55f, 0.34f, 0.40f, 0.72f, 0.52f, 0.28f,
              1 /* 16 */, 2 /* 1/4 */, 8, 0.12f, 2 /* PINGPONG */, 0.55f, 0.35f, 1217,
              FractureShape { 16, 0.10f, 0.50f, 0.22f, 0.50f, 0.48f, 0.74f, 0.30f, 0.90f,
                              1.0f, 0.9f, 0.60f, 0.75f, kFifthTerrace, "X.XLX.XH", nullptr });
    space (s, SpacePresets::Chamber, 0.40f, 0.62f, 0.56f, 0.36f);

    env (s, 2, 17.0f, 21.0f, 0.45f, 8.0f, 0.55f, true);
    lfo (s, 1, 0.032f, 1 /* TRIANGLE */, 1.0f, false, 4.0f);
    chaos (s, 1, 0 /* WALK */, 0.07f, 0.48f, 0.80f, 0.5f, 1223);
    macros (s, 0.42f, 0.42f, 0.46f, 0.50f);

    Routings r;
    r.bi  (ModSource::Env2,      Param::shapeDecay,       0.300f)
     .bi  (ModSource::Env2,      Param::shapeBlend,       0.460f)
     .bi  (ModSource::Env2,      Param::shapeDistribution, 0.220f)
     .bi  (ModSource::LFO1,      Param::shapeTension,     0.180f)
     .bi  (ModSource::LFO1,      Param::shapeForm,        0.140f)
     .bi  (ModSource::Chaos1,    Param::impactRate,       0.300f)
     .bi  (ModSource::Chaos1,    Param::impactBrightness, 0.320f)
     .bi  (ModSource::Chaos1,    Param::shapeDecay,       0.180f)
     .uni (ModSource::Velocity,  Param::impactLevel,      0.300f)
     .bi  (ModSource::KeyTrack,  Param::shapeDecay,      -0.220f)
     .bi  (ModSource::NoteRandom, Param::impactRandom,    0.140f)
     .uni (ModSource::Macro1,    Param::evolveMotion,     0.380f)
     .uni (ModSource::Macro1,    Param::impactRate,       0.240f)
     .uni (ModSource::Macro2,    Param::impactBrightness, 0.300f)
     .uni (ModSource::Macro2,    Param::wavePosition,     0.240f)
     .uni (ModSource::Macro3,    Param::spaceMix,         0.300f)
     .uni (ModSource::Macro4,    Param::shapeMass,        0.280f)
     .uni (ModSource::Macro4,    Param::evolveMagnet,     0.260f);
    sharedMacros (r, Param::fractureDecay, Param::impactRandom);
    r.commit (s);
}});

manager.addFactory ({ "Ember Bed", "DRONE", { "warm", "dirty", "granular", "chaotic", "low" }, [] (PatchState& s)
{
    // A bed of coals still working. The NoiseBurst built-in read granularly
    // gives the settling, DUST in CRACKLE mode gives the spitting, and the
    // material underneath is CUSTOM — a plain harmonic body whose FORM is the
    // only thing deciding its structure, so the chaos generator walking FORM
    // rebuilds what the fire is burning in every twenty seconds or so.
    dust (s, 5 /* CRACKLE */, 0.46f, 0.34f, 0.44f, 0.66f, 0.60f, 0.70f, 1229, 0.42f);
    sample (s, BuiltInSamples::Kind::NoiseBurst, 3 /* GRANULAR */, 0.05f, 0.80f, 0.52f, 0.60f, 45, 0.80f);
    layerSources (s, 1 /* DUST */, 3 /* SAMPLE */);
    amp (s, 1.40f, 3.0f, 0.90f, 3.5f, 0.58f);
    shape (s, 0.50f, 0.30f, 0.66f, 0.42f, 0.74f, 0.48f);
    material (s, MaterialType::Custom, MaterialType::Organic, 0.44f);
    topology (s, 3 /* LATTICE */, 0.48f, 0.48f, 1231);
    matter (s, 0.96f, 0.58f, 0.18f, 0.76f);
    evolve (s, 0.0f, 0.24f, 0.0f, 0.0f, 0.54f, 0.32f, 0.0f, 0.13f, 0.46f);
    set (s, Param::evolveScatterSeed, 1237);
    space (s, SpacePresets::Nebula, 0.40f, 0.58f, 0.44f, 0.40f);

    env (s, 2, 12.0f, 15.0f, 0.45f, 7.0f, 0.55f, true);
    lfo (s, 1, 0.061f, 5 /* SMOOTH RANDOM */, 1.0f, false, 3.0f);
    chaos (s, 1, 1 /* BROWNIAN */, 0.05f, 0.60f, 0.88f, 0.5f, 1249);
    chaos (s, 2, 2 /* LOGISTIC */, 0.24f, 0.45f, 0.62f, 0.5f, 1259);
    macros (s, 0.50f, 0.36f, 0.42f, 0.55f);

    Routings r;
    r.bi  (ModSource::Chaos1,    Param::shapeForm,        0.420f)
     .bi  (ModSource::Chaos2,    Param::dustDensity,      0.280f)
     .bi  (ModSource::Env2,      Param::sampleGrain,      0.280f)
     .uni (ModSource::Env2,      Param::evolveMelt,       0.240f)
     .bi  (ModSource::LFO1,      Param::sampleStart,      0.220f)
     .uni (ModSource::Velocity,  Param::dustDensity,      0.240f)
     .bi  (ModSource::KeyTrack,  Param::dustColor,        0.220f)
     .bi  (ModSource::NoteRandom, Param::sampleSpread,    0.140f)
     .uni (ModSource::Macro1,    Param::evolveMotion,     0.400f)
     .uni (ModSource::Macro2,    Param::dustColor,        0.320f)
     .uni (ModSource::Macro2,    Param::shapeExcite,      0.240f)
     .uni (ModSource::Macro3,    Param::spaceMix,         0.300f)
     .uni (ModSource::Macro4,    Param::sampleGrain,      0.320f)
     .uni (ModSource::Macro4,    Param::shapeSurface,     0.260f);
    sharedMacros (r, Param::spaceSize, Param::chaos1Depth);
    r.commit (s);
}});
}

} // namespace am::FactoryContent
