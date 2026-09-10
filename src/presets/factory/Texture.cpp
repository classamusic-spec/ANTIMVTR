#include "presets/FactoryBuilders.h"

namespace am::FactoryContent
{

void registerTexture (PresetManager& manager)
{
//==========================================================================
// TEXTURE — surfaces, not notes
//==========================================================================

manager.addFactory ({ "Titanium Skin", "TEXTURE", { "texture", "metal", "scrape", "gesture" }, [] (PatchState& s)
{
    gesture (s, 1 /* SCRAPE */, 0.55f, 0.62f, 0.68f, 0.42f, 0.35f, 0.62f);
    amp (s, 0.35f, 1.20f, 0.80f, 1.60f, 0.55f);
    shape (s, 0.62f, 0.54f, 0.42f, 0.60f, 0.62f, 0.55f);
    material (s, MaterialType::Metal, MaterialType::Membrane, 0.35f);
    topology (s, 3 /* LATTICE */, 0.62f, 0.48f, 173);
    matter (s, 0.96f, 0.72f, 0.25f, 0.85f);
    evolve (s, 0.0f, 0.0f, 0.30f, 0.0f, 0.52f, 0.34f, 0.0f, 0.35f, 0.45f);
    set (s, Param::evolveScatterSeed, 421);
    space (s, SpacePresets::Dust, 0.42f, 0.58f, 0.50f, 0.35f);

    lfo (s, 1, 0.34f, 5 /* SMOOTH RANDOM */, 1.0f, false);
    lfo (s, 2, 0.19f, 1 /* TRIANGLE */, 1.0f, false, 1.0f);
    chaos (s, 1, 3 /* LORENZ */, 0.55f, 0.60f, 0.65f, 0.5f, 907);
    env (s, 2, 1.20f, 3.0f, 0.55f, 2.5f);
    macros (s, 0.50f, 0.45f, 0.40f, 0.55f);

    Routings r;
    r.bi  (ModSource::LFO1,   Param::gestureSpeed,   0.180f)
     .bi  (ModSource::LFO2,   Param::gesturePosition, 0.160f)
     .bi  (ModSource::Chaos1, Param::evolveScatter,  0.140f)
     .uni (ModSource::Env2,   Param::evolveTear,     0.200f)
     .uni (ModSource::Velocity, Param::gesturePressure, 0.250f)
     .uni (ModSource::Macro1, Param::gestureMotion,  0.450f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.300f)
     .uni (ModSource::Macro2, Param::gestureBandwidth, 0.300f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.300f)
     .uni (ModSource::Macro4, Param::gestureRoughness, 0.400f)
     .uni (ModSource::Macro4, Param::shapeSurface,   0.250f);
    sharedMacros (r, Param::spaceSize, Param::gestureRoughness);
    r.commit (s);
}});

manager.addFactory ({ "Glass Creature", "TEXTURE", { "texture", "granular", "glass", "alive" }, [] (PatchState& s)
{
    sample (s, BuiltInSamples::Kind::GlassStrike, 3 /* GRANULAR */, 0.02f, 0.85f, 0.34f, 0.62f);
    amp (s, 0.25f, 1.50f, 0.72f, 1.80f, 0.55f);
    shape (s, 0.58f, 0.86f, 0.30f, 0.64f, 0.66f, 0.28f);
    material (s, MaterialType::Crystal, MaterialType::Liquid, 0.40f);
    topology (s, 2 /* CLUSTERS */, 0.46f, 0.55f, 233);
    matter (s, 0.88f, 0.66f, 0.30f, 0.82f);
    evolve (s, 0.0f, 0.20f, 0.24f, 0.30f, 0.48f, 0.30f, 0.0f, 0.42f, 0.40f);
    set (s, Param::evolveScatterSeed, 137);
    set (s, Param::evolveMagnetTarget, 5 /* SCALE */);
    space (s, SpacePresets::Dust, 0.44f, 0.55f, 0.58f, 0.38f);

    lfo (s, 1, 0.46f, 5 /* SMOOTH RANDOM */, 1.0f, true);
    chaos (s, 1, 4 /* TARGETS */, 1.10f, 0.55f, 0.55f, 0.5f, 373);
    env (s, 2, 0.80f, 2.20f, 0.50f, 2.0f);
    macros (s, 0.50f, 0.45f, 0.40f, 0.45f);

    Routings r;
    r.bi  (ModSource::LFO1,   Param::sampleGrain,    0.150f)
     .bi  (ModSource::Chaos1, Param::sampleStart,    0.120f)
     .uni (ModSource::Env2,   Param::evolveTear,     0.180f)
     .uni (ModSource::Velocity, Param::sampleSpread, 0.250f)
     .bi  (ModSource::NoteRandom, Param::samplePitch, 0.030f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.400f)
     .uni (ModSource::Macro1, Param::evolveScatter,  0.250f)
     .uni (ModSource::Macro2, Param::shapeExcite,    0.300f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.300f)
     .uni (ModSource::Macro4, Param::sampleGrain,    0.350f)
     .uni (ModSource::Macro4, Param::sampleSpread,   0.250f);
    sharedMacros (r, Param::spaceSize, Param::sampleSpread);
    r.commit (s);
}});

manager.addFactory ({ "Organic Circuit", "TEXTURE", { "texture", "electric", "restless", "granular" }, [] (PatchState& s)
{
    gesture (s, 5 /* ELECTRICAL */, 0.48f, 0.66f, 0.55f, 0.30f, 0.45f, 0.55f);
    amp (s, 0.10f, 0.90f, 0.75f, 1.10f, 0.5f);
    shape (s, 0.66f, 0.68f, 0.36f, 0.56f, 0.54f, 0.48f);
    material (s, MaterialType::Organic, MaterialType::Metal, 0.45f);
    topology (s, 4 /* RANDOM */, 0.58f, 0.45f, 283);
    matter (s, 0.90f, 0.70f, 0.35f, 0.78f);
    evolve (s, 0.0f, 0.24f, 0.0f, 0.20f, 0.55f, 0.42f, 0.14f, 0.55f, 0.50f);
    set (s, Param::evolveScatterSeed, 787);
    fracture (s, 3 /* EVOLVE */, 0.42f, 0.55f, 0.45f, 0.50f, 0.30f, 0.35f, 0.50f, 0.55f, 0.55f,
              1 /* 16 */, 4 /* 1/16 */, 8, 0.0f, 0 /* FORWARD */, 0.85f, 0.35f, 421,
              FractureShape { 16, 0.05f, 0.35f, 0.15f, 0.45f, 0.40f, 0.65f, 0.25f, 0.85f,
                              1.0f, 1.0f, 0.6f, 1.0f, kFifthTerrace, "XoXoXoXo", nullptr });
    space (s, SpacePresets::Machine, 0.32f, 0.40f, 0.52f, 0.35f);

    lfo (s, 1, 0.72f, 5 /* SMOOTH RANDOM */, 1.0f, true);
    chaos (s, 1, 2 /* LOGISTIC */, 2.60f, 0.55f, 0.62f, 0.5f, 541);
    chaos (s, 2, 0 /* WALK */, 0.35f, 0.45f, 0.80f, 0.5f, 653);
    macros (s, 0.55f, 0.45f, 0.30f, 0.50f);

    Routings r;
    r.bi  (ModSource::LFO1,   Param::gestureRoughness, 0.180f)
     .bi  (ModSource::Chaos1, Param::evolveScatter,  0.160f)
     .bi  (ModSource::Chaos2, Param::fractureSpread, 0.180f)
     .uni (ModSource::Velocity, Param::gestureSpeed, 0.250f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.400f)
     .uni (ModSource::Macro1, Param::gestureMotion,  0.300f)
     .uni (ModSource::Macro2, Param::gestureBandwidth, 0.300f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.280f)
     .uni (ModSource::Macro4, Param::fractureAmount, 0.350f)
     .uni (ModSource::Macro4, Param::evolveCrush,    0.200f);
    sharedMacros (r, Param::fractureDecay, Param::fractureRandom);
    r.commit (s);
}});

manager.addFactory ({ "Rain Membrane", "TEXTURE", { "organic", "granular", "breathing", "roomy", "mid" }, [] (PatchState& s)
{
    dust (s, 6 /* IMPULSE */, 0.34f, 0.44f, 0.36f, 0.66f, 0.70f, 0.78f, 4409);
    amp (s, 0.30f, 1.80f, 0.72f, 2.20f, 0.55f);
    shape (s, 0.56f, 0.30f, 0.52f, 0.42f, 0.66f, 0.38f);
    material (s, MaterialType::Membrane, MaterialType::Wood, 0.38f);
    topology (s, 3 /* LATTICE */, 0.56f, 0.50f, 4409);
    matter (s, 0.96f, 0.72f, 0.18f, 0.86f, 0.45f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.52f, 0.32f, 0.0f, 0.32f, 0.46f);
    set (s, Param::evolveScatterSeed, 4409);
    space (s, SpacePresets::Dust, 0.44f, 0.56f, 0.44f, 0.34f);

    lfo (s, 1, 0.42f, 5 /* SMOOTH RANDOM */, 1.0f, false);
    lfo (s, 2, 0.13f, 1 /* TRIANGLE */, 1.0f, false);
    chaos (s, 1, 0 /* WALK */, 0.90f, 0.50f, 0.60f, 0.5f, 4409);
    env (s, 2, 0.90f, 1.60f, 0.35f, 1.40f, 0.5f, true);
    macros (s, 0.50f, 0.40f, 0.45f, 0.50f);

    Routings r;
    r.bi  (ModSource::LFO1,       Param::dustDensity,   0.220f)
     .bi  (ModSource::LFO2,       Param::dustColor,     0.180f)
     .bi  (ModSource::Chaos1,     Param::dustJitter,    0.200f)
     .uni (ModSource::Env2,       Param::dustDensity,   0.240f)
     .uni (ModSource::Velocity,   Param::dustDensity,   0.180f)
     .bi  (ModSource::NoteRandom, Param::dustSeed,      0.200f)
     .bi  (ModSource::KeyTrack,   Param::dustColor,    -0.160f)
     .uni (ModSource::Macro1,     Param::lfo1Depth,     0.450f)
     .uni (ModSource::Macro1,     Param::evolveMotion,  0.300f)
     .uni (ModSource::Macro2,     Param::dustColor,     0.320f)
     .uni (ModSource::Macro3,     Param::spaceMix,      0.300f)
     .uni (ModSource::Macro4,     Param::shapeSurface,  0.300f)
     .uni (ModSource::Macro4,     Param::dustGrain,     0.280f);
    sharedMacros (r, Param::spaceSize, Param::dustJitter);
    r.commit (s);
}});

manager.addFactory ({ "Rust Bloom", "TEXTURE", { "metallic", "dirty", "evolving", "scraped", "mid" }, [] (PatchState& s)
{
    gesture (s, 4 /* FRICTION */, 0.46f, 0.34f, 0.64f, 0.52f, 0.42f, 0.56f);
    amp (s, 0.55f, 2.40f, 0.76f, 2.00f, 0.55f);
    shape (s, 0.60f, 0.46f, 0.44f, 0.52f, 0.62f, 0.52f);
    material (s, MaterialType::Metal, MaterialType::Organic, 0.42f);
    topology (s, 4 /* RANDOM */, 0.60f, 0.44f, 5023);
    matter (s, 0.94f, 0.70f, 0.20f, 0.72f, 0.40f);
    evolve (s, 0.0f, 0.34f, 0.26f, 0.0f, 0.54f, 0.28f, 0.0f, 0.26f, 0.52f);
    set (s, Param::evolveScatterSeed, 5023);
    space (s, SpacePresets::Machine, 0.38f, 0.46f, 0.42f, 0.36f);

    lfo (s, 1, 0.28f, 5 /* SMOOTH RANDOM */, 1.0f, false);
    lfo (s, 2, 0.62f, 1 /* TRIANGLE */, 1.0f, false);
    chaos (s, 1, 1 /* BROWNIAN */, 0.45f, 0.55f, 0.55f, 0.5f, 5023);
    env (s, 2, 1.20f, 3.00f, 0.70f, 2.00f, 0.6f);
    macros (s, 0.55f, 0.40f, 0.35f, 0.55f);

    Routings r;
    r.bi  (ModSource::LFO1,      Param::gestureRoughness, 0.200f)
     .bi  (ModSource::LFO2,      Param::gesturePressure,  0.150f)
     .bi  (ModSource::Chaos1,    Param::gestureSpeed,     0.180f)
     .uni (ModSource::Env2,      Param::evolveMelt,       0.300f)
     .uni (ModSource::Env2,      Param::evolveTear,       0.220f)
     .uni (ModSource::Velocity,  Param::gesturePressure,  0.240f)
     .bi  (ModSource::KeyTrack,  Param::gestureBandwidth, 0.180f)
     .uni (ModSource::Macro1,    Param::gestureMotion,    0.420f)
     .uni (ModSource::Macro1,    Param::evolveMotion,     0.300f)
     .uni (ModSource::Macro2,    Param::gestureBandwidth, 0.320f)
     .uni (ModSource::Macro3,    Param::spaceMix,         0.300f)
     .uni (ModSource::Macro4,    Param::evolveTear,       0.340f)
     .uni (ModSource::Macro4,    Param::gestureRoughness, 0.260f);
    sharedMacros (r, Param::spaceSize, Param::evolveMelt);
    r.commit (s);
}});

manager.addFactory ({ "Cavern Wind", "TEXTURE", { "dark", "hollow", "huge", "low", "drift" }, [] (PatchState& s)
{
    dust (s, 2 /* BROWN */, 0.72f, 0.30f, 0.48f, 0.36f, 0.82f, 0.88f, 5501);
    amp (s, 0.80f, 2.60f, 0.86f, 3.00f, 0.6f);
    shape (s, 0.48f, 0.20f, 0.74f, 0.36f, 0.72f, 0.28f);
    material (s, MaterialType::Void, MaterialType::Wood, 0.40f);
    topology (s, 0 /* CHAIN */, 0.42f, 0.60f, 5501);
    matter (s, 0.90f, 0.66f, 0.16f, 0.78f, 0.30f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.62f, 0.22f, 0.0f, 0.18f, 0.44f);
    set (s, Param::evolveScatterSeed, 5501);
    space (s, SpacePresets::Void, 0.52f, 0.82f, 0.32f, 0.42f);

    lfo (s, 1, 0.24f, 5 /* SMOOTH RANDOM */, 1.0f, false);
    lfo (s, 2, 0.09f, 0 /* SINE */, 1.0f, false);
    lfo (s, 3, 0.55f, 1 /* TRIANGLE */, 1.0f, false);
    env (s, 2, 1.40f, 2.80f, 0.55f, 2.40f, 0.6f, true);
    macros (s, 0.50f, 0.35f, 0.55f, 0.45f);

    Routings r;
    r.bi  (ModSource::LFO1,      Param::dustColor,      0.240f)
     .bi  (ModSource::LFO2,      Param::spaceSize,      0.140f)
     .bi  (ModSource::LFO3,      Param::dustDensity,    0.180f)
     .uni (ModSource::Env2,      Param::dustGrain,      0.260f)
     .uni (ModSource::Velocity,  Param::dustDensity,    0.160f)
     .bi  (ModSource::KeyTrack,  Param::shapeMass,     -0.200f)
     .uni (ModSource::Macro1,    Param::lfo1Depth,      0.480f)
     .uni (ModSource::Macro1,    Param::evolveMotion,   0.320f)
     .uni (ModSource::Macro2,    Param::dustColor,      0.340f)
     .uni (ModSource::Macro2,    Param::spaceTone,      0.260f)
     .uni (ModSource::Macro3,    Param::spaceMix,       0.280f)
     .uni (ModSource::Macro4,    Param::shapeSurface,   0.280f)
     .uni (ModSource::Macro4,    Param::dustGrain,      0.300f);
    sharedMacros (r, Param::spaceSize, Param::dustJitter);
    r.commit (s);
}});

manager.addFactory ({ "Gravel Weather", "TEXTURE", { "dirty", "granular", "chaotic", "wide", "noisy" }, [] (PatchState& s)
{
    sample (s, BuiltInSamples::Kind::StoneDrop, 3 /* GRANULAR */, 0.02f, 0.92f, 0.46f, 0.74f, 60);
    amp (s, 0.35f, 1.90f, 0.74f, 1.80f, 0.5f);
    shape (s, 0.64f, 0.52f, 0.46f, 0.48f, 0.58f, 0.46f);
    material (s, MaterialType::Wood, MaterialType::Membrane, 0.46f);
    topology (s, 2 /* CLUSTERS */, 0.50f, 0.42f, 6089);
    matter (s, 0.88f, 0.68f, 0.22f, 0.80f, 0.35f);
    evolve (s, 0.0f, 0.0f, 0.20f, 0.0f, 0.50f, 0.36f, 0.0f, 0.44f, 0.48f);
    set (s, Param::evolveScatterSeed, 6089);
    fracture (s, 0 /* SPECTRAL */, 0.44f, 0.42f, 0.62f, 0.40f, 0.28f, 0.42f, 0.56f, 0.44f, 0.40f,
              1 /* 16 */, 3 /* 1/8 */, 8, 0.0f, 3 /* RANDOM */, 0.60f, 0.45f, 6089,
              FractureShape { 16, 0.08f, 0.48f, 0.12f, 0.40f, 0.42f, 0.66f, 0.30f, 0.95f,
                              1.0f, 0.85f, 0.70f, 0.60f, kFallingTerrace, "XLoH", nullptr });
    space (s, SpacePresets::Dust, 0.46f, 0.58f, 0.48f, 0.38f);

    lfo (s, 1, 0.66f, 5 /* SMOOTH RANDOM */, 1.0f, false);
    chaos (s, 1, 4 /* TARGETS */, 1.40f, 0.60f, 0.50f, 0.5f, 6089);
    chaos (s, 2, 0 /* WALK */, 0.30f, 0.45f, 0.70f, 0.5f, 6091);
    macros (s, 0.55f, 0.45f, 0.45f, 0.50f);

    Routings r;
    r.bi  (ModSource::LFO1,       Param::sampleGrain,      0.200f)
     .bi  (ModSource::Chaos1,     Param::sampleStart,      0.180f)
     .bi  (ModSource::Chaos2,     Param::fractureSpread,   0.200f)
     .uni (ModSource::Velocity,   Param::sampleSpread,     0.220f)
     .bi  (ModSource::NoteRandom, Param::samplePitch,      0.120f)
     .bi  (ModSource::KeyTrack,   Param::sampleGrain,     -0.150f)
     .uni (ModSource::Macro1,     Param::evolveMotion,     0.400f)
     .uni (ModSource::Macro1,     Param::evolveScatter,    0.280f)
     .uni (ModSource::Macro2,     Param::fractureTone,     0.300f)
     .uni (ModSource::Macro2,     Param::shapeExcite,      0.240f)
     .uni (ModSource::Macro3,     Param::spaceMix,         0.320f)
     .uni (ModSource::Macro4,     Param::sampleGrain,      0.360f)
     .uni (ModSource::Macro4,     Param::fractureAmount,   0.260f);
    sharedMacros (r, Param::fractureDecay, Param::fractureProbability);
    r.commit (s);
}});

manager.addFactory ({ "Slate Scrape", "TEXTURE", { "wooden", "scraped", "dry", "close", "mid" }, [] (PatchState& s)
{
    gesture (s, 1 /* SCRAPE */, 0.50f, 0.44f, 0.58f, 0.62f, 0.30f, 0.42f);
    amp (s, 0.18f, 1.40f, 0.70f, 1.00f, 0.5f);
    shape (s, 0.52f, 0.36f, 0.56f, 0.44f, 0.54f, 0.58f);
    material (s, MaterialType::Wood, MaterialType::String, 0.36f);
    topology (s, 2 /* CLUSTERS */, 0.48f, 0.46f, 6553);
    matter (s, 0.92f, 0.74f, 0.24f, 0.58f, 0.50f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.48f, 0.26f, 0.0f, 0.48f, 0.42f);
    set (s, Param::evolveScatterSeed, 6553);
    space (s, SpacePresets::Chamber, 0.24f, 0.30f, 0.46f, 0.24f);

    lfo (s, 1, 0.78f, 5 /* SMOOTH RANDOM */, 1.0f, true);
    lfo (s, 2, 0.21f, 2 /* SAW */, 1.0f, false);
    chaos (s, 1, 2 /* LOGISTIC */, 1.80f, 0.45f, 0.62f, 0.5f, 6553);
    env (s, 2, 0.60f, 1.20f, 0.30f, 0.90f, 0.5f, true);
    macros (s, 0.55f, 0.40f, 0.25f, 0.55f);

    Routings r;
    r.bi  (ModSource::LFO1,      Param::gestureSpeed,     0.240f)
     .bi  (ModSource::LFO2,      Param::gesturePosition,  0.200f)
     .bi  (ModSource::Chaos1,    Param::gestureRoughness, 0.180f)
     .uni (ModSource::Env2,      Param::gesturePressure,  0.260f)
     .uni (ModSource::Velocity,  Param::gestureSpeed,     0.240f)
     .bi  (ModSource::KeyTrack,  Param::gesturePosition, -0.180f)
     .uni (ModSource::Macro1,    Param::gestureMotion,    0.440f)
     .uni (ModSource::Macro1,    Param::lfo1Rate,         0.120f)
     .uni (ModSource::Macro2,    Param::gestureBandwidth, 0.320f)
     .uni (ModSource::Macro3,    Param::spaceMix,         0.320f)
     .uni (ModSource::Macro4,    Param::gestureRoughness, 0.380f)
     .uni (ModSource::Macro4,    Param::shapeSurface,     0.240f);
    sharedMacros (r, Param::spaceSize, Param::gestureRoughness);
    r.commit (s);
}});

manager.addFactory ({ "Frozen Choir", "TEXTURE", { "glassy", "cold", "drift", "air", "wide" }, [] (PatchState& s)
{
    dust (s, 8 /* FROZEN */, 0.62f, 0.62f, 0.42f, 0.22f, 0.66f, 0.74f, 6421);
    amp (s, 0.90f, 2.60f, 0.92f, 3.20f, 0.6f);
    shape (s, 0.46f, 0.76f, 0.28f, 0.66f, 0.74f, 0.18f);
    material (s, MaterialType::Crystal, MaterialType::Liquid, 0.44f);
    topology (s, 5 /* STAR */, 0.36f, 0.64f, 6421);
    matter (s, 0.90f, 0.74f, 0.20f, 0.82f, 0.40f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.44f, 0.34f, 0.0f, 0.14f, 0.50f);
    set (s, Param::evolveFreeze, 1.0f);
    set (s, Param::evolveScatterSeed, 6421);
    space (s, SpacePresets::Shimmer, 0.46f, 0.68f, 0.62f, 0.36f);

    lfo (s, 1, 0.17f, 0 /* SINE */, 1.0f, false);
    lfo (s, 2, 0.46f, 5 /* SMOOTH RANDOM */, 1.0f, false);
    chaos (s, 1, 1 /* BROWNIAN */, 0.30f, 0.50f, 0.60f, 0.5f, 6421);
    env (s, 2, 1.60f, 3.20f, 0.60f, 2.60f, 0.6f);
    macros (s, 0.45f, 0.45f, 0.50f, 0.45f);

    Routings r;
    r.bi  (ModSource::LFO1,      Param::dustColor,      0.200f)
     .bi  (ModSource::LFO2,      Param::dustDensity,    0.220f)
     .bi  (ModSource::Chaos1,    Param::evolveScatter,  0.180f)
     .uni (ModSource::Env2,      Param::shapeSurface,   0.200f)
     .uni (ModSource::Velocity,  Param::dustGrain,      0.200f)
     .bi  (ModSource::NoteRandom, Param::dustSeed,      0.240f)
     .bi  (ModSource::KeyTrack,  Param::dustColor,     -0.140f)
     .uni (ModSource::Macro1,    Param::evolveMotion,   0.420f)
     .uni (ModSource::Macro2,    Param::dustColor,      0.320f)
     .uni (ModSource::Macro2,    Param::spaceTone,      0.240f)
     .uni (ModSource::Macro3,    Param::spaceMix,       0.320f)
     .uni (ModSource::Macro4,    Param::evolveScatter,  0.340f)
     .uni (ModSource::Macro4,    Param::dustGrain,      0.260f);
    sharedMacros (r, Param::spaceSize, Param::dustJitter);
    r.commit (s);
}});

manager.addFactory ({ "Vinyl Weather", "TEXTURE", { "dirty", "noisy", "breathing", "close", "granular" }, [] (PatchState& s)
{
    sample (s, BuiltInSamples::Kind::VinylDust, 1 /* LOOP */, 0.0f, 1.0f, 0.32f, 0.44f, 60);
    amp (s, 0.40f, 1.60f, 0.82f, 1.40f, 0.55f);
    shape (s, 0.58f, 0.42f, 0.48f, 0.40f, 0.50f, 0.48f);
    material (s, MaterialType::Void, MaterialType::Membrane, 0.48f);
    topology (s, 4 /* RANDOM */, 0.46f, 0.38f, 7013);
    matter (s, 0.72f, 0.64f, 0.20f, 0.62f, 0.25f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.50f, 0.30f, 0.34f, 0.36f, 0.44f);
    set (s, Param::evolveScatterSeed, 7013);
    space (s, SpacePresets::Chamber, 0.26f, 0.34f, 0.40f, 0.26f);

    lfo (s, 1, 0.52f, 5 /* SMOOTH RANDOM */, 1.0f, false);
    lfo (s, 2, 0.19f, 1 /* TRIANGLE */, 1.0f, false);
    chaos (s, 1, 0 /* WALK */, 1.20f, 0.45f, 0.58f, 0.5f, 7013);
    env (s, 2, 0.70f, 1.40f, 0.40f, 1.20f, 0.5f, true);
    macros (s, 0.50f, 0.35f, 0.30f, 0.55f);

    Routings r;
    r.bi  (ModSource::LFO1,      Param::sampleStart,    0.160f)
     .bi  (ModSource::LFO2,      Param::evolveCrush,    0.180f)
     .bi  (ModSource::Chaos1,    Param::sampleGrain,    0.200f)
     .uni (ModSource::Env2,      Param::sampleSpread,   0.240f)
     .uni (ModSource::Velocity,  Param::sampleLevel,    0.200f)
     .bi  (ModSource::KeyTrack,  Param::samplePitch,   -0.120f)
     .uni (ModSource::Macro1,    Param::evolveMotion,   0.400f)
     .uni (ModSource::Macro1,    Param::lfo1Depth,      0.320f)
     .uni (ModSource::Macro2,    Param::shapeExcite,    0.300f)
     .uni (ModSource::Macro2,    Param::spaceTone,      0.240f)
     .uni (ModSource::Macro3,    Param::spaceMix,       0.300f)
     .uni (ModSource::Macro4,    Param::evolveCrush,    0.360f)
     .uni (ModSource::Macro4,    Param::sampleGrain,    0.240f);
    sharedMacros (r, Param::spaceSize, Param::evolveScatter);
    r.commit (s);
}});

manager.addFactory ({ "Breath Cathedral", "TEXTURE", { "hollow", "huge", "breathing", "air", "granular" }, [] (PatchState& s)
{
    sample (s, BuiltInSamples::Kind::Breath, 3 /* GRANULAR */, 0.05f, 0.95f, 0.54f, 0.82f, 60);
    amp (s, 0.85f, 2.40f, 0.86f, 3.20f, 0.6f);
    shape (s, 0.50f, 0.34f, 0.52f, 0.46f, 0.70f, 0.22f);
    material (s, MaterialType::Void, MaterialType::Membrane, 0.36f);
    topology (s, 2 /* CLUSTERS */, 0.44f, 0.56f, 7549);
    matter (s, 0.90f, 0.62f, 0.18f, 0.84f, 0.35f);
    evolve (s, 0.0f, 0.16f, 0.0f, 0.0f, 0.56f, 0.24f, 0.0f, 0.20f, 0.42f);
    set (s, Param::evolveScatterSeed, 7549);
    space (s, SpacePresets::Void, 0.54f, 0.86f, 0.44f, 0.44f);

    lfo (s, 1, 0.31f, 5 /* SMOOTH RANDOM */, 1.0f, false);
    lfo (s, 2, 0.12f, 0 /* SINE */, 1.0f, false);
    env (s, 2, 1.30f, 2.60f, 0.50f, 2.40f, 0.6f, true);
    chaos (s, 1, 1 /* BROWNIAN */, 0.40f, 0.50f, 0.62f, 0.5f, 7549);
    macros (s, 0.50f, 0.40f, 0.60f, 0.45f);

    Routings r;
    r.bi  (ModSource::LFO1,       Param::sampleGrain,   0.220f)
     .bi  (ModSource::LFO2,       Param::sampleStart,   0.140f)
     .bi  (ModSource::Chaos1,     Param::sampleSpread,  0.200f)
     .uni (ModSource::Env2,       Param::sampleGrain,   0.260f)
     .uni (ModSource::Velocity,   Param::sampleLevel,   0.180f)
     .bi  (ModSource::NoteRandom, Param::sampleStart,   0.140f)
     .bi  (ModSource::KeyTrack,   Param::sampleSpread,  0.160f)
     .uni (ModSource::Macro1,     Param::evolveMotion,  0.380f)
     .uni (ModSource::Macro2,     Param::spaceTone,     0.300f)
     .uni (ModSource::Macro2,     Param::shapeForm,     0.240f)
     .uni (ModSource::Macro3,     Param::spaceMix,      0.320f)
     .uni (ModSource::Macro4,     Param::sampleGrain,   0.340f)
     .uni (ModSource::Macro4,     Param::shapeSurface,  0.220f);
    sharedMacros (r, Param::spaceSize, Param::sampleSpread);
    r.commit (s);
}});
}

} // namespace am::FactoryContent
