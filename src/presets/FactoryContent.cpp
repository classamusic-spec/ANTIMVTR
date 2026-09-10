#include "FactoryContent.h"

#include "dsp/fracture/Fragment.h"
#include "dsp/fx/SpacePresets.h"
#include "dsp/matter/MaterialProfile.h"
#include "dsp/source/SampleData.h"
#include "state/ModRouting.h"

#include <atomic>

namespace am::FactoryContent
{

//==============================================================================
// Building blocks
//
// The builders below read like a patch sheet: SOURCE, amplitude, SHAPE,
// EVOLVE, FRACTURE, SPACE, modulation, macros — in that order, always. Every
// helper clamps into the declared parameter range, so a patch can never write
// an illegal value even while it is being edited.
//
// Macro convention (the names a player would see on the eight knobs):
//   1 MOTION     how much the patch moves on its own
//   2 BRIGHT     spectral tilt / opening
//   3 SPACE      how far away it is
//   4 CHARACTER  the one control this particular sound lives on
//   5 LENGTH     how long the object rings after the key
//   6 CHAOS      how unpredictable it is
//   7-8          left free for the player
//
// MACRO 5 and MACRO 6 mean the same thing on every patch (see `sharedMacros`),
// so a player who learns them on one preset knows them on all of them. Both sit
// at zero as shipped: a macro adds to what the patch already is.
//==============================================================================

namespace
{

std::atomic<int> gRejectedRoutings { 0 };

inline void set (PatchState& s, Param p, float v)
{
    s.params[(size_t) paramIndex (p)] = ParameterRegistry::get (p).clampValue (v);
}

//------------------------------------------------------------------ amplitude
inline void amp (PatchState& s, float attack, float decay, float sustain, float release, float curve = 0.5f)
{
    set (s, Param::ampAttack, attack);
    set (s, Param::ampDecay, decay);
    set (s, Param::ampSustain, sustain);
    set (s, Param::ampRelease, release);
    set (s, Param::ampCurve, curve);
}

//--------------------------------------------------------------------- sources
inline void wave (PatchState& s, int table, float position, float morph, float scan,
                  int unison, float detune, float spread, int octave = 0, float level = 1.0f)
{
    set (s, Param::sourceSelected, 0);
    set (s, Param::waveLevel, level);
    set (s, Param::waveTable, (float) table);
    set (s, Param::wavePosition, position);
    set (s, Param::waveMorph, morph);
    set (s, Param::waveScan, scan);
    set (s, Param::waveUnison, (float) unison);
    set (s, Param::waveDetune, detune);
    set (s, Param::waveSpread, spread);
    set (s, Param::waveOctave, (float) octave);
}

inline void dust (PatchState& s, int mode, float density, float colour, float grain,
                  float jitter, float spread, float stereo, int seed, float level = 1.0f)
{
    set (s, Param::sourceSelected, 1);
    set (s, Param::dustLevel, level);
    set (s, Param::dustMode, (float) mode);
    set (s, Param::dustDensity, density);
    set (s, Param::dustColor, colour);
    set (s, Param::dustGrain, grain);
    set (s, Param::dustJitter, jitter);
    set (s, Param::dustSpread, spread);
    set (s, Param::dustStereo, stereo);
    set (s, Param::dustSeed, (float) seed);
}

inline void impact (PatchState& s, int mode, float hardness, float brightness, float length,
                    float velocity, float curve, float random, float rate = 0.0f, float level = 1.0f)
{
    set (s, Param::sourceSelected, 2);
    set (s, Param::impactLevel, level);
    set (s, Param::impactMode, (float) mode);
    set (s, Param::impactHardness, hardness);
    set (s, Param::impactBrightness, brightness);
    set (s, Param::impactLength, length);
    set (s, Param::impactVelocity, velocity);
    set (s, Param::impactCurve, curve);
    set (s, Param::impactRandom, random);
    set (s, Param::impactRate, rate);
}

/** SAMPLE from the built-in (procedurally generated) set — the product ships no audio files. */
inline void sample (PatchState& s, BuiltInSamples::Kind kind, int mode, float start, float end,
                    float grain, float spread, int root = 60, float level = 1.0f)
{
    set (s, Param::sourceSelected, 3);
    set (s, Param::sampleLevel, level);
    set (s, Param::sampleMode, (float) mode);
    set (s, Param::sampleStart, start);
    set (s, Param::sampleEnd, end);
    set (s, Param::sampleGrain, grain);
    set (s, Param::sampleSpread, spread);
    set (s, Param::sampleRoot, (float) root);
    set (s, Param::sampleKeytrack, 1.0f);

    auto* reference = new juce::DynamicObject();
    reference->setProperty ("name", BuiltInSamples::name ((int) kind));
    reference->setProperty ("path", "");
    reference->setProperty ("builtIn", (int) kind);
    s.sample = juce::var (reference);
}

inline void gesture (PatchState& s, int mode, float pressure, float speed, float roughness,
                     float position, float motion, float bandwidth, float level = 1.0f)
{
    set (s, Param::sourceSelected, 4);
    set (s, Param::gestureLevel, level);
    set (s, Param::gestureMode, (float) mode);
    set (s, Param::gesturePressure, pressure);
    set (s, Param::gestureSpeed, speed);
    set (s, Param::gestureRoughness, roughness);
    set (s, Param::gesturePosition, position);
    set (s, Param::gestureMotion, motion);
    set (s, Param::gestureBandwidth, bandwidth);
}

//----------------------------------------------------------------------- shape
inline void shape (PatchState& s, float density, float form, float mass,
                   float tension, float decay, float surface)
{
    set (s, Param::shapeDensity, density);
    set (s, Param::shapeForm, form);
    set (s, Param::shapeMass, mass);
    set (s, Param::shapeTension, tension);
    set (s, Param::shapeDecay, decay);
    set (s, Param::shapeSurface, surface);
}

inline void material (PatchState& s, MaterialType a, MaterialType b, float blend)
{
    set (s, Param::shapeMaterialA, (float) a);
    set (s, Param::shapeMaterialB, (float) b);
    set (s, Param::shapeBlend, blend);
}

inline void topology (PatchState& s, int topo, float coupling, float distribution, int seed)
{
    set (s, Param::shapeTopology, (float) topo);
    set (s, Param::shapeCoupling, coupling);
    set (s, Param::shapeDistribution, distribution);
    set (s, Param::shapeSeed, (float) seed);
}

inline void matter (PatchState& s, float mix, float excite, float strike, float stereo, float keytrack = 1.0f)
{
    set (s, Param::shapeMix, mix);
    set (s, Param::shapeExcite, excite);
    set (s, Param::shapeStrike, strike);
    set (s, Param::shapeStereo, stereo);
    set (s, Param::shapeKeytrack, keytrack);
}

//---------------------------------------------------------------------- evolve
inline void evolve (PatchState& s, float bend, float melt, float tear, float magnet,
                    float gravity, float scatter, float crush, float speed, float motion)
{
    set (s, Param::evolveBend, bend);
    set (s, Param::evolveMelt, melt);
    set (s, Param::evolveTear, tear);
    set (s, Param::evolveMagnet, magnet);
    set (s, Param::evolveGravity, gravity);
    set (s, Param::evolveScatter, scatter);
    set (s, Param::evolveCrush, crush);
    set (s, Param::evolveSpeed, speed);
    set (s, Param::evolveMotion, motion);
}

//----------------------------------------------------------------------- space
inline void space (PatchState& s, int type, float mix, float size, float tone, float feedback)
{
    set (s, Param::spaceType, (float) type);
    set (s, Param::spaceMix, mix);
    set (s, Param::spaceSize, size);
    set (s, Param::spaceTone, tone);
    set (s, Param::spaceFeedback, feedback);
}

//------------------------------------------------------------- modulation slots
inline Param lfoParam (int slot, int which)
{
    // which: 0 rate, 1 shape, 2 depth, 3 retrig, 4 fade, 5 symmetry, 6 phase
    static const Param table[4][7] =
    {
        { Param::lfo1Rate, Param::lfo1Shape, Param::lfo1Depth, Param::lfo1Retrig, Param::lfo1Fade, Param::lfo1Symmetry, Param::lfo1Phase },
        { Param::lfo2Rate, Param::lfo2Shape, Param::lfo2Depth, Param::lfo2Retrig, Param::lfo2Fade, Param::lfo2Symmetry, Param::lfo2Phase },
        { Param::lfo3Rate, Param::lfo3Shape, Param::lfo3Depth, Param::lfo3Retrig, Param::lfo3Fade, Param::lfo3Symmetry, Param::lfo3Phase },
        { Param::lfo4Rate, Param::lfo4Shape, Param::lfo4Depth, Param::lfo4Retrig, Param::lfo4Fade, Param::lfo4Symmetry, Param::lfo4Phase }
    };
    return table[juce::jlimit (0, 3, slot - 1)][juce::jlimit (0, 6, which)];
}

inline Param envParam (int slot, int which)
{
    // which: 0 attack, 1 decay, 2 sustain, 3 release, 4 curve, 5 loop
    static const Param table[4][6] =
    {
        { Param::env1Attack, Param::env1Decay, Param::env1Sustain, Param::env1Release, Param::env1Curve, Param::env1Loop },
        { Param::env2Attack, Param::env2Decay, Param::env2Sustain, Param::env2Release, Param::env2Curve, Param::env2Loop },
        { Param::env3Attack, Param::env3Decay, Param::env3Sustain, Param::env3Release, Param::env3Curve, Param::env3Loop },
        { Param::env4Attack, Param::env4Decay, Param::env4Sustain, Param::env4Release, Param::env4Curve, Param::env4Loop }
    };
    return table[juce::jlimit (0, 3, slot - 1)][juce::jlimit (0, 5, which)];
}

inline Param chaosParam (int slot, int which)
{
    // which: 0 type, 1 rate, 2 depth, 3 stability, 4 symmetry, 5 seed
    static const Param table[4][6] =
    {
        { Param::chaos1Type, Param::chaos1Rate, Param::chaos1Depth, Param::chaos1Stability, Param::chaos1Symmetry, Param::chaos1Seed },
        { Param::chaos2Type, Param::chaos2Rate, Param::chaos2Depth, Param::chaos2Stability, Param::chaos2Symmetry, Param::chaos2Seed },
        { Param::chaos3Type, Param::chaos3Rate, Param::chaos3Depth, Param::chaos3Stability, Param::chaos3Symmetry, Param::chaos3Seed },
        { Param::chaos4Type, Param::chaos4Rate, Param::chaos4Depth, Param::chaos4Stability, Param::chaos4Symmetry, Param::chaos4Seed }
    };
    return table[juce::jlimit (0, 3, slot - 1)][juce::jlimit (0, 5, which)];
}

inline void lfo (PatchState& s, int slot, float rateHz, int lfoShape, float depth,
                 bool retrigger, float fadeIn = 0.0f, float symmetry = 0.5f)
{
    set (s, lfoParam (slot, 0), rateHz);
    set (s, lfoParam (slot, 1), (float) lfoShape);
    set (s, lfoParam (slot, 2), depth);
    set (s, lfoParam (slot, 3), retrigger ? 1.0f : 0.0f);
    set (s, lfoParam (slot, 4), fadeIn);
    set (s, lfoParam (slot, 5), symmetry);
}

inline void env (PatchState& s, int slot, float attack, float decay, float sustain,
                 float release, float curve = 0.5f, bool loop = false)
{
    set (s, envParam (slot, 0), attack);
    set (s, envParam (slot, 1), decay);
    set (s, envParam (slot, 2), sustain);
    set (s, envParam (slot, 3), release);
    set (s, envParam (slot, 4), curve);
    set (s, envParam (slot, 5), loop ? 1.0f : 0.0f);
}

inline void chaos (PatchState& s, int slot, int type, float rateHz, float depth,
                   float stability, float symmetry, int seed)
{
    set (s, chaosParam (slot, 0), (float) type);
    set (s, chaosParam (slot, 1), rateHz);
    set (s, chaosParam (slot, 2), depth);
    set (s, chaosParam (slot, 3), stability);
    set (s, chaosParam (slot, 4), symmetry);
    set (s, chaosParam (slot, 5), (float) seed);
}

inline void macros (PatchState& s, float m1, float m2, float m3, float m4,
                    float m5 = 0.0f, float m6 = 0.0f, float m7 = 0.0f, float m8 = 0.0f)
{
    set (s, Param::macro1, m1); set (s, Param::macro2, m2);
    set (s, Param::macro3, m3); set (s, Param::macro4, m4);
    set (s, Param::macro5, m5); set (s, Param::macro6, m6);
    set (s, Param::macro7, m7); set (s, Param::macro8, m8);
}

//----------------------------------------------------------- modulation matrix
/**
    Collects the routings of one patch and writes them into `PatchState::mod`.

    Anything the table refuses (a source that does not exist, a target that is
    not modulatable, a duplicate) is counted so the factory tests can prove
    every routing a builder asked for actually survived.
*/
struct Routings
{
    ModRoutingTable table;

    Routings& bi (ModSource source, Param target, float depth, float curve = 0.0f)
    {
        return add ({ source, target, depth, curve, true, true });
    }

    Routings& uni (ModSource source, Param target, float depth, float curve = 0.0f)
    {
        return add ({ source, target, depth, curve, false, true });
    }

    Routings& add (const ModRouting& r)
    {
        if (table.add (r) < 0)
            gRejectedRoutings.fetch_add (1, std::memory_order_relaxed);
        return *this;
    }

    void commit (PatchState& s) const { s.mod = table.toVar(); }
};

/**
    MACRO 5 (LENGTH) and MACRO 6 (CHAOS) on every patch.

    `lengthExtra` and `chaosExtra` add the one destination that makes the pair
    mean something on this particular sound (a Fracture decay, a grain size, a
    scatter seed's partner). Both macros ship at zero, so they only ever add.
*/
inline void sharedMacros (Routings& r, Param lengthExtra = Param::Count, Param chaosExtra = Param::Count)
{
    r.uni (ModSource::Macro5, Param::shapeDecay, 0.28f)
     .uni (ModSource::Macro5, Param::ampRelease, 0.15f)
     .uni (ModSource::Macro6, Param::evolveScatter, 0.30f);

    if (lengthExtra != Param::Count) r.uni (ModSource::Macro5, lengthExtra, 0.25f);
    if (chaosExtra  != Param::Count) r.uni (ModSource::Macro6, chaosExtra, 0.30f);
}

//--------------------------------------------------------------- fracture table
/**
    A fragment table described by its musical intent instead of 32 × 8 numbers.

    Low bands keep their timing and their pitch; the ramps run low → high
    across the spectrum, `pitchCycle` terraces eight bands of transposition and
    `pattern` spells the sequencer steps:

        X  every fragment, full gate      L  the lower half of the spectrum
        H  the upper half                 o  every other fragment (comb)
        .  rest
*/
struct FractureShape
{
    int   fragments    = 16;
    float delayLow     = 0.05f, delayHigh     = 0.45f;
    float feedbackLow  = 0.15f, feedbackHigh  = 0.55f;
    float decayLow     = 0.45f, decayHigh     = 0.70f;
    float spreadLow    = 0.20f, spreadHigh    = 0.90f;
    float gainLow      = 1.00f, gainHigh      = 1.00f;
    float panWidth     = 0.65f;
    float probability  = 1.00f;
    const float* pitchCycle = nullptr;    ///< eight semitone values, repeated across the bands
    const char*  pattern    = "XLXHXLXH";
    const float* stepPitch  = nullptr;    ///< one semitone value per pattern step
};

FractureTable makeFracture (const FractureShape& f)
{
    FractureTable t;
    t.numFragments = juce::jlimit (1, kMaxFractureFragments, f.fragments);
    t.numSteps = juce::jlimit (1, kMaxSequencerSteps, (int) juce::String (f.pattern).length());

    for (int i = 0; i < kMaxFractureFragments; ++i)
    {
        const float u = (float) i / (float) (kMaxFractureFragments - 1);   // 0 = low band, 1 = top band
        auto& fr = t.fragments[(size_t) i];
        fr.pitch       = f.pitchCycle != nullptr ? f.pitchCycle[i & 7] : 0.0f;
        fr.delay       = juce::jlimit (0.0f, 1.0f, f.delayLow + (f.delayHigh - f.delayLow) * u * u);
        fr.pan         = ((i & 1) == 0 ? -1.0f : 1.0f) * f.panWidth * (0.25f + 0.75f * u);
        fr.decay       = juce::jlimit (0.0f, 1.0f, f.decayLow + (f.decayHigh - f.decayLow) * u);
        fr.probability = juce::jlimit (0.0f, 1.0f, f.probability);
        fr.feedback    = juce::jlimit (0.0f, 1.0f, f.feedbackLow + (f.feedbackHigh - f.feedbackLow) * u);
        fr.spread      = juce::jlimit (0.0f, 1.0f, f.spreadLow + (f.spreadHigh - f.spreadLow) * u);
        fr.gain        = juce::jlimit (0.0f, 2.0f, f.gainLow + (f.gainHigh - f.gainLow) * u);
    }

    const juce::String pattern (f.pattern);
    for (int i = 0; i < kMaxSequencerSteps; ++i)
    {
        auto& st = t.steps[(size_t) i];
        const int len = juce::jmax (1, pattern.length());
        const juce::juce_wchar c = pattern[i % len];

        switch (c)
        {
            case 'L': st.mask = 0x0000FFFFu; st.gate = 0.90f; break;
            case 'H': st.mask = 0xFFFF0000u; st.gate = 0.90f; break;
            case 'o': st.mask = 0xAAAAAAAAu; st.gate = 0.70f; break;
            case '.': st.mask = 0xFFFFFFFFu; st.gate = 0.00f; break;
            default:  st.mask = 0xFFFFFFFFu; st.gate = 1.00f; break;
        }

        st.pitch       = f.stepPitch != nullptr ? f.stepPitch[i % len] : 0.0f;
        st.pan         = ((i & 2) == 0 ? -0.25f : 0.25f);
        st.gain        = 1.0f;
        st.probability = c == '.' ? 1.0f : juce::jlimit (0.0f, 1.0f, f.probability);
        st.evolve      = (float) (i % 4) * 0.25f;
        st.shape       = (float) (i % 8) / 7.0f;
    }

    return t;
}

/** Turns FRACTURE on and stores the table the patch was designed around. */
inline void fracture (PatchState& s, int mode, float amount, float mix, float spread, float sequence,
                      float feedback, float delay, float decay, float tone, float evolveAmount,
                      int fragments, int division, int steps, float swing, int direction,
                      float probability, float randomAmount, int seed, const FractureShape& shapeSpec)
{
    set (s, Param::fractureOn, 1.0f);
    set (s, Param::fractureMode, (float) mode);
    set (s, Param::fractureAmount, amount);
    set (s, Param::fractureMix, mix);
    set (s, Param::fractureSpread, spread);
    set (s, Param::fractureSequence, sequence);
    set (s, Param::fractureFeedback, feedback);
    set (s, Param::fractureDelay, delay);
    set (s, Param::fractureDecay, decay);
    set (s, Param::fractureTone, tone);
    set (s, Param::fractureEvolve, evolveAmount);
    set (s, Param::fractureFragments, (float) fragments);
    set (s, Param::fractureSync, 1.0f);
    set (s, Param::fractureDivision, (float) division);
    set (s, Param::fractureSteps, (float) steps);
    set (s, Param::fractureSwing, swing);
    set (s, Param::fractureDirection, (float) direction);
    set (s, Param::fractureProbability, probability);
    set (s, Param::fractureRandom, randomAmount);
    set (s, Param::fractureSeed, (float) seed);
    s.fracture = makeFracture (shapeSpec).toVar();
}

//==============================================================================
// Fragment pitch terraces used by several patches.
constexpr float kOctaveTerrace[8]  = { 0.0f, 0.0f, 12.0f, 0.0f, 12.0f, 19.0f, 0.0f, 24.0f };
constexpr float kFifthTerrace[8]   = { 0.0f, 7.0f, 0.0f, 12.0f, 7.0f, 0.0f, 19.0f, 12.0f };
constexpr float kFallingTerrace[8] = { 0.0f, -5.0f, 0.0f, -12.0f, -7.0f, -12.0f, -19.0f, -24.0f };
constexpr float kMinorTerrace[8]   = { 0.0f, 3.0f, 7.0f, 10.0f, 12.0f, 15.0f, 7.0f, 3.0f };

} // namespace

//==============================================================================
// The library
//==============================================================================
void registerAll (PresetManager& manager)
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

    //==========================================================================
    // BASS — weight that still belongs to this instrument
    //==========================================================================

    manager.addFactory ({ "Carbon Bass", "BASS", { "bass", "organic", "mono", "dark" }, [] (PatchState& s)
    {
        wave (s, 3 /* FOLDED */, 0.22f, 0.18f, 0.0f, 2, 0.08f, 0.30f, -1);
        set (s, Param::waveRing, 0.12f);
        set (s, Param::waveModRatio, 1.5f);
        amp (s, 0.003f, 0.45f, 0.68f, 0.20f, 0.35f);
        shape (s, 0.26f, 0.05f, 0.80f, 0.34f, 0.30f, 0.42f);
        material (s, MaterialType::Wood, MaterialType::Organic, 0.35f);
        topology (s, 0 /* CHAIN */, 0.30f, 0.35f, 53);
        matter (s, 0.82f, 0.45f, 0.45f, 0.35f);
        evolve (s, 0.0f, 0.0f, 0.0f, 0.30f, 0.62f, 0.0f, 0.0f, 0.30f, 0.10f);
        set (s, Param::evolveMagnetTarget, 0 /* OCTAVE */);
        set (s, Param::masterMode, 1 /* MONO */);
        set (s, Param::masterGlide, 0.06f);
        space (s, SpacePresets::Chamber, 0.16f, 0.30f, 0.45f, 0.20f);

        env (s, 1, 0.001f, 0.28f, 0.0f, 0.20f, 0.3f);
        lfo (s, 1, 4.80f, 0 /* SINE */, 1.0f, true, 0.35f);
        macros (s, 0.20f, 0.35f, 0.20f, 0.40f);

        Routings r;
        r.uni (ModSource::Env1,   Param::shapeSurface,   0.260f)
         .uni (ModSource::Env1,   Param::waveRing,       0.180f)
         .bi  (ModSource::LFO1,   Param::shapePitch,     0.006f)
         .uni (ModSource::Velocity, Param::shapeStrike,  0.300f)
         .bi  (ModSource::KeyTrack, Param::shapeMass,   -0.150f)
         .uni (ModSource::Macro1, Param::evolveMotion,   0.300f)
         .uni (ModSource::Macro2, Param::shapeExcite,    0.350f)
         .uni (ModSource::Macro2, Param::waveMorph,      0.250f)
         .uni (ModSource::Macro3, Param::spaceMix,       0.250f)
         .uni (ModSource::Macro4, Param::waveRing,       0.400f)
         .uni (ModSource::Macro4, Param::shapeSurface,   0.200f);
        sharedMacros (r, Param::ampDecay, Param::evolveTear);
        r.commit (s);
    }});

    manager.addFactory ({ "Torn Bass", "BASS", { "bass", "aggressive", "distorted", "tear" }, [] (PatchState& s)
    {
        wave (s, 6 /* FRACTURED */, 0.48f, 0.30f, 0.05f, 3, 0.14f, 0.45f, -1);
        amp (s, 0.002f, 0.35f, 0.72f, 0.24f, 0.3f);
        set (s, Param::masterGain, -3.0f);   // struck material has a hot transient: keep headroom at the bottom of the keyboard
        shape (s, 0.42f, 0.52f, 0.72f, 0.40f, 0.34f, 0.52f);
        material (s, MaterialType::Metal, MaterialType::Wood, 0.55f);
        topology (s, 4 /* RANDOM */, 0.55f, 0.40f, 89);
        matter (s, 0.88f, 0.55f, 0.58f, 0.45f);
        evolve (s, 0.0f, 0.0f, 0.42f, 0.24f, 0.58f, 0.20f, 0.16f, 0.42f, 0.30f);
        set (s, Param::evolveScatterSeed, 617);
        set (s, Param::masterMode, 1 /* MONO */);
        set (s, Param::masterGlide, 0.03f);
        space (s, SpacePresets::Machine, 0.22f, 0.28f, 0.42f, 0.30f);
        set (s, Param::spaceDistDrive, 0.42f);

        env (s, 1, 0.001f, 0.22f, 0.10f, 0.18f, 0.25f);
        lfo (s, 1, 7.20f, 3 /* SQUARE */, 1.0f, true, 0.15f);
        chaos (s, 1, 2 /* LOGISTIC */, 3.40f, 0.45f, 0.80f, 0.5f, 733);
        macros (s, 0.30f, 0.45f, 0.20f, 0.45f);

        Routings r;
        r.uni (ModSource::Env1,   Param::evolveTear,     0.300f)
         .uni (ModSource::Env1,   Param::shapeSurface,   0.220f)
         .bi  (ModSource::LFO1,   Param::evolveCrush,    0.080f)
         .bi  (ModSource::Chaos1, Param::evolveScatter,  0.100f)
         .uni (ModSource::Velocity, Param::spaceDistDrive, 0.220f)
         .uni (ModSource::Macro1, Param::evolveMotion,   0.350f)
         .uni (ModSource::Macro2, Param::shapeExcite,    0.300f)
         .uni (ModSource::Macro2, Param::spaceTone,      0.250f)
         .uni (ModSource::Macro3, Param::spaceMix,       0.250f)
         .uni (ModSource::Macro4, Param::evolveTear,     0.400f)
         .uni (ModSource::Macro4, Param::evolveCrush,    0.200f);
        sharedMacros (r, Param::ampDecay, Param::chaos1Depth);
        r.commit (s);
    }});

    manager.addFactory ({ "Rust Sub", "BASS", { "bass", "sub", "gritty", "gesture" }, [] (PatchState& s)
    {
        gesture (s, 4 /* FRICTION */, 0.58f, 0.26f, 0.28f, 0.35f, 0.18f, 0.22f);
        amp (s, 0.02f, 0.60f, 0.80f, 0.35f, 0.4f);
        shape (s, 0.22f, 0.02f, 0.86f, 0.30f, 0.28f, 0.36f);
        material (s, MaterialType::Organic, MaterialType::Membrane, 0.30f);
        topology (s, 0 /* CHAIN */, 0.26f, 0.30f, 167);
        matter (s, 0.92f, 0.30f, 0.25f, 0.30f);
        evolve (s, 0.0f, 0.12f, 0.0f, 0.36f, 0.66f, 0.0f, 0.0f, 0.22f, 0.15f);
        set (s, Param::evolveMagnetTarget, 0 /* OCTAVE */);
        set (s, Param::masterMode, 2 /* LEGATO */);
        set (s, Param::masterGlide, 0.12f);
        space (s, SpacePresets::Void, 0.14f, 0.45f, 0.30f, 0.25f);

        lfo (s, 1, 0.35f, 1 /* TRIANGLE */, 1.0f, true, 0.5f);
        env (s, 1, 0.05f, 0.80f, 0.40f, 0.60f);
        macros (s, 0.25f, 0.30f, 0.20f, 0.50f);

        Routings r;
        r.bi  (ModSource::LFO1,   Param::gesturePressure, 0.120f)
         .uni (ModSource::Env1,   Param::gestureSpeed,    0.250f)
         .uni (ModSource::Velocity, Param::gestureRoughness, 0.300f)
         .bi  (ModSource::KeyTrack, Param::gestureBandwidth, 0.180f)
         .uni (ModSource::Macro1, Param::gestureMotion,   0.400f)
         .uni (ModSource::Macro2, Param::gestureBandwidth, 0.350f)
         .uni (ModSource::Macro2, Param::shapeExcite,     0.250f)
         .uni (ModSource::Macro3, Param::spaceMix,        0.250f)
         .uni (ModSource::Macro4, Param::gestureRoughness, 0.400f)
         .uni (ModSource::Macro4, Param::shapeSurface,    0.250f);
        sharedMacros (r, Param::ampDecay, Param::gestureRoughness);
        r.commit (s);
    }});

    //==========================================================================
    // KEYS — struck and played, an object under the fingers
    //==========================================================================

    manager.addFactory ({ "Crystal Ghost", "KEYS", { "keys", "bells", "crystal", "bright" }, [] (PatchState& s)
    {
        impact (s, 4 /* METAL STRIKE */, 0.72f, 0.68f, 0.18f, 0.80f, 0.45f, 0.12f);
        amp (s, 0.001f, 1.60f, 0.0f, 1.40f, 0.35f);
        shape (s, 0.46f, 0.84f, 0.24f, 0.68f, 0.82f, 0.10f);
        material (s, MaterialType::Crystal, MaterialType::Void, 0.30f);
        topology (s, 5 /* STAR */, 0.28f, 0.62f, 3);
        matter (s, 0.98f, 0.62f, 0.72f, 0.70f);
        evolve (s, 0.0f, 0.0f, 0.0f, 0.52f, 0.44f, 0.08f, 0.0f, 0.25f, 0.12f);
        set (s, Param::evolveMagnetTarget, 5 /* SCALE */);
        space (s, SpacePresets::Shimmer, 0.40f, 0.60f, 0.62f, 0.30f);

        env (s, 1, 0.001f, 0.90f, 0.0f, 0.90f, 0.3f);
        lfo (s, 1, 0.24f, 0 /* SINE */, 1.0f, true, 0.6f);
        macros (s, 0.25f, 0.45f, 0.40f, 0.35f);

        Routings r;
        r.uni (ModSource::Env1,   Param::shapeSurface,   0.200f)
         .bi  (ModSource::LFO1,   Param::shapeTension,   0.030f)
         .uni (ModSource::Velocity, Param::impactBrightness, 0.320f)
         .uni (ModSource::Velocity, Param::shapeStrike,  0.200f)
         .bi  (ModSource::KeyTrack, Param::shapeDecay,  -0.180f)
         .uni (ModSource::Macro1, Param::evolveMotion,   0.300f)
         .uni (ModSource::Macro2, Param::impactBrightness, 0.350f)
         .uni (ModSource::Macro2, Param::shapeExcite,    0.250f)
         .uni (ModSource::Macro3, Param::spaceMix,       0.320f)
         .uni (ModSource::Macro4, Param::evolveMagnet,   0.350f)
         .uni (ModSource::Macro4, Param::shapeTension,   0.200f);
        sharedMacros (r, Param::spaceSize, Param::impactRandom);
        r.commit (s);
    }});

    manager.addFactory ({ "Dust Piano", "KEYS", { "keys", "piano", "noisy", "intimate" }, [] (PatchState& s)
    {
        dust (s, 5 /* CRACKLE */, 0.68f, 0.55f, 0.22f, 0.45f, 0.35f, 0.55f, 17);
        amp (s, 0.002f, 1.10f, 0.14f, 0.90f, 0.35f);
        shape (s, 0.52f, 0.14f, 0.44f, 0.56f, 0.60f, 0.20f);
        material (s, MaterialType::String, MaterialType::Wood, 0.42f);
        topology (s, 0 /* CHAIN */, 0.42f, 0.52f, 41);
        matter (s, 0.96f, 0.58f, 0.45f, 0.55f);
        evolve (s, 0.0f, 0.10f, 0.0f, 0.30f, 0.50f, 0.14f, 0.0f, 0.20f, 0.18f);
        set (s, Param::evolveMagnetTarget, 6 /* CUSTOM (harmonic) */);
        space (s, SpacePresets::Chamber, 0.30f, 0.35f, 0.52f, 0.22f);

        env (s, 1, 0.001f, 0.60f, 0.0f, 0.50f, 0.3f);
        env (s, 2, 0.30f, 2.50f, 0.30f, 2.0f);
        chaos (s, 1, 0 /* WALK */, 0.60f, 0.35f, 0.85f, 0.5f, 59);
        macros (s, 0.20f, 0.40f, 0.30f, 0.45f);

        Routings r;
        r.uni (ModSource::Env1,   Param::dustDensity,    0.300f)
         .uni (ModSource::Env2,   Param::shapeSurface,   0.150f)
         .bi  (ModSource::Chaos1, Param::dustColor,      0.100f)
         .uni (ModSource::Velocity, Param::dustGrain,    0.260f)
         .uni (ModSource::Velocity, Param::shapeStrike,  0.250f)
         .bi  (ModSource::KeyTrack, Param::shapeDecay,  -0.200f)
         .uni (ModSource::Macro1, Param::evolveMotion,   0.300f)
         .uni (ModSource::Macro2, Param::dustColor,      0.320f)
         .uni (ModSource::Macro3, Param::spaceMix,       0.280f)
         .uni (ModSource::Macro4, Param::dustDensity,    0.350f)
         .uni (ModSource::Macro4, Param::dustGrain,      0.250f);
        sharedMacros (r, Param::ampDecay, Param::dustJitter);
        r.commit (s);
    }});

    manager.addFactory ({ "Ash Keys", "KEYS", { "keys", "electric", "soft", "vintage" }, [] (PatchState& s)
    {
        impact (s, 2 /* PLUCK */, 0.38f, 0.42f, 0.26f, 0.72f, 0.55f, 0.08f, 0.0f, 0.85f);
        amp (s, 0.002f, 1.30f, 0.22f, 0.80f, 0.45f);
        shape (s, 0.34f, 0.16f, 0.48f, 0.50f, 0.58f, 0.30f);
        material (s, MaterialType::Metal, MaterialType::Wood, 0.60f);
        topology (s, 2 /* CLUSTERS */, 0.36f, 0.48f, 71);
        matter (s, 0.94f, 0.48f, 0.50f, 0.50f);
        evolve (s, 0.10f, 0.0f, 0.0f, 0.40f, 0.52f, 0.06f, 0.0f, 0.18f, 0.14f);
        set (s, Param::evolveBendPivot, 0.35f);
        set (s, Param::evolveMagnetTarget, 6 /* CUSTOM (harmonic) */);
        space (s, SpacePresets::Chamber, 0.28f, 0.40f, 0.50f, 0.25f);
        set (s, Param::spaceChorusOn, 1.0f);
        set (s, Param::spaceChorusRate, 0.55f);
        set (s, Param::spaceChorusDepth, 0.35f);
        set (s, Param::spaceChorusMix, 0.35f);

        env (s, 1, 0.001f, 0.70f, 0.0f, 0.60f, 0.35f);
        lfo (s, 1, 4.20f, 0 /* SINE */, 1.0f, true, 0.8f);
        macros (s, 0.20f, 0.40f, 0.30f, 0.35f);

        Routings r;
        r.uni (ModSource::Env1,   Param::shapeExcite,    0.220f)
         .bi  (ModSource::LFO1,   Param::shapePitch,     0.004f)
         .uni (ModSource::Velocity, Param::impactHardness, 0.300f)
         .uni (ModSource::Velocity, Param::shapeSurface, 0.180f)
         .bi  (ModSource::KeyTrack, Param::shapeMass,   -0.140f)
         .uni (ModSource::Macro1, Param::lfo1Depth,      0.350f)
         .uni (ModSource::Macro2, Param::impactBrightness, 0.350f)
         .uni (ModSource::Macro2, Param::shapeTension,   0.150f)
         .uni (ModSource::Macro3, Param::spaceMix,       0.300f)
         .uni (ModSource::Macro4, Param::evolveBend,     0.250f)
         .uni (ModSource::Macro4, Param::shapeSurface,   0.200f);
        sharedMacros (r, Param::ampDecay, Param::impactRandom);
        r.commit (s);
    }});

    //==========================================================================
    // PLUCK — short, precise, physical
    //==========================================================================

    manager.addFactory ({ "Magnet Bells", "PLUCK", { "pluck", "bells", "tuned", "magnet" }, [] (PatchState& s)
    {
        impact (s, 4 /* METAL STRIKE */, 0.82f, 0.60f, 0.12f, 0.85f, 0.40f, 0.10f);
        amp (s, 0.001f, 0.85f, 0.0f, 0.70f, 0.3f);
        shape (s, 0.38f, 0.66f, 0.30f, 0.62f, 0.66f, 0.14f);
        material (s, MaterialType::Metal, MaterialType::Crystal, 0.45f);
        topology (s, 1 /* RING */, 0.34f, 0.58f, 13);
        matter (s, 1.0f, 0.60f, 0.80f, 0.65f);
        evolve (s, 0.0f, 0.0f, 0.0f, 0.72f, 0.40f, 0.10f, 0.0f, 0.30f, 0.18f);
        set (s, Param::evolveMagnetTarget, 5 /* SCALE */);
        space (s, SpacePresets::Orbit, 0.34f, 0.45f, 0.55f, 0.40f);

        env (s, 1, 0.001f, 0.45f, 0.0f, 0.40f, 0.3f);
        lfo (s, 1, 0.60f, 5 /* SMOOTH RANDOM */, 1.0f, true);
        macros (s, 0.25f, 0.40f, 0.35f, 0.55f);

        Routings r;
        r.uni (ModSource::Env1,   Param::evolveScatter,  0.120f)
         .bi  (ModSource::LFO1,   Param::shapeTension,   0.040f)
         .uni (ModSource::Velocity, Param::impactHardness, 0.300f)
         .uni (ModSource::Velocity, Param::shapeStrike,  0.220f)
         .bi  (ModSource::NoteRandom, Param::shapePitch, 0.004f)
         .uni (ModSource::Macro1, Param::evolveMotion,   0.350f)
         .uni (ModSource::Macro2, Param::impactBrightness, 0.320f)
         .uni (ModSource::Macro3, Param::spaceMix,       0.300f)
         .uni (ModSource::Macro3, Param::spaceFeedback,  0.200f)
         .uni (ModSource::Macro4, Param::evolveMagnet,   0.280f)
         .uni (ModSource::Macro4, Param::shapeForm,      0.150f);
        sharedMacros (r, Param::spaceFeedback, Param::impactRandom);
        r.commit (s);
    }});

    manager.addFactory ({ "Anti-String", "PLUCK", { "pluck", "string", "impossible", "inharmonic" }, [] (PatchState& s)
    {
        impact (s, 2 /* PLUCK */, 0.62f, 0.55f, 0.16f, 0.78f, 0.42f, 0.06f);
        amp (s, 0.001f, 1.40f, 0.0f, 1.10f, 0.3f);
        shape (s, 0.44f, 0.18f, 0.36f, 0.74f, 0.70f, 0.24f);
        material (s, MaterialType::String, MaterialType::Void, 0.38f);
        topology (s, 0 /* CHAIN */, 0.48f, 0.55f, 29);
        matter (s, 1.0f, 0.56f, 0.68f, 0.60f);
        evolve (s, 0.32f, 0.0f, 0.0f, 0.0f, 0.46f, 0.16f, 0.0f, 0.22f, 0.20f);
        set (s, Param::evolveBendPivot, 0.28f);
        set (s, Param::evolveBendRange, 0.42f);
        set (s, Param::evolveBendCurve, 0.62f);
        space (s, SpacePresets::Dream, 0.32f, 0.55f, 0.55f, 0.28f);

        env (s, 1, 0.001f, 1.20f, 0.0f, 1.0f, 0.35f);
        lfo (s, 1, 0.32f, 1 /* TRIANGLE */, 1.0f, true, 0.4f);
        macros (s, 0.30f, 0.40f, 0.30f, 0.45f);

        Routings r;
        r.uni (ModSource::Env1,   Param::evolveBend,     0.180f)
         .bi  (ModSource::LFO1,   Param::shapeSurface,   0.050f)
         .uni (ModSource::Velocity, Param::impactBrightness, 0.280f)
         .uni (ModSource::Velocity, Param::shapeExcite,  0.200f)
         .bi  (ModSource::KeyTrack, Param::evolveBendPivot, 0.150f)
         .uni (ModSource::Macro1, Param::evolveMotion,   0.320f)
         .uni (ModSource::Macro2, Param::shapeExcite,    0.300f)
         .uni (ModSource::Macro3, Param::spaceMix,       0.300f)
         .uni (ModSource::Macro4, Param::evolveBend,     0.350f)
         .uni (ModSource::Macro4, Param::shapeTension,   0.180f);
        sharedMacros (r, Param::ampDecay, Param::evolveTear);
        r.commit (s);
    }});

    manager.addFactory ({ "Copper Thorn", "PLUCK", { "pluck", "metal", "short", "percussive" }, [] (PatchState& s)
    {
        impact (s, 1 /* CLICK */, 0.88f, 0.78f, 0.08f, 0.85f, 0.30f, 0.14f);
        amp (s, 0.001f, 0.40f, 0.0f, 0.30f, 0.25f);
        shape (s, 0.30f, 0.50f, 0.28f, 0.58f, 0.48f, 0.34f);
        material (s, MaterialType::Metal, MaterialType::Wood, 0.30f);
        topology (s, 4 /* RANDOM */, 0.60f, 0.42f, 97);
        matter (s, 1.0f, 0.66f, 0.70f, 0.55f);
        evolve (s, 0.0f, 0.0f, 0.20f, 0.34f, 0.50f, 0.24f, 0.22f, 0.45f, 0.20f);
        set (s, Param::evolveScatterSeed, 331);
        set (s, Param::evolveMagnetTarget, 3 /* MINOR */);
        space (s, SpacePresets::Orbit, 0.30f, 0.35f, 0.50f, 0.45f);

        env (s, 1, 0.001f, 0.20f, 0.0f, 0.18f, 0.25f);
        chaos (s, 1, 4 /* TARGETS */, 2.20f, 0.40f, 0.60f, 0.5f, 811);
        macros (s, 0.30f, 0.50f, 0.30f, 0.35f);

        Routings r;
        r.uni (ModSource::Env1,   Param::evolveCrush,    0.150f)
         .bi  (ModSource::Chaos1, Param::evolveScatter,  0.120f)
         .uni (ModSource::Velocity, Param::impactHardness, 0.320f)
         .uni (ModSource::Velocity, Param::shapeStrike,  0.250f)
         .bi  (ModSource::NoteRandom, Param::shapeSurface, 0.060f)
         .uni (ModSource::Macro1, Param::evolveMotion,   0.400f)
         .uni (ModSource::Macro2, Param::impactBrightness, 0.300f)
         .uni (ModSource::Macro3, Param::spaceMix,       0.300f)
         .uni (ModSource::Macro4, Param::evolveTear,     0.300f)
         .uni (ModSource::Macro4, Param::evolveCrush,    0.200f);
        sharedMacros (r, Param::spaceFeedback, Param::impactRandom);
        r.commit (s);
    }});

    //==========================================================================
    // LEAD — one line, in front
    //==========================================================================

    manager.addFactory ({ "Liquid Teeth", "LEAD", { "lead", "liquid", "biting", "mono" }, [] (PatchState& s)
    {
        wave (s, 4 /* METALLIC */, 0.55f, 0.42f, 0.12f, 2, 0.10f, 0.35f);
        set (s, Param::waveSync, 0.28f);
        set (s, Param::waveModRatio, 3.0f);
        amp (s, 0.006f, 0.30f, 0.90f, 0.45f, 0.4f);
        shape (s, 0.46f, 0.62f, 0.38f, 0.66f, 0.66f, 0.44f);
        material (s, MaterialType::Liquid, MaterialType::Metal, 0.42f);
        topology (s, 1 /* RING */, 0.52f, 0.50f, 199);
        matter (s, 0.70f, 0.55f, 0.40f, 0.45f);
        evolve (s, 0.22f, 0.16f, 0.0f, 0.30f, 0.46f, 0.14f, 0.0f, 0.38f, 0.28f);
        set (s, Param::evolveMagnetTarget, 4 /* CHROMATIC */);
        set (s, Param::masterMode, 2 /* LEGATO */);
        set (s, Param::masterGlide, 0.09f);
        space (s, SpacePresets::Orbit, 0.28f, 0.42f, 0.55f, 0.42f);

        lfo (s, 1, 5.20f, 0 /* SINE */, 1.0f, true, 0.45f);
        lfo (s, 2, 0.28f, 1 /* TRIANGLE */, 1.0f, false);
        env (s, 1, 0.004f, 0.35f, 0.35f, 0.30f, 0.3f);
        macros (s, 0.30f, 0.45f, 0.30f, 0.40f);

        Routings r;
        r.bi  (ModSource::LFO1,   Param::shapePitch,     0.005f)
         .bi  (ModSource::LFO2,   Param::waveMorph,      0.140f)
         .uni (ModSource::Env1,   Param::waveSync,       0.260f)
         .uni (ModSource::Velocity, Param::shapeSurface, 0.240f)
         .uni (ModSource::ModWheel, Param::lfo1Depth,    0.600f)
         .uni (ModSource::Macro1, Param::evolveMotion,   0.350f)
         .uni (ModSource::Macro2, Param::wavePosition,   0.300f)
         .uni (ModSource::Macro2, Param::shapeExcite,    0.220f)
         .uni (ModSource::Macro3, Param::spaceMix,       0.280f)
         .uni (ModSource::Macro4, Param::waveSync,       0.400f)
         .uni (ModSource::Macro4, Param::evolveBend,     0.200f);
        sharedMacros (r, Param::ampDecay, Param::evolveTear);
        r.commit (s);
    }});

    manager.addFactory ({ "Impossible String", "LEAD", { "lead", "bowed", "string", "expressive" }, [] (PatchState& s)
    {
        gesture (s, 0 /* BOW */, 0.58f, 0.48f, 0.24f, 0.32f, 0.22f, 0.45f);
        amp (s, 0.12f, 0.50f, 0.88f, 0.45f, 0.5f);
        shape (s, 0.48f, 0.12f, 0.40f, 0.62f, 0.56f, 0.22f);
        material (s, MaterialType::String, MaterialType::Organic, 0.30f);
        topology (s, 0 /* CHAIN */, 0.50f, 0.55f, 61);
        matter (s, 0.95f, 0.52f, 0.25f, 0.50f);
        evolve (s, 0.16f, 0.0f, 0.0f, 0.44f, 0.48f, 0.08f, 0.0f, 0.28f, 0.24f);
        set (s, Param::evolveMagnetTarget, 6 /* CUSTOM (harmonic) */);
        set (s, Param::masterMode, 2 /* LEGATO */);
        set (s, Param::masterGlide, 0.10f);
        space (s, SpacePresets::Chamber, 0.34f, 0.45f, 0.55f, 0.28f);

        lfo (s, 1, 5.60f, 0 /* SINE */, 1.0f, true, 0.60f);
        lfo (s, 2, 0.22f, 5 /* SMOOTH RANDOM */, 1.0f, false);
        env (s, 1, 0.18f, 0.60f, 0.60f, 0.50f);
        macros (s, 0.30f, 0.45f, 0.35f, 0.40f);

        Routings r;
        r.bi  (ModSource::LFO1,   Param::shapePitch,     0.006f)
         .bi  (ModSource::LFO2,   Param::gesturePressure, 0.100f)
         .uni (ModSource::Env1,   Param::gestureSpeed,   0.220f)
         .uni (ModSource::Velocity, Param::gesturePressure, 0.260f)
         .uni (ModSource::Pressure, Param::gestureSpeed, 0.300f)
         .uni (ModSource::ModWheel, Param::lfo1Depth,    0.550f)
         .uni (ModSource::Macro1, Param::gestureMotion,  0.350f)
         .uni (ModSource::Macro2, Param::gestureBandwidth, 0.300f)
         .uni (ModSource::Macro2, Param::shapeExcite,    0.250f)
         .uni (ModSource::Macro3, Param::spaceMix,       0.300f)
         .uni (ModSource::Macro4, Param::gestureRoughness, 0.350f)
         .uni (ModSource::Macro4, Param::shapeSurface,   0.180f);
        sharedMacros (r, Param::ampDecay, Param::gestureRoughness);
        r.commit (s);
    }});

    manager.addFactory ({ "Solar Filament", "LEAD", { "lead", "bright", "burning", "wide" }, [] (PatchState& s)
    {
        wave (s, 2 /* FORMANT */, 0.62f, 0.55f, 0.18f, 2, 0.12f, 0.40f);
        set (s, Param::waveFM, 0.18f);
        set (s, Param::waveModRatio, 2.0f);
        amp (s, 0.02f, 0.45f, 0.85f, 0.55f, 0.5f);
        shape (s, 0.55f, 0.58f, 0.46f, 0.70f, 0.62f, 0.30f);
        material (s, MaterialType::Crystal, MaterialType::Metal, 0.55f);
        topology (s, 5 /* STAR */, 0.44f, 0.60f, 149);
        matter (s, 0.62f, 0.48f, 0.35f, 0.60f);
        evolve (s, 0.26f, 0.0f, 0.0f, 0.38f, 0.36f, 0.18f, 0.0f, 0.40f, 0.30f);
        set (s, Param::evolveBendPivot, 0.65f);
        set (s, Param::evolveMagnetTarget, 2 /* MAJOR */);
        space (s, SpacePresets::Shimmer, 0.36f, 0.55f, 0.62f, 0.35f);

        lfo (s, 1, 6.20f, 0 /* SINE */, 1.0f, true, 0.55f);
        lfo (s, 2, 0.42f, 2 /* SAW */, 1.0f, true);
        env (s, 1, 0.02f, 0.55f, 0.45f, 0.50f);
        macros (s, 0.35f, 0.55f, 0.35f, 0.30f);

        Routings r;
        r.bi  (ModSource::LFO1,   Param::shapePitch,     0.005f)
         .bi  (ModSource::LFO2,   Param::waveScan,       0.120f)
         .uni (ModSource::Env1,   Param::waveFM,         0.200f)
         .uni (ModSource::Velocity, Param::waveFM,       0.150f)
         .bi  (ModSource::KeyTrack, Param::shapeForm,   -0.100f)
         .uni (ModSource::Macro1, Param::evolveMotion,   0.400f)
         .uni (ModSource::Macro2, Param::wavePosition,   0.300f)
         .uni (ModSource::Macro2, Param::shapeExcite,    0.250f)
         .uni (ModSource::Macro3, Param::spaceMix,       0.300f)
         .uni (ModSource::Macro4, Param::waveFM,         0.250f)
         .uni (ModSource::Macro4, Param::evolveBend,     0.250f);
        sharedMacros (r, Param::spaceSize, Param::waveScan);
        r.commit (s);
    }});

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

    //==========================================================================
    // PERCUSSION — struck objects, tuned across the keyboard
    //==========================================================================

    manager.addFactory ({ "Bone Marimba", "PERCUSSION", { "percussion", "mallet", "wood", "dry" }, [] (PatchState& s)
    {
        impact (s, 6 /* MEMBRANE HIT */, 0.45f, 0.40f, 0.10f, 0.85f, 0.45f, 0.14f, 0.0f, 0.85f);
        amp (s, 0.001f, 0.45f, 0.0f, 0.35f, 0.25f);
        set (s, Param::masterGain, -3.0f);   // struck material has a hot transient: keep headroom at the bottom of the keyboard
        shape (s, 0.28f, 0.20f, 0.42f, 0.48f, 0.42f, 0.22f);
        material (s, MaterialType::Wood, MaterialType::Membrane, 0.30f);
        topology (s, 0 /* CHAIN */, 0.30f, 0.40f, 311);
        matter (s, 1.0f, 0.52f, 0.55f, 0.45f);
        evolve (s, 0.0f, 0.0f, 0.0f, 0.34f, 0.52f, 0.10f, 0.0f, 0.25f, 0.12f);
        set (s, Param::evolveMagnetTarget, 6 /* CUSTOM (harmonic) */);
        space (s, SpacePresets::Chamber, 0.26f, 0.28f, 0.50f, 0.20f);

        env (s, 1, 0.001f, 0.16f, 0.0f, 0.14f, 0.25f);
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

    //==========================================================================
    // FX — sound design, not notes
    //==========================================================================

    manager.addFactory ({ "Fractured Voice", "FX", { "fx", "vocal", "spectral", "fracture" }, [] (PatchState& s)
    {
        gesture (s, 3 /* BREATH */, 0.72f, 0.45f, 0.26f, 0.55f, 0.35f, 0.48f);
        amp (s, 0.15f, 1.0f, 0.75f, 1.20f, 0.5f);
        shape (s, 0.58f, 0.14f, 0.44f, 0.58f, 0.74f, 0.30f);
        material (s, MaterialType::Organic, MaterialType::Membrane, 0.45f);
        topology (s, 2 /* CLUSTERS */, 0.50f, 0.52f, 601);
        matter (s, 0.94f, 0.40f, 0.30f, 0.75f);
        evolve (s, 0.0f, 0.18f, 0.34f, 0.24f, 0.48f, 0.26f, 0.0f, 0.32f, 0.35f);
        set (s, Param::evolveScatterSeed, 1483);
        fracture (s, 0 /* SPECTRAL */, 0.62f, 0.70f, 0.62f, 0.45f, 0.40f, 0.55f, 0.62f, 0.48f, 0.25f,
                  1 /* 16 */, 3 /* 1/8 */, 8, 0.15f, 2 /* PINGPONG */, 0.90f, 0.35f, 887,
                  FractureShape { 16, 0.12f, 0.72f, 0.25f, 0.60f, 0.50f, 0.80f, 0.30f, 0.95f,
                                  1.0f, 1.0f, 0.75f, 0.95f, kMinorTerrace, "XLHXoLHX", nullptr });
        space (s, SpacePresets::Dream, 0.48f, 0.68f, 0.58f, 0.40f);

        lfo (s, 1, 0.28f, 5 /* SMOOTH RANDOM */, 1.0f, true);
        chaos (s, 1, 4 /* TARGETS */, 0.80f, 0.55f, 0.55f, 0.5f, 1543);
        env (s, 2, 0.60f, 2.0f, 0.55f, 2.0f);
        macros (s, 0.45f, 0.45f, 0.45f, 0.55f);

        Routings r;
        r.bi  (ModSource::LFO1,   Param::gesturePosition, 0.180f)
         .bi  (ModSource::Chaos1, Param::fracturePitch,  0.070f)
         .uni (ModSource::Env2,   Param::fractureSpread, 0.250f)
         .uni (ModSource::Velocity, Param::gesturePressure, 0.250f)
         .bi  (ModSource::NoteRandom, Param::fractureSequence, 0.150f)
         .uni (ModSource::Macro1, Param::evolveMotion,   0.350f)
         .uni (ModSource::Macro1, Param::fractureEvolve, 0.300f)
         .uni (ModSource::Macro2, Param::fractureTone,   0.300f)
         .uni (ModSource::Macro3, Param::spaceMix,       0.280f)
         .uni (ModSource::Macro4, Param::fractureAmount, 0.300f)
         .uni (ModSource::Macro4, Param::fractureFeedback, 0.200f);
        sharedMacros (r, Param::fractureDecay, Param::fractureRandom);
        r.commit (s);
    }});

    manager.addFactory ({ "Reactor Fault", "FX", { "fx", "noise", "unstable", "machine" }, [] (PatchState& s)
    {
        gesture (s, 5 /* ELECTRICAL */, 0.72f, 0.78f, 0.72f, 0.25f, 0.62f, 0.70f);
        amp (s, 0.05f, 0.80f, 0.70f, 0.90f, 0.45f);
        shape (s, 0.72f, 0.94f, 0.28f, 0.72f, 0.46f, 0.62f);
        material (s, MaterialType::Metal, MaterialType::Void, 0.55f);
        topology (s, 4 /* RANDOM */, 0.72f, 0.48f, 653);
        matter (s, 0.86f, 0.78f, 0.42f, 0.85f);
        evolve (s, 0.0f, 0.0f, 0.46f, 0.0f, 0.44f, 0.62f, 0.30f, 0.70f, 0.60f);
        set (s, Param::evolveScatterSeed, 1601);
        fracture (s, 1 /* RHYTHMIC */, 0.58f, 0.60f, 0.70f, 0.62f, 0.45f, 0.45f, 0.55f, 0.60f, 0.40f,
                  2 /* 32 */, 4 /* 1/16 */, 12, 0.20f, 3 /* RANDOM */, 0.75f, 0.55f, 1699,
                  FractureShape { 32, 0.04f, 0.55f, 0.30f, 0.62f, 0.35f, 0.70f, 0.35f, 1.0f,
                                  1.0f, 1.0f, 0.85f, 0.85f, kFallingTerrace, "XoH.XLo.", nullptr });
        space (s, SpacePresets::Machine, 0.40f, 0.45f, 0.55f, 0.45f);

        lfo (s, 1, 1.40f, 4 /* RANDOM */, 1.0f, true);
        chaos (s, 1, 2 /* LOGISTIC */, 6.20f, 0.65f, 0.40f, 0.5f, 1777);
        chaos (s, 2, 3 /* LORENZ */, 0.90f, 0.55f, 0.55f, 0.5f, 1811);
        macros (s, 0.60f, 0.50f, 0.35f, 0.55f);

        Routings r;
        r.bi  (ModSource::LFO1,   Param::gestureRoughness, 0.200f)
         .bi  (ModSource::Chaos1, Param::evolveScatter,  0.180f)
         .bi  (ModSource::Chaos2, Param::fractureSpread, 0.200f)
         .uni (ModSource::Velocity, Param::evolveCrush,  0.200f)
         .uni (ModSource::Macro1, Param::evolveMotion,   0.400f)
         .uni (ModSource::Macro1, Param::fractureRandom, 0.300f)
         .uni (ModSource::Macro2, Param::fractureTone,   0.300f)
         .uni (ModSource::Macro3, Param::spaceMix,       0.300f)
         .uni (ModSource::Macro4, Param::evolveTear,     0.300f)
         .uni (ModSource::Macro4, Param::evolveCrush,    0.250f);
        sharedMacros (r, Param::fractureDecay, Param::fractureRandom);
        r.commit (s);
    }});

    manager.addFactory ({ "Signal Decay", "FX", { "fx", "granular", "collapse", "sample" }, [] (PatchState& s)
    {
        sample (s, BuiltInSamples::Kind::NoiseBurst, 3 /* GRANULAR */, 0.0f, 0.70f, 0.52f, 0.75f);
        amp (s, 0.02f, 1.60f, 0.45f, 2.20f, 0.4f);
        shape (s, 0.64f, 0.78f, 0.34f, 0.52f, 0.62f, 0.40f);
        material (s, MaterialType::Void, MaterialType::Crystal, 0.50f);
        topology (s, 3 /* LATTICE */, 0.54f, 0.48f, 719);
        matter (s, 0.90f, 0.70f, 0.35f, 0.88f);
        evolve (s, 0.0f, 0.40f, 0.0f, 0.18f, 0.34f, 0.34f, 0.24f, 0.28f, 0.45f);
        set (s, Param::evolveScatterSeed, 1873);
        fracture (s, 3 /* EVOLVE */, 0.55f, 0.62f, 0.55f, 0.55f, 0.42f, 0.62f, 0.70f, 0.42f, 0.65f,
                  1 /* 16 */, 2 /* 1/4 */, 8, 0.0f, 1 /* BACKWARD */, 0.95f, 0.40f, 1907,
                  FractureShape { 16, 0.20f, 0.85f, 0.30f, 0.65f, 0.55f, 0.85f, 0.35f, 1.0f,
                                  1.0f, 0.9f, 0.7f, 1.0f, kFallingTerrace, "XXLLHHoo", nullptr });
        space (s, SpacePresets::Dust, 0.52f, 0.68f, 0.45f, 0.45f);

        lfo (s, 1, 0.24f, 2 /* SAW */, 1.0f, true);
        env (s, 2, 0.05f, 3.0f, 0.20f, 3.0f, 0.3f);
        chaos (s, 1, 1 /* BROWNIAN */, 0.45f, 0.60f, 0.70f, 0.5f, 1949);
        macros (s, 0.50f, 0.35f, 0.45f, 0.60f);

        Routings r;
        r.bi  (ModSource::LFO1,   Param::sampleStart,    0.150f)
         .uni (ModSource::Env2,   Param::evolveMelt,     0.250f)
         .bi  (ModSource::Chaos1, Param::fracturePitch,  0.080f)
         .uni (ModSource::Velocity, Param::sampleGrain,  0.250f)
         .uni (ModSource::Macro1, Param::evolveMotion,   0.400f)
         .uni (ModSource::Macro1, Param::fractureEvolve, 0.300f)
         .uni (ModSource::Macro2, Param::fractureTone,   0.300f)
         .uni (ModSource::Macro3, Param::spaceMix,       0.280f)
         .uni (ModSource::Macro4, Param::evolveMelt,     0.300f)
         .uni (ModSource::Macro4, Param::evolveCrush,    0.250f);
        sharedMacros (r, Param::fractureDecay, Param::sampleSpread);
        r.commit (s);
    }});

    //==========================================================================
    // SEQUENCE — hold a note, the patch plays the pattern
    //==========================================================================

    manager.addFactory ({ "Frozen Machine", "SEQUENCE", { "sequence", "rhythmic", "metal", "fracture" }, [] (PatchState& s)
    {
        dust (s, 6 /* IMPULSE */, 0.45f, 0.30f, 0.50f, 0.16f, 0.35f, 0.45f, 761);
        amp (s, 0.001f, 0.40f, 0.92f, 0.50f, 0.3f);
        shape (s, 0.44f, 0.56f, 0.32f, 0.62f, 0.50f, 0.24f);
        material (s, MaterialType::Metal, MaterialType::Crystal, 0.40f);
        topology (s, 1 /* RING */, 0.42f, 0.55f, 761);
        matter (s, 0.55f, 0.55f, 0.0f, 0.68f);
        evolve (s, 0.0f, 0.0f, 0.0f, 0.44f, 0.46f, 0.16f, 0.0f, 0.35f, 0.20f);
        set (s, Param::evolveMagnetTarget, 5 /* SCALE */);
        fracture (s, 1 /* RHYTHMIC */, 0.85f, 1.0f, 0.45f, 1.0f, 0.35f, 0.40f, 0.55f, 0.30f, 0.20f,
                  1 /* 16 */, 4 /* 1/16 */, 8, 0.12f, 0 /* FORWARD */, 1.0f, 0.15f, 2003,
                  FractureShape { 16, 0.03f, 0.30f, 0.20f, 0.45f, 0.35f, 0.60f, 0.25f, 0.85f,
                                  2.0f, 2.0f, 0.70f, 1.0f, kOctaveTerrace, "XLoHXLoH", kOctaveTerrace });
        space (s, SpacePresets::Machine, 0.30f, 0.40f, 0.55f, 0.40f);
        set (s, Param::spaceEqHigh, -8.0f);

        env (s, 1, 0.001f, 0.30f, 0.0f, 0.25f, 0.25f);
        lfo (s, 1, 0.50f, 3 /* SQUARE */, 1.0f, true);
        macros (s, 0.35f, 0.45f, 0.35f, 0.55f);

        Routings r;
        r.uni (ModSource::Env1,   Param::fractureAmount, 0.200f)
         .bi  (ModSource::LFO1,   Param::fractureSequence, 0.150f)
         .uni (ModSource::Velocity, Param::dustColor,    0.300f)
         .uni (ModSource::Velocity, Param::fractureAmount, 0.150f)
         .bi  (ModSource::KeyTrack, Param::fractureTone,  0.150f)
         .uni (ModSource::Macro1, Param::fractureEvolve, 0.350f)
         .uni (ModSource::Macro1, Param::evolveMotion,   0.250f)
         .uni (ModSource::Macro2, Param::fractureTone,   0.300f)
         .uni (ModSource::Macro3, Param::spaceMix,       0.300f)
         .uni (ModSource::Macro4, Param::fractureSpread, 0.350f)
         .uni (ModSource::Macro4, Param::fractureFeedback, 0.200f);
        sharedMacros (r, Param::fractureDecay, Param::fractureRandom);
        r.commit (s);
    }});

    manager.addFactory ({ "Pulse Lattice", "SEQUENCE", { "sequence", "pulsing", "bright", "wave" }, [] (PatchState& s)
    {
        wave (s, 1 /* HARMONIC */, 0.38f, 0.30f, 0.15f, 2, 0.12f, 0.50f);
        amp (s, 0.004f, 0.50f, 0.88f, 0.35f, 0.35f);
        shape (s, 0.48f, 0.44f, 0.34f, 0.60f, 0.50f, 0.26f);
        material (s, MaterialType::Crystal, MaterialType::Metal, 0.35f);
        topology (s, 3 /* LATTICE */, 0.48f, 0.52f, 811);
        matter (s, 0.82f, 0.60f, 0.04f, 0.70f);
        evolve (s, 0.0f, 0.0f, 0.0f, 0.36f, 0.46f, 0.18f, 0.0f, 0.48f, 0.25f);
        set (s, Param::evolveMagnetTarget, 5 /* SCALE */);
        fracture (s, 1 /* RHYTHMIC */, 0.62f, 1.0f, 0.38f, 1.0f, 0.28f, 0.28f, 0.42f, 0.62f, 0.15f,
                  0 /* 8 */, 4 /* 1/16 */, 8, 0.18f, 2 /* PINGPONG */, 1.0f, 0.10f, 2087,
                  FractureShape { 8, 0.02f, 0.25f, 0.15f, 0.35f, 0.30f, 0.50f, 0.20f, 0.80f,
                                  2.0f, 2.0f, 0.60f, 1.0f, kFifthTerrace, "XoLoHoXo", kFifthTerrace });
        space (s, SpacePresets::Orbit, 0.34f, 0.40f, 0.60f, 0.50f);

        env (s, 1, 0.002f, 0.25f, 0.20f, 0.20f, 0.3f);
        lfo (s, 1, 2.40f, 3 /* SQUARE */, 1.0f, true);
        lfo (s, 2, 0.31f, 1 /* TRIANGLE */, 1.0f, false);
        macros (s, 0.40f, 0.50f, 0.35f, 0.50f);

        Routings r;
        r.bi  (ModSource::LFO1,   Param::wavePosition,   0.120f)
         .bi  (ModSource::LFO2,   Param::fractureSequence, 0.180f)
         .uni (ModSource::Env1,   Param::shapeExcite,    0.200f)
         .uni (ModSource::Velocity, Param::waveMorph,    0.250f)
         .bi  (ModSource::KeyTrack, Param::fractureTone,  0.150f)
         .uni (ModSource::Macro1, Param::fractureEvolve, 0.300f)
         .uni (ModSource::Macro1, Param::evolveMotion,   0.250f)
         .uni (ModSource::Macro2, Param::wavePosition,   0.300f)
         .uni (ModSource::Macro3, Param::spaceMix,       0.300f)
         .uni (ModSource::Macro4, Param::fractureSpread, 0.300f)
         .uni (ModSource::Macro4, Param::fractureSwing,  0.250f);
        sharedMacros (r, Param::fractureDecay, Param::fractureSwing);
        r.commit (s);
    }});

    manager.addFactory ({ "Ghost Arpeggio", "SEQUENCE", { "sequence", "arp", "soft", "spectral" }, [] (PatchState& s)
    {
        wave (s, 5 /* SPECTRAL */, 0.30f, 0.22f, 0.04f, 2, 0.10f, 0.55f);
        amp (s, 0.02f, 0.60f, 0.88f, 1.20f, 0.4f);
        shape (s, 0.50f, 0.82f, 0.28f, 0.64f, 0.62f, 0.18f);
        material (s, MaterialType::Crystal, MaterialType::Void, 0.42f);
        topology (s, 5 /* STAR */, 0.34f, 0.58f, 863);
        matter (s, 0.80f, 0.52f, 0.08f, 0.75f);
        evolve (s, 0.0f, 0.0f, 0.0f, 0.58f, 0.42f, 0.14f, 0.0f, 0.32f, 0.22f);
        set (s, Param::evolveMagnetTarget, 5 /* SCALE */);
        fracture (s, 1 /* RHYTHMIC */, 0.75f, 1.0f, 0.55f, 0.96f, 0.42f, 0.50f, 0.62f, 0.55f, 0.25f,
                  1 /* 16 */, 3 /* 1/8 */, 8, 0.22f, 2 /* PINGPONG */, 0.92f, 0.20f, 2131,
                  FractureShape { 16, 0.06f, 0.45f, 0.25f, 0.55f, 0.45f, 0.72f, 0.25f, 0.90f,
                                  1.8f, 1.8f, 0.70f, 0.95f, kMinorTerrace, "XLHoXLHo", kMinorTerrace });
        space (s, SpacePresets::Dream, 0.42f, 0.62f, 0.58f, 0.38f);

        env (s, 1, 0.001f, 0.50f, 0.0f, 0.40f, 0.3f);
        lfo (s, 1, 0.38f, 1 /* TRIANGLE */, 1.0f, true);
        chaos (s, 1, 4 /* TARGETS */, 1.60f, 0.40f, 0.60f, 0.5f, 2213);
        macros (s, 0.35f, 0.45f, 0.42f, 0.50f);

        Routings r;
        r.uni (ModSource::Env1,   Param::shapeExcite,    0.180f)
         .bi  (ModSource::LFO1,   Param::fractureSequence, 0.120f)
         .bi  (ModSource::Chaos1, Param::fracturePitch,  0.050f)
         .uni (ModSource::Velocity, Param::wavePosition, 0.280f)
         .bi  (ModSource::KeyTrack, Param::shapeDecay,  -0.150f)
         .uni (ModSource::Macro1, Param::fractureEvolve, 0.300f)
         .uni (ModSource::Macro1, Param::evolveMotion,   0.250f)
         .uni (ModSource::Macro2, Param::waveMorph,      0.300f)
         .uni (ModSource::Macro3, Param::spaceMix,       0.300f)
         .uni (ModSource::Macro4, Param::fractureProbability, -0.300f)
         .uni (ModSource::Macro4, Param::fractureSpread, 0.300f);
        sharedMacros (r, Param::fractureDecay, Param::fractureRandom);
        r.commit (s);
    }});

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
}

//==============================================================================
const std::vector<CategorySpec>& categories()
{
    static const std::vector<CategorySpec> specs
    {
        { "INIT",       0.004f, 0.60f, 0.02f, 1.0f, "The blank starting point" },
        { "PAD",        0.020f, 0.50f, 0.08f, 1.0f, "Held chords, slow movement" },
        { "BASS",       0.020f, 0.55f, 0.08f, 1.0f, "Weight and definition below the mix" },
        { "KEYS",       0.010f, 0.50f, 0.08f, 1.0f, "Struck and played" },
        { "PLUCK",      0.006f, 0.45f, 0.08f, 1.0f, "Short, precise, physical" },
        { "LEAD",       0.020f, 0.55f, 0.10f, 1.0f, "One line, in front" },
        { "TEXTURE",    0.008f, 0.50f, 0.06f, 1.0f, "Surfaces rather than notes" },
        { "PERCUSSION", 0.004f, 0.45f, 0.06f, 1.0f, "Struck objects, tuned" },
        { "DRONE",      0.015f, 0.55f, 0.08f, 1.0f, "One note, held forever" },
        { "FX",         0.006f, 0.50f, 0.06f, 1.0f, "Sound design material" },
        { "SEQUENCE",   0.008f, 0.50f, 0.08f, 1.0f, "The patch plays the pattern" },
        { "EVOLVING",   0.010f, 0.50f, 0.08f, 1.0f, "Different at the end of the note" },
        { "CINEMATIC",  0.010f, 0.55f, 0.08f, 1.0f, "Scoring material" }
    };
    return specs;
}

const CategorySpec& categorySpec (const juce::String& category) noexcept
{
    for (const auto& c : categories())
        if (category.equalsIgnoreCase (c.category))
            return c;

    static const CategorySpec unknown { "UNKNOWN", 0.002f, 0.70f, 0.02f, 1.0f, "Unclassified" };
    return unknown;
}

bool isKnownCategory (const juce::String& category) noexcept
{
    for (const auto& c : categories())
        if (category.equalsIgnoreCase (c.category))
            return true;
    return false;
}

int  rejectedRoutings() noexcept      { return gRejectedRoutings.load (std::memory_order_relaxed); }
void resetRejectedRoutings() noexcept { gRejectedRoutings.store (0, std::memory_order_relaxed); }

} // namespace am::FactoryContent
