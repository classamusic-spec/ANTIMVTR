#pragma once

#include "presets/FactoryContent.h"
#include "presets/PresetManager.h"

#include "dsp/fracture/Fragment.h"
#include "dsp/fx/SpacePresets.h"
#include "dsp/matter/MaterialProfile.h"
#include "dsp/source/SampleData.h"
#include "state/ModRouting.h"

#include <atomic>

//==============================================================================
// FACTORY BUILDERS — the patch-sheet vocabulary every factory file writes in.
//
// This header exists so the library can be split across translation units: one
// file per category, each owning its own presets, so the bank can grow to
// hundreds of patches without any file becoming unreviewable and without two
// authors ever touching the same file.
//
// The builders read like a patch sheet: SOURCE, amplitude, SHAPE, EVOLVE,
// FRACTURE, SPACE, modulation, macros — in that order, always. Every helper
// clamps into the declared parameter range, so a patch can never write an
// illegal value even while it is being edited.
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

namespace am::FactoryContent
{

inline std::atomic<int> gRejectedRoutings { 0 };

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

inline FractureTable makeFracture (const FractureShape& f)
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

} // namespace am::FactoryContent
