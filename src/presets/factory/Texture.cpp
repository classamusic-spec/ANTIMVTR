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
}

} // namespace am::FactoryContent
