#include "presets/FactoryBuilders.h"

namespace am::FactoryContent
{

void registerPad (PresetManager& manager)
{
//==========================================================================
// PAD — held chords that stay interesting for a whole bar
//==========================================================================

manager.addFactory ({ "Void Bloom", "PAD", { "pad", "dark", "evolving", "cinematic" }, [] (PatchState& s)
{
    wave (s, 5 /* SPECTRAL */, 0.34f, 0.28f, 0.06f, 4, 0.20f, 0.85f);
    amp (s, 0.90f, 1.40f, 0.78f, 3.20f, 0.55f);
    shape (s, 0.68f, 0.30f, 0.55f, 0.42f, 0.72f, 0.22f);
    material (s, MaterialType::Void, MaterialType::Organic, 0.35f);
    topology (s, 2 /* CLUSTERS */, 0.38f, 0.55f, 7);
    matter (s, 0.86f, 0.55f, 0.22f, 0.78f);
    evolve (s, 0.0f, 0.26f, 0.0f, 0.34f, 0.44f, 0.12f, 0.0f, 0.18f, 0.40f);
    set (s, Param::evolveMagnetTarget, 1 /* FIFTH */);
    space (s, SpacePresets::Nebula, 0.52f, 0.74f, 0.55f, 0.38f);

    lfo (s, 1, 0.11f, 0 /* SINE */, 1.0f, false, 1.5f);
    lfo (s, 2, 0.07f, 1 /* TRIANGLE */, 1.0f, false, 0.0f, 0.6f);
    env (s, 2, 3.20f, 4.0f, 0.85f, 4.0f, 0.6f);
    chaos (s, 1, 0 /* WALK */, 0.13f, 0.6f, 0.75f, 0.5f, 211);
    macros (s, 0.40f, 0.35f, 0.45f, 0.30f);

    Routings r;
    r.bi  (ModSource::LFO1,   Param::shapeForm,      0.055f)
     .bi  (ModSource::LFO2,   Param::shapeSurface,   0.070f)
     .uni (ModSource::Env2,   Param::evolveMelt,     0.220f)
     .bi  (ModSource::Chaos1, Param::wavePosition,   0.090f)
     .uni (ModSource::Velocity, Param::shapeExcite,  0.180f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.400f)
     .uni (ModSource::Macro1, Param::evolveSpeed,    0.250f)
     .uni (ModSource::Macro2, Param::shapeExcite,    0.300f)
     .uni (ModSource::Macro2, Param::spaceTone,      0.250f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.280f)
     .uni (ModSource::Macro3, Param::spaceSize,      0.200f)
     .uni (ModSource::Macro4, Param::shapeDensity,   0.250f)
     .uni (ModSource::Macro4, Param::evolveMagnet,   0.300f);
    sharedMacros (r, Param::spaceSize, Param::chaos1Depth);
    r.commit (s);
}});

manager.addFactory ({ "Nebula Pad", "PAD", { "pad", "wide", "warm", "chords" }, [] (PatchState& s)
{
    wave (s, 1 /* HARMONIC */, 0.42f, 0.35f, 0.10f, 5, 0.26f, 0.95f);
    amp (s, 0.55f, 1.80f, 0.72f, 2.60f, 0.5f);
    shape (s, 0.60f, 0.08f, 0.55f, 0.52f, 0.74f, 0.30f);
    material (s, MaterialType::Organic, MaterialType::String, 0.40f);
    topology (s, 1 /* RING */, 0.45f, 0.60f, 23);
    matter (s, 0.62f, 0.45f, 0.18f, 0.85f);
    evolve (s, 0.14f, 0.0f, 0.0f, 0.22f, 0.42f, 0.16f, 0.0f, 0.14f, 0.30f);
    set (s, Param::evolveBendPivot, 0.60f);
    set (s, Param::evolveMagnetTarget, 2 /* MAJOR */);
    space (s, SpacePresets::Dream, 0.46f, 0.62f, 0.60f, 0.32f);

    lfo (s, 1, 0.18f, 0 /* SINE */, 1.0f, false, 0.8f);
    lfo (s, 2, 0.09f, 5 /* SMOOTH RANDOM */, 1.0f, false);
    env (s, 2, 1.80f, 3.0f, 0.60f, 3.0f);
    macros (s, 0.35f, 0.50f, 0.40f, 0.25f);

    Routings r;
    r.bi  (ModSource::LFO1,   Param::waveMorph,      0.120f)
     .bi  (ModSource::LFO2,   Param::shapeTension,   0.045f)
     .uni (ModSource::Env2,   Param::shapeDensity,   0.180f)
     .uni (ModSource::Velocity, Param::waveScan,     0.150f)
     .bi  (ModSource::KeyTrack, Param::shapeDecay,  -0.120f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.350f)
     .uni (ModSource::Macro1, Param::lfo1Rate,       0.020f)
     .uni (ModSource::Macro2, Param::wavePosition,   0.320f)
     .uni (ModSource::Macro2, Param::spaceTone,      0.220f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.300f)
     .uni (ModSource::Macro4, Param::waveDetune,     0.260f)
     .uni (ModSource::Macro4, Param::shapeStereo,    0.150f);
    sharedMacros (r, Param::spaceReverbDecay, Param::waveDetune);
    r.commit (s);
}});

manager.addFactory ({ "Membrane Sky", "PAD", { "pad", "airy", "membrane", "breathing" }, [] (PatchState& s)
{
    dust (s, 2 /* BROWN */, 0.88f, 0.20f, 0.30f, 0.22f, 0.70f, 0.80f, 91);
    amp (s, 0.40f, 2.00f, 0.92f, 3.60f, 0.6f);
    shape (s, 0.72f, 0.34f, 0.60f, 0.44f, 0.86f, 0.24f);
    material (s, MaterialType::Membrane, MaterialType::Liquid, 0.45f);
    topology (s, 3 /* LATTICE */, 0.52f, 0.38f, 131);
    matter (s, 0.50f, 0.30f, 0.05f, 0.90f);
    evolve (s, 0.0f, 0.18f, 0.0f, 0.0f, 0.38f, 0.22f, 0.0f, 0.20f, 0.45f);
    space (s, SpacePresets::Shimmer, 0.45f, 0.68f, 0.45f, 0.35f);

    lfo (s, 1, 0.13f, 5 /* SMOOTH RANDOM */, 1.0f, false, 2.0f);
    lfo (s, 2, 0.26f, 0 /* SINE */, 1.0f, false);
    env (s, 2, 2.50f, 5.0f, 0.70f, 5.0f, 0.7f);
    chaos (s, 2, 1 /* BROWNIAN */, 0.22f, 0.55f, 0.70f, 0.5f, 407);
    macros (s, 0.45f, 0.40f, 0.42f, 0.35f);

    Routings r;
    r.bi  (ModSource::LFO1,   Param::dustColor,      0.140f)
     .bi  (ModSource::LFO2,   Param::shapeForm,      0.040f)
     .uni (ModSource::Env2,   Param::shapeDensity,   0.200f)
     .bi  (ModSource::Chaos2, Param::dustDensity,    0.150f)
     .uni (ModSource::Velocity, Param::dustGrain,    0.200f)
     .uni (ModSource::Macro1, Param::evolveMotion,   0.400f)
     .uni (ModSource::Macro1, Param::dustJitter,     0.300f)
     .uni (ModSource::Macro2, Param::dustColor,      0.300f)
     .uni (ModSource::Macro2, Param::shapeExcite,    0.200f)
     .uni (ModSource::Macro3, Param::spaceMix,       0.300f)
     .uni (ModSource::Macro4, Param::dustDensity,    0.350f);
    sharedMacros (r, Param::spaceSize, Param::dustJitter);
    r.commit (s);
}});
}

} // namespace am::FactoryContent
