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
    matter (s, 0.96f, 0.72f, 0.18f, 0.86f, 0.80f);
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
     .bi  (ModSource::NoteRandom, Param::dustPosition,  0.220f)
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
    matter (s, 0.94f, 0.70f, 0.20f, 0.72f, 0.80f);
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
    matter (s, 0.90f, 0.66f, 0.16f, 0.78f, 0.76f);
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
    matter (s, 0.88f, 0.68f, 0.22f, 0.80f, 0.80f);
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
    matter (s, 0.92f, 0.74f, 0.24f, 0.58f, 0.84f);
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
    matter (s, 0.90f, 0.74f, 0.20f, 0.82f, 0.80f);
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
     .bi  (ModSource::NoteRandom, Param::dustPosition,  0.240f)
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
    matter (s, 0.72f, 0.64f, 0.20f, 0.62f, 0.76f);
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
    sharedMacros (r, Param::spaceSize, Param::sampleStart);
    r.commit (s);
}});

manager.addFactory ({ "Breath Cathedral", "TEXTURE", { "hollow", "huge", "breathing", "air", "granular" }, [] (PatchState& s)
{
    sample (s, BuiltInSamples::Kind::Breath, 3 /* GRANULAR */, 0.05f, 0.95f, 0.54f, 0.82f, 60);
    amp (s, 0.85f, 2.40f, 0.86f, 3.20f, 0.6f);
    shape (s, 0.50f, 0.34f, 0.52f, 0.46f, 0.70f, 0.22f);
    material (s, MaterialType::Void, MaterialType::Membrane, 0.36f);
    topology (s, 2 /* CLUSTERS */, 0.44f, 0.56f, 7549);
    matter (s, 0.90f, 0.62f, 0.18f, 0.84f, 0.80f);
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

manager.addFactory ({ "Insect Field", "TEXTURE", { "organic", "chaotic", "high", "noisy", "drift" }, [] (PatchState& s)
{
    dust (s, 5 /* CRACKLE */, 0.46f, 0.74f, 0.26f, 0.72f, 0.64f, 0.80f, 8093);
    amp (s, 0.25f, 1.60f, 0.78f, 1.60f, 0.5f);
    shape (s, 0.44f, 0.64f, 0.24f, 0.62f, 0.48f, 0.42f);
    material (s, MaterialType::Organic, MaterialType::Crystal, 0.40f);
    topology (s, 4 /* RANDOM */, 0.40f, 0.52f, 8093);
    matter (s, 0.94f, 0.72f, 0.18f, 0.84f, 0.80f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.40f, 0.34f, 0.0f, 0.58f, 0.48f);
    set (s, Param::evolveScatterSeed, 8093);
    space (s, SpacePresets::Chamber, 0.30f, 0.38f, 0.62f, 0.28f);

    lfo (s, 1, 0.86f, 5 /* SMOOTH RANDOM */, 1.0f, false);
    lfo (s, 2, 0.17f, 1 /* TRIANGLE */, 1.0f, false);
    chaos (s, 1, 2 /* LOGISTIC */, 3.40f, 0.55f, 0.58f, 0.5f, 8093);
    chaos (s, 2, 0 /* WALK */, 0.60f, 0.40f, 0.66f, 0.5f, 8101);
    macros (s, 0.55f, 0.50f, 0.30f, 0.50f);

    Routings r;
    r.bi  (ModSource::LFO1,       Param::dustDensity,  0.240f)
     .bi  (ModSource::LFO2,       Param::dustColor,    0.200f)
     .bi  (ModSource::Chaos1,     Param::dustJitter,   0.220f)
     .bi  (ModSource::Chaos2,     Param::dustGrain,    0.180f)
     .uni (ModSource::Velocity,   Param::dustDensity,  0.200f)
     .bi  (ModSource::NoteRandom, Param::dustPosition, 0.260f)
     .bi  (ModSource::KeyTrack,   Param::dustColor,    0.180f)
     .uni (ModSource::Macro1,     Param::lfo1Rate,     0.160f)
     .uni (ModSource::Macro1,     Param::evolveMotion, 0.320f)
     .uni (ModSource::Macro2,     Param::dustColor,    0.340f)
     .uni (ModSource::Macro3,     Param::spaceMix,     0.320f)
     .uni (ModSource::Macro4,     Param::dustGrain,    0.320f)
     .uni (ModSource::Macro4,     Param::shapeSurface, 0.240f);
    sharedMacros (r, Param::spaceSize, Param::dustJitter);
    r.commit (s);
}});

manager.addFactory ({ "Tape Shards", "TEXTURE", { "dirty", "synthetic", "rhythmic", "close", "noisy" }, [] (PatchState& s)
{
    wave (s, 7 /* NOISE */, 0.52f, 0.44f, 0.38f, 1, 0.06f, 0.30f, 0, 0.46f);
    dust (s, 1 /* PINK */, 0.58f, 0.48f, 0.34f, 0.30f, 0.58f, 0.66f, 8623, 0.42f);
    set (s, Param::sourceMode, 1 /* LAYER */);
    set (s, Param::impactLevel, 0.0f);
    set (s, Param::sampleLevel, 0.0f);
    set (s, Param::gestureLevel, 0.0f);
    amp (s, 0.15f, 1.20f, 0.76f, 0.90f, 0.5f);
    shape (s, 0.54f, 0.58f, 0.38f, 0.56f, 0.44f, 0.44f);
    material (s, MaterialType::Membrane, MaterialType::Metal, 0.42f);
    topology (s, 5 /* STAR */, 0.44f, 0.46f, 8623);
    matter (s, 0.74f, 0.66f, 0.22f, 0.70f, 0.76f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.48f, 0.30f, 0.26f, 0.50f, 0.40f);
    set (s, Param::evolveScatterSeed, 8623);
    fracture (s, 2 /* TRANSIENT */, 0.52f, 0.48f, 0.44f, 0.66f, 0.26f, 0.30f, 0.42f, 0.52f, 0.34f,
              1 /* 16 */, 4 /* 1/16 */, 8, 0.18f, 2 /* PINGPONG */, 0.70f, 0.40f, 8623,
              FractureShape { 16, 0.03f, 0.30f, 0.10f, 0.36f, 0.34f, 0.58f, 0.28f, 0.88f,
                              1.0f, 0.90f, 0.65f, 0.70f, kFallingTerrace, "Xo.XoXH.", nullptr });
    space (s, SpacePresets::Machine, 0.30f, 0.34f, 0.46f, 0.30f);

    lfo (s, 1, 0.64f, 5 /* SMOOTH RANDOM */, 1.0f, false);
    lfo (s, 2, 0.22f, 2 /* SAW */, 1.0f, false);
    chaos (s, 1, 4 /* TARGETS */, 2.20f, 0.50f, 0.52f, 0.5f, 8627);
    macros (s, 0.55f, 0.45f, 0.30f, 0.55f);

    Routings r;
    r.bi  (ModSource::LFO1,      Param::dustDensity,      0.220f)
     .bi  (ModSource::LFO2,      Param::waveScan,         0.200f)
     .bi  (ModSource::Chaos1,    Param::fractureSpread,   0.220f)
     .uni (ModSource::Velocity,  Param::fractureAmount,   0.240f)
     .bi  (ModSource::KeyTrack,  Param::fractureTone,    -0.200f)
     .bi  (ModSource::NoteRandom, Param::fractureDelay,   0.160f)
     .uni (ModSource::Macro1,    Param::fractureSequence, 0.400f)
     .uni (ModSource::Macro1,    Param::evolveMotion,     0.280f)
     .uni (ModSource::Macro2,    Param::dustColor,        0.320f)
     .uni (ModSource::Macro2,    Param::fractureTone,     0.260f)
     .uni (ModSource::Macro3,    Param::spaceMix,         0.300f)
     .uni (ModSource::Macro4,    Param::evolveCrush,      0.340f)
     .uni (ModSource::Macro4,    Param::fractureAmount,   0.240f);
    sharedMacros (r, Param::fractureDecay, Param::fractureProbability);
    r.commit (s);
}});

manager.addFactory ({ "Deep Sediment", "TEXTURE", { "dark", "sub", "drift", "huge", "soft" }, [] (PatchState& s)
{
    dust (s, 2 /* BROWN */, 0.80f, 0.18f, 0.62f, 0.28f, 0.70f, 0.72f, 9109);
    amp (s, 1.00f, 2.80f, 0.88f, 3.00f, 0.6f);
    shape (s, 0.42f, 0.14f, 0.86f, 0.28f, 0.78f, 0.20f);
    material (s, MaterialType::Membrane, MaterialType::Organic, 0.44f);
    topology (s, 0 /* CHAIN */, 0.50f, 0.36f, 9109);
    matter (s, 0.92f, 0.60f, 0.14f, 0.66f, 0.76f);
    set (s, Param::shapePitch, -7.0f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.74f, 0.20f, 0.0f, 0.12f, 0.40f);
    set (s, Param::evolveScatterSeed, 9109);
    space (s, SpacePresets::Void, 0.46f, 0.88f, 0.24f, 0.40f);

    lfo (s, 1, 0.19f, 0 /* SINE */, 1.0f, false);
    lfo (s, 2, 0.38f, 5 /* SMOOTH RANDOM */, 1.0f, false);
    chaos (s, 1, 1 /* BROWNIAN */, 0.22f, 0.55f, 0.60f, 0.5f, 9109);
    env (s, 2, 1.60f, 2.40f, 0.45f, 2.20f, 0.6f, true);
    macros (s, 0.45f, 0.30f, 0.55f, 0.50f);

    Routings r;
    r.bi  (ModSource::LFO1,      Param::dustColor,      0.200f)
     .bi  (ModSource::LFO2,      Param::dustGrain,      0.220f)
     .bi  (ModSource::Chaos1,    Param::evolveGravity,  0.140f)
     .uni (ModSource::Env2,      Param::dustDensity,    0.220f)
     .uni (ModSource::Velocity,  Param::shapeExcite,    0.200f)
     .bi  (ModSource::KeyTrack,  Param::shapeMass,     -0.240f)
     .uni (ModSource::Macro1,    Param::evolveMotion,   0.360f)
     .uni (ModSource::Macro1,    Param::lfo2Depth,      0.300f)
     .uni (ModSource::Macro2,    Param::dustColor,      0.360f)
     .uni (ModSource::Macro2,    Param::spaceTone,      0.280f)
     .uni (ModSource::Macro3,    Param::spaceMix,       0.300f)
     .uni (ModSource::Macro4,    Param::shapeSurface,   0.260f)
     .uni (ModSource::Macro4,    Param::dustGrain,      0.300f);
    sharedMacros (r, Param::spaceSize, Param::dustJitter);
    r.commit (s);
}});

manager.addFactory ({ "Plate Corrosion", "TEXTURE", { "metallic", "rhythmic", "dirty", "roomy", "struck" }, [] (PatchState& s)
{
    impact (s, 4 /* METAL STRIKE */, 0.56f, 0.52f, 0.20f, 0.60f, 0.50f, 0.42f, 0.58f, 0.70f);
    dust (s, 5 /* CRACKLE */, 0.38f, 0.56f, 0.30f, 0.62f, 0.60f, 0.70f, 9631, 0.34f);
    set (s, Param::sourceMode, 1 /* LAYER */);
    set (s, Param::waveLevel, 0.0f);
    set (s, Param::sampleLevel, 0.0f);
    set (s, Param::gestureLevel, 0.0f);
    amp (s, 0.20f, 1.40f, 0.74f, 1.30f, 0.5f);
    shape (s, 0.58f, 0.62f, 0.36f, 0.64f, 0.56f, 0.48f);
    material (s, MaterialType::Metal, MaterialType::Crystal, 0.46f);
    topology (s, 3 /* LATTICE */, 0.62f, 0.44f, 9631);
    matter (s, 0.88f, 0.70f, 0.24f, 0.76f, 0.80f);
    evolve (s, 0.0f, 0.22f, 0.24f, 0.0f, 0.52f, 0.32f, 0.0f, 0.46f, 0.44f);
    set (s, Param::evolveScatterSeed, 9631);
    fracture (s, 3 /* EVOLVE */, 0.46f, 0.44f, 0.50f, 0.52f, 0.32f, 0.36f, 0.54f, 0.48f, 0.50f,
              1 /* 16 */, 3 /* 1/8 */, 8, 0.0f, 3 /* RANDOM */, 0.65f, 0.38f, 9631,
              FractureShape { 16, 0.06f, 0.40f, 0.14f, 0.42f, 0.40f, 0.66f, 0.30f, 0.90f,
                              1.0f, 0.85f, 0.65f, 0.65f, kMinorTerrace, "XLoH", nullptr });
    space (s, SpacePresets::Machine, 0.36f, 0.46f, 0.44f, 0.34f);

    lfo (s, 1, 0.48f, 5 /* SMOOTH RANDOM */, 1.0f, false);
    lfo (s, 2, 0.15f, 1 /* TRIANGLE */, 1.0f, false);
    chaos (s, 1, 0 /* WALK */, 1.60f, 0.50f, 0.56f, 0.5f, 9631);
    env (s, 2, 1.00f, 2.00f, 0.40f, 1.60f, 0.55f, true);
    macros (s, 0.55f, 0.45f, 0.40f, 0.50f);

    Routings r;
    r.bi  (ModSource::LFO1,       Param::impactRate,       0.200f)
     .bi  (ModSource::LFO2,       Param::impactBrightness, 0.180f)
     .bi  (ModSource::Chaos1,     Param::dustDensity,      0.220f)
     .uni (ModSource::Env2,       Param::evolveTear,       0.220f)
     .uni (ModSource::Velocity,   Param::impactHardness,   0.240f)
     .bi  (ModSource::NoteRandom, Param::impactRandom,     0.180f)
     .bi  (ModSource::KeyTrack,   Param::impactLength,    -0.200f)
     .uni (ModSource::Macro1,     Param::fractureEvolve,   0.380f)
     .uni (ModSource::Macro1,     Param::evolveMotion,     0.280f)
     .uni (ModSource::Macro2,     Param::fractureTone,     0.320f)
     .uni (ModSource::Macro3,     Param::spaceMix,         0.300f)
     .uni (ModSource::Macro4,     Param::evolveMelt,       0.300f)
     .uni (ModSource::Macro4,     Param::impactRandom,     0.260f);
    sharedMacros (r, Param::fractureDecay, Param::fractureRandom);
    r.commit (s);
}});

manager.addFactory ({ "Ice Shelf", "TEXTURE", { "cold", "distant", "glassy", "struck", "unstable" }, [] (PatchState& s)
{
    impact (s, 1 /* CLICK */, 0.74f, 0.68f, 0.10f, 0.55f, 0.60f, 0.55f, 0.22f);
    amp (s, 0.30f, 2.00f, 0.70f, 2.40f, 0.5f);
    shape (s, 0.40f, 0.80f, 0.26f, 0.70f, 0.72f, 0.30f);
    material (s, MaterialType::Crystal, MaterialType::Void, 0.42f);
    topology (s, 5 /* STAR */, 0.34f, 0.60f, 2657);
    matter (s, 0.96f, 0.64f, 0.30f, 0.80f, 0.80f);
    evolve (s, 0.0f, 0.0f, 0.38f, 0.0f, 0.44f, 0.36f, 0.0f, 0.30f, 0.46f);
    set (s, Param::evolveScatterSeed, 2657);
    space (s, SpacePresets::Void, 0.50f, 0.76f, 0.56f, 0.38f);

    lfo (s, 1, 0.36f, 5 /* SMOOTH RANDOM */, 1.0f, false);
    lfo (s, 2, 0.12f, 0 /* SINE */, 1.0f, false);
    chaos (s, 1, 3 /* LORENZ */, 0.70f, 0.55f, 0.58f, 0.5f, 2657);
    env (s, 2, 1.20f, 2.40f, 0.40f, 2.00f, 0.6f, true);
    macros (s, 0.50f, 0.45f, 0.55f, 0.50f);

    Routings r;
    r.bi  (ModSource::LFO1,       Param::impactRate,       0.180f)
     .bi  (ModSource::LFO2,       Param::impactBrightness, 0.200f)
     .bi  (ModSource::Chaos1,     Param::evolveTear,       0.180f)
     .uni (ModSource::Env2,       Param::evolveScatter,    0.220f)
     .uni (ModSource::Velocity,   Param::impactHardness,   0.260f)
     .bi  (ModSource::NoteRandom, Param::impactRandom,     0.220f)
     .bi  (ModSource::KeyTrack,   Param::shapeForm,        0.160f)
     .uni (ModSource::Macro1,     Param::evolveMotion,     0.400f)
     .uni (ModSource::Macro1,     Param::impactRate,       0.240f)
     .uni (ModSource::Macro2,     Param::impactBrightness, 0.320f)
     .uni (ModSource::Macro3,     Param::spaceMix,         0.320f)
     .uni (ModSource::Macro4,     Param::evolveTear,       0.340f)
     .uni (ModSource::Macro4,     Param::shapeSurface,     0.240f);
    sharedMacros (r, Param::spaceSize, Param::impactRandom);
    r.commit (s);
}});

manager.addFactory ({ "Mains Ghost", "TEXTURE", { "synthetic", "dirty", "pulsing", "close", "harsh" }, [] (PatchState& s)
{
    gesture (s, 5 /* ELECTRICAL */, 0.52f, 0.40f, 0.48f, 0.34f, 0.36f, 0.44f);
    amp (s, 0.12f, 1.10f, 0.80f, 0.80f, 0.5f);
    shape (s, 0.66f, 0.50f, 0.42f, 0.58f, 0.46f, 0.50f);
    material (s, MaterialType::Custom, MaterialType::Metal, 0.52f);
    topology (s, 4 /* RANDOM */, 0.54f, 0.40f, 2861);
    matter (s, 0.80f, 0.68f, 0.22f, 0.62f, 0.76f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.50f, 0.30f, 0.42f, 0.54f, 0.42f);
    set (s, Param::evolveScatterSeed, 2861);
    fracture (s, 1 /* RHYTHMIC */, 0.48f, 0.42f, 0.34f, 0.70f, 0.28f, 0.24f, 0.40f, 0.44f, 0.30f,
              0 /* 8 */, 3 /* 1/8 */, 8, 0.0f, 0 /* FORWARD */, 0.90f, 0.28f, 2861,
              FractureShape { 8, 0.04f, 0.24f, 0.12f, 0.34f, 0.36f, 0.56f, 0.20f, 0.70f,
                              1.0f, 0.90f, 0.45f, 0.90f, nullptr, "XXoXXo.X", nullptr });
    space (s, SpacePresets::Machine, 0.28f, 0.32f, 0.40f, 0.32f);

    lfo (s, 1, 1.20f, 3 /* SQUARE */, 1.0f, false, 0.0f, 0.35f);
    lfo (s, 2, 0.26f, 5 /* SMOOTH RANDOM */, 1.0f, false);
    chaos (s, 1, 2 /* LOGISTIC */, 2.80f, 0.45f, 0.66f, 0.5f, 2861);
    macros (s, 0.50f, 0.40f, 0.25f, 0.55f);

    Routings r;
    r.bi  (ModSource::LFO1,      Param::gesturePressure,  0.220f)
     .bi  (ModSource::LFO2,      Param::gestureBandwidth, 0.200f)
     .bi  (ModSource::Chaos1,    Param::evolveCrush,      0.180f)
     .uni (ModSource::Velocity,  Param::gestureSpeed,     0.220f)
     .bi  (ModSource::KeyTrack,  Param::gestureBandwidth, 0.200f)
     .bi  (ModSource::NoteRandom, Param::gestureRoughness, 0.180f)
     .uni (ModSource::Macro1,    Param::fractureSequence, 0.380f)
     .uni (ModSource::Macro1,    Param::gestureMotion,    0.300f)
     .uni (ModSource::Macro2,    Param::gestureBandwidth, 0.320f)
     .uni (ModSource::Macro2,    Param::fractureTone,     0.240f)
     .uni (ModSource::Macro3,    Param::spaceMix,         0.300f)
     .uni (ModSource::Macro4,    Param::evolveCrush,      0.360f)
     .uni (ModSource::Macro4,    Param::gestureRoughness, 0.260f);
    sharedMacros (r, Param::fractureDecay, Param::fractureRandom);
    r.commit (s);
}});

manager.addFactory ({ "Cymbal Weather", "TEXTURE", { "metallic", "bowed", "wide", "high", "evolving" }, [] (PatchState& s)
{
    gesture (s, 0 /* BOW */, 0.42f, 0.66f, 0.44f, 0.30f, 0.38f, 0.72f);
    amp (s, 0.70f, 2.20f, 0.82f, 2.20f, 0.6f);
    shape (s, 0.72f, 0.88f, 0.30f, 0.74f, 0.66f, 0.36f);
    material (s, MaterialType::Metal, MaterialType::Crystal, 0.56f);
    topology (s, 1 /* RING */, 0.72f, 0.56f, 3079);
    matter (s, 0.90f, 0.62f, 0.18f, 0.88f, 0.76f);
    evolve (s, 0.0f, 0.0f, 0.22f, 0.0f, 0.46f, 0.30f, 0.0f, 0.24f, 0.50f);
    set (s, Param::evolveScatterSeed, 3079);
    space (s, SpacePresets::Nebula, 0.44f, 0.66f, 0.60f, 0.36f);

    lfo (s, 1, 0.33f, 5 /* SMOOTH RANDOM */, 1.0f, false);
    lfo (s, 2, 0.08f, 0 /* SINE */, 1.0f, false);
    chaos (s, 1, 1 /* BROWNIAN */, 0.50f, 0.50f, 0.58f, 0.5f, 3079);
    env (s, 2, 1.40f, 3.00f, 0.55f, 2.40f, 0.6f);
    macros (s, 0.50f, 0.50f, 0.50f, 0.45f);

    Routings r;
    r.bi  (ModSource::LFO1,      Param::gestureSpeed,     0.220f)
     .bi  (ModSource::LFO2,      Param::gesturePosition,  0.180f)
     .bi  (ModSource::Chaos1,    Param::gesturePressure,  0.160f)
     .uni (ModSource::Env2,      Param::shapeCoupling,    0.200f)
     .uni (ModSource::Env2,      Param::evolveTear,       0.180f)
     .uni (ModSource::Velocity,  Param::gesturePressure,  0.220f)
     .bi  (ModSource::KeyTrack,  Param::gestureBandwidth, 0.160f)
     .uni (ModSource::Macro1,    Param::gestureMotion,    0.420f)
     .uni (ModSource::Macro2,    Param::gestureBandwidth, 0.320f)
     .uni (ModSource::Macro2,    Param::shapeForm,        0.220f)
     .uni (ModSource::Macro3,    Param::spaceMix,         0.320f)
     .uni (ModSource::Macro4,    Param::shapeCoupling,    0.340f)
     .uni (ModSource::Macro4,    Param::gestureRoughness, 0.260f);
    sharedMacros (r, Param::spaceSize, Param::evolveTear);
    r.commit (s);
}});

manager.addFactory ({ "Buried Engine", "TEXTURE", { "dark", "pulsing", "distant", "low", "hollow" }, [] (PatchState& s)
{
    dust (s, 7 /* CLOUD */, 0.62f, 0.12f, 0.66f, 0.40f, 0.56f, 0.62f, 3271);
    amp (s, 0.60f, 2.20f, 0.84f, 1.80f, 0.55f);
    shape (s, 0.56f, 0.32f, 0.70f, 0.40f, 0.60f, 0.34f);
    material (s, MaterialType::Wood, MaterialType::Metal, 0.48f);
    topology (s, 3 /* LATTICE */, 0.66f, 0.38f, 3271);
    matter (s, 0.86f, 0.62f, 0.20f, 0.60f, 0.76f);
    evolve (s, 0.0f, 0.18f, 0.0f, 0.0f, 0.62f, 0.26f, 0.20f, 0.28f, 0.44f);
    set (s, Param::evolveScatterSeed, 3271);
    fracture (s, 3 /* EVOLVE */, 0.40f, 0.38f, 0.30f, 0.58f, 0.34f, 0.44f, 0.62f, 0.16f, 0.46f,
              0 /* 8 */, 2 /* 1/4 */, 4, 0.0f, 0 /* FORWARD */, 0.95f, 0.22f, 3271,
              FractureShape { 8, 0.10f, 0.52f, 0.16f, 0.38f, 0.44f, 0.62f, 0.20f, 0.60f,
                              1.0f, 0.75f, 0.40f, 0.95f, kFallingTerrace, "XLXL", nullptr });
    space (s, SpacePresets::Machine, 0.40f, 0.62f, 0.14f, 0.40f);

    lfo (s, 1, 0.44f, 0 /* SINE */, 1.0f, false);
    lfo (s, 2, 0.11f, 5 /* SMOOTH RANDOM */, 1.0f, false);
    env (s, 2, 1.10f, 1.80f, 0.35f, 1.60f, 0.55f, true);
    chaos (s, 1, 0 /* WALK */, 0.34f, 0.45f, 0.64f, 0.5f, 3271);
    macros (s, 0.50f, 0.30f, 0.45f, 0.50f);

    Routings r;
    r.bi  (ModSource::LFO1,      Param::dustDensity,    0.240f)
     .bi  (ModSource::LFO2,      Param::dustColor,      0.180f)
     .bi  (ModSource::Chaos1,    Param::fractureDelay,  0.160f)
     .uni (ModSource::Env2,      Param::fractureAmount, 0.240f)
     .uni (ModSource::Velocity,  Param::dustDensity,    0.180f)
     .bi  (ModSource::KeyTrack,  Param::shapeMass,     -0.220f)
     .uni (ModSource::Macro1,    Param::fractureEvolve, 0.360f)
     .uni (ModSource::Macro1,    Param::evolveMotion,   0.300f)
     .uni (ModSource::Macro2,    Param::spaceTone,      0.340f)
     .uni (ModSource::Macro2,    Param::dustColor,      0.280f)
     .uni (ModSource::Macro3,    Param::spaceMix,       0.300f)
     .uni (ModSource::Macro4,    Param::evolveCrush,    0.300f)
     .uni (ModSource::Macro4,    Param::fractureFeedback, 0.240f);
    sharedMacros (r, Param::fractureDecay, Param::fractureRandom);
    r.commit (s);
}});

manager.addFactory ({ "Simmer Skin", "TEXTURE", { "organic", "granular", "breathing", "mid", "wide" }, [] (PatchState& s)
{
    dust (s, 7 /* CLOUD */, 0.66f, 0.20f, 0.66f, 0.54f, 0.68f, 0.76f, 3457);
    amp (s, 0.35f, 1.70f, 0.86f, 1.60f, 0.5f);
    shape (s, 0.62f, 0.34f, 0.46f, 0.50f, 0.52f, 0.44f);
    material (s, MaterialType::Liquid, MaterialType::Membrane, 0.42f);
    topology (s, 2 /* CLUSTERS */, 0.52f, 0.50f, 3457);
    matter (s, 0.94f, 0.82f, 0.24f, 0.80f, 0.80f);
    evolve (s, 0.0f, 0.20f, 0.0f, 0.0f, 0.48f, 0.32f, 0.0f, 0.52f, 0.50f);
    set (s, Param::evolveScatterSeed, 3457);
    space (s, SpacePresets::Dream, 0.40f, 0.52f, 0.54f, 0.34f);

    lfo (s, 1, 0.72f, 5 /* SMOOTH RANDOM */, 1.0f, false);
    lfo (s, 2, 0.29f, 0 /* SINE */, 1.0f, false);
    chaos (s, 1, 4 /* TARGETS */, 1.90f, 0.55f, 0.50f, 0.5f, 3457);
    env (s, 2, 0.80f, 1.40f, 0.35f, 1.20f, 0.5f, true);
    macros (s, 0.55f, 0.45f, 0.40f, 0.50f);

    Routings r;
    r.bi  (ModSource::LFO1,       Param::dustDensity,   0.240f)
     .bi  (ModSource::LFO2,       Param::dustGrain,     0.200f)
     .bi  (ModSource::Chaos1,     Param::dustPosition,  0.220f)
     .uni (ModSource::Env2,       Param::evolveMelt,    0.220f)
     .uni (ModSource::Velocity,   Param::dustDensity,   0.200f)
     .bi  (ModSource::NoteRandom, Param::dustJitter,    0.220f)
     .bi  (ModSource::KeyTrack,   Param::dustColor,    -0.180f)
     .uni (ModSource::Macro1,     Param::evolveMotion,  0.400f)
     .uni (ModSource::Macro1,     Param::lfo1Rate,      0.140f)
     .uni (ModSource::Macro2,     Param::dustColor,     0.340f)
     .uni (ModSource::Macro3,     Param::spaceMix,      0.320f)
     .uni (ModSource::Macro4,     Param::dustGrain,     0.320f)
     .uni (ModSource::Macro4,     Param::shapeSurface,  0.240f);
    sharedMacros (r, Param::spaceSize, Param::dustJitter);
    r.commit (s);
}});

manager.addFactory ({ "Leaf Litter", "TEXTURE", { "wooden", "dry", "close", "rhythmic", "granular" }, [] (PatchState& s)
{
    dust (s, 5 /* CRACKLE */, 0.52f, 0.46f, 0.38f, 0.68f, 0.52f, 0.60f, 3673);
    amp (s, 0.14f, 1.10f, 0.82f, 0.80f, 0.45f);
    shape (s, 0.50f, 0.44f, 0.44f, 0.46f, 0.42f, 0.52f);
    material (s, MaterialType::Wood, MaterialType::Organic, 0.38f);
    topology (s, 3 /* LATTICE */, 0.48f, 0.44f, 3673);
    matter (s, 0.96f, 0.84f, 0.26f, 0.66f, 0.80f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.46f, 0.34f, 0.0f, 0.62f, 0.46f);
    set (s, Param::evolveScatterSeed, 3673);
    fracture (s, 2 /* TRANSIENT */, 0.42f, 0.40f, 0.38f, 0.60f, 0.18f, 0.20f, 0.34f, 0.50f, 0.28f,
              1 /* 16 */, 4 /* 1/16 */, 8, 0.22f, 3 /* RANDOM */, 0.55f, 0.42f, 3673,
              FractureShape { 16, 0.02f, 0.22f, 0.06f, 0.26f, 0.28f, 0.48f, 0.24f, 0.80f,
                              1.0f, 0.85f, 0.60f, 0.55f, nullptr, "XoLoXHo.", nullptr });
    space (s, SpacePresets::Chamber, 0.22f, 0.28f, 0.50f, 0.22f);

    lfo (s, 1, 0.94f, 5 /* SMOOTH RANDOM */, 1.0f, false);
    lfo (s, 2, 0.21f, 1 /* TRIANGLE */, 1.0f, false);
    chaos (s, 1, 0 /* WALK */, 2.40f, 0.50f, 0.60f, 0.5f, 3673);
    macros (s, 0.55f, 0.45f, 0.25f, 0.50f);

    Routings r;
    r.bi  (ModSource::LFO1,       Param::dustDensity,      0.260f)
     .bi  (ModSource::LFO2,       Param::dustGrain,        0.200f)
     .bi  (ModSource::Chaos1,     Param::dustJitter,       0.240f)
     .uni (ModSource::Velocity,   Param::dustDensity,      0.220f)
     .bi  (ModSource::NoteRandom, Param::dustPosition,     0.240f)
     .bi  (ModSource::KeyTrack,   Param::dustColor,        0.160f)
     .uni (ModSource::Macro1,     Param::fractureSequence, 0.380f)
     .uni (ModSource::Macro1,     Param::evolveMotion,     0.300f)
     .uni (ModSource::Macro2,     Param::dustColor,        0.320f)
     .uni (ModSource::Macro2,     Param::fractureTone,     0.240f)
     .uni (ModSource::Macro3,     Param::spaceMix,         0.300f)
     .uni (ModSource::Macro4,     Param::shapeSurface,     0.300f)
     .uni (ModSource::Macro4,     Param::dustGrain,        0.260f);
    sharedMacros (r, Param::fractureDecay, Param::fractureProbability);
    r.commit (s);
}});

manager.addFactory ({ "Radio Ghosts", "TEXTURE", { "vocal", "noisy", "morphing", "distant", "formant" }, [] (PatchState& s)
{
    wave (s, 2 /* FORMANT */, 0.40f, 0.50f, 0.44f, 2, 0.14f, 0.40f, 0, 0.50f);
    dust (s, 0 /* WHITE */, 0.62f, 0.30f, 0.38f, 0.44f, 0.64f, 0.72f, 3881, 0.28f);
    set (s, Param::sourceMode, 1 /* LAYER */);
    set (s, Param::impactLevel, 0.0f);
    set (s, Param::sampleLevel, 0.0f);
    set (s, Param::gestureLevel, 0.0f);
    amp (s, 0.40f, 1.80f, 0.80f, 1.60f, 0.55f);
    shape (s, 0.58f, 0.54f, 0.40f, 0.60f, 0.54f, 0.34f);
    material (s, MaterialType::Organic, MaterialType::Void, 0.44f);
    topology (s, 5 /* STAR */, 0.40f, 0.58f, 3881);
    matter (s, 0.86f, 0.70f, 0.22f, 0.74f, 0.76f);
    evolve (s, 0.0f, 0.0f, 0.20f, 0.0f, 0.50f, 0.36f, 0.0f, 0.40f, 0.52f);
    set (s, Param::evolveScatterSeed, 3881);
    space (s, SpacePresets::Orbit, 0.42f, 0.56f, 0.48f, 0.44f);

    lfo (s, 1, 0.58f, 5 /* SMOOTH RANDOM */, 1.0f, false);
    lfo (s, 2, 0.14f, 1 /* TRIANGLE */, 1.0f, false);
    chaos (s, 1, 4 /* TARGETS */, 0.80f, 0.60f, 0.45f, 0.5f, 3881);
    env (s, 2, 1.00f, 1.80f, 0.40f, 1.40f, 0.55f, true);
    macros (s, 0.55f, 0.45f, 0.45f, 0.55f);

    Routings r;
    r.bi  (ModSource::LFO1,       Param::wavePosition,  0.260f)
     .bi  (ModSource::LFO2,       Param::waveMorph,     0.220f)
     .bi  (ModSource::Chaos1,     Param::waveScan,      0.280f)
     .uni (ModSource::Env2,       Param::dustDensity,   0.240f)
     .uni (ModSource::Velocity,   Param::waveLevel,     0.200f)
     .bi  (ModSource::NoteRandom, Param::wavePosition,  0.180f)
     .bi  (ModSource::KeyTrack,   Param::dustColor,    -0.160f)
     .uni (ModSource::Macro1,     Param::lfo1Depth,     0.420f)
     .uni (ModSource::Macro1,     Param::evolveMotion,  0.300f)
     .uni (ModSource::Macro2,     Param::waveScan,      0.320f)
     .uni (ModSource::Macro3,     Param::spaceMix,      0.320f)
     .uni (ModSource::Macro4,     Param::waveMorph,     0.360f)
     .uni (ModSource::Macro4,     Param::dustLevel,     0.240f);
    sharedMacros (r, Param::spaceSize, Param::evolveTear);
    r.commit (s);
}});

manager.addFactory ({ "Sand Drum", "TEXTURE", { "soft", "scraped", "breathing", "close", "mid" }, [] (PatchState& s)
{
    gesture (s, 2 /* RUB */, 0.54f, 0.30f, 0.44f, 0.48f, 0.44f, 0.48f);
    amp (s, 0.40f, 1.60f, 0.80f, 1.20f, 0.55f);
    shape (s, 0.54f, 0.26f, 0.58f, 0.42f, 0.50f, 0.46f);
    material (s, MaterialType::Membrane, MaterialType::String, 0.34f);
    topology (s, 0 /* CHAIN */, 0.46f, 0.42f, 4079);
    matter (s, 0.94f, 0.72f, 0.20f, 0.56f, 0.80f);
    evolve (s, 0.0f, 0.0f, 0.0f, 0.0f, 0.56f, 0.24f, 0.0f, 0.20f, 0.44f);
    set (s, Param::evolveScatterSeed, 4079);
    space (s, SpacePresets::Chamber, 0.28f, 0.36f, 0.38f, 0.26f);

    lfo (s, 1, 0.24f, 0 /* SINE */, 1.0f, false);
    lfo (s, 2, 0.61f, 5 /* SMOOTH RANDOM */, 1.0f, false);
    chaos (s, 1, 1 /* BROWNIAN */, 0.36f, 0.50f, 0.62f, 0.5f, 4079);
    env (s, 2, 0.90f, 1.60f, 0.40f, 1.20f, 0.5f, true);
    macros (s, 0.50f, 0.40f, 0.30f, 0.50f);

    Routings r;
    r.bi  (ModSource::LFO1,      Param::gestureSpeed,     0.240f)
     .bi  (ModSource::LFO2,      Param::gesturePosition,  0.220f)
     .bi  (ModSource::Chaos1,    Param::gesturePressure,  0.180f)
     .uni (ModSource::Env2,      Param::gestureRoughness, 0.240f)
     .uni (ModSource::Velocity,  Param::gesturePressure,  0.240f)
     .bi  (ModSource::KeyTrack,  Param::gestureBandwidth, 0.180f)
     .uni (ModSource::Macro1,    Param::gestureMotion,    0.440f)
     .uni (ModSource::Macro1,    Param::evolveMotion,     0.280f)
     .uni (ModSource::Macro2,    Param::gestureBandwidth, 0.320f)
     .uni (ModSource::Macro2,    Param::shapeSurface,     0.220f)
     .uni (ModSource::Macro3,    Param::spaceMix,         0.300f)
     .uni (ModSource::Macro4,    Param::gestureRoughness, 0.360f);
    sharedMacros (r, Param::spaceSize, Param::gestureRoughness);
    r.commit (s);
}});

manager.addFactory ({ "After Blast", "TEXTURE", { "dark", "huge", "impact", "drift", "hollow" }, [] (PatchState& s)
{
    impact (s, 3 /* NOISE STRIKE */, 0.42f, 0.40f, 0.62f, 0.90f, 0.65f, 0.20f, 0.0f, 0.86f);
    dust (s, 1 /* PINK */, 0.48f, 0.28f, 0.46f, 0.34f, 0.66f, 0.74f, 4271, 0.26f);
    set (s, Param::sourceMode, 1 /* LAYER */);
    set (s, Param::waveLevel, 0.0f);
    set (s, Param::sampleLevel, 0.0f);
    set (s, Param::gestureLevel, 0.0f);
    amp (s, 0.006f, 2.60f, 0.42f, 3.00f, 0.7f);
    shape (s, 0.52f, 0.36f, 0.76f, 0.34f, 0.80f, 0.30f);
    material (s, MaterialType::Metal, MaterialType::Void, 0.50f);
    topology (s, 2 /* CLUSTERS */, 0.58f, 0.40f, 4271);
    matter (s, 0.90f, 0.60f, 0.34f, 0.78f, 0.76f);
    evolve (s, 0.0f, 0.24f, 0.0f, 0.0f, 0.66f, 0.26f, 0.0f, 0.16f, 0.38f);
    set (s, Param::evolveScatterSeed, 4271);
    space (s, SpacePresets::Void, 0.56f, 0.90f, 0.30f, 0.46f);

    lfo (s, 1, 0.16f, 5 /* SMOOTH RANDOM */, 1.0f, true);
    lfo (s, 2, 0.07f, 0 /* SINE */, 1.0f, false);
    env (s, 1, 0.004f, 1.20f, 0.10f, 1.00f, 0.7f);
    chaos (s, 1, 1 /* BROWNIAN */, 0.28f, 0.45f, 0.64f, 0.5f, 4271);
    macros (s, 0.45f, 0.35f, 0.60f, 0.50f);

    Routings r;
    r.uni (ModSource::Env1,       Param::dustDensity,   0.280f)
     .bi  (ModSource::LFO1,       Param::dustColor,     0.200f)
     .bi  (ModSource::LFO2,       Param::spaceSize,     0.120f)
     .bi  (ModSource::Chaos1,     Param::evolveScatter, 0.160f)
     .uni (ModSource::Velocity,   Param::impactVelocity, 0.240f)
     .bi  (ModSource::NoteRandom, Param::impactRandom,  0.160f)
     .bi  (ModSource::KeyTrack,   Param::shapeMass,    -0.200f)
     .uni (ModSource::Macro1,     Param::evolveMotion,  0.360f)
     .uni (ModSource::Macro2,     Param::impactBrightness, 0.320f)
     .uni (ModSource::Macro2,     Param::spaceTone,     0.280f)
     .uni (ModSource::Macro3,     Param::spaceMix,      0.300f)
     .uni (ModSource::Macro4,     Param::evolveMelt,    0.300f)
     .uni (ModSource::Macro4,     Param::dustLevel,     0.240f);
    sharedMacros (r, Param::spaceSize, Param::dustJitter);
    r.commit (s);
}});

manager.addFactory ({ "Stretched Tape", "TEXTURE", { "metallic", "morphing", "unstable", "roomy", "cold" }, [] (PatchState& s)
{
    sample (s, BuiltInSamples::Kind::MetalPing, 2 /* REVERSE */, 0.04f, 0.94f, 0.36f, 0.52f, 60);
    amp (s, 0.50f, 2.20f, 0.80f, 2.00f, 0.55f);
    shape (s, 0.56f, 0.66f, 0.42f, 0.58f, 0.64f, 0.36f);
    material (s, MaterialType::Metal, MaterialType::String, 0.46f);
    topology (s, 1 /* RING */, 0.56f, 0.48f, 4463);
    matter (s, 0.82f, 0.62f, 0.22f, 0.70f, 0.80f);
    evolve (s, 0.34f, 0.30f, 0.0f, 0.0f, 0.52f, 0.28f, 0.0f, 0.30f, 0.48f);
    set (s, Param::evolveBendPivot, 0.38f);
    set (s, Param::evolveBendRange, 0.42f);
    set (s, Param::evolveScatterSeed, 4463);
    space (s, SpacePresets::Dream, 0.42f, 0.60f, 0.50f, 0.38f);

    lfo (s, 1, 0.20f, 0 /* SINE */, 1.0f, false);
    lfo (s, 2, 0.47f, 5 /* SMOOTH RANDOM */, 1.0f, false);
    chaos (s, 1, 3 /* LORENZ */, 0.40f, 0.55f, 0.56f, 0.5f, 4463);
    env (s, 2, 1.10f, 2.60f, 0.60f, 2.00f, 0.6f);
    macros (s, 0.50f, 0.45f, 0.45f, 0.55f);

    Routings r;
    r.bi  (ModSource::LFO1,       Param::evolveBend,    0.180f)
     .bi  (ModSource::LFO2,       Param::sampleStart,   0.160f)
     .bi  (ModSource::Chaos1,     Param::samplePitch,   0.140f)
     .uni (ModSource::Env2,       Param::evolveMelt,    0.260f)
     .uni (ModSource::Env2,       Param::sampleGrain,   0.200f)
     .uni (ModSource::Velocity,   Param::sampleLevel,   0.200f)
     .bi  (ModSource::NoteRandom, Param::sampleStart,   0.140f)
     .bi  (ModSource::KeyTrack,   Param::sampleSpread,  0.160f)
     .uni (ModSource::Macro1,     Param::evolveMotion,  0.400f)
     .uni (ModSource::Macro2,     Param::sampleGrain,   0.300f)
     .uni (ModSource::Macro3,     Param::spaceMix,      0.320f)
     .uni (ModSource::Macro4,     Param::evolveBend,    0.340f)
     .uni (ModSource::Macro4,     Param::evolveBendRange, 0.260f);
    sharedMacros (r, Param::spaceSize, Param::evolveMelt);
    r.commit (s);
}});
}

} // namespace am::FactoryContent
