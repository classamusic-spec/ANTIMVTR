#include "presets/FactoryBuilders.h"

namespace am::FactoryContent
{

void registerCinematic (PresetManager& manager)
{
//==========================================================================
// CINEMATIC — scoring material: one note tells a story
//==========================================================================

manager.addFactory ({ "Broken Choir", "CINEMATIC", { "cinematic", "vocal", "haunting", "wide" }, [] (PatchState& s)
{
    wave (s, 2 /* FORMANT */, 0.30f, 0.45f, 0.08f, 5, 0.24f, 0.90f);
    amp (s, 0.70f, 2.20f, 0.80f, 3.0f, 0.55f);
    shape (s, 0.64f, 0.22f, 0.46f, 0.54f, 0.70f, 0.26f);
    material (s, MaterialType::Organic, MaterialType::Membrane, 0.40f);
    topology (s, 2 /* CLUSTERS */, 0.48f, 0.55f, 1103);
    matter (s, 0.80f, 0.62f, 0.20f, 0.88f);
    evolve (s, 0.0f, 0.22f, 0.26f, 0.30f, 0.46f, 0.22f, 0.0f, 0.18f, 0.40f);
    set (s, Param::evolveMagnetTarget, 3 /* MINOR */);
    fracture (s, 0 /* SPECTRAL */, 0.40f, 0.45f, 0.55f, 0.40f, 0.32f, 0.50f, 0.60f, 0.52f, 0.30f,
              1 /* 16 */, 2 /* 1/4 */, 8, 0.0f, 0 /* FORWARD */, 1.0f, 0.20f, 2521,
              FractureShape { 16, 0.15f, 0.65f, 0.20f, 0.50f, 0.50f, 0.78f, 0.30f, 0.90f,
                              1.0f, 0.85f, 0.70f, 1.0f, kMinorTerrace, "XXLLHHXX", nullptr });
    space (s, SpacePresets::Dream, 0.52f, 0.75f, 0.58f, 0.40f);

    lfo (s, 1, 0.12f, 0 /* SINE */, 1.0f, false, 2.0f);
    lfo (s, 2, 0.08f, 5 /* SMOOTH RANDOM */, 1.0f, false);
    env (s, 2, 2.80f, 6.0f, 0.70f, 5.0f, 0.6f);
    macros (s, 0.45f, 0.40f, 0.50f, 0.45f);

    Routings r;
    r.bi  (ModSource::LFO1,   Param::waveMorph,      0.120f)
     .bi  (ModSource::LFO2,   Param::wavePosition,   0.100f)
     .uni (ModSource::Env2,   Param::evolveTear,     0.220f)
     .uni (ModSource::Velocity, Param::waveScan,     0.180f)
     .bi  (ModSource::KeyTrack, Param::shapeForm,   -0.080f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.350f)
     .uni (ModSource::Macro2, Param::wavePosition,   0.280f)
     .uni (ModSource::Macro2, Param::spaceTone,      0.200f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.280f)
     .uni (ModSource::Macro4, Param::fractureAmount, 0.300f)
     .uni (ModSource::Macro4, Param::evolveTear,     0.250f);
    sharedMacros (r, Param::fractureDecay, Param::waveScan);
    r.commit (s);
}});

manager.addFactory ({ "Event Horizon", "CINEMATIC", { "cinematic", "riser", "granular", "huge" }, [] (PatchState& s)
{
    sample (s, BuiltInSamples::Kind::StoneDrop, 3 /* GRANULAR */, 0.0f, 0.90f, 0.45f, 0.85f);
    amp (s, 1.80f, 3.0f, 0.90f, 3.50f, 0.7f);
    shape (s, 0.70f, 0.32f, 0.62f, 0.48f, 0.78f, 0.30f);
    material (s, MaterialType::Void, MaterialType::Membrane, 0.38f);
    topology (s, 2 /* CLUSTERS */, 0.58f, 0.45f, 1181);
    matter (s, 0.90f, 0.58f, 0.18f, 0.92f);
    evolve (s, 0.34f, 0.18f, 0.0f, 0.24f, 0.40f, 0.24f, 0.0f, 0.14f, 0.48f);
    set (s, Param::evolveBendPivot, 0.30f);
    set (s, Param::evolveBendRange, 0.60f);
    space (s, SpacePresets::Void, 0.56f, 0.92f, 0.42f, 0.50f);

    env (s, 2, 5.0f, 8.0f, 0.85f, 6.0f, 0.75f);
    lfo (s, 1, 0.05f, 2 /* SAW */, 1.0f, true, 2.0f);
    lfo (s, 2, 0.11f, 0 /* SINE */, 1.0f, false, 3.0f);
    chaos (s, 1, 1 /* BROWNIAN */, 0.09f, 0.50f, 0.80f, 0.5f, 2617);
    macros (s, 0.55f, 0.35f, 0.55f, 0.50f);

    Routings r;
    r.uni (ModSource::Env2,   Param::evolveBend,     0.300f)
     .uni (ModSource::Env2,   Param::shapeDensity,   0.180f)
     .bi  (ModSource::LFO1,   Param::sampleGrain,    0.140f)
     .bi  (ModSource::LFO2,   Param::shapeMass,      0.070f)
     .bi  (ModSource::Chaos1, Param::sampleSpread,   0.150f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.400f)
     .uni (ModSource::Macro1, Param::evolveSpeed,    0.200f)
     .uni (ModSource::Macro2, Param::shapeExcite,    0.300f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.250f)
     .uni (ModSource::Macro3, Param::spaceSize,      0.150f)
     .uni (ModSource::Macro4, Param::evolveBend,     0.300f)
     .uni (ModSource::Macro4, Param::sampleGrain,    0.250f);
    sharedMacros (r, Param::spaceSize, Param::sampleSpread);
    r.commit (s);
}});

manager.addFactory ({ "Iron Lullaby", "CINEMATIC", { "cinematic", "music box", "fragile", "metal" }, [] (PatchState& s)
{
    sample (s, BuiltInSamples::Kind::MetalPing, 0 /* ONE SHOT */, 0.0f, 0.55f, 0.25f, 0.35f);
    amp (s, 0.004f, 1.80f, 0.15f, 1.60f, 0.35f);
    shape (s, 0.40f, 0.88f, 0.26f, 0.70f, 0.74f, 0.16f);
    material (s, MaterialType::Crystal, MaterialType::Metal, 0.32f);
    topology (s, 5 /* STAR */, 0.30f, 0.60f, 1259);
    matter (s, 0.94f, 0.60f, 0.66f, 0.72f);
    evolve (s, 0.0f, 0.14f, 0.0f, 0.62f, 0.44f, 0.12f, 0.0f, 0.20f, 0.24f);
    set (s, Param::evolveMagnetTarget, 3 /* MINOR */);
    space (s, SpacePresets::Dream, 0.48f, 0.68f, 0.60f, 0.35f);

    env (s, 1, 0.002f, 1.0f, 0.0f, 0.90f, 0.3f);
    env (s, 2, 1.20f, 4.0f, 0.50f, 4.0f);
    lfo (s, 1, 0.17f, 0 /* SINE */, 1.0f, true, 1.0f);
    macros (s, 0.30f, 0.40f, 0.48f, 0.45f);

    Routings r;
    r.uni (ModSource::Env1,   Param::shapeSurface,   0.180f)
     .uni (ModSource::Env2,   Param::evolveMelt,     0.180f)
     .bi  (ModSource::LFO1,   Param::shapeTension,   0.035f)
     .uni (ModSource::Velocity, Param::sampleStart,  0.150f)
     .bi  (ModSource::KeyTrack, Param::shapeDecay,  -0.180f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.300f)
     .uni (ModSource::Macro2, Param::shapeExcite,    0.300f)
     .uni (ModSource::Macro2, Param::samplePitch,    0.100f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.300f)
     .uni (ModSource::Macro4, Param::evolveMagnet,   0.300f)
     .uni (ModSource::Macro4, Param::shapeTension,   0.180f);
    sharedMacros (r, Param::spaceSize, Param::evolveMelt);
    r.commit (s);
}});

manager.addFactory ({ "Slow Verdict", "CINEMATIC", { "dark", "cold", "static", "huge", "low" }, [] (PatchState& s)
{
    // The bed under bad news. Almost nothing moves: a low void object, a very
    // long swell and a room the size of a hangar. It is meant to sit under
    // dialogue for a minute at a time without ever asking to be listened to.
    wave (s, 0 /* BASIC */, 0.16f, 0.10f, 0.0f, 3, 0.10f, 0.55f, -1, 0.70f);
    set (s, Param::masterGain, -5.0f);
    amp (s, 1.10f, 6.0f, 0.90f, 4.0f, 0.60f);
    shape (s, 0.40f, 0.20f, 0.70f, 0.30f, 0.84f, 0.10f);
    material (s, MaterialType::Void, MaterialType::Membrane, 0.30f);
    topology (s, 0 /* CHAIN */, 0.30f, 0.34f, 4001);
    matter (s, 0.90f, 0.44f, 0.14f, 0.62f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.56f, 0.05f, 0.0f, 0.08f, 0.14f);
    space (s, SpacePresets::Void, 0.52f, 0.90f, 0.34f, 0.44f);

    env (s, 2, 3.0f, 8.0f, 0.80f, 6.0f, 0.6f);
    lfo (s, 1, 0.05f, 0 /* SINE */, 1.0f, false, 3.0f);
    macros (s, 0.25f, 0.30f, 0.55f, 0.40f);

    Routings r;
    r.bi  (ModSource::LFO1,      Param::shapeMass,       0.060f)
     .uni (ModSource::Env2,      Param::shapeDensity,    0.140f)
     .uni (ModSource::Velocity,  Param::shapeExcite,     0.220f)
     .bi  (ModSource::KeyTrack,  Param::shapeDecay,     -0.120f)
     .uni (ModSource::Macro1,    Param::evolveMotion,    0.260f)
     .uni (ModSource::Macro2,    Param::wavePosition,    0.300f)
     .uni (ModSource::Macro2,    Param::spaceTone,       0.240f)
     .uni (ModSource::Macro3,    Param::spaceMix,        0.280f)
     .uni (ModSource::Macro3,    Param::spaceSize,       0.180f)
     .uni (ModSource::Macro4,    Param::shapeMass,       0.300f)
     .uni (ModSource::Macro4,    Param::evolveGravity,   0.200f);
    sharedMacros (r, Param::spaceFeedback, Param::shapeSurface);
    r.commit (s);
}});

manager.addFactory ({ "Under Ice", "CINEMATIC", { "cold", "dark", "bowed", "breathing", "distant" }, [] (PatchState& s)
{
    // Dread with something alive in it. A slow bow on a liquid membrane, far
    // back in a dark room, breathing once every eight seconds. Plays as a bed
    // but has enough motion to survive a two minute hold under picture.
    gesture (s, 0 /* BOW */, 0.34f, 0.22f, 0.30f, 0.52f, 0.30f, 0.40f, 0.80f);
    amp (s, 0.90f, 6.0f, 0.88f, 3.20f, 0.60f);
    shape (s, 0.56f, 0.28f, 0.56f, 0.40f, 0.76f, 0.22f);
    material (s, MaterialType::Liquid, MaterialType::Membrane, 0.40f);
    topology (s, 2 /* CLUSTERS */, 0.44f, 0.46f, 4003);
    matter (s, 0.90f, 0.50f, 0.16f, 0.82f);
    evolve (s, 0.0f, 0.08f, 0.0f, 0.0f, 0.54f, 0.10f, 0.0f, 0.10f, 0.22f);
    space (s, SpacePresets::Nebula, 0.54f, 0.82f, 0.38f, 0.42f);

    env (s, 2, 2.60f, 8.0f, 0.85f, 5.0f, 0.6f);
    lfo (s, 1, 0.12f, 0 /* SINE */, 1.0f, false, 2.0f);
    lfo (s, 2, 0.06f, 5 /* SMOOTH RANDOM */, 1.0f, false, 2.0f);
    macros (s, 0.35f, 0.30f, 0.55f, 0.45f);

    Routings r;
    r.bi  (ModSource::LFO1,      Param::gesturePressure, 0.100f)
     .bi  (ModSource::LFO2,      Param::gesturePosition, 0.120f)
     .uni (ModSource::Env2,      Param::gestureBandwidth, 0.200f)
     .uni (ModSource::Velocity,  Param::gesturePressure, 0.240f)
     .bi  (ModSource::KeyTrack,  Param::gestureSpeed,    0.160f)
     .uni (ModSource::Macro1,    Param::gestureMotion,   0.320f)
     .uni (ModSource::Macro2,    Param::gestureBandwidth, 0.300f)
     .uni (ModSource::Macro3,    Param::spaceMix,        0.300f)
     .uni (ModSource::Macro4,    Param::shapeMass,       0.280f)
     .uni (ModSource::Macro4,    Param::evolveMelt,      0.200f);
    sharedMacros (r, Param::spaceSize, Param::gestureRoughness);
    r.commit (s);
}});

manager.addFactory ({ "Black Harvest", "CINEMATIC", { "dirty", "dark", "harsh", "low", "roomy" }, [] (PatchState& s)
{
    // An industrial bed with teeth: brown weather driving a metal object in a
    // saturated room, with CRUSH holding the partials on a coarse grid so the
    // whole thing reads as machinery rather than as a note.
    dust (s, 2 /* BROWN */, 0.66f, 0.22f, 0.40f, 0.30f, 0.46f, 0.54f, 4007, 0.80f);
    amp (s, 0.70f, 6.0f, 0.86f, 2.40f, 0.55f);
    shape (s, 0.56f, 0.34f, 0.60f, 0.42f, 0.70f, 0.34f);
    material (s, MaterialType::Metal, MaterialType::Wood, 0.52f);
    topology (s, 4 /* RANDOM */, 0.50f, 0.42f, 4013);
    matter (s, 0.88f, 0.50f, 0.20f, 0.66f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.56f, 0.10f, 0.44f, 0.16f, 0.26f);
    space (s, SpacePresets::Machine, 0.38f, 0.62f, 0.42f, 0.34f);

    env (s, 2, 2.20f, 8.0f, 0.85f, 4.0f, 0.55f);
    lfo (s, 1, 0.16f, 5 /* SMOOTH RANDOM */, 1.0f, true, 1.5f);
    chaos (s, 1, 1 /* BROWNIAN */, 0.14f, 0.40f, 0.80f, 0.5f, 4019);
    macros (s, 0.35f, 0.30f, 0.45f, 0.50f);

    Routings r;
    r.bi  (ModSource::LFO1,      Param::dustColor,       0.120f)
     .bi  (ModSource::Chaos1,    Param::shapeSurface,    0.100f)
     .uni (ModSource::Env2,      Param::evolveCrush,     0.440f)
     .uni (ModSource::Velocity,  Param::dustDensity,     0.240f)
     .bi  (ModSource::KeyTrack,  Param::dustColor,       0.180f)
     .uni (ModSource::Macro1,    Param::evolveMotion,    0.300f)
     .uni (ModSource::Macro2,    Param::dustColor,       0.320f)
     .uni (ModSource::Macro3,    Param::spaceMix,        0.300f)
     .uni (ModSource::Macro4,    Param::evolveCrush,     0.320f)
     .uni (ModSource::Macro4,    Param::spaceDistDrive,  0.240f);
    sharedMacros (r, Param::spaceSize, Param::dustJitter);
    r.commit (s);
}});

manager.addFactory ({ "Rosin Air", "CINEMATIC", { "warm", "organic", "bowed", "breathing", "roomy" }, [] (PatchState& s)
{
    // Strings-adjacent, on purpose: not a string library, but the thing a
    // section does under a scene. A wooden string body bowed slowly in a real
    // room, opening with pressure so a player can lean on it and be heard.
    gesture (s, 0 /* BOW */, 0.44f, 0.36f, 0.20f, 0.36f, 0.22f, 0.46f, 0.78f);
    amp (s, 0.80f, 6.0f, 0.86f, 2.20f, 0.55f);
    shape (s, 0.54f, 0.24f, 0.42f, 0.60f, 0.68f, 0.20f);
    material (s, MaterialType::String, MaterialType::Wood, 0.32f);
    topology (s, 0 /* CHAIN */, 0.36f, 0.40f, 4021);
    matter (s, 0.88f, 0.54f, 0.20f, 0.60f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.50f, 0.06f, 0.0f, 0.12f, 0.20f);
    space (s, SpacePresets::Chamber, 0.40f, 0.58f, 0.52f, 0.30f);

    env (s, 2, 1.80f, 8.0f, 0.80f, 4.0f, 0.6f);
    lfo (s, 1, 0.16f, 0 /* SINE */, 1.0f, false, 1.60f);
    macros (s, 0.35f, 0.40f, 0.40f, 0.45f);

    Routings r;
    r.bi  (ModSource::LFO1,      Param::gesturePressure, 0.080f)
     .uni (ModSource::Env2,      Param::gestureBandwidth, 0.220f)
     .uni (ModSource::Velocity,  Param::gesturePressure, 0.320f)
     .uni (ModSource::Velocity,  Param::shapeExcite,     0.200f)
     .bi  (ModSource::KeyTrack,  Param::gesturePosition, 0.200f)
     .uni (ModSource::Macro1,    Param::gestureMotion,   0.320f)
     .uni (ModSource::Macro2,    Param::gestureBandwidth, 0.320f)
     .uni (ModSource::Macro3,    Param::spaceMix,        0.320f)
     .uni (ModSource::Macro4,    Param::gestureRoughness, 0.280f)
     .uni (ModSource::Macro4,    Param::shapeTension,    0.200f);
    sharedMacros (r, Param::spaceSize, Param::gestureSpeed);
    r.commit (s);
}});

manager.addFactory ({ "Widow Strings", "CINEMATIC", { "cold", "dark", "bowed", "melodic", "distant" }, [] (PatchState& s)
{
    // The same family as Rosin Air and a different scene: further away, higher
    // up, and pulled onto a minor grid so held intervals read as grief rather
    // than as tension. Slower attack, no wood in it, no room to speak of.
    gesture (s, 0 /* BOW */, 0.40f, 0.30f, 0.16f, 0.60f, 0.20f, 0.34f, 0.74f);
    amp (s, 1.0f, 6.0f, 0.84f, 3.20f, 0.6f);
    shape (s, 0.48f, 0.34f, 0.34f, 0.66f, 0.74f, 0.14f);
    material (s, MaterialType::String, MaterialType::Crystal, 0.24f);
    topology (s, 1 /* RING */, 0.30f, 0.52f, 4027);
    matter (s, 0.92f, 0.52f, 0.18f, 0.80f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.44f, 0.48f, 0.06f, 0.0f, 0.10f, 0.18f);
    set (s, Param::evolveMagnetTarget, 3 /* MINOR */);
    space (s, SpacePresets::Dream, 0.52f, 0.78f, 0.50f, 0.36f);

    env (s, 2, 2.40f, 8.0f, 0.82f, 5.0f, 0.6f);
    lfo (s, 1, 0.10f, 0 /* SINE */, 1.0f, false, 2.40f);
    macros (s, 0.30f, 0.40f, 0.55f, 0.45f);

    Routings r;
    r.bi  (ModSource::LFO1,      Param::gestureSpeed,    0.090f)
     .uni (ModSource::Env2,      Param::evolveMagnet,    0.460f)
     .uni (ModSource::Velocity,  Param::gesturePressure, 0.300f)
     .bi  (ModSource::KeyTrack,  Param::shapeDecay,     -0.180f)
     .uni (ModSource::NoteRandom, Param::gesturePosition, 0.160f)
     .uni (ModSource::Macro1,    Param::gestureMotion,   0.300f)
     .uni (ModSource::Macro2,    Param::gestureBandwidth, 0.300f)
     .uni (ModSource::Macro3,    Param::spaceMix,        0.300f)
     .uni (ModSource::Macro4,    Param::evolveMagnet,    0.300f)
     .uni (ModSource::Macro4,    Param::shapeTension,    0.220f);
    sharedMacros (r, Param::spaceSize, Param::gestureRoughness);
    r.commit (s);
}});

manager.addFactory ({ "Slow Tide", "CINEMATIC", { "warm", "organic", "breathing", "wide", "chords" }, [] (PatchState& s)
{
    // A swell that holds chords. Two layers: a soft harmonic wave for the body
    // and a breath behind it for the air, moving on one slow sine so a held
    // triad rises and falls once every six seconds without drifting in pitch.
    set (s, Param::sourceMode, 1 /* LAYER */);
    set (s, Param::dustLevel, 0.0f);
    set (s, Param::impactLevel, 0.0f);
    set (s, Param::sampleLevel, 0.0f);
    gesture (s, 3 /* BREATH */, 0.34f, 0.40f, 0.24f, 0.46f, 0.26f, 0.66f, 0.30f);
    wave (s, 1 /* HARMONIC */, 0.26f, 0.24f, 0.03f, 4, 0.16f, 0.76f, 0, 0.66f);
    amp (s, 0.95f, 6.0f, 0.88f, 2.80f, 0.6f);
    shape (s, 0.56f, 0.26f, 0.44f, 0.54f, 0.72f, 0.18f);
    material (s, MaterialType::Organic, MaterialType::String, 0.36f);
    topology (s, 2 /* CLUSTERS */, 0.42f, 0.50f, 4049);
    matter (s, 0.80f, 0.52f, 0.18f, 0.78f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.50f, 0.08f, 0.0f, 0.12f, 0.22f);
    space (s, SpacePresets::Chamber, 0.42f, 0.66f, 0.54f, 0.32f);

    env (s, 2, 2.20f, 8.0f, 0.85f, 5.0f, 0.6f);
    lfo (s, 1, 0.17f, 0 /* SINE */, 1.0f, false, 1.80f);
    macros (s, 0.35f, 0.40f, 0.45f, 0.45f);

    Routings r;
    r.bi  (ModSource::LFO1,      Param::gestureLevel,    0.140f)
     .bi  (ModSource::LFO1,      Param::waveMorph,       0.060f)
     .uni (ModSource::Env2,      Param::shapeDensity,    0.180f)
     .uni (ModSource::Velocity,  Param::waveLevel,       0.200f)
     .uni (ModSource::Velocity,  Param::gesturePressure, 0.240f)
     .bi  (ModSource::KeyTrack,  Param::wavePosition,    0.160f)
     .uni (ModSource::Macro1,    Param::gestureMotion,   0.320f)
     .uni (ModSource::Macro2,    Param::wavePosition,    0.300f)
     .uni (ModSource::Macro3,    Param::spaceMix,        0.300f)
     .uni (ModSource::Macro4,    Param::gestureLevel,    0.280f)
     .uni (ModSource::Macro4,    Param::shapeExcite,     0.200f);
    sharedMacros (r, Param::spaceSize, Param::waveDetune);
    r.commit (s);
}});

manager.addFactory ({ "Winter Carousel", "CINEMATIC", { "glassy", "cold", "plucked", "melodic", "distant" }, [] (PatchState& s)
{
    // A music box left out in the cold. Short crystal plucks on a star, tuned
    // by a light MAGNET so single notes stay sweet, with a soft pitched tail
    // behind them. Written to be played slowly with one finger.
    impact (s, 2 /* PLUCK */, 0.70f, 0.58f, 0.14f, 0.80f, 0.40f, 0.06f);
    amp (s, 0.002f, 2.20f, 0.10f, 1.60f, 0.32f);
    shape (s, 0.36f, 0.82f, 0.20f, 0.74f, 0.66f, 0.10f);
    material (s, MaterialType::Crystal, MaterialType::Metal, 0.22f);
    topology (s, 5 /* STAR */, 0.26f, 0.62f, 4051);
    matter (s, 0.96f, 0.62f, 0.66f, 0.70f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.34f, 0.46f, 0.06f, 0.0f, 0.14f, 0.16f);
    set (s, Param::evolveMagnetTarget, 0 /* OCTAVE */);
    space (s, SpacePresets::Dream, 0.46f, 0.66f, 0.58f, 0.32f);

    env (s, 1, 0.002f, 0.90f, 0.0f, 0.80f, 0.30f);
    lfo (s, 1, 0.13f, 0 /* SINE */, 1.0f, true, 1.20f);
    macros (s, 0.25f, 0.40f, 0.45f, 0.45f);

    Routings r;
    r.uni (ModSource::Env1,      Param::shapeSurface,    0.140f)
     .bi  (ModSource::LFO1,      Param::shapeTension,    0.040f)
     .uni (ModSource::Velocity,  Param::impactBrightness, 0.340f)
     .uni (ModSource::Velocity,  Param::shapeStrike,     0.200f)
     .bi  (ModSource::KeyTrack,  Param::shapeDecay,     -0.220f)
     .uni (ModSource::Macro1,    Param::evolveMotion,    0.260f)
     .uni (ModSource::Macro2,    Param::impactBrightness, 0.300f)
     .uni (ModSource::Macro3,    Param::spaceMix,        0.320f)
     .uni (ModSource::Macro4,    Param::evolveMagnet,    0.300f)
     .uni (ModSource::Macro4,    Param::shapeTension,    0.200f);
    sharedMacros (r, Param::spaceSize, Param::impactRandom);
    r.commit (s);
}});

manager.addFactory ({ "Paper Lantern", "CINEMATIC", { "wooden", "soft", "struck", "close", "dry" }, [] (PatchState& s)
{
    // The small, close cousin of the music box: a light wooden click on a
    // hollow body in a little room, with almost no tail. For the quiet scene
    // where something has to tick along under two people not talking.
    impact (s, 1 /* CLICK */, 0.38f, 0.34f, 0.10f, 0.56f, 0.55f, 0.12f);
    amp (s, 0.002f, 1.40f, 0.08f, 0.90f, 0.30f);
    shape (s, 0.42f, 0.30f, 0.36f, 0.44f, 0.44f, 0.28f);
    material (s, MaterialType::Wood, MaterialType::Organic, 0.36f);
    topology (s, 2 /* CLUSTERS */, 0.34f, 0.44f, 4057);
    matter (s, 0.94f, 0.56f, 0.44f, 0.48f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.50f, 0.08f, 0.0f, 0.12f, 0.18f);
    space (s, SpacePresets::Chamber, 0.26f, 0.34f, 0.52f, 0.22f);

    env (s, 1, 0.002f, 0.60f, 0.0f, 0.50f, 0.30f);
    lfo (s, 1, 0.22f, 5 /* SMOOTH RANDOM */, 1.0f, true, 0.80f);
    macros (s, 0.25f, 0.40f, 0.30f, 0.45f);

    Routings r;
    r.uni (ModSource::Env1,      Param::shapeSurface,    0.160f)
     .bi  (ModSource::LFO1,      Param::shapeDistribution, 0.080f)
     .uni (ModSource::Velocity,  Param::impactHardness,  0.340f)
     .uni (ModSource::Velocity,  Param::impactBrightness, 0.260f)
     .uni (ModSource::NoteRandom, Param::impactRandom,   0.200f)
     .bi  (ModSource::KeyTrack,  Param::shapeMass,      -0.160f)
     .uni (ModSource::Macro1,    Param::evolveMotion,    0.260f)
     .uni (ModSource::Macro2,    Param::impactBrightness, 0.320f)
     .uni (ModSource::Macro3,    Param::spaceMix,        0.340f)
     .uni (ModSource::Macro4,    Param::shapeSurface,    0.280f)
     .uni (ModSource::Macro4,    Param::impactHardness,  0.200f);
    sharedMacros (r, Param::spaceSize, Param::impactRandom);
    r.commit (s);
}});

manager.addFactory ({ "Glass Nursery", "CINEMATIC", { "glassy", "cold", "struck", "high", "distant" }, [] (PatchState& s)
{
    // A small brittle figure a long way off. One glass strike per note, caught
    // by a spectral terrace that answers it a fifth up, and a long soft tail.
    // Two or three notes of this is a whole cue for a child's empty room.
    sample (s, BuiltInSamples::Kind::GlassStrike, 0 /* ONE SHOT */, 0.0f, 0.42f, 0.20f, 0.40f, 72, 0.90f);
    amp (s, 0.003f, 2.60f, 0.14f, 2.20f, 0.34f);
    shape (s, 0.34f, 0.80f, 0.18f, 0.76f, 0.72f, 0.10f);
    material (s, MaterialType::Crystal, MaterialType::Void, 0.20f);
    topology (s, 5 /* STAR */, 0.24f, 0.64f, 4073);
    matter (s, 0.92f, 0.60f, 0.58f, 0.82f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.26f, 0.44f, 0.06f, 0.0f, 0.10f, 0.16f);
    set (s, Param::evolveMagnetTarget, 1 /* FIFTH */);
    fracture (s, 0 /* SPECTRAL */, 0.34f, 0.42f, 0.62f, 0.30f, 0.22f, 0.58f, 0.68f, 0.62f, 0.10f,
              1 /* 16 */, 2 /* 1/4 */, 8, 0.0f, 0 /* FORWARD */, 1.0f, 0.08f, 4079,
              FractureShape { 16, 0.22f, 0.72f, 0.16f, 0.44f, 0.52f, 0.80f, 0.34f, 0.92f,
                              0.85f, 0.60f, 0.74f, 1.0f, kFifthTerrace, "XXXHXXXH", nullptr });
    space (s, SpacePresets::Dream, 0.54f, 0.80f, 0.58f, 0.36f);

    env (s, 1, 0.002f, 1.20f, 0.0f, 1.0f, 0.30f);
    lfo (s, 1, 0.09f, 0 /* SINE */, 1.0f, true, 1.50f);
    macros (s, 0.30f, 0.45f, 0.55f, 0.45f);

    Routings r;
    r.uni (ModSource::Env1,      Param::shapeSurface,    0.140f)
     .bi  (ModSource::LFO1,      Param::fractureDelay,   0.060f)
     .uni (ModSource::Velocity,  Param::sampleLevel,     0.240f)
     .uni (ModSource::Velocity,  Param::fractureAmount,  0.200f)
     .bi  (ModSource::KeyTrack,  Param::shapeDecay,     -0.200f)
     .uni (ModSource::Macro1,    Param::fractureEvolve,  0.300f)
     .uni (ModSource::Macro2,    Param::fractureTone,    0.300f)
     .uni (ModSource::Macro3,    Param::spaceMix,        0.320f)
     .uni (ModSource::Macro4,    Param::fractureAmount,  0.300f)
     .uni (ModSource::Macro4,    Param::evolveMagnet,    0.200f);
    sharedMacros (r, Param::fractureDecay, Param::fractureRandom);
    r.commit (s);
}});

manager.addFactory ({ "Foundry Night", "CINEMATIC", { "metallic", "dirty", "rhythmic", "wide", "roomy" }, [] (PatchState& s)
{
    // An atmosphere with a factory in it. Crackling weather drives a metal
    // lattice and FRACTURE puts the debris on a sixteenth grid, so the room
    // has a working rhythm in it that no one has to play.
    dust (s, 5 /* CRACKLE */, 0.44f, 0.52f, 0.36f, 0.52f, 0.60f, 0.72f, 4091, 0.74f);
    amp (s, 0.50f, 6.0f, 0.84f, 2.20f, 0.5f);
    shape (s, 0.58f, 0.50f, 0.46f, 0.52f, 0.64f, 0.36f);
    material (s, MaterialType::Metal, MaterialType::Wood, 0.34f);
    topology (s, 3 /* LATTICE */, 0.52f, 0.48f, 4093);
    matter (s, 0.86f, 0.54f, 0.24f, 0.80f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.50f, 0.16f, 0.0f, 0.22f, 0.30f);
    fracture (s, 1 /* RHYTHMIC */, 0.46f, 0.44f, 0.58f, 0.62f, 0.26f, 0.34f, 0.48f, 0.54f, 0.24f,
              1 /* 16 */, 4 /* 1/16 */, 8, 0.18f, 2 /* PINGPONG */, 0.80f, 0.28f, 4099,
              FractureShape { 16, 0.06f, 0.42f, 0.14f, 0.42f, 0.36f, 0.66f, 0.26f, 0.86f,
                              1.0f, 0.85f, 0.72f, 0.80f, nullptr, "XoLoXoHo", nullptr });
    space (s, SpacePresets::Machine, 0.40f, 0.64f, 0.46f, 0.34f);

    env (s, 2, 2.40f, 8.0f, 0.82f, 4.0f, 0.55f);
    lfo (s, 1, 0.19f, 5 /* SMOOTH RANDOM */, 1.0f, true, 1.50f);
    chaos (s, 1, 0 /* WALK */, 0.30f, 0.45f, 0.70f, 0.5f, 4111);
    macros (s, 0.45f, 0.40f, 0.45f, 0.50f);

    Routings r;
    r.bi  (ModSource::LFO1,      Param::dustDensity,     0.140f)
     .bi  (ModSource::Chaos1,    Param::fractureSwing,   0.100f)
     .uni (ModSource::Env2,      Param::fractureAmount,  0.220f)
     .uni (ModSource::Velocity,  Param::dustDensity,     0.260f)
     .bi  (ModSource::KeyTrack,  Param::fractureTone,    0.180f)
     .uni (ModSource::Macro1,    Param::fractureEvolve,  0.320f)
     .uni (ModSource::Macro2,    Param::dustColor,       0.300f)
     .uni (ModSource::Macro3,    Param::spaceMix,        0.300f)
     .uni (ModSource::Macro4,    Param::fractureProbability, -0.300f)
     .uni (ModSource::Macro4,    Param::shapeSurface,    0.220f);
    sharedMacros (r, Param::fractureDecay, Param::fractureRandom);
    r.commit (s);
}});

manager.addFactory ({ "Iron Weather", "CINEMATIC", { "cold", "noisy", "scraped", "huge", "distant" }, [] (PatchState& s)
{
    // Wind across something enormous and empty. Friction drives a void object
    // in the largest room the plug-in has; the note is barely a note, which is
    // the point - it is the outside of the building, not anything inside it.
    gesture (s, 4 /* FRICTION */, 0.30f, 0.26f, 0.52f, 0.56f, 0.42f, 0.72f, 0.82f);
    amp (s, 1.20f, 6.0f, 0.90f, 4.0f, 0.6f);
    shape (s, 0.62f, 0.22f, 0.64f, 0.34f, 0.80f, 0.26f);
    material (s, MaterialType::Void, MaterialType::Organic, 0.34f);
    topology (s, 4 /* RANDOM */, 0.38f, 0.38f, 4127);
    matter (s, 0.72f, 0.48f, 0.12f, 0.90f);
    evolve (s, 0.0f, 0.06f, 0.0f, 0.0f, 0.54f, 0.14f, 0.0f, 0.08f, 0.24f);
    space (s, SpacePresets::Void, 0.58f, 0.94f, 0.36f, 0.48f);

    env (s, 2, 3.20f, 8.0f, 0.88f, 6.0f, 0.6f);
    lfo (s, 1, 0.05f, 5 /* SMOOTH RANDOM */, 1.0f, false, 3.0f);
    lfo (s, 2, 0.11f, 0 /* SINE */, 1.0f, false, 2.0f);
    macros (s, 0.40f, 0.30f, 0.60f, 0.45f);

    Routings r;
    r.bi  (ModSource::LFO1,      Param::gestureSpeed,    0.180f)
     .bi  (ModSource::LFO2,      Param::gestureBandwidth, 0.120f)
     .uni (ModSource::Env2,      Param::gesturePressure, 0.200f)
     .uni (ModSource::Velocity,  Param::gestureSpeed,    0.220f)
     .uni (ModSource::NoteRandom, Param::gesturePosition, 0.240f)
     .uni (ModSource::Macro1,    Param::gestureMotion,   0.340f)
     .uni (ModSource::Macro2,    Param::gestureBandwidth, 0.320f)
     .uni (ModSource::Macro3,    Param::spaceMix,        0.300f)
     .uni (ModSource::Macro4,    Param::gestureRoughness, 0.300f)
     .uni (ModSource::Macro4,    Param::shapeMass,       0.200f);
    sharedMacros (r, Param::spaceSize, Param::gestureRoughness);
    r.commit (s);
}});

manager.addFactory ({ "Turbine Dusk", "CINEMATIC", { "synthetic", "pulsing", "low", "wide", "transition" }, [] (PatchState& s)
{
    // Machinery two streets away. A brown hum holds the bottom while a thin
    // metallic layer pulses on top of it through the synced delays, which is
    // enough movement to carry a scene change without a single new note.
    set (s, Param::sourceMode, 1 /* LAYER */);
    set (s, Param::impactLevel, 0.0f);
    set (s, Param::sampleLevel, 0.0f);
    set (s, Param::gestureLevel, 0.0f);
    wave (s, 4 /* METALLIC */, 0.30f, 0.26f, 0.02f, 2, 0.14f, 0.70f, 0, 0.34f);
    dust (s, 2 /* BROWN */, 0.58f, 0.18f, 0.42f, 0.22f, 0.44f, 0.56f, 4129, 0.70f);
    amp (s, 0.80f, 6.0f, 0.88f, 2.60f, 0.55f);
    shape (s, 0.50f, 0.30f, 0.58f, 0.42f, 0.72f, 0.22f);
    material (s, MaterialType::Custom, MaterialType::Metal, 0.30f);
    topology (s, 0 /* CHAIN */, 0.40f, 0.44f, 4133);
    matter (s, 0.84f, 0.48f, 0.16f, 0.72f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.52f, 0.08f, 0.0f, 0.14f, 0.22f);
    space (s, SpacePresets::Orbit, 0.42f, 0.62f, 0.48f, 0.40f);

    env (s, 2, 2.60f, 8.0f, 0.84f, 4.0f, 0.55f);
    lfo (s, 1, 1.30f, 1 /* TRIANGLE */, 1.0f, true, 1.20f, 0.32f);
    lfo (s, 2, 0.08f, 0 /* SINE */, 1.0f, false, 2.0f);
    macros (s, 0.40f, 0.35f, 0.50f, 0.45f);

    Routings r;
    r.bi  (ModSource::LFO1,      Param::waveLevel,       0.180f)
     .bi  (ModSource::LFO1,      Param::wavePosition,    0.080f)
     .bi  (ModSource::LFO2,      Param::dustColor,       0.100f)
     .uni (ModSource::Env2,      Param::shapeDensity,    0.160f)
     .uni (ModSource::Velocity,  Param::waveLevel,       0.200f)
     .bi  (ModSource::KeyTrack,  Param::dustColor,       0.160f)
     .uni (ModSource::Macro1,    Param::evolveMotion,    0.300f)
     .uni (ModSource::Macro2,    Param::wavePosition,    0.300f)
     .uni (ModSource::Macro3,    Param::spaceMix,        0.300f)
     .uni (ModSource::Macro4,    Param::waveLevel,       0.260f)
     .uni (ModSource::Macro4,    Param::spaceDelayFeedback, 0.220f);
    sharedMacros (r, Param::spaceSize, Param::dustJitter);
    r.commit (s);
}});

manager.addFactory ({ "First Daylight", "CINEMATIC", { "warm", "bright", "glassy", "high", "wide" }, [] (PatchState& s)
{
    // The hopeful one. A soft harmonic swell with glass ringing an octave over
    // it in a shimmering room, tuned to the octave so nothing in it can sound
    // uncertain. This is the patch for the shot where the weather changes.
    set (s, Param::sourceMode, 1 /* LAYER */);
    set (s, Param::dustLevel, 0.0f);
    set (s, Param::impactLevel, 0.0f);
    set (s, Param::gestureLevel, 0.0f);
    sample (s, BuiltInSamples::Kind::GlassStrike, 1 /* LOOP */, 0.0f, 0.30f, 0.26f, 0.46f, 72, 0.22f);
    wave (s, 1 /* HARMONIC */, 0.24f, 0.20f, 0.02f, 5, 0.14f, 0.82f, 0, 0.60f);
    amp (s, 1.0f, 6.0f, 0.88f, 3.20f, 0.6f);
    shape (s, 0.52f, 0.44f, 0.28f, 0.66f, 0.80f, 0.14f);
    material (s, MaterialType::Crystal, MaterialType::String, 0.34f);
    topology (s, 1 /* RING */, 0.36f, 0.56f, 4139);
    matter (s, 0.80f, 0.56f, 0.20f, 0.84f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.30f, 0.42f, 0.06f, 0.0f, 0.10f, 0.20f);
    set (s, Param::evolveMagnetTarget, 0 /* OCTAVE */);
    space (s, SpacePresets::Shimmer, 0.50f, 0.74f, 0.62f, 0.36f);

    env (s, 2, 2.80f, 8.0f, 0.86f, 5.0f, 0.6f);
    lfo (s, 1, 0.13f, 0 /* SINE */, 1.0f, false, 2.20f);
    macros (s, 0.30f, 0.45f, 0.50f, 0.45f);

    Routings r;
    r.bi  (ModSource::LFO1,      Param::waveMorph,       0.070f)
     .bi  (ModSource::LFO1,      Param::sampleLevel,     0.090f)
     .uni (ModSource::Env2,      Param::shapeDensity,    0.180f)
     .uni (ModSource::Velocity,  Param::waveLevel,       0.220f)
     .uni (ModSource::Velocity,  Param::sampleLevel,     0.160f)
     .bi  (ModSource::KeyTrack,  Param::wavePosition,    0.140f)
     .uni (ModSource::Macro1,    Param::evolveMotion,    0.300f)
     .uni (ModSource::Macro2,    Param::wavePosition,    0.320f)
     .uni (ModSource::Macro3,    Param::spaceMix,        0.300f)
     .uni (ModSource::Macro4,    Param::sampleLevel,     0.240f)
     .uni (ModSource::Macro4,    Param::evolveMagnet,    0.200f);
    sharedMacros (r, Param::spaceSize, Param::waveDetune);
    r.commit (s);
}});

manager.addFactory ({ "Salt Horizon", "CINEMATIC", { "cold", "soft", "blown", "air", "drift" }, [] (PatchState& s)
{
    // Nothing but air and a long way to see. Breath through a crystal object
    // at the top of the keyboard, wide and slow, with no bottom to it at all.
    // Sits over a bed without taking any of its room.
    gesture (s, 3 /* BREATH */, 0.44f, 0.40f, 0.22f, 0.48f, 0.30f, 0.86f, 1.0f);
    set (s, Param::masterGain, 4.0f);
    amp (s, 0.85f, 6.0f, 0.92f, 3.60f, 0.6f);
    shape (s, 0.44f, 0.72f, 0.16f, 0.74f, 0.78f, 0.12f);
    material (s, MaterialType::Crystal, MaterialType::Liquid, 0.22f);
    topology (s, 5 /* STAR */, 0.22f, 0.62f, 4153);
    matter (s, 0.68f, 0.74f, 0.12f, 0.90f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.36f, 0.10f, 0.0f, 0.08f, 0.20f);
    space (s, SpacePresets::Dream, 0.56f, 0.84f, 0.66f, 0.38f);

    env (s, 2, 3.40f, 8.0f, 0.86f, 6.0f, 0.6f);
    lfo (s, 1, 0.07f, 5 /* SMOOTH RANDOM */, 1.0f, false, 2.60f);
    macros (s, 0.35f, 0.45f, 0.60f, 0.40f);

    Routings r;
    r.bi  (ModSource::LFO1,      Param::gestureBandwidth, 0.130f)
     .uni (ModSource::Env2,      Param::gesturePressure, 0.180f)
     .uni (ModSource::Velocity,  Param::gestureSpeed,    0.240f)
     .bi  (ModSource::KeyTrack,  Param::gestureBandwidth, 0.160f)
     .uni (ModSource::NoteRandom, Param::gesturePosition, 0.180f)
     .uni (ModSource::Macro1,    Param::gestureMotion,   0.320f)
     .uni (ModSource::Macro2,    Param::gestureBandwidth, 0.320f)
     .uni (ModSource::Macro3,    Param::spaceMix,        0.300f)
     .uni (ModSource::Macro4,    Param::shapeMix,        0.280f)
     .uni (ModSource::Macro4,    Param::gestureRoughness, 0.200f);
    sharedMacros (r, Param::spaceSize, Param::gestureSpeed);
    r.commit (s);
}});

manager.addFactory ({ "Lantern Chorus", "CINEMATIC", { "vocal", "warm", "formant", "chords", "wide" }, [] (PatchState& s)
{
    // Voices that are pleased to see you: a formant wave on an organic body,
    // consonant, wide and slow. The counterpart to the bank's broken choir -
    // same family, opposite mood, and it holds a triad without smearing.
    wave (s, 2 /* FORMANT */, 0.36f, 0.28f, 0.04f, 4, 0.18f, 0.84f, 0, 0.82f);
    amp (s, 1.0f, 6.0f, 0.86f, 3.0f, 0.6f);
    shape (s, 0.58f, 0.30f, 0.40f, 0.56f, 0.70f, 0.18f);
    material (s, MaterialType::Organic, MaterialType::Membrane, 0.26f);
    topology (s, 2 /* CLUSTERS */, 0.46f, 0.52f, 4157);
    matter (s, 0.82f, 0.58f, 0.18f, 0.86f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.22f, 0.48f, 0.08f, 0.0f, 0.12f, 0.22f);
    set (s, Param::evolveMagnetTarget, 2 /* MAJOR */);
    space (s, SpacePresets::Nebula, 0.48f, 0.72f, 0.56f, 0.36f);

    env (s, 2, 2.20f, 8.0f, 0.84f, 5.0f, 0.6f);
    lfo (s, 1, 0.15f, 0 /* SINE */, 1.0f, false, 2.0f);
    lfo (s, 2, 0.09f, 5 /* SMOOTH RANDOM */, 1.0f, false, 2.0f);
    macros (s, 0.35f, 0.45f, 0.50f, 0.45f);

    Routings r;
    r.bi  (ModSource::LFO1,      Param::wavePosition,    0.090f)
     .bi  (ModSource::LFO2,      Param::waveMorph,       0.080f)
     .uni (ModSource::Env2,      Param::evolveMagnet,    0.180f)
     .uni (ModSource::Velocity,  Param::waveScan,        0.220f)
     .uni (ModSource::Velocity,  Param::shapeExcite,     0.180f)
     .bi  (ModSource::KeyTrack,  Param::wavePosition,   -0.160f)
     .uni (ModSource::Macro1,    Param::evolveMotion,    0.300f)
     .uni (ModSource::Macro2,    Param::wavePosition,    0.320f)
     .uni (ModSource::Macro3,    Param::spaceMix,        0.300f)
     .uni (ModSource::Macro4,    Param::waveMorph,       0.300f)
     .uni (ModSource::Macro4,    Param::evolveMagnet,    0.220f);
    sharedMacros (r, Param::spaceSize, Param::waveScan);
    r.commit (s);
}});

manager.addFactory ({ "Pendulum Dread", "CINEMATIC", { "dark", "pulsing", "low", "close", "transition" }, [] (PatchState& s)
{
    // Tension you can count. A membrane pulse on a quarter-note grid, dry and
    // close, with the low bands kept and the high ones thrown away, so it
    // reads as a clock in the room rather than as music in the score.
    impact (s, 5 /* DAMPED SINE */, 0.44f, 0.24f, 0.34f, 0.52f, 0.55f, 0.10f, 0.36f, 0.70f);
    set (s, Param::masterGain, -3.0f);
    amp (s, 0.006f, 6.0f, 0.72f, 1.40f, 0.36f);
    shape (s, 0.46f, 0.24f, 0.62f, 0.34f, 0.56f, 0.20f);
    material (s, MaterialType::Membrane, MaterialType::Void, 0.36f);
    topology (s, 3 /* LATTICE */, 0.42f, 0.36f, 4159);
    matter (s, 0.92f, 0.46f, 0.30f, 0.54f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.58f, 0.06f, 0.0f, 0.10f, 0.16f);
    fracture (s, 1 /* RHYTHMIC */, 0.52f, 0.46f, 0.40f, 0.72f, 0.32f, 0.42f, 0.56f, 0.38f, 0.12f,
              0 /* 8 */, 2 /* 1/4 */, 8, 0.0f, 0 /* FORWARD */, 1.0f, 0.10f, 4177,
              FractureShape { 8, 0.10f, 0.40f, 0.20f, 0.40f, 0.40f, 0.60f, 0.16f, 0.60f,
                              1.0f, 0.45f, 0.40f, 1.0f, kFallingTerrace, "X.L.X.L.", nullptr });
    space (s, SpacePresets::Chamber, 0.32f, 0.46f, 0.40f, 0.28f);

    env (s, 2, 1.80f, 8.0f, 0.70f, 3.0f, 0.5f);
    lfo (s, 1, 0.10f, 0 /* SINE */, 1.0f, true, 2.0f);
    macros (s, 0.35f, 0.35f, 0.35f, 0.50f);

    Routings r;
    r.bi  (ModSource::LFO1,      Param::fractureDecay,   0.080f)
     .uni (ModSource::Env2,      Param::fractureFeedback, 0.200f)
     .uni (ModSource::Velocity,  Param::impactHardness,  0.300f)
     .uni (ModSource::Velocity,  Param::fractureAmount,  0.200f)
     .bi  (ModSource::KeyTrack,  Param::fractureTone,    0.160f)
     .uni (ModSource::Macro1,    Param::fractureEvolve,  0.300f)
     .uni (ModSource::Macro2,    Param::fractureTone,    0.320f)
     .uni (ModSource::Macro3,    Param::spaceMix,        0.320f)
     .uni (ModSource::Macro4,    Param::fractureFeedback, 0.300f)
     .uni (ModSource::Macro4,    Param::shapeMass,       0.200f);
    sharedMacros (r, Param::fractureDecay, Param::fractureRandom);
    r.commit (s);
}});

manager.addFactory ({ "Night Ferry", "CINEMATIC", { "dark", "static", "drift", "distant", "low" }, [] (PatchState& s)
{
    // Something big moving slowly a long way off. FREEZE holds the object's
    // node state and floors its damping, so the resonance never dies and the
    // patch keeps exactly the same weight however long the key is held.
    wave (s, 5 /* SPECTRAL */, 0.18f, 0.14f, 0.02f, 3, 0.12f, 0.62f, -1, 0.70f);
    amp (s, 1.20f, 6.0f, 0.90f, 4.0f, 0.6f);
    shape (s, 0.44f, 0.26f, 0.66f, 0.36f, 0.72f, 0.12f);
    material (s, MaterialType::Void, MaterialType::Liquid, 0.28f);
    topology (s, 1 /* RING */, 0.36f, 0.40f, 4201);
    matter (s, 0.86f, 0.46f, 0.14f, 0.76f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.52f, 0.06f, 0.0f, 0.06f, 0.12f);
    set (s, Param::evolveFreeze, 1.0f);
    space (s, SpacePresets::Void, 0.56f, 0.88f, 0.36f, 0.46f);

    env (s, 2, 3.60f, 8.0f, 0.86f, 6.0f, 0.6f);
    lfo (s, 1, 0.06f, 0 /* SINE */, 1.0f, false, 3.0f);
    lfo (s, 2, 0.28f, 0 /* SINE */, 1.0f, false, 2.0f);
    macros (s, 0.30f, 0.30f, 0.60f, 0.40f);

    Routings r;
    r.bi  (ModSource::LFO1,      Param::wavePosition,    0.090f)
     .bi  (ModSource::LFO2,      Param::waveLevel,       0.080f)
     .uni (ModSource::Env2,      Param::spaceMix,        0.140f)
     .uni (ModSource::Velocity,  Param::waveLevel,       0.220f)
     .bi  (ModSource::KeyTrack,  Param::shapeMass,      -0.160f)
     .uni (ModSource::Macro1,    Param::evolveMotion,    0.280f)
     .uni (ModSource::Macro2,    Param::wavePosition,    0.300f)
     .uni (ModSource::Macro2,    Param::spaceTone,       0.220f)
     .uni (ModSource::Macro3,    Param::spaceMix,        0.280f)
     .uni (ModSource::Macro4,    Param::shapeMass,       0.280f)
     .uni (ModSource::Macro4,    Param::waveMorph,       0.200f);
    sharedMacros (r, Param::spaceFeedback, Param::waveDetune);
    r.commit (s);
}});

manager.addFactory ({ "Cold Engine", "CINEMATIC", { "synthetic", "rhythmic", "close", "dry", "low" }, [] (PatchState& s)
{
    // A tight, dry pulse that does not sound like a drum. Repeated impulses
    // into a stiff custom object with almost no room, so it can run under a
    // scene at a fixed distance and never bloom into the picture.
    impact (s, 1 /* CLICK */, 0.55f, 0.40f, 0.62f, 0.60f, 0.45f, 0.06f, 0.46f, 1.0f);
    amp (s, 0.004f, 6.0f, 0.66f, 0.90f, 0.34f);
    shape (s, 0.46f, 0.36f, 0.50f, 0.58f, 0.74f, 0.18f);
    material (s, MaterialType::Custom, MaterialType::Metal, 0.24f);
    topology (s, 0 /* CHAIN */, 0.38f, 0.44f, 4211);
    matter (s, 0.90f, 0.62f, 0.12f, 0.44f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.52f, 0.08f, 0.16f, 0.20f, 0.20f);
    space (s, SpacePresets::Machine, 0.24f, 0.36f, 0.44f, 0.24f);

    env (s, 2, 2.0f, 8.0f, 0.80f, 3.0f, 0.5f);
    lfo (s, 1, 0.24f, 5 /* SMOOTH RANDOM */, 1.0f, true, 1.20f);
    macros (s, 0.35f, 0.40f, 0.30f, 0.50f);

    Routings r;
    r.bi  (ModSource::LFO1,      Param::impactRate,      0.070f)
     .uni (ModSource::Env2,      Param::shapeSurface,    0.160f)
     .uni (ModSource::Velocity,  Param::impactHardness,  0.300f)
     .uni (ModSource::Velocity,  Param::impactBrightness, 0.240f)
     .bi  (ModSource::KeyTrack,  Param::impactLength,   -0.180f)
     .uni (ModSource::Macro1,    Param::impactRate,      0.280f)
     .uni (ModSource::Macro2,    Param::impactBrightness, 0.320f)
     .uni (ModSource::Macro3,    Param::spaceMix,        0.320f)
     .uni (ModSource::Macro4,    Param::evolveCrush,     0.300f)
     .uni (ModSource::Macro4,    Param::shapeTension,    0.200f);
    sharedMacros (r, Param::spaceSize, Param::impactRandom);
    r.commit (s);
}});

manager.addFactory ({ "Vault Strike", "CINEMATIC", { "dark", "impact", "huge", "low", "struck" }, [] (PatchState& s)
{
    // The hit that marks the cut. Two layers land together - a membrane thump
    // for the weight and a burst of dust for the air it moves - into a room
    // with a six second tail. One note, no sustain, nothing to play.
    set (s, Param::sourceMode, 1 /* LAYER */);
    set (s, Param::waveLevel, 0.0f);
    set (s, Param::sampleLevel, 0.0f);
    set (s, Param::gestureLevel, 0.0f);
    dust (s, 6 /* IMPULSE */, 0.16f, 0.44f, 0.52f, 0.40f, 0.70f, 0.80f, 4217, 0.30f);
    impact (s, 6 /* MEMBRANE HIT */, 0.30f, 0.30f, 0.46f, 0.62f, 0.62f, 0.10f, 0.0f, 0.78f);
    set (s, Param::masterGain, -3.0f);
    amp (s, 0.003f, 4.0f, 0.06f, 5.0f, 0.30f);
    shape (s, 0.56f, 0.24f, 0.72f, 0.30f, 0.82f, 0.22f);
    material (s, MaterialType::Membrane, MaterialType::Void, 0.44f);
    topology (s, 2 /* CLUSTERS */, 0.46f, 0.38f, 4219);
    matter (s, 0.92f, 0.52f, 0.56f, 0.78f);
    evolve (s, 0.0f, 0.14f, 0.0f, 0.0f, 0.58f, 0.10f, 0.0f, 0.10f, 0.20f);
    space (s, SpacePresets::Void, 0.58f, 0.96f, 0.34f, 0.52f);

    env (s, 1, 0.002f, 2.20f, 0.0f, 2.0f, 0.28f);
    env (s, 2, 0.90f, 8.0f, 0.40f, 6.0f, 0.5f);
    lfo (s, 1, 0.08f, 0 /* SINE */, 1.0f, true, 1.50f);
    macros (s, 0.30f, 0.35f, 0.65f, 0.45f);

    Routings r;
    r.uni (ModSource::Env1,      Param::dustLevel,      -0.260f)
     .uni (ModSource::Env2,      Param::evolveMelt,      0.220f)
     .bi  (ModSource::LFO1,      Param::shapeMass,       0.060f)
     .uni (ModSource::Velocity,  Param::impactHardness,  0.340f)
     .uni (ModSource::Velocity,  Param::dustLevel,       0.240f)
     .bi  (ModSource::KeyTrack,  Param::shapeDecay,     -0.140f)
     .uni (ModSource::Macro1,    Param::evolveMotion,    0.260f)
     .uni (ModSource::Macro2,    Param::impactBrightness, 0.320f)
     .uni (ModSource::Macro3,    Param::spaceMix,        0.280f)
     .uni (ModSource::Macro3,    Param::spaceSize,       0.160f)
     .uni (ModSource::Macro4,    Param::shapeMass,       0.280f)
     .uni (ModSource::Macro4,    Param::dustLevel,       0.200f);
    sharedMacros (r, Param::spaceFeedback, Param::dustJitter);
    r.commit (s);
}});

manager.addFactory ({ "Hollow Anvil", "CINEMATIC", { "metallic", "harsh", "impact", "mid", "wide" }, [] (PatchState& s)
{
    // The other kind of hit: bright, metallic and answered. FRACTURE throws
    // the strike back a fifth up on a delayed grid, so one note lands and then
    // rings around the room after it, which is what a cut to a wide shot wants.
    impact (s, 4 /* METAL STRIKE */, 0.72f, 0.64f, 0.26f, 0.58f, 0.45f, 0.10f);
    set (s, Param::masterGain, -2.0f);
    amp (s, 0.002f, 3.60f, 0.08f, 3.20f, 0.32f);
    shape (s, 0.60f, 0.62f, 0.36f, 0.62f, 0.78f, 0.24f);
    material (s, MaterialType::Metal, MaterialType::Crystal, 0.36f);
    topology (s, 3 /* LATTICE */, 0.50f, 0.54f, 4229);
    matter (s, 0.96f, 0.58f, 0.62f, 0.84f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.46f, 0.10f, 0.0f, 0.16f, 0.24f);
    fracture (s, 2 /* TRANSIENT */, 0.44f, 0.42f, 0.66f, 0.44f, 0.30f, 0.52f, 0.62f, 0.58f, 0.14f,
              1 /* 16 */, 3 /* 1/8 */, 8, 0.0f, 0 /* FORWARD */, 0.90f, 0.16f, 4231,
              FractureShape { 16, 0.16f, 0.66f, 0.18f, 0.48f, 0.46f, 0.74f, 0.30f, 0.90f,
                              1.0f, 0.70f, 0.76f, 0.90f, kFifthTerrace, "XXHXXLXX", nullptr });
    space (s, SpacePresets::Orbit, 0.46f, 0.72f, 0.54f, 0.42f);

    env (s, 1, 0.002f, 1.60f, 0.0f, 1.40f, 0.30f);
    lfo (s, 1, 0.12f, 1 /* TRIANGLE */, 1.0f, true, 1.20f);
    macros (s, 0.35f, 0.45f, 0.50f, 0.50f);

    Routings r;
    r.uni (ModSource::Env1,      Param::fractureAmount,  0.200f)
     .bi  (ModSource::LFO1,      Param::fractureDelay,   0.070f)
     .uni (ModSource::Velocity,  Param::impactHardness,  0.320f)
     .uni (ModSource::Velocity,  Param::impactBrightness, 0.260f)
     .bi  (ModSource::KeyTrack,  Param::shapeDecay,     -0.200f)
     .uni (ModSource::Macro1,    Param::fractureEvolve,  0.300f)
     .uni (ModSource::Macro2,    Param::impactBrightness, 0.320f)
     .uni (ModSource::Macro3,    Param::spaceMix,        0.300f)
     .uni (ModSource::Macro4,    Param::fractureFeedback, 0.300f)
     .uni (ModSource::Macro4,    Param::shapeTension,    0.220f);
    sharedMacros (r, Param::fractureDecay, Param::impactRandom);
    r.commit (s);
}});

manager.addFactory ({ "Ascension Wire", "CINEMATIC", { "bright", "metallic", "evolving", "transition", "huge" }, [] (PatchState& s)
{
    // The riser. Hold it and BEND levers the whole object upward around a low
    // pivot while the spectral bands open above it; let go and the shimmer
    // takes four seconds to leave. Written to be held into a cut, not played.
    wave (s, 6 /* FRACTURED */, 0.22f, 0.24f, 0.06f, 4, 0.20f, 0.80f, 0, 0.74f);
    amp (s, 1.50f, 8.0f, 0.92f, 4.0f, 0.70f);
    shape (s, 0.54f, 0.58f, 0.30f, 0.62f, 0.76f, 0.20f);
    material (s, MaterialType::Metal, MaterialType::Crystal, 0.30f);
    topology (s, 4 /* RANDOM */, 0.44f, 0.56f, 4241);
    matter (s, 0.84f, 0.56f, 0.22f, 0.86f);
    evolve (s, 0.06f, 0.0f, 0.0f, 0.16f, 0.44f, 0.10f, 0.0f, 0.20f, 0.30f);
    set (s, Param::evolveBendPivot, 0.20f);
    set (s, Param::evolveBendRange, 0.72f);
    set (s, Param::evolveMagnetTarget, 0 /* OCTAVE */);
    fracture (s, 0 /* SPECTRAL */, 0.12f, 0.48f, 0.62f, 0.38f, 0.26f, 0.54f, 0.66f, 0.60f, 0.24f,
              2 /* 32 */, 2 /* 1/4 */, 8, 0.0f, 0 /* FORWARD */, 1.0f, 0.12f, 4243,
              FractureShape { 32, 0.20f, 0.74f, 0.20f, 0.54f, 0.50f, 0.80f, 0.32f, 0.94f,
                              0.90f, 0.78f, 0.74f, 1.0f, kOctaveTerrace, "XXHXXHXX", nullptr });
    space (s, SpacePresets::Shimmer, 0.52f, 0.80f, 0.62f, 0.42f);

    env (s, 2, 5.0f, 8.0f, 1.0f, 4.0f, 0.70f);
    lfo (s, 1, 0.09f, 0 /* SINE */, 1.0f, true, 2.50f);
    macros (s, 0.45f, 0.50f, 0.55f, 0.50f);

    Routings r;
    r.uni (ModSource::Env2,      Param::evolveBend,      0.760f)
     .uni (ModSource::Env2,      Param::fractureAmount,  0.560f)
     .uni (ModSource::Env2,      Param::evolveMagnet,    0.300f)
     .uni (ModSource::Env2,      Param::shapeDensity,    0.220f)
     .bi  (ModSource::LFO1,      Param::wavePosition,    0.080f)
     .uni (ModSource::Velocity,  Param::waveMorph,       0.220f)
     .bi  (ModSource::KeyTrack,  Param::evolveBendPivot, -0.160f)
     .uni (ModSource::Macro1,    Param::evolveSpeed,     0.300f)
     .uni (ModSource::Macro2,    Param::fractureTone,    0.300f)
     .uni (ModSource::Macro3,    Param::spaceMix,        0.300f)
     .uni (ModSource::Macro4,    Param::evolveBend,      0.300f)
     .uni (ModSource::Macro4,    Param::fractureAmount,  0.240f);
    sharedMacros (r, Param::fractureDecay, Param::evolveBendRange);
    r.commit (s);
}});

manager.addFactory ({ "Undertow Riser", "CINEMATIC", { "dark", "granular", "morphing", "transition", "huge" }, [] (PatchState& s)
{
    // The riser that goes the other way. Hold it and the weight moves down:
    // GRAVITY drags the drive into the bottom partials, the cloud loses its
    // colour and the room darkens around it. Use it into a drop, not a cut.
    dust (s, 7 /* CLOUD */, 0.52f, 0.64f, 0.44f, 0.40f, 0.62f, 0.76f, 4253, 0.82f);
    amp (s, 1.20f, 8.0f, 0.90f, 3.60f, 0.65f);
    shape (s, 0.58f, 0.40f, 0.44f, 0.48f, 0.74f, 0.28f);
    material (s, MaterialType::Liquid, MaterialType::Void, 0.26f);
    topology (s, 2 /* CLUSTERS */, 0.48f, 0.46f, 4259);
    matter (s, 0.82f, 0.54f, 0.20f, 0.84f);
    evolve (s, 0.0f, 0.06f, 0.0f, 0.0f, 0.34f, 0.14f, 0.0f, 0.14f, 0.28f);
    fracture (s, 3 /* EVOLVE */, 0.30f, 0.40f, 0.60f, 0.48f, 0.26f, 0.46f, 0.58f, 0.44f, 0.40f,
              1 /* 16 */, 2 /* 1/4 */, 8, 0.0f, 1 /* BACKWARD */, 1.0f, 0.24f, 4261,
              FractureShape { 16, 0.14f, 0.60f, 0.16f, 0.46f, 0.44f, 0.72f, 0.28f, 0.88f,
                              1.0f, 0.60f, 0.68f, 1.0f, kFallingTerrace, "XLXLXLXL", nullptr });
    space (s, SpacePresets::Dust, 0.50f, 0.78f, 0.42f, 0.40f);

    env (s, 2, 4.60f, 8.0f, 1.0f, 4.0f, 0.65f);
    lfo (s, 1, 0.07f, 0 /* SINE */, 1.0f, true, 2.50f);
    chaos (s, 1, 1 /* BROWNIAN */, 0.10f, 0.45f, 0.82f, 0.5f, 4271);
    macros (s, 0.45f, 0.35f, 0.55f, 0.50f);

    Routings r;
    r.uni (ModSource::Env2,      Param::evolveGravity,   0.560f)
     .uni (ModSource::Env2,      Param::shapeMass,       0.380f)
     .uni (ModSource::Env2,      Param::fracturePitch,  -0.260f)
     .uni (ModSource::Env2,      Param::dustColor,      -0.580f)
     .uni (ModSource::Env2,      Param::spaceTone,      -0.300f)
     .uni (ModSource::Env2,      Param::fractureTone,   -0.340f)
     .uni (ModSource::Env2,      Param::fractureAmount, -0.180f)
     .bi  (ModSource::LFO1,      Param::dustDensity,     0.100f)
     .bi  (ModSource::Chaos1,    Param::dustJitter,      0.120f)
     .uni (ModSource::Velocity,  Param::dustDensity,     0.240f)
     .bi  (ModSource::KeyTrack,  Param::dustColor,       0.160f)
     .uni (ModSource::Macro1,    Param::evolveMotion,    0.320f)
     .uni (ModSource::Macro2,    Param::dustColor,       0.320f)
     .uni (ModSource::Macro3,    Param::spaceMix,        0.300f)
     .uni (ModSource::Macro4,    Param::evolveGravity,   0.300f)
     .uni (ModSource::Macro4,    Param::evolveMelt,      0.220f);
    sharedMacros (r, Param::fractureDecay, Param::dustJitter);
    r.commit (s);
}});
}

} // namespace am::FactoryContent
