#pragma once

#include "SourceBase.h"
#include "NoiseGenerators.h"

#include <vector>

namespace am
{

/**
    IMPACT — transient excitation (SPEC §12).

    A strike fires at note-on and, when `source.impact.rate` is above zero,
    repeats while the key is held (mallet rolls). Each strike is an independent
    voice inside the source with its own filters, envelope and seeded random
    variation, so overlapping repeats never share state.

    IMPACT is the primary exciter of MATTER: the modes are deliberately short,
    DC free and click free at both ends, and cover the useful excitation space
    from a razor band-limited impulse to a soft membrane thump.

    The output is mono (both channels carry the same strike): stereo placement
    belongs to Matter and Space, and keeping the exciter mono halves its cost.
*/
class ImpactSource final : public SourceBase
{
public:
    enum class Mode : int
    {
        Impulse = 0, Click, Pluck, NoiseStrike, MetalStrike, DampedSine, MembraneHit, Count
    };

    void prepare (double sampleRate, int maxBlockSize) override;
    void reset() override;
    void noteOn (const NoteState& note, const ParamValues& params) override;
    void noteOff() override;
    void render (float* l, float* r, int n, const RenderContext& ctx, const NoteState& note) override;

    bool  isActive() const noexcept override;
    float energy() const noexcept override { return lastEnergy; }

    /** Number of strikes still sounding (diagnostics / tests). */
    int activeStrikes() const noexcept { return numStrikes; }

private:
    static constexpr int kMaxStrikes    = 12;
    static constexpr int kMetalPartials = 7;

    struct Params
    {
        Mode  mode       = Mode::Pluck;
        float level      = 1.0f;
        float hardness   = 0.5f;
        float brightness = 0.5f;
        float length     = 0.3f;
        float velSens    = 0.7f;
        float curve      = 0.5f;
        float random     = 0.1f;
        float rate       = 0.0f;
        float velocity   = 1.0f;
        double freq      = 261.6256;
    };

    /** One sounding strike. Everything it needs is here so repeats never interfere. */
    struct Strike
    {
        Mode  mode = Mode::Pluck;
        int   age = 0;
        float amp = 0.0f;
        float env = 1.0f, envFast = 1.0f, envClick = 1.0f;
        float decay = 0.99f, decayFast = 0.9f, decayClick = 0.9f;
        float curve = 0.5f;
        float attack = 0.0f, attackInc = 1.0f;
        bool  spike = false;                 ///< IMPULSE: one unit sample still to be injected
        float phase = 0.0f, inc = 0.0f, incTarget = 0.0f, glide = 0.0f;
        float harm2 = 0.0f, noiseAmt = 0.0f, clickAmt = 0.0f;
        float mPhase[kMetalPartials] {}, mInc[kMetalPartials] {};
        float mAmp[kMetalPartials] {}, mDec[kMetalPartials] {}, mEnv[kMetalPartials] {};
        excitation::Svf            svf;
        excitation::ExciterLowpass lp;
        excitation::TiltFilter     tilt;
        Rng   rng { 1 };

        float level() const noexcept { return env * attack; }
    };

    void  readParams (const RenderContext& ctx, const NoteState& note);
    void  spawnStrike();
    void  initStrike (Strike& s, Rng& rng);
    inline float renderStrike (Strike& s) noexcept;
    double nextInterval() noexcept;

    double sr = 48000.0;
    Params p;

    std::array<Strike, kMaxStrikes> strikes {};
    int numStrikes = 0;

    std::vector<float> comb;
    int   combWrite = 0, combSize = 0, combDelay = 0;

    excitation::DcBlocker dc;
    Rng   repeatRng { 11 };
    uint32_t noteId = 0, strikeCounter = 0;
    bool  gate = false, pendingStrike = false;
    double repeatCountdown = 0.0;
    float rateHz = 0.0f;
    float lastEnergy = 0.0f;
};

} // namespace am
