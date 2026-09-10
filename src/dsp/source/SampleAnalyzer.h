#pragma once

#include "SampleData.h"

#include <juce_data_structures/juce_data_structures.h>

namespace am
{

/**
    The modal description of a recorded sound: the ratios, relative weights
    and ring times of its strongest partials.

    This is what ANALYZE → MATTER produces (SPEC §47): the imported audio
    becomes a synthetic material, not sample playback. The table travels in
    the patch's `matter` JSON section as `"custom"`.
*/
struct PartialTable
{
    static constexpr int kMaxPartials = 64;

    struct Partial
    {
        float ratio  = 1.0f;   ///< frequency / fundamental
        float weight = 0.0f;   ///< 0..1, relative to the strongest partial
        float t60    = 1.0f;   ///< seconds to -60 dB
    };

    juce::String source;              ///< sample the table came from
    float fundamentalHz = 0.0f;
    float attackSeconds = 0.0f;       ///< time from onset to peak level
    float noiseFloorDb  = -120.0f;    ///< spectral floor relative to the loudest partial
    float harmonicity   = 0.0f;       ///< 0 = perfectly harmonic, 1 = unrelated partials
    int   count = 0;
    std::array<Partial, kMaxPartials> partials {};

    bool isValid() const noexcept { return count > 0 && fundamentalHz > 0.0f; }

    juce::var toVar() const;
    static PartialTable fromVar (const juce::var& v);
};

//==============================================================================
/**
    ANALYZE → MATTER (message thread only).

    Estimates the fundamental and the strongest partials of a sample's attack
    with an FFT peak picker (parabolic interpolation on both frequency and
    magnitude) and measures each partial's T60 from the decay of its bin
    across the frames.

    Nothing here is real-time safe: it allocates, runs FFTs and is meant to be
    called from the UI (the ANALYZE button) or a tool.
*/
class SampleAnalyzer
{
public:
    struct Options
    {
        int   fftOrder    = 13;      ///< 8192 point frames (5.9 Hz at 48 kHz)
        float maxSeconds  = 2.5f;    ///< analysed window after the onset
        int   maxPartials = PartialTable::kMaxPartials;
        float floorDb     = -55.0f;  ///< peaks quieter than this (re. the loudest) are ignored
        float minHz       = 25.0f;
        float maxHz       = 12000.0f;
    };

    static PartialTable analyse (const SampleData& sample, const Options& options);
    /** Analyses with the default options. */
    static PartialTable analyse (const SampleData& sample);

    /**
        The Shape macros that best reproduce a partial table with the current
        Matter engine. A pragmatic fit (harmonicity → FORM, partial expansion →
        TENSION, ring time → DECAY, spectral tilt → MASS, partial count →
        DENSITY) used until Matter can consume the table directly.
    */
    struct MatterFit
    {
        float form = 0.3f, tension = 0.5f, decay = 0.5f, mass = 0.4f, density = 0.5f, distribution = 0.5f;
    };

    static MatterFit fitShape (const PartialTable& table);
};

} // namespace am
