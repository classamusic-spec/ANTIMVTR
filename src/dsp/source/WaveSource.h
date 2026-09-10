#pragma once

#include "SourceBase.h"
#include "WavetableBank.h"

namespace am
{

/**
    WAVE — the wavetable / virtual-analogue energy source.

    Eight procedurally designed banks (see WavetableGenerator) are played back
    from mip-mapped, band-limited tables selected by the current increment, so
    the oscillator stays clean from sub-audio to the top of the keyboard.

    Per voice:
      * position / morph / scan  — frame interpolation, a phase-distortion warp
        and a deterministic position sweep, all ramped inside the block so
        automation never zippers.
      * unison 1..8 with non-linear detune, centre-weighted stereo spread and
        power-preserving gain compensation.
      * through-zero FM, PM, AM, ring modulation and hard sync driven by one
        sine modulator at source.wave.modRatio x frequency. The sync reset is
        band limited with a BLEP residual, which costs a constant
        kLatencySamples group delay on this source.

    Real-time safe: the shared table cache is built in prepare(), render()
    allocates nothing and takes no locks.
*/
class WaveSource final : public SourceBase
{
public:
    static constexpr int kMaxUnison = 8;

    /** Constant group delay of the band-limited sync corrector, in samples. */
    static constexpr int kLatencySamples = kWaveBlepZ;

    void prepare (double sampleRate, int maxBlockSize) override;
    void reset() override;
    void noteOn (const NoteState& note, const ParamValues& params) override;
    void render (float* l, float* r, int n, const RenderContext& ctx, const NoteState& note) override;

    float energy() const noexcept override { return lastEnergy; }

private:
    //==========================================================================
    struct Osc
    {
        double phase    = 0.0;      ///< slave / carrier phase, cycles
        double master   = 0.0;      ///< sync master phase, cycles
        double modPhase = 0.0;      ///< modulator phase, cycles
        float  ratio    = 1.0f;     ///< detune multiplier
        float  gainL    = 0.0f;
        float  gainR    = 0.0f;

        // Band-limited step correction for hard sync.
        float    rawRing[kWaveBlepLen]  {};
        float    corrRing[kWaveBlepLen] {};
        uint32_t ringIndex = 0;

        void clearRings() noexcept;
    };

    double sr = 48000.0;
    std::array<Osc, kMaxUnison> oscs {};

    // block-to-block smoothing state (ramped inside every block)
    float smPosition = 0.0f, smMorph = 0.0f, smAm = 0.0f, smRing = 0.0f;
    float smFm = 0.0f, smPm = 0.0f;
    bool  primed = false;

    double scanPhase = 0.0;
    int    fadeCounter = 0;
    float  lastEnergy = 0.0f;

    const float* sineTab  = nullptr;
    const float* blepTab  = nullptr;
    const float* blampTab = nullptr;

    static constexpr int kFadeLength = 32;   ///< click guard on note start
};

} // namespace am
