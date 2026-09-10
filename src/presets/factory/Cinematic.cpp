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
     .uni (ModSource::Env2,      Param::evolveCrush,     0.220f)
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
     .uni (ModSource::Env2,      Param::evolveMagnet,    0.200f)
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
}

} // namespace am::FactoryContent
