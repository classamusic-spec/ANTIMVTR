#include "presets/FactoryBuilders.h"

namespace am::FactoryContent
{

void registerEvolving (PresetManager& manager)
{
//==========================================================================
// EVOLVING — the patch is not the same at the end of the note
//==========================================================================

manager.addFactory ({ "Metal Bloom", "EVOLVING", { "evolving", "metal", "blooming", "long" }, [] (PatchState& s)
{
    impact (s, 4 /* METAL STRIKE */, 0.60f, 0.45f, 0.22f, 0.78f, 0.50f, 0.10f);
    amp (s, 0.002f, 2.40f, 0.55f, 2.60f, 0.45f);
    shape (s, 0.62f, 0.50f, 0.42f, 0.58f, 0.76f, 0.28f);
    material (s, MaterialType::Metal, MaterialType::Organic, 0.35f);
    topology (s, 2 /* CLUSTERS */, 0.52f, 0.52f, 907);
    matter (s, 0.98f, 0.55f, 0.58f, 0.78f);
    evolve (s, 0.30f, 0.24f, 0.0f, 0.30f, 0.42f, 0.22f, 0.0f, 0.16f, 0.55f);
    set (s, Param::evolveBendPivot, 0.45f);
    set (s, Param::evolveBendRange, 0.55f);
    set (s, Param::evolveMagnetTarget, 1 /* FIFTH */);
    space (s, SpacePresets::Shimmer, 0.46f, 0.70f, 0.60f, 0.40f);

    env (s, 1, 0.001f, 1.20f, 0.0f, 1.0f, 0.35f);
    env (s, 2, 2.20f, 5.0f, 0.75f, 5.0f, 0.6f);
    lfo (s, 1, 0.14f, 0 /* SINE */, 1.0f, true, 1.2f);
    chaos (s, 1, 0 /* WALK */, 0.20f, 0.50f, 0.75f, 0.5f, 2281);
    macros (s, 0.50f, 0.45f, 0.45f, 0.50f);

    Routings r;
    r.uni (ModSource::Env2,   Param::evolveBend,     0.320f)
     .uni (ModSource::Env2,   Param::evolveMelt,     0.200f)
     .bi  (ModSource::LFO1,   Param::shapeForm,      0.060f)
     .bi  (ModSource::Chaos1, Param::evolveScatter,  0.120f)
     .uni (ModSource::Velocity, Param::impactHardness, 0.280f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.400f)
     .uni (ModSource::Macro1, Param::evolveSpeed,    0.250f)
     .uni (ModSource::Macro2, Param::impactBrightness, 0.300f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.300f)
     .uni (ModSource::Macro4, Param::evolveBend,     0.300f)
     .uni (ModSource::Macro4, Param::evolveMagnet,   0.250f);
    sharedMacros (r, Param::spaceSize, Param::evolveTear);
    r.commit (s);
}});

manager.addFactory ({ "Slow Collapse", "EVOLVING", { "evolving", "melting", "descending", "dark" }, [] (PatchState& s)
{
    gesture (s, 2 /* RUB */, 0.60f, 0.30f, 0.42f, 0.48f, 0.28f, 0.45f);
    amp (s, 0.60f, 2.60f, 0.80f, 3.0f, 0.55f);
    shape (s, 0.66f, 0.40f, 0.58f, 0.44f, 0.72f, 0.36f);
    material (s, MaterialType::Membrane, MaterialType::Void, 0.48f);
    topology (s, 0 /* CHAIN */, 0.58f, 0.42f, 977);
    matter (s, 0.94f, 0.52f, 0.20f, 0.72f);
    evolve (s, 0.0f, 0.30f, 0.0f, 0.0f, 0.62f, 0.20f, 0.0f, 0.12f, 0.50f);
    space (s, SpacePresets::Void, 0.50f, 0.80f, 0.42f, 0.42f);

    env (s, 2, 1.0f, 9.0f, 0.10f, 6.0f, 0.35f);
    env (s, 3, 4.0f, 8.0f, 0.60f, 8.0f);
    lfo (s, 1, 0.06f, 2 /* SAW */, 1.0f, true, 1.0f);
    chaos (s, 1, 1 /* BROWNIAN */, 0.10f, 0.45f, 0.85f, 0.5f, 2377);
    macros (s, 0.50f, 0.30f, 0.50f, 0.60f);

    Routings r;
    r.uni (ModSource::Env2,   Param::evolveMelt,     0.400f)
     .uni (ModSource::Env3,   Param::evolveGravity,  0.220f)
     .bi  (ModSource::LFO1,   Param::shapeMass,      0.080f)
     .bi  (ModSource::Chaos1, Param::gesturePressure, 0.120f)
     .uni (ModSource::Velocity, Param::gestureSpeed, 0.200f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.400f)
     .uni (ModSource::Macro1, Param::gestureMotion,  0.300f)
     .uni (ModSource::Macro2, Param::gestureBandwidth, 0.300f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.250f)
     .uni (ModSource::Macro4, Param::evolveMelt,     0.350f)
     .uni (ModSource::Macro4, Param::shapeMass,      0.200f);
    sharedMacros (r, Param::spaceSize, Param::gestureRoughness);
    r.commit (s);
}});

manager.addFactory ({ "Living Alloy", "EVOLVING", { "evolving", "sample", "morphing", "metal" }, [] (PatchState& s)
{
    sample (s, BuiltInSamples::Kind::MetalPing, 1 /* LOOP */, 0.06f, 0.88f, 0.30f, 0.42f);
    amp (s, 0.30f, 1.80f, 0.88f, 2.0f, 0.5f);
    shape (s, 0.60f, 0.60f, 0.40f, 0.60f, 0.80f, 0.34f);
    material (s, MaterialType::Metal, MaterialType::Liquid, 0.48f);
    topology (s, 1 /* RING */, 0.60f, 0.50f, 1049);
    matter (s, 0.88f, 0.62f, 0.32f, 0.80f);
    evolve (s, 0.24f, 0.20f, 0.22f, 0.32f, 0.48f, 0.26f, 0.0f, 0.34f, 0.52f);
    set (s, Param::evolveBendPivot, 0.55f);
    set (s, Param::evolveMagnetTarget, 1 /* FIFTH */);
    space (s, SpacePresets::Nebula, 0.44f, 0.65f, 0.56f, 0.38f);

    env (s, 2, 1.50f, 6.0f, 0.55f, 5.0f);
    lfo (s, 1, 0.21f, 5 /* SMOOTH RANDOM */, 1.0f, true);
    lfo (s, 2, 0.13f, 0 /* SINE */, 1.0f, false, 1.5f);
    chaos (s, 1, 3 /* LORENZ */, 0.28f, 0.55f, 0.65f, 0.5f, 2447);
    macros (s, 0.50f, 0.45f, 0.42f, 0.50f);

    Routings r;
    r.uni (ModSource::Env2,   Param::evolveTear,     0.280f)
     .bi  (ModSource::LFO1,   Param::sampleStart,    0.120f)
     .bi  (ModSource::LFO2,   Param::shapeForm,      0.060f)
     .bi  (ModSource::Chaos1, Param::evolveScatter,  0.140f)
     .uni (ModSource::Velocity, Param::sampleGrain,  0.220f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.400f)
     .uni (ModSource::Macro1, Param::evolveSpeed,    0.250f)
     .uni (ModSource::Macro2, Param::shapeExcite,    0.300f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.280f)
     .uni (ModSource::Macro4, Param::evolveBend,     0.300f)
     .uni (ModSource::Macro4, Param::evolveTear,     0.250f);
    sharedMacros (r, Param::spaceSize, Param::sampleSpread);
    r.commit (s);
}});

manager.addFactory ({ "Cedar Furnace", "EVOLVING", { "wooden", "metallic", "morphing", "struck", "mid" }, [] (PatchState& s)
{
    // A plank rolled with a mallet that turns into a bell: the material pair
    // migrates WOOD -> METAL under a five second envelope while the clusters
    // pull together, so the same strike ends up ringing a different object.
    impact (s, 3 /* NOISE STRIKE */, 0.42f, 0.34f, 0.20f, 0.62f, 0.55f, 0.16f, 0.30f);
    amp (s, 0.006f, 6.0f, 0.74f, 2.20f, 0.42f);
    shape (s, 0.54f, 0.34f, 0.52f, 0.40f, 0.58f, 0.30f);
    material (s, MaterialType::Wood, MaterialType::Metal, 0.04f);
    topology (s, 2 /* CLUSTERS */, 0.20f, 0.44f, 3121);
    matter (s, 0.96f, 0.52f, 0.48f, 0.62f);
    evolve (s, 0.14f, 0.0f, 0.0f, 0.0f, 0.44f, 0.10f, 0.0f, 0.22f, 0.34f);
    set (s, Param::evolveBendPivot, 0.35f);
    set (s, Param::evolveBendRange, 0.40f);
    space (s, SpacePresets::Chamber, 0.34f, 0.52f, 0.56f, 0.30f);

    env (s, 2, 4.60f, 6.0f, 1.0f, 4.0f, 0.55f);
    env (s, 3, 1.40f, 5.0f, 0.70f, 3.0f);
    lfo (s, 1, 0.19f, 5 /* SMOOTH RANDOM */, 1.0f, true, 1.5f);
    macros (s, 0.45f, 0.40f, 0.35f, 0.50f);

    Routings r;
    r.uni (ModSource::Env2,     Param::shapeBlend,      0.860f)
     .uni (ModSource::Env2,     Param::shapeCoupling,   0.480f)
     .uni (ModSource::Env2,     Param::shapeTension,    0.260f)
     .uni (ModSource::Env2,     Param::evolveBend,      0.420f)
     .uni (ModSource::Env3,     Param::impactRate,      0.280f)
     .uni (ModSource::Env3,     Param::evolveGravity,  -0.220f)
     .bi  (ModSource::LFO1,     Param::impactHardness,  0.070f)
     .uni (ModSource::Velocity, Param::impactBrightness, 0.300f)
     .bi  (ModSource::KeyTrack, Param::shapeDecay,     -0.150f)
     .uni (ModSource::Macro1,   Param::evolveMotion,    0.380f)
     .uni (ModSource::Macro1,   Param::evolveSpeed,     0.240f)
     .uni (ModSource::Macro2,   Param::impactBrightness, 0.320f)
     .uni (ModSource::Macro3,   Param::spaceMix,        0.320f)
     .uni (ModSource::Macro4,   Param::shapeBlend,      0.300f)
     .uni (ModSource::Macro4,   Param::shapeCoupling,   0.220f);
    sharedMacros (r, Param::spaceSize, Param::impactRandom);
    r.commit (s);
}});

manager.addFactory ({ "Ice Mouth", "EVOLVING", { "cold", "noisy", "blown", "morphing", "glassy" }, [] (PatchState& s)
{
    // Breath that freezes into a note. MATTER MIX climbs from almost nothing
    // to full while the gesture band narrows, so a wide hiss condenses onto a
    // crystal lattice: the centroid falls by more than half across the note.
    gesture (s, 3 /* BREATH */, 0.46f, 0.52f, 0.34f, 0.42f, 0.30f, 0.92f);
    amp (s, 0.34f, 3.0f, 0.86f, 1.80f, 0.5f);
    shape (s, 0.46f, 0.62f, 0.30f, 0.66f, 0.66f, 0.22f);
    material (s, MaterialType::Crystal, MaterialType::Membrane, 0.26f);
    topology (s, 5 /* STAR */, 0.24f, 0.60f, 3187);
    matter (s, 0.16f, 0.70f, 0.14f, 0.70f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.24f, 0.46f, 0.14f, 0.0f, 0.12f, 0.28f);
    set (s, Param::evolveMagnetTarget, 0 /* OCTAVE */);
    space (s, SpacePresets::Dream, 0.44f, 0.62f, 0.62f, 0.34f);

    env (s, 2, 3.60f, 6.0f, 1.0f, 3.0f, 0.60f);
    env (s, 3, 5.20f, 6.0f, 1.0f, 4.0f);
    lfo (s, 1, 0.10f, 0 /* SINE */, 1.0f, true, 2.0f);
    macros (s, 0.40f, 0.45f, 0.45f, 0.55f);

    Routings r;
    r.uni (ModSource::Env2,     Param::shapeMix,         0.780f)
     .uni (ModSource::Env2,     Param::gestureBandwidth, -0.640f)
     .uni (ModSource::Env3,     Param::evolveMagnet,     0.400f)
     .uni (ModSource::Env3,     Param::gestureRoughness, -0.180f)
     .bi  (ModSource::LFO1,     Param::gesturePressure,  0.080f)
     .uni (ModSource::Velocity, Param::gestureSpeed,     0.240f)
     .uni (ModSource::NoteRandom, Param::gesturePosition, 0.200f)
     .uni (ModSource::Macro1,   Param::gestureMotion,    0.380f)
     .uni (ModSource::Macro2,   Param::gestureBandwidth, 0.300f)
     .uni (ModSource::Macro3,   Param::spaceMix,         0.300f)
     .uni (ModSource::Macro4,   Param::shapeMix,         0.280f)
     .uni (ModSource::Macro4,   Param::evolveMagnet,     0.260f);
    sharedMacros (r, Param::spaceSize, Param::gestureRoughness);
    r.commit (s);
}});

manager.addFactory ({ "Tin Migration", "EVOLVING", { "metallic", "dirty", "morphing", "granular", "wide" }, [] (PatchState& s)
{
    // Two sources at once, and the note hands over from one to the other: a
    // pitched tin wave fades out as a crackling field fades in, so the patch
    // ends as weather rather than as a note.
    set (s, Param::sourceMode, 1 /* LAYER */);
    set (s, Param::impactLevel, 0.0f);
    set (s, Param::sampleLevel, 0.0f);
    set (s, Param::gestureLevel, 0.0f);
    dust (s, 5 /* CRACKLE */, 0.30f, 0.56f, 0.42f, 0.46f, 0.62f, 0.70f, 3251, 0.10f);
    wave (s, 4 /* METALLIC */, 0.34f, 0.22f, 0.0f, 3, 0.20f, 0.70f, 0, 0.52f);
    amp (s, 0.05f, 3.40f, 0.72f, 2.40f, 0.45f);
    shape (s, 0.58f, 0.48f, 0.44f, 0.54f, 0.62f, 0.34f);
    material (s, MaterialType::Metal, MaterialType::Void, 0.22f);
    topology (s, 4 /* RANDOM */, 0.42f, 0.50f, 3299);
    matter (s, 0.82f, 0.56f, 0.34f, 0.76f);
    evolve (s, 0.0f, 0.18f, 0.0f, 0.0f, 0.50f, 0.24f, 0.0f, 0.26f, 0.44f);
    space (s, SpacePresets::Dust, 0.44f, 0.62f, 0.50f, 0.36f);

    env (s, 2, 3.80f, 6.0f, 1.0f, 3.0f, 0.5f);
    lfo (s, 1, 0.16f, 5 /* SMOOTH RANDOM */, 1.0f, true, 1.0f);
    chaos (s, 1, 0 /* WALK */, 0.24f, 0.45f, 0.70f, 0.5f, 3319);
    macros (s, 0.50f, 0.40f, 0.45f, 0.55f);

    Routings r;
    r.uni (ModSource::Env2,     Param::waveLevel,      -0.480f)
     .uni (ModSource::Env2,     Param::dustLevel,       0.620f)
     .uni (ModSource::Env2,     Param::shapeBlend,      0.500f)
     .uni (ModSource::Env2,     Param::dustDensity,     0.360f)
     .bi  (ModSource::LFO1,     Param::wavePosition,    0.100f)
     .bi  (ModSource::Chaos1,   Param::dustJitter,      0.160f)
     .uni (ModSource::Velocity, Param::waveMorph,       0.220f)
     .uni (ModSource::Macro1,   Param::evolveMotion,    0.360f)
     .uni (ModSource::Macro2,   Param::dustColor,       0.320f)
     .uni (ModSource::Macro3,   Param::spaceMix,        0.300f)
     .uni (ModSource::Macro4,   Param::dustGrain,       0.320f)
     .uni (ModSource::Macro4,   Param::evolveMelt,      0.240f);
    sharedMacros (r, Param::spaceSize, Param::dustJitter);
    r.commit (s);
}});

manager.addFactory ({ "Rope Bridge", "EVOLVING", { "wooden", "organic", "bowed", "evolving", "roomy" }, [] (PatchState& s)
{
    // Six loose strings on a chain that tighten into one body: COUPLING goes
    // from almost nothing to nearly full over six seconds, so separate voices
    // become a single coupled resonance with new sum-and-difference modes.
    gesture (s, 0 /* BOW */, 0.38f, 0.34f, 0.26f, 0.34f, 0.24f, 0.44f, 0.72f);
    amp (s, 0.42f, 3.0f, 0.66f, 1.60f, 0.5f);
    shape (s, 0.50f, 0.26f, 0.46f, 0.62f, 0.60f, 0.24f);
    material (s, MaterialType::String, MaterialType::Wood, 0.36f);
    topology (s, 0 /* CHAIN */, 0.06f, 0.38f, 3361);
    matter (s, 0.86f, 0.50f, 0.22f, 0.66f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.48f, 0.10f, 0.0f, 0.14f, 0.26f);
    space (s, SpacePresets::Chamber, 0.36f, 0.56f, 0.52f, 0.30f);

    env (s, 2, 5.60f, 6.0f, 1.0f, 4.0f, 0.65f);
    env (s, 3, 2.40f, 5.0f, 0.80f, 3.0f);
    lfo (s, 1, 0.13f, 0 /* SINE */, 1.0f, true, 2.5f);
    macros (s, 0.40f, 0.45f, 0.40f, 0.50f);

    Routings r;
    r.uni (ModSource::Env2,     Param::shapeCoupling,    0.880f)
     .uni (ModSource::Env2,     Param::shapeDistribution, 0.320f)
     .uni (ModSource::Env2,     Param::shapeForm,        0.460f)
     .uni (ModSource::Env2,     Param::shapeTension,     0.360f)
     .uni (ModSource::Env2,     Param::shapeMass,       -0.320f)
     .uni (ModSource::Env3,     Param::evolveGravity,   -0.200f)
     .uni (ModSource::Env3,     Param::gesturePressure,  0.220f)
     .bi  (ModSource::LFO1,     Param::gestureSpeed,     0.090f)
     .uni (ModSource::Velocity, Param::gesturePressure,  0.260f)
     .bi  (ModSource::KeyTrack, Param::gesturePosition,  0.180f)
     .uni (ModSource::Macro1,   Param::gestureMotion,    0.400f)
     .uni (ModSource::Macro2,   Param::gestureBandwidth, 0.300f)
     .uni (ModSource::Macro3,   Param::spaceMix,         0.320f)
     .uni (ModSource::Macro4,   Param::shapeCoupling,    0.300f)
     .uni (ModSource::Macro4,   Param::shapeTension,     0.200f);
    sharedMacros (r, Param::spaceSize, Param::gestureRoughness);
    r.commit (s);
}});

manager.addFactory ({ "Salt Lantern", "EVOLVING", { "glassy", "bright", "resonant", "evolving", "wide" }, [] (PatchState& s)
{
    // A crystal that is quietly failing. MELT sags the partials and eats the
    // high weights while GRAVITY tips the spectrum downward, so a bright
    // struck lattice ends as a dull hum with the same fundamental.
    impact (s, 4 /* METAL STRIKE */, 0.66f, 0.62f, 0.24f, 0.80f, 0.45f, 0.08f, 0.30f);
    amp (s, 0.003f, 6.0f, 0.70f, 2.60f, 0.40f);
    shape (s, 0.58f, 0.72f, 0.30f, 0.70f, 0.78f, 0.20f);
    material (s, MaterialType::Crystal, MaterialType::Liquid, 0.18f);
    topology (s, 3 /* LATTICE */, 0.44f, 0.56f, 3391);
    matter (s, 0.98f, 0.62f, 0.62f, 0.74f);
    evolve (s, 0.0f, 0.06f, 0.0f, 0.0f, 0.38f, 0.12f, 0.0f, 0.16f, 0.30f);
    space (s, SpacePresets::Shimmer, 0.42f, 0.66f, 0.58f, 0.34f);

    env (s, 2, 3.20f, 6.0f, 1.0f, 4.0f, 0.55f);
    env (s, 3, 5.0f, 6.0f, 1.0f, 4.0f);
    lfo (s, 1, 0.09f, 1 /* TRIANGLE */, 1.0f, true, 2.0f);
    macros (s, 0.45f, 0.50f, 0.45f, 0.55f);

    Routings r;
    r.uni (ModSource::Env2,     Param::evolveMelt,      0.480f)
     .uni (ModSource::Env2,     Param::evolveGravity,   0.420f)
     .uni (ModSource::Env2,     Param::impactBrightness, -0.300f)
     .uni (ModSource::Env2,     Param::shapeDecay,      0.220f)
     .uni (ModSource::Env3,     Param::impactRate,      -0.180f)
     .uni (ModSource::Env3,     Param::shapeDecay,     -0.300f)
     .uni (ModSource::Env3,     Param::shapeBlend,      0.420f)
     .bi  (ModSource::LFO1,     Param::shapeSurface,    0.060f)
     .uni (ModSource::Velocity, Param::impactHardness,  0.300f)
     .bi  (ModSource::KeyTrack, Param::shapeMass,      -0.140f)
     .uni (ModSource::Macro1,   Param::evolveMotion,    0.360f)
     .uni (ModSource::Macro2,   Param::impactBrightness, 0.320f)
     .uni (ModSource::Macro3,   Param::spaceMix,        0.300f)
     .uni (ModSource::Macro4,   Param::evolveMelt,      0.300f)
     .uni (ModSource::Macro4,   Param::evolveGravity,   0.200f);
    sharedMacros (r, Param::spaceSize, Param::impactRandom);
    r.commit (s);
}});

manager.addFactory ({ "Ember Lattice", "EVOLVING", { "dark", "harsh", "unstable", "struck", "low" }, [] (PatchState& s)
{
    // A hot, dense lattice cooling into the dark. TEAR turns the quiet nodes
    // into detuned twins of the loud ones while the material walks METAL ->
    // VOID and the body swells, so a harsh struck grid ends as a beating hum.
    impact (s, 3 /* NOISE STRIKE */, 0.58f, 0.66f, 0.26f, 0.78f, 0.55f, 0.14f, 0.30f);
    amp (s, 0.006f, 6.0f, 0.64f, 2.40f, 0.45f);
    shape (s, 0.68f, 0.52f, 0.26f, 0.62f, 0.70f, 0.30f);
    material (s, MaterialType::Metal, MaterialType::Void, 0.05f);
    topology (s, 3 /* LATTICE */, 0.54f, 0.42f, 3457);
    matter (s, 0.94f, 0.60f, 0.48f, 0.80f);
    evolve (s, 0.0f, 0.0f, 0.06f, 0.0f, 0.44f, 0.16f, 0.0f, 0.30f, 0.48f);
    space (s, SpacePresets::Void, 0.46f, 0.76f, 0.44f, 0.40f);

    env (s, 2, 3.40f, 6.0f, 1.0f, 3.0f, 0.5f);
    lfo (s, 1, 0.23f, 5 /* SMOOTH RANDOM */, 1.0f, true, 1.0f);
    chaos (s, 1, 3 /* LORENZ */, 0.30f, 0.50f, 0.62f, 0.5f, 3499);
    macros (s, 0.55f, 0.40f, 0.50f, 0.55f);

    Routings r;
    r.uni (ModSource::Env2,     Param::evolveTear,      0.740f)
     .uni (ModSource::Env2,     Param::shapeBlend,      0.820f)
     .uni (ModSource::Env2,     Param::shapeMass,       0.480f)
     .uni (ModSource::Env2,     Param::impactBrightness, -0.520f)
     .uni (ModSource::Env2,     Param::evolveGravity,   0.300f)
     .uni (ModSource::Env2,     Param::evolveSpeed,     0.220f)
     .bi  (ModSource::LFO1,     Param::shapeDistribution, 0.110f)
     .bi  (ModSource::Chaos1,   Param::evolveScatter,   0.130f)
     .uni (ModSource::Velocity, Param::impactHardness,  0.280f)
     .uni (ModSource::NoteRandom, Param::shapeDistribution, 0.240f)
     .uni (ModSource::Macro1,   Param::evolveMotion,    0.400f)
     .uni (ModSource::Macro2,   Param::impactBrightness, 0.300f)
     .uni (ModSource::Macro3,   Param::spaceMix,        0.300f)
     .uni (ModSource::Macro4,   Param::evolveTear,      0.300f)
     .uni (ModSource::Macro4,   Param::shapeDistribution, 0.220f);
    sharedMacros (r, Param::spaceSize, Param::impactRandom);
    r.commit (s);
}});

manager.addFactory ({ "Feral Organ", "EVOLVING", { "vocal", "organic", "morphing", "resonant", "mid" }, [] (PatchState& s)
{
    // The reverse journey: a detuned, scattered cloud is dragged onto a minor
    // grid. SCATTER falls away while MAGNET rises, so noise-with-a-pitch-in-it
    // resolves into a chord the ear can name.
    wave (s, 2 /* FORMANT */, 0.42f, 0.30f, 0.10f, 4, 0.34f, 0.80f);
    amp (s, 0.28f, 3.20f, 0.80f, 2.0f, 0.5f);
    shape (s, 0.62f, 0.40f, 0.42f, 0.52f, 0.64f, 0.30f);
    material (s, MaterialType::Organic, MaterialType::String, 0.44f);
    topology (s, 1 /* RING */, 0.50f, 0.48f, 3527);
    matter (s, 0.86f, 0.62f, 0.26f, 0.78f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.05f, 0.50f, 0.62f, 0.0f, 0.34f, 0.60f);
    set (s, Param::evolveMagnetTarget, 3 /* MINOR */);
    set (s, Param::evolveScatterSeed, 41);
    space (s, SpacePresets::Nebula, 0.42f, 0.64f, 0.54f, 0.36f);

    env (s, 2, 4.40f, 6.0f, 1.0f, 3.0f, 0.6f);
    env (s, 3, 2.80f, 5.0f, 0.90f, 3.0f);
    lfo (s, 1, 0.11f, 0 /* SINE */, 1.0f, true, 1.5f);
    macros (s, 0.50f, 0.45f, 0.42f, 0.55f);

    Routings r;
    r.uni (ModSource::Env2,     Param::evolveMagnet,    0.780f)
     .uni (ModSource::Env2,     Param::evolveScatter,  -0.560f)
     .uni (ModSource::Env2,     Param::wavePosition,    0.400f)
     .uni (ModSource::Env2,     Param::shapeBlend,      0.420f)
     .uni (ModSource::Env3,     Param::waveDetune,     -0.220f)
     .uni (ModSource::Env3,     Param::shapeForm,      -0.200f)
     .bi  (ModSource::LFO1,     Param::waveMorph,       0.090f)
     .uni (ModSource::Velocity, Param::wavePosition,    0.240f)
     .bi  (ModSource::KeyTrack, Param::waveScan,        0.150f)
     .uni (ModSource::Macro1,   Param::evolveMotion,    0.360f)
     .uni (ModSource::Macro1,   Param::evolveSpeed,     0.220f)
     .uni (ModSource::Macro2,   Param::wavePosition,    0.300f)
     .uni (ModSource::Macro3,   Param::spaceMix,        0.300f)
     .uni (ModSource::Macro4,   Param::evolveMagnet,    0.300f)
     .uni (ModSource::Macro4,   Param::shapeCoupling,   0.240f);
    sharedMacros (r, Param::spaceSize, Param::waveScan);
    r.commit (s);
}});

manager.addFactory ({ "Kiln Skin", "EVOLVING", { "warm", "glassy", "morphing", "struck", "huge" }, [] (PatchState& s)
{
    // A drum head that vitrifies. The material pair walks MEMBRANE -> CRYSTAL
    // while FRACTURE opens from almost closed to wide, spraying octave copies
    // of the top bands: the tail is a different instrument from the hit.
    impact (s, 6 /* MEMBRANE HIT */, 0.40f, 0.40f, 0.26f, 0.76f, 0.50f, 0.12f, 0.26f);
    amp (s, 0.005f, 6.0f, 0.66f, 2.80f, 0.45f);
    shape (s, 0.56f, 0.36f, 0.50f, 0.46f, 0.62f, 0.26f);
    material (s, MaterialType::Membrane, MaterialType::Crystal, 0.08f);
    topology (s, 2 /* CLUSTERS */, 0.38f, 0.50f, 3559);
    matter (s, 0.94f, 0.56f, 0.56f, 0.72f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.44f, 0.12f, 0.0f, 0.18f, 0.30f);
    set (s, Param::evolveBendPivot, 0.18f);
    set (s, Param::evolveBendRange, 0.58f);
    fracture (s, 0 /* SPECTRAL */, 0.06f, 0.62f, 0.60f, 0.40f, 0.30f, 0.45f, 0.62f, 0.58f, 0.30f,
              1 /* 16 */, 3 /* 1/8 */, 8, 0.0f, 0 /* FORWARD */, 1.0f, 0.15f, 3581,
              FractureShape { 16, 0.10f, 0.58f, 0.18f, 0.48f, 0.44f, 0.74f, 0.25f, 0.88f,
                              1.0f, 0.70f, 0.62f, 1.0f, kOctaveTerrace, "XXXLXXHX", nullptr });
    space (s, SpacePresets::Orbit, 0.40f, 0.70f, 0.56f, 0.38f);

    env (s, 2, 4.20f, 6.0f, 1.0f, 4.0f, 0.6f);
    env (s, 3, 2.60f, 6.0f, 0.90f, 3.0f);
    lfo (s, 1, 0.15f, 1 /* TRIANGLE */, 1.0f, true, 2.0f);
    macros (s, 0.45f, 0.45f, 0.45f, 0.50f);

    Routings r;
    r.uni (ModSource::Env2,     Param::fractureAmount,  0.760f)
     .uni (ModSource::Env2,     Param::shapeBlend,      0.780f)
     .uni (ModSource::Env3,     Param::fractureFeedback, 0.240f)
     .uni (ModSource::Env3,     Param::impactRate,       0.180f)
     .uni (ModSource::Env2,     Param::evolveBend,       0.520f)
     .uni (ModSource::Env2,     Param::shapeForm,        0.340f)
     .uni (ModSource::Env2,     Param::shapeTension,     0.420f)
     .uni (ModSource::Env2,     Param::shapeMass,       -0.380f)
     .uni (ModSource::Env2,     Param::impactHardness,   0.460f)
     .uni (ModSource::Env3,     Param::fractureTone,     0.220f)
     .bi  (ModSource::LFO1,     Param::fractureTone,    0.090f)
     .uni (ModSource::Velocity, Param::impactBrightness, 0.280f)
     .bi  (ModSource::KeyTrack, Param::fractureDelay,  -0.160f)
     .uni (ModSource::Macro1,   Param::evolveMotion,    0.340f)
     .uni (ModSource::Macro2,   Param::fractureTone,    0.300f)
     .uni (ModSource::Macro3,   Param::spaceMix,        0.300f)
     .uni (ModSource::Macro4,   Param::fractureAmount,  0.280f)
     .uni (ModSource::Macro4,   Param::shapeBlend,      0.260f);
    sharedMacros (r, Param::fractureDecay, Param::fractureRandom);
    r.commit (s);
}});

manager.addFactory ({ "Wire Harvest", "EVOLVING", { "metallic", "granular", "morphing", "bright", "wide" }, [] (PatchState& s)
{
    // One wire cut into shards. The grain window shrinks from a third of a
    // second to a few milliseconds while the read scatter opens and MAGNET
    // lets go, so a single fused tone ends as a shower of separate events.
    sample (s, BuiltInSamples::Kind::MetalPing, 3 /* GRANULAR */, 0.02f, 0.34f, 0.86f, 0.18f, 60, 1.0f);
    amp (s, 0.06f, 5.0f, 0.80f, 2.20f, 0.45f);
    shape (s, 0.60f, 0.62f, 0.32f, 0.62f, 0.90f, 0.26f);
    material (s, MaterialType::Metal, MaterialType::Crystal, 0.20f);
    topology (s, 1 /* RING */, 0.46f, 0.54f, 3607);
    matter (s, 0.80f, 0.66f, 0.28f, 0.82f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.52f, 0.42f, 0.06f, 0.0f, 0.28f, 0.44f);
    set (s, Param::evolveMagnetTarget, 1 /* FIFTH */);
    space (s, SpacePresets::Nebula, 0.42f, 0.62f, 0.58f, 0.34f);

    env (s, 2, 4.20f, 6.0f, 1.0f, 3.0f, 0.55f);
    env (s, 3, 2.40f, 5.0f, 0.85f, 3.0f);
    lfo (s, 1, 0.12f, 0 /* SINE */, 1.0f, true, 1.5f);
    macros (s, 0.45f, 0.45f, 0.45f, 0.55f);

    Routings r;
    r.uni (ModSource::Env2,      Param::sampleGrain,   -0.800f)
     .uni (ModSource::Env2,      Param::sampleSpread,   0.680f)
     .uni (ModSource::Env2,      Param::evolveScatter,  0.340f)
     .uni (ModSource::Env2,      Param::shapeMix,      -0.240f)
     .uni (ModSource::Env3,      Param::shapeBlend,     0.600f)
     .uni (ModSource::Env3,      Param::evolveMagnet,  -0.440f)
     .bi  (ModSource::LFO1,      Param::sampleStart,    0.100f)
     .uni (ModSource::Velocity,  Param::sampleLevel,    0.180f)
     .bi  (ModSource::KeyTrack,  Param::sampleGrain,   -0.150f)
     .uni (ModSource::Macro1,    Param::evolveMotion,   0.360f)
     .uni (ModSource::Macro2,    Param::shapeExcite,    0.300f)
     .uni (ModSource::Macro3,    Param::spaceMix,       0.300f)
     .uni (ModSource::Macro4,    Param::sampleGrain,    0.280f)
     .uni (ModSource::Macro4,    Param::shapeBlend,     0.240f);
    sharedMacros (r, Param::spaceSize, Param::sampleSpread);
    r.commit (s);
}});

manager.addFactory ({ "Bone Telegraph", "EVOLVING", { "wooden", "dry", "rhythmic", "struck", "close" }, [] (PatchState& s)
{
    // A message winding down. The loop window over the knock grows from a
    // fiftieth of a second to more than a third, so the repetition slows from
    // a buzz to separate taps while the fragments thin out and fall in pitch.
    sample (s, BuiltInSamples::Kind::WoodKnock, 1 /* LOOP */, 0.0f, 0.10f, 0.24f, 0.30f, 60, 1.0f);
    set (s, Param::sampleKeytrack, 0.0f);   // the machine runs at its own rate whatever you play
    amp (s, 0.004f, 6.0f, 0.72f, 1.40f, 0.35f);
    shape (s, 0.44f, 0.30f, 0.44f, 0.48f, 0.62f, 0.34f);
    material (s, MaterialType::Wood, MaterialType::Custom, 0.24f);
    topology (s, 0 /* CHAIN */, 0.34f, 0.40f, 3671);
    matter (s, 0.90f, 0.54f, 0.52f, 0.52f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.48f, 0.14f, 0.0f, 0.20f, 0.30f);
    fracture (s, 1 /* RHYTHMIC */, 0.62f, 0.52f, 0.45f, 0.70f, 0.28f, 0.30f, 0.40f, 0.52f, 0.20f,
              1 /* 16 */, 4 /* 1/16 */, 8, 0.14f, 0 /* FORWARD */, 1.0f, 0.20f, 3673,
              FractureShape { 16, 0.06f, 0.34f, 0.10f, 0.40f, 0.34f, 0.66f, 0.20f, 0.70f,
                              1.0f, 0.80f, 0.55f, 1.0f, kFallingTerrace, "XoXoXLXH", nullptr });
    space (s, SpacePresets::Machine, 0.32f, 0.44f, 0.48f, 0.30f);

    env (s, 2, 3.60f, 6.0f, 1.0f, 3.0f, 0.5f);
    env (s, 3, 5.0f, 6.0f, 1.0f, 3.0f);
    lfo (s, 1, 0.20f, 4 /* RANDOM */, 1.0f, true, 1.0f);
    macros (s, 0.40f, 0.40f, 0.35f, 0.50f);

    Routings r;
    r.uni (ModSource::Env2,      Param::fractureProbability, -0.360f)
     .uni (ModSource::Env2,      Param::fractureTone,   -0.420f)
     .uni (ModSource::Env2,      Param::fracturePitch,  -0.320f)
     .uni (ModSource::Env2,      Param::sampleEnd,       0.560f)
     .uni (ModSource::Env2,      Param::shapeMass,       0.360f)
     .uni (ModSource::Env2,      Param::fractureDecay,   0.400f)
     .uni (ModSource::Env2,      Param::fractureFeedback, 0.300f)
     .uni (ModSource::Env3,      Param::shapeBlend,      0.560f)
     .uni (ModSource::Env3,      Param::shapeDecay,      0.320f)
     .bi  (ModSource::LFO1,      Param::fracturePitch,   0.060f)
     .uni (ModSource::Velocity,  Param::shapeStrike,     0.260f)
     .bi  (ModSource::KeyTrack,  Param::fractureTone,    0.180f)
     .uni (ModSource::Macro1,    Param::fractureEvolve,  0.380f)
     .uni (ModSource::Macro2,    Param::fractureTone,    0.300f)
     .uni (ModSource::Macro3,    Param::spaceMix,        0.300f)
     .uni (ModSource::Macro4,    Param::fractureFeedback, 0.300f)
     .uni (ModSource::Macro4,    Param::shapeSurface,    0.220f);
    sharedMacros (r, Param::fractureDecay, Param::fractureRandom);
    r.commit (s);
}});

manager.addFactory ({ "Slag Bellows", "EVOLVING", { "dirty", "metallic", "morphing", "scraped", "close" }, [] (PatchState& s)
{
    // Friction with a pitch buried in it. The two layers change places: the
    // scraping falls back while a struck sine core comes up, and CRUSH lets
    // go of the partials, so grit resolves into a clean ringing machine.
    set (s, Param::sourceMode, 1 /* LAYER */);
    set (s, Param::waveLevel, 0.0f);
    set (s, Param::dustLevel, 0.0f);
    set (s, Param::sampleLevel, 0.0f);
    impact (s, 5 /* DAMPED SINE */, 0.52f, 0.40f, 0.30f, 0.70f, 0.50f, 0.10f, 0.26f, 0.10f);
    gesture (s, 4 /* FRICTION */, 0.52f, 0.46f, 0.60f, 0.38f, 0.26f, 0.52f, 0.66f);
    amp (s, 0.05f, 5.0f, 0.74f, 1.60f, 0.45f);
    shape (s, 0.56f, 0.46f, 0.40f, 0.54f, 0.60f, 0.42f);
    material (s, MaterialType::Metal, MaterialType::Membrane, 0.42f);
    topology (s, 2 /* CLUSTERS */, 0.48f, 0.46f, 3691);
    matter (s, 0.84f, 0.56f, 0.44f, 0.60f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.48f, 0.16f, 0.52f, 0.24f, 0.36f);
    space (s, SpacePresets::Chamber, 0.34f, 0.46f, 0.52f, 0.28f);

    env (s, 2, 4.0f, 6.0f, 1.0f, 3.0f, 0.5f);
    lfo (s, 1, 0.18f, 5 /* SMOOTH RANDOM */, 1.0f, true, 1.0f);
    chaos (s, 1, 0 /* WALK */, 0.22f, 0.40f, 0.72f, 0.5f, 3697);
    macros (s, 0.45f, 0.45f, 0.35f, 0.55f);

    Routings r;
    r.uni (ModSource::Env2,      Param::gestureLevel,   -0.520f)
     .uni (ModSource::Env2,      Param::impactLevel,     0.760f)
     .uni (ModSource::Env2,      Param::evolveCrush,    -0.500f)
     .uni (ModSource::Env2,      Param::gestureRoughness, -0.340f)
     .bi  (ModSource::LFO1,      Param::gesturePressure, 0.090f)
     .bi  (ModSource::Chaos1,    Param::gestureSpeed,    0.130f)
     .uni (ModSource::Velocity,  Param::impactHardness,  0.300f)
     .uni (ModSource::NoteRandom, Param::gesturePosition, 0.220f)
     .uni (ModSource::Macro1,    Param::gestureMotion,   0.360f)
     .uni (ModSource::Macro2,    Param::impactBrightness, 0.320f)
     .uni (ModSource::Macro3,    Param::spaceMix,        0.320f)
     .uni (ModSource::Macro4,    Param::evolveCrush,     0.300f)
     .uni (ModSource::Macro4,    Param::gestureRoughness, 0.240f);
    sharedMacros (r, Param::spaceSize, Param::impactRandom);
    r.commit (s);
}});

manager.addFactory ({ "Radium Bloom", "EVOLVING", { "bright", "glassy", "evolving", "resonant", "air" }, [] (PatchState& s)
{
    // Growth upward. FRACTURE opens an octave terrace above the object and
    // feeds it back on itself while MAGNET locks the new bands to the octave
    // grid: a thin spectral tone builds a tower over its own fundamental.
    wave (s, 5 /* SPECTRAL */, 0.24f, 0.20f, 0.06f, 3, 0.14f, 0.72f, 0, 0.80f);
    amp (s, 0.40f, 5.0f, 0.82f, 2.40f, 0.55f);
    shape (s, 0.52f, 0.66f, 0.26f, 0.68f, 0.74f, 0.18f);
    material (s, MaterialType::Crystal, MaterialType::Metal, 0.30f);
    topology (s, 3 /* LATTICE */, 0.40f, 0.58f, 3719);
    matter (s, 0.84f, 0.70f, 0.22f, 0.84f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.10f, 0.40f, 0.12f, 0.0f, 0.18f, 0.30f);
    set (s, Param::evolveMagnetTarget, 0 /* OCTAVE */);
    fracture (s, 0 /* SPECTRAL */, 0.10f, 0.50f, 0.62f, 0.35f, 0.24f, 0.55f, 0.66f, 0.62f, 0.25f,
              2 /* 32 */, 2 /* 1/4 */, 8, 0.0f, 0 /* FORWARD */, 1.0f, 0.10f, 3727,
              FractureShape { 32, 0.18f, 0.70f, 0.20f, 0.56f, 0.50f, 0.80f, 0.30f, 0.92f,
                              0.90f, 0.75f, 0.72f, 1.0f, kOctaveTerrace, "XXHXXHXX", nullptr });
    space (s, SpacePresets::Shimmer, 0.44f, 0.72f, 0.62f, 0.38f);

    env (s, 2, 5.0f, 6.0f, 1.0f, 4.0f, 0.6f);
    env (s, 3, 2.60f, 6.0f, 0.90f, 3.0f);
    lfo (s, 1, 0.08f, 0 /* SINE */, 1.0f, true, 2.5f);
    macros (s, 0.40f, 0.50f, 0.50f, 0.50f);

    Routings r;
    r.uni (ModSource::Env2,      Param::fractureAmount,  0.720f)
     .uni (ModSource::Env2,      Param::fractureFeedback, 0.420f)
     .uni (ModSource::Env2,      Param::evolveMagnet,    0.480f)
     .uni (ModSource::Env3,      Param::wavePosition,    0.320f)
     .uni (ModSource::Env3,      Param::shapeDensity,    0.240f)
     .bi  (ModSource::LFO1,      Param::waveMorph,       0.070f)
     .uni (ModSource::Velocity,  Param::waveScan,        0.200f)
     .bi  (ModSource::KeyTrack,  Param::fractureDelay,  -0.180f)
     .uni (ModSource::Macro1,    Param::fractureEvolve,  0.360f)
     .uni (ModSource::Macro2,    Param::fractureTone,    0.300f)
     .uni (ModSource::Macro3,    Param::spaceMix,        0.300f)
     .uni (ModSource::Macro4,    Param::fractureAmount,  0.300f)
     .uni (ModSource::Macro4,    Param::evolveMagnet,    0.240f);
    sharedMacros (r, Param::fractureDecay, Param::fractureRandom);
    r.commit (s);
}});

manager.addFactory ({ "Peat Engine", "EVOLVING", { "dark", "dirty", "pulsing", "sub", "hollow" }, [] (PatchState& s)
{
    // Low weather that turns into machinery. CRUSH quantises the partials
    // onto a coarser and coarser grid while the mass grows, so a soft brown
    // rumble hardens into a stepped, mechanical drone under the picture.
    dust (s, 2 /* BROWN */, 0.62f, 0.12f, 0.46f, 0.24f, 0.42f, 0.50f, 3733, 0.82f);
    amp (s, 0.24f, 5.0f, 0.84f, 1.80f, 0.5f);
    shape (s, 0.48f, 0.24f, 0.62f, 0.30f, 0.62f, 0.20f);
    material (s, MaterialType::Organic, MaterialType::Wood, 0.30f);
    topology (s, 0 /* CHAIN */, 0.44f, 0.36f, 3739);
    matter (s, 0.90f, 0.44f, 0.18f, 0.48f);
    set (s, Param::shapePitch, -12.0f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.50f, 0.14f, 0.04f, 0.20f, 0.34f);
    space (s, SpacePresets::Machine, 0.30f, 0.50f, 0.40f, 0.30f);

    env (s, 2, 4.40f, 6.0f, 1.0f, 3.0f, 0.5f);
    lfo (s, 1, 0.90f, 3 /* SQUARE */, 1.0f, true, 2.0f, 0.35f);
    lfo (s, 2, 0.07f, 0 /* SINE */, 1.0f, true, 1.0f);
    macros (s, 0.45f, 0.35f, 0.35f, 0.60f);

    Routings r;
    r.uni (ModSource::Env2,      Param::evolveCrush,     0.700f)
     .uni (ModSource::Env2,      Param::shapeMass,       0.360f)
     .uni (ModSource::Env2,      Param::evolveGravity,   0.240f)
     .uni (ModSource::Env2,      Param::dustColor,       0.300f)
     .bi  (ModSource::LFO1,      Param::shapeExcite,     0.130f)
     .bi  (ModSource::LFO2,      Param::dustDensity,     0.120f)
     .uni (ModSource::Velocity,  Param::dustDensity,     0.220f)
     .bi  (ModSource::KeyTrack,  Param::dustColor,       0.200f)
     .uni (ModSource::Macro1,    Param::evolveMotion,    0.340f)
     .uni (ModSource::Macro2,    Param::dustColor,       0.320f)
     .uni (ModSource::Macro3,    Param::spaceMix,        0.300f)
     .uni (ModSource::Macro4,    Param::evolveCrush,     0.320f)
     .uni (ModSource::Macro4,    Param::shapeSurface,    0.200f);
    sharedMacros (r, Param::spaceSize, Param::dustJitter);
    r.commit (s);
}});

manager.addFactory ({ "Moth Lattice", "EVOLVING", { "soft", "scraped", "morphing", "unstable", "mid" }, [] (PatchState& s)
{
    // Something soft with something dry crawling on it. The folded tone holds
    // the note while a scrape climbs out of it and takes the foreground, and
    // FRACTURE catches the transients the scrape makes into a shifting mesh.
    set (s, Param::sourceMode, 1 /* LAYER */);
    set (s, Param::dustLevel, 0.0f);
    set (s, Param::impactLevel, 0.0f);
    set (s, Param::sampleLevel, 0.0f);
    gesture (s, 1 /* SCRAPE */, 0.40f, 0.40f, 0.52f, 0.40f, 0.30f, 0.56f, 0.14f);
    wave (s, 3 /* FOLDED */, 0.28f, 0.34f, 0.04f, 2, 0.18f, 0.66f, 0, 0.78f);
    amp (s, 0.30f, 5.0f, 0.78f, 1.80f, 0.5f);
    shape (s, 0.54f, 0.44f, 0.36f, 0.56f, 0.58f, 0.36f);
    material (s, MaterialType::Liquid, MaterialType::Crystal, 0.26f);
    topology (s, 3 /* LATTICE */, 0.42f, 0.52f, 3761);
    matter (s, 0.78f, 0.68f, 0.30f, 0.74f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.46f, 0.22f, 0.0f, 0.26f, 0.42f);
    fracture (s, 2 /* TRANSIENT */, 0.20f, 0.48f, 0.58f, 0.50f, 0.26f, 0.40f, 0.54f, 0.56f, 0.30f,
              1 /* 16 */, 4 /* 1/16 */, 8, 0.10f, 2 /* PINGPONG */, 0.90f, 0.30f, 3767,
              FractureShape { 16, 0.08f, 0.46f, 0.16f, 0.44f, 0.40f, 0.68f, 0.30f, 0.86f,
                              1.0f, 0.80f, 0.68f, 0.90f, kFifthTerrace, "XoLoXoHo", nullptr });
    space (s, SpacePresets::Dust, 0.40f, 0.58f, 0.54f, 0.34f);

    env (s, 2, 4.60f, 6.0f, 1.0f, 3.0f, 0.5f);
    lfo (s, 1, 0.26f, 5 /* SMOOTH RANDOM */, 1.0f, true, 1.0f);
    macros (s, 0.50f, 0.45f, 0.45f, 0.50f);

    Routings r;
    r.uni (ModSource::Env2,      Param::gestureLevel,    0.740f)
     .uni (ModSource::Env2,      Param::waveLevel,      -0.380f)
     .uni (ModSource::Env2,      Param::fractureAmount,  0.480f)
     .uni (ModSource::Env2,      Param::shapeBlend,      0.440f)
     .bi  (ModSource::LFO1,      Param::gesturePosition, 0.140f)
     .uni (ModSource::Velocity,  Param::gestureSpeed,    0.240f)
     .uni (ModSource::NoteRandom, Param::gestureRoughness, 0.200f)
     .uni (ModSource::Macro1,    Param::gestureMotion,   0.360f)
     .uni (ModSource::Macro2,    Param::fractureTone,    0.300f)
     .uni (ModSource::Macro3,    Param::spaceMix,        0.300f)
     .uni (ModSource::Macro4,    Param::gestureRoughness, 0.320f)
     .uni (ModSource::Macro4,    Param::fractureAmount,  0.240f);
    sharedMacros (r, Param::fractureDecay, Param::gestureSpeed);
    r.commit (s);
}});

manager.addFactory ({ "Tide Organ", "EVOLVING", { "organic", "breathing", "bowed", "warm", "roomy" }, [] (PatchState& s)
{
    // Two clocks at once: a looping envelope breathes the bow every few
    // seconds, and underneath it the material walks ORGANIC -> LIQUID and the
    // ring loosens, so each breath arrives on a softer, wetter instrument.
    gesture (s, 0 /* BOW */, 0.44f, 0.40f, 0.22f, 0.46f, 0.34f, 0.50f, 0.76f);
    amp (s, 0.50f, 5.0f, 0.80f, 2.0f, 0.55f);
    shape (s, 0.58f, 0.36f, 0.44f, 0.52f, 0.64f, 0.24f);
    material (s, MaterialType::Organic, MaterialType::Liquid, 0.16f);
    topology (s, 1 /* RING */, 0.62f, 0.46f, 3793);
    matter (s, 0.88f, 0.54f, 0.20f, 0.72f);
    evolve (s, 0.0f, 0.08f, 0.0f, 0.0f, 0.48f, 0.16f, 0.0f, 0.14f, 0.30f);
    space (s, SpacePresets::Chamber, 0.38f, 0.60f, 0.54f, 0.32f);

    env (s, 2, 5.20f, 6.0f, 1.0f, 4.0f, 0.6f);
    env (s, 3, 1.60f, 2.20f, 0.30f, 1.60f, 0.5f, true /* loop */);
    lfo (s, 1, 0.09f, 0 /* SINE */, 1.0f, true, 2.0f);
    macros (s, 0.45f, 0.40f, 0.45f, 0.50f);

    Routings r;
    r.uni (ModSource::Env2,      Param::shapeBlend,      0.760f)
     .uni (ModSource::Env2,      Param::shapeCoupling,  -0.440f)
     .uni (ModSource::Env2,      Param::evolveMelt,      0.240f)
     .uni (ModSource::Env3,      Param::gesturePressure, 0.300f)
     .uni (ModSource::Env3,      Param::gestureBandwidth, 0.260f)
     .bi  (ModSource::LFO1,      Param::gesturePosition, 0.100f)
     .uni (ModSource::Velocity,  Param::gesturePressure, 0.220f)
     .bi  (ModSource::KeyTrack,  Param::shapeDecay,     -0.160f)
     .uni (ModSource::Macro1,    Param::gestureMotion,   0.360f)
     .uni (ModSource::Macro2,    Param::gestureBandwidth, 0.300f)
     .uni (ModSource::Macro3,    Param::spaceMix,        0.320f)
     .uni (ModSource::Macro4,    Param::shapeBlend,      0.300f)
     .uni (ModSource::Macro4,    Param::evolveMelt,      0.220f);
    sharedMacros (r, Param::spaceSize, Param::gestureRoughness);
    r.commit (s);
}});

manager.addFactory ({ "Copper Winter", "EVOLVING", { "cold", "metallic", "plucked", "evolving", "distant" }, [] (PatchState& s)
{
    // Warm copper that freezes over. The plucks come further apart, the
    // FRACTURE bands drift under their own evolve clock, and the material
    // pair walks METAL -> CRYSTAL, so the object gets colder as it thins.
    impact (s, 2 /* PLUCK */, 0.56f, 0.52f, 0.22f, 0.74f, 0.45f, 0.14f, 0.34f);
    amp (s, 0.004f, 6.0f, 0.62f, 2.60f, 0.40f);
    shape (s, 0.50f, 0.58f, 0.34f, 0.62f, 0.82f, 0.22f);
    material (s, MaterialType::Metal, MaterialType::Crystal, 0.12f);
    topology (s, 5 /* STAR */, 0.36f, 0.56f, 3803);
    matter (s, 0.94f, 0.58f, 0.54f, 0.78f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.44f, 0.18f, 0.0f, 0.22f, 0.38f);
    fracture (s, 3 /* EVOLVE */, 0.44f, 0.52f, 0.66f, 0.55f, 0.30f, 0.48f, 0.60f, 0.58f, 0.55f,
              1 /* 16 */, 3 /* 1/8 */, 8, 0.06f, 3 /* RANDOM */, 0.85f, 0.35f, 3821,
              FractureShape { 16, 0.12f, 0.62f, 0.18f, 0.52f, 0.46f, 0.76f, 0.28f, 0.90f,
                              1.0f, 0.72f, 0.70f, 0.85f, kFifthTerrace, "XLXHXLXH", nullptr });
    space (s, SpacePresets::Dream, 0.46f, 0.72f, 0.58f, 0.36f);

    env (s, 2, 4.80f, 6.0f, 1.0f, 4.0f, 0.55f);
    env (s, 3, 2.20f, 6.0f, 0.90f, 3.0f);
    lfo (s, 1, 0.14f, 1 /* TRIANGLE */, 1.0f, true, 1.5f);
    macros (s, 0.50f, 0.45f, 0.50f, 0.50f);

    Routings r;
    r.uni (ModSource::Env2,      Param::shapeBlend,      0.820f)
     .uni (ModSource::Env2,      Param::impactRate,     -0.120f)
     .uni (ModSource::Env2,      Param::fractureEvolve,  0.380f)
     .uni (ModSource::Env3,      Param::shapeTension,    0.300f)
     .uni (ModSource::Env3,      Param::fractureSpread,  0.240f)
     .bi  (ModSource::LFO1,      Param::fractureDelay,   0.080f)
     .uni (ModSource::Velocity,  Param::impactHardness,  0.280f)
     .bi  (ModSource::KeyTrack,  Param::impactLength,   -0.200f)
     .uni (ModSource::Macro1,    Param::fractureEvolve,  0.320f)
     .uni (ModSource::Macro2,    Param::impactBrightness, 0.300f)
     .uni (ModSource::Macro3,    Param::spaceMix,        0.300f)
     .uni (ModSource::Macro4,    Param::shapeBlend,      0.300f)
     .uni (ModSource::Macro4,    Param::fractureSpread,  0.220f);
    sharedMacros (r, Param::fractureDecay, Param::impactRandom);
    r.commit (s);
}});

manager.addFactory ({ "Marrow Drift", "EVOLVING", { "organic", "granular", "vocal", "morphing", "huge" }, [] (PatchState& s)
{
    // Weather that turns out to have someone in it. A sparse dust cloud holds
    // the first seconds; underneath it a granular breath grows its grains and
    // its level until the formants are the foreground and the weather is gone.
    set (s, Param::sourceMode, 1 /* LAYER */);
    set (s, Param::waveLevel, 0.0f);
    set (s, Param::impactLevel, 0.0f);
    set (s, Param::gestureLevel, 0.0f);
    sample (s, BuiltInSamples::Kind::Breath, 3 /* GRANULAR */, 0.10f, 0.86f, 0.22f, 0.60f, 60, 0.16f);
    dust (s, 7 /* CLOUD */, 0.54f, 0.72f, 0.36f, 0.44f, 0.66f, 0.74f, 3847, 0.60f);
    amp (s, 0.60f, 6.0f, 0.86f, 2.60f, 0.55f);
    shape (s, 0.60f, 0.34f, 0.46f, 0.44f, 0.72f, 0.28f);
    material (s, MaterialType::Organic, MaterialType::Membrane, 0.34f);
    topology (s, 2 /* CLUSTERS */, 0.50f, 0.52f, 3851);
    matter (s, 0.72f, 0.56f, 0.16f, 0.86f);
    evolve (s, 0.0f, 0.10f, 0.0f, 0.0f, 0.52f, 0.20f, 0.0f, 0.12f, 0.34f);
    space (s, SpacePresets::Void, 0.50f, 0.84f, 0.46f, 0.42f);

    env (s, 2, 4.60f, 6.0f, 1.0f, 4.0f, 0.6f);
    lfo (s, 1, 0.07f, 0 /* SINE */, 1.0f, true, 2.0f);
    chaos (s, 1, 1 /* BROWNIAN */, 0.12f, 0.45f, 0.80f, 0.5f, 3853);
    macros (s, 0.45f, 0.40f, 0.55f, 0.50f);

    Routings r;
    r.uni (ModSource::Env2,      Param::sampleLevel,     0.700f)
     .uni (ModSource::Env2,      Param::dustLevel,      -0.520f)
     .uni (ModSource::Env2,      Param::sampleGrain,     0.420f)
     .uni (ModSource::Env2,      Param::dustDensity,    -0.300f)
     .bi  (ModSource::LFO1,      Param::sampleStart,     0.110f)
     .bi  (ModSource::Chaos1,    Param::dustColor,       0.150f)
     .uni (ModSource::Velocity,  Param::sampleSpread,    0.200f)
     .uni (ModSource::NoteRandom, Param::dustJitter,     0.220f)
     .uni (ModSource::Macro1,    Param::evolveMotion,    0.340f)
     .uni (ModSource::Macro2,    Param::dustColor,       0.320f)
     .uni (ModSource::Macro3,    Param::spaceMix,        0.300f)
     .uni (ModSource::Macro4,    Param::sampleGrain,     0.300f)
     .uni (ModSource::Macro4,    Param::evolveMelt,      0.220f);
    sharedMacros (r, Param::spaceSize, Param::sampleSpread);
    r.commit (s);
}});

manager.addFactory ({ "Salt Mirror", "EVOLVING", { "cold", "glassy", "morphing", "wide", "high" }, [] (PatchState& s)
{
    // One object pulled into two. BEND levers the partials around a pivot in
    // the middle of the spectrum: everything above it climbs almost an octave
    // while everything below sinks, and the note splits open as you hold it.
    wave (s, 0 /* BASIC */, 0.20f, 0.16f, 0.04f, 4, 0.18f, 0.78f, 0, 0.86f);
    amp (s, 0.32f, 6.0f, 0.84f, 2.20f, 0.5f);
    shape (s, 0.56f, 0.54f, 0.28f, 0.64f, 0.82f, 0.20f);
    material (s, MaterialType::Crystal, MaterialType::Void, 0.46f);
    topology (s, 4 /* RANDOM */, 0.38f, 0.58f, 3877);
    matter (s, 0.88f, 0.62f, 0.24f, 0.86f);
    evolve (s, 0.05f, 0.0f, 0.0f, 0.0f, 0.46f, 0.10f, 0.0f, 0.16f, 0.28f);
    set (s, Param::evolveBendPivot, 0.46f);
    set (s, Param::evolveBendRange, 0.90f);
    set (s, Param::evolveBendCurve, 0.62f);
    space (s, SpacePresets::Orbit, 0.42f, 0.68f, 0.58f, 0.36f);

    env (s, 2, 5.0f, 6.0f, 1.0f, 4.0f, 0.6f);
    env (s, 3, 2.80f, 6.0f, 0.90f, 3.0f);
    lfo (s, 1, 0.10f, 0 /* SINE */, 1.0f, true, 2.0f);
    macros (s, 0.40f, 0.45f, 0.45f, 0.55f);

    Routings r;
    r.uni (ModSource::Env2,      Param::evolveBend,      0.840f)
     .uni (ModSource::Env2,      Param::shapeDensity,    0.260f)
     .uni (ModSource::Env3,      Param::evolveBendPivot, 0.220f)
     .uni (ModSource::Env3,      Param::waveDetune,      0.240f)
     .bi  (ModSource::LFO1,      Param::wavePosition,    0.080f)
     .uni (ModSource::Velocity,  Param::waveMorph,       0.220f)
     .bi  (ModSource::KeyTrack,  Param::evolveBendPivot, -0.180f)
     .uni (ModSource::Macro1,    Param::evolveMotion,    0.360f)
     .uni (ModSource::Macro2,    Param::wavePosition,    0.300f)
     .uni (ModSource::Macro3,    Param::spaceMix,        0.300f)
     .uni (ModSource::Macro4,    Param::evolveBend,      0.300f)
     .uni (ModSource::Macro4,    Param::evolveBendRange, 0.240f);
    sharedMacros (r, Param::spaceSize, Param::evolveBendCurve);
    r.commit (s);
}});

manager.addFactory ({ "Thaw Machine", "EVOLVING", { "cold", "metallic", "unstable", "noisy", "drift" }, [] (PatchState& s)
{
    // A frozen additive texture coming loose. The jitter and the grain of the
    // frozen dust rise while MAGNET lets go of the grid and the material turns
    // to liquid, so a rigid ice tone slumps into something wet and wandering.
    dust (s, 8 /* FROZEN */, 0.46f, 0.64f, 0.16f, 0.04f, 0.50f, 0.62f, 3907, 0.86f);
    amp (s, 0.20f, 6.0f, 0.88f, 2.0f, 0.5f);
    shape (s, 0.54f, 0.60f, 0.32f, 0.62f, 0.78f, 0.18f);
    material (s, MaterialType::Metal, MaterialType::Liquid, 0.12f);
    topology (s, 1 /* RING */, 0.44f, 0.50f, 3911);
    matter (s, 0.84f, 0.58f, 0.20f, 0.80f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.66f, 0.18f, 0.06f, 0.0f, 0.20f, 0.32f);
    set (s, Param::evolveMagnetTarget, 4 /* CHROMATIC */);
    space (s, SpacePresets::Nebula, 0.44f, 0.66f, 0.52f, 0.38f);

    env (s, 2, 4.80f, 6.0f, 1.0f, 3.0f, 0.55f);
    lfo (s, 1, 0.13f, 5 /* SMOOTH RANDOM */, 1.0f, true, 1.5f);
    chaos (s, 1, 2 /* LOGISTIC */, 0.26f, 0.50f, 0.55f, 0.5f, 3917);
    macros (s, 0.50f, 0.40f, 0.45f, 0.55f);

    Routings r;
    r.uni (ModSource::Env2,      Param::evolveMagnet,   -0.600f)
     .uni (ModSource::Env2,      Param::evolveGravity,   0.760f)
     .uni (ModSource::Env2,      Param::shapeBlend,      0.780f)
     .uni (ModSource::Env2,      Param::shapeMix,       -0.260f)
     .uni (ModSource::Env2,      Param::dustJitter,      0.640f)
     .uni (ModSource::Env2,      Param::dustGrain,       0.240f)
     .uni (ModSource::Env2,      Param::shapeSurface,   -0.120f)
     .bi  (ModSource::LFO1,      Param::dustColor,       0.120f)
     .bi  (ModSource::Chaos1,    Param::dustPitch,       0.060f)
     .uni (ModSource::Velocity,  Param::dustDensity,     0.220f)
     .bi  (ModSource::KeyTrack,  Param::dustColor,       0.160f)
     .uni (ModSource::Macro1,    Param::evolveMotion,    0.360f)
     .uni (ModSource::Macro2,    Param::dustColor,       0.320f)
     .uni (ModSource::Macro3,    Param::spaceMix,        0.300f)
     .uni (ModSource::Macro4,    Param::dustJitter,      0.300f)
     .uni (ModSource::Macro4,    Param::shapeBlend,      0.240f);
    sharedMacros (r, Param::spaceSize, Param::dustStereo);
    r.commit (s);
}});

manager.addFactory ({ "Vellum Throat", "EVOLVING", { "vocal", "warm", "scraped", "morphing", "roomy" }, [] (PatchState& s)
{
    // A rubbed skin that finds a voice. The contact point walks along the
    // surface while FRACTURE opens a minor terrace over the top of it, so a
    // dull rub grows formants and ends singing a chord it did not start with.
    gesture (s, 2 /* RUB */, 0.46f, 0.30f, 0.44f, 0.20f, 0.28f, 0.34f, 0.78f);
    amp (s, 0.36f, 6.0f, 0.82f, 2.0f, 0.55f);
    shape (s, 0.58f, 0.30f, 0.44f, 0.50f, 0.74f, 0.30f);
    material (s, MaterialType::Membrane, MaterialType::Organic, 0.30f);
    topology (s, 2 /* CLUSTERS */, 0.46f, 0.54f, 3919);
    matter (s, 0.86f, 0.58f, 0.18f, 0.78f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.20f, 0.44f, 0.14f, 0.0f, 0.16f, 0.32f);
    set (s, Param::evolveMagnetTarget, 3 /* MINOR */);
    fracture (s, 0 /* SPECTRAL */, 0.08f, 0.46f, 0.58f, 0.42f, 0.28f, 0.52f, 0.62f, 0.60f, 0.28f,
              1 /* 16 */, 2 /* 1/4 */, 8, 0.0f, 0 /* FORWARD */, 1.0f, 0.18f, 3923,
              FractureShape { 16, 0.14f, 0.62f, 0.18f, 0.50f, 0.48f, 0.76f, 0.28f, 0.88f,
                              1.0f, 0.82f, 0.66f, 1.0f, kMinorTerrace, "XXLXXHXX", nullptr });
    space (s, SpacePresets::Dream, 0.46f, 0.70f, 0.58f, 0.36f);

    env (s, 2, 4.40f, 6.0f, 1.0f, 4.0f, 0.6f);
    env (s, 3, 2.40f, 6.0f, 0.90f, 3.0f);
    lfo (s, 1, 0.08f, 0 /* SINE */, 1.0f, true, 2.0f);
    macros (s, 0.45f, 0.45f, 0.50f, 0.50f);

    Routings r;
    r.uni (ModSource::Env2,      Param::fractureAmount,  0.700f)
     .uni (ModSource::Env2,      Param::gesturePosition, 0.560f)
     .uni (ModSource::Env2,      Param::evolveMagnet,    0.380f)
     .uni (ModSource::Env3,      Param::gestureBandwidth, 0.420f)
     .uni (ModSource::Env3,      Param::shapeBlend,      0.400f)
     .bi  (ModSource::LFO1,      Param::gesturePressure, 0.090f)
     .uni (ModSource::Velocity,  Param::gestureSpeed,    0.240f)
     .bi  (ModSource::KeyTrack,  Param::gesturePosition, 0.180f)
     .uni (ModSource::Macro1,    Param::gestureMotion,   0.360f)
     .uni (ModSource::Macro2,    Param::fractureTone,    0.300f)
     .uni (ModSource::Macro3,    Param::spaceMix,        0.300f)
     .uni (ModSource::Macro4,    Param::fractureAmount,  0.300f)
     .uni (ModSource::Macro4,    Param::gestureRoughness, 0.240f);
    sharedMacros (r, Param::fractureDecay, Param::gestureRoughness);
    r.commit (s);
}});

manager.addFactory ({ "Ferric Swell", "EVOLVING", { "metallic", "resonant", "evolving", "struck", "huge" }, [] (PatchState& s)
{
    // The opposite of a bridge tightening: one welded body comes apart. The
    // coupling falls from nearly full to almost nothing while the strikes get
    // harder, so a single fused ring separates into individual iron strings.
    impact (s, 4 /* METAL STRIKE */, 0.44f, 0.38f, 0.28f, 0.72f, 0.50f, 0.12f, 0.26f);
    amp (s, 0.004f, 6.0f, 0.70f, 3.0f, 0.45f);
    shape (s, 0.62f, 0.48f, 0.40f, 0.50f, 0.82f, 0.24f);
    material (s, MaterialType::Metal, MaterialType::String, 0.30f);
    topology (s, 3 /* LATTICE */, 0.92f, 0.46f, 3929);
    matter (s, 0.94f, 0.56f, 0.50f, 0.82f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.46f, 0.14f, 0.0f, 0.18f, 0.32f);
    space (s, SpacePresets::Void, 0.48f, 0.82f, 0.50f, 0.42f);

    env (s, 2, 4.20f, 6.0f, 1.0f, 4.0f, 0.55f);
    env (s, 3, 2.0f, 6.0f, 0.90f, 3.0f);
    lfo (s, 1, 0.11f, 1 /* TRIANGLE */, 1.0f, true, 1.5f);
    macros (s, 0.45f, 0.45f, 0.55f, 0.50f);

    Routings r;
    r.uni (ModSource::Env2,      Param::shapeCoupling,  -0.840f)
     .uni (ModSource::Env2,      Param::impactHardness,  0.420f)
     .uni (ModSource::Env2,      Param::shapeMass,      -0.300f)
     .uni (ModSource::Env3,      Param::shapeDistribution, 0.320f)
     .uni (ModSource::Env3,      Param::impactBrightness, 0.360f)
     .bi  (ModSource::LFO1,      Param::shapeStereo,     0.100f)
     .uni (ModSource::Velocity,  Param::impactHardness,  0.260f)
     .bi  (ModSource::KeyTrack,  Param::shapeCoupling,  -0.160f)
     .uni (ModSource::Macro1,    Param::evolveMotion,    0.340f)
     .uni (ModSource::Macro2,    Param::impactBrightness, 0.300f)
     .uni (ModSource::Macro3,    Param::spaceMix,        0.320f)
     .uni (ModSource::Macro4,    Param::shapeCoupling,   0.300f)
     .uni (ModSource::Macro4,    Param::shapeDistribution, 0.220f);
    sharedMacros (r, Param::spaceSize, Param::impactRandom);
    r.commit (s);
}});

manager.addFactory ({ "Lantern Tide", "EVOLVING", { "glassy", "bright", "drift", "air", "distant" }, [] (PatchState& s)
{
    // The tide going out from under a lantern. GRAVITY tips from the bottom of
    // the object to the top, so the body and its ring time drain away and the
    // note is left standing on its own lit upper partials, two octaves up.
    sample (s, BuiltInSamples::Kind::GlassStrike, 1 /* LOOP */, 0.0f, 0.24f, 0.24f, 0.44f, 60, 0.94f);
    amp (s, 0.44f, 6.0f, 0.84f, 3.0f, 0.55f);
    shape (s, 0.50f, 0.68f, 0.34f, 0.70f, 0.84f, 0.16f);
    material (s, MaterialType::Crystal, MaterialType::Membrane, 0.30f);
    topology (s, 5 /* STAR */, 0.32f, 0.60f, 3931);
    matter (s, 0.86f, 0.60f, 0.22f, 0.84f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.58f, 0.10f, 0.0f, 0.12f, 0.26f);
    space (s, SpacePresets::Shimmer, 0.48f, 0.78f, 0.62f, 0.38f);

    env (s, 2, 5.20f, 6.0f, 1.0f, 4.0f, 0.6f);
    env (s, 3, 3.0f, 6.0f, 0.90f, 4.0f);
    lfo (s, 1, 0.06f, 0 /* SINE */, 1.0f, true, 2.5f);
    macros (s, 0.35f, 0.50f, 0.55f, 0.50f);

    Routings r;
    r.uni (ModSource::Env2,      Param::evolveGravity,  -0.480f)
     .uni (ModSource::Env2,      Param::shapeMass,      -0.300f)
     .uni (ModSource::Env2,      Param::shapeBlend,     -0.280f)
     .uni (ModSource::Env3,      Param::sampleEnd,      -0.160f)
     .uni (ModSource::Env3,      Param::shapeSurface,    0.180f)
     .bi  (ModSource::LFO1,      Param::sampleSpread,    0.120f)
     .uni (ModSource::Velocity,  Param::sampleLevel,     0.180f)
     .bi  (ModSource::KeyTrack,  Param::shapeDecay,     -0.140f)
     .uni (ModSource::Macro1,    Param::evolveMotion,    0.340f)
     .uni (ModSource::Macro2,    Param::shapeExcite,     0.300f)
     .uni (ModSource::Macro3,    Param::spaceMix,        0.320f)
     .uni (ModSource::Macro4,    Param::evolveGravity,  -0.280f)
     .uni (ModSource::Macro4,    Param::shapeBlend,      0.300f);
    sharedMacros (r, Param::spaceSize, Param::sampleGrain);
    r.commit (s);
}});
}

} // namespace am::FactoryContent
